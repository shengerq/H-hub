#pragma once
#include "DeviceReader.h"

#include <map>
#include <set>
#include <memory>

namespace hub {
// Native bridge for reviewed offline WebHID protocol adapters.
// Enumeration tokens belong to one plugin; pages never supply Windows device paths.
struct PluginHidV2 {
 struct Device {
  std::wstring path;JsonObject info,policy;HIDP_CAPS caps{};HANDLE handle=INVALID_HANDLE_VALUE,event{};
  OVERLAPPED read{};std::vector<unsigned char> bytes;bool reading=false;ULONGLONG touched=GetTickCount64();
  ~Device(){close();}
  void close(){if(handle!=INVALID_HANDLE_VALUE){if(reading){CancelIoEx(handle,&read);DWORD ignored{};GetOverlappedResult(handle,&read,&ignored,TRUE);}CloseHandle(handle);handle=INVALID_HANDLE_VALUE;}if(event){CloseHandle(event);event=nullptr;}reading=false;}
  void beginRead(){if(reading||!event||handle==INVALID_HANDLE_VALUE)return;bytes.assign(caps.InputReportByteLength,0);read={};read.hEvent=event;ResetEvent(event);DWORD received{};BOOL ok=ReadFile(handle,bytes.data(),(DWORD)bytes.size(),&received,&read);if(!ok&&GetLastError()!=ERROR_IO_PENDING)throw hresult_error(HRESULT_FROM_WIN32(GetLastError()),L"HID 输入通道已断开");reading=true;}
 };
 std::map<std::wstring,std::map<std::wstring,std::unique_ptr<Device>>> sessions;
 std::map<std::wstring,JsonObject> batterySamples;
 void decorate(JsonObject detected){for(auto key:{L"devices",L"peripherals"})for(auto const& value:detected.GetNamedArray(key,JsonArray{})){auto d=value.GetObject();if(str(d,L"batterySource")==L"插件协议回读"){for(auto field:{L"battery",L"lowBattery",L"batteryIsCharging",L"batterySource",L"batteryAgeSeconds"})if(d.HasKey(field))d.Remove(field);}for(auto const& [key,s]:batterySamples){auto age=clockSeconds()-num(s,L"sampleTime");if(age<0||age>60||str(d,L"containerId").empty()||str(d,L"containerId")!=str(s,L"containerId")||num(d,L"vid",-1)!=num(s,L"vendorId")||num(d,L"pid",-1)!=num(s,L"productId"))continue;d.SetNamedValue(L"battery",s.GetNamedValue(L"battery"));put(d,L"lowBattery",num(s,L"battery")<=15);put(d,L"batteryIsCharging",flag(s,L"batteryIsCharging"));put(d,L"batterySource",std::wstring(L"插件协议回读"));put(d,L"batteryAgeSeconds",age);}}}
 static bool contains(JsonArray list,double number){for(auto const& value:list)if(value.GetNumber()==number)return true;return false;}
 static bool matches(JsonObject rule,unsigned vid,unsigned pid,unsigned page,unsigned usage,std::wstring name){
  if(!contains(rule.GetNamedArray(L"vendorIds",JsonArray{}),vid))return false;
  auto pids=rule.GetNamedArray(L"productIds",JsonArray{});if(pids.Size()&&!contains(pids,pid))return false;
  auto excluded=rule.GetNamedArray(L"excludedProductIds",JsonArray{});if(contains(excluded,pid))return false;
  auto pages=rule.GetNamedArray(L"usagePages",JsonArray{});if(pages.Size()&&!contains(pages,page))return false;
  auto usages=rule.GetNamedArray(L"usages",JsonArray{});if(usages.Size()&&!contains(usages,usage))return false;
  auto expected=str(rule,L"productName");if(!expected.empty()){std::transform(name.begin(),name.end(),name.begin(),towlower);std::transform(expected.begin(),expected.end(),expected.begin(),towlower);if(name!=expected)return false;}
  return true;
 }
 static JsonArray reportIds(PHIDP_PREPARSED_DATA prep,HIDP_REPORT_TYPE type,USHORT values,USHORT buttons,USHORT length){
  std::set<unsigned> ids;std::vector<HIDP_VALUE_CAPS> valueCaps(values);if(values&&HidP_GetValueCaps(type,valueCaps.data(),&values,prep)==HIDP_STATUS_SUCCESS)for(unsigned i=0;i<values;i++)ids.insert(valueCaps[i].ReportID);
  std::vector<HIDP_BUTTON_CAPS> buttonCaps(buttons);if(buttons&&HidP_GetButtonCaps(type,buttonCaps.data(),&buttons,prep)==HIDP_STATUS_SUCCESS)for(unsigned i=0;i<buttons;i++)ids.insert(buttonCaps[i].ReportID);
  if(ids.empty()&&length>0)ids.insert(0);JsonArray result;for(auto id:ids)result.Append(JsonValue::CreateNumberValue(id));return result;
 }
 static JsonArray reports(JsonArray ids,unsigned length){JsonArray result;for(auto const& value:ids){JsonObject item;item.SetNamedValue(L"reportId",value);item.SetNamedValue(L"items",JsonArray{});result.Append(item);}return result;}
 static std::wstring hex(std::vector<unsigned char> const& data,unsigned offset=0,unsigned length=0){std::wstring result;wchar_t const* digits=L"0123456789ABCDEF";if(!length)length=(unsigned)data.size();for(unsigned i=offset;i<length;i++){if(!result.empty())result+=L' ';result+=digits[data[i]>>4];result+=digits[data[i]&15];}return result;}
 JsonObject enumerate(std::wstring const& pluginId,JsonObject plugin){
  auto& devices=sessions[pluginId];devices.clear();JsonArray result;GUID hid{};HidD_GetHidGuid(&hid);auto set=SetupDiGetClassDevsW(&hid,nullptr,nullptr,DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);if(set==INVALID_HANDLE_VALUE)throw hresult_error(HRESULT_FROM_WIN32(GetLastError()));
  try{for(DWORD i=0;;i++){SP_DEVICE_INTERFACE_DATA iface{sizeof(iface)};if(!SetupDiEnumDeviceInterfaces(set,nullptr,&hid,i,&iface))break;DWORD size{};SetupDiGetDeviceInterfaceDetailW(set,&iface,nullptr,0,&size,nullptr);if(size<sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W))continue;std::vector<unsigned char> storage(size);auto detail=(SP_DEVICE_INTERFACE_DETAIL_DATA_W*)storage.data();detail->cbSize=sizeof(*detail);SP_DEVINFO_DATA node{sizeof(node)};if(!SetupDiGetDeviceInterfaceDetailW(set,&iface,detail,size,nullptr,&node))continue;
   HANDLE h=CreateFileW(detail->DevicePath,0,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);if(h==INVALID_HANDLE_VALUE)continue;HIDD_ATTRIBUTES attr{sizeof(attr)};PHIDP_PREPARSED_DATA prep{};HIDP_CAPS caps{};wchar_t product[256]{};HidD_GetProductString(h,product,sizeof(product));
   if(!HidD_GetAttributes(h,&attr)||!HidD_GetPreparsedData(h,&prep)){CloseHandle(h);continue;}if(HidP_GetCaps(prep,&caps)!=HIDP_STATUS_SUCCESS){HidD_FreePreparsedData(prep);CloseHandle(h);continue;}
   JsonObject policy{nullptr};for(auto const& rule:plugin.GetNamedArray(L"hidV2Targets",JsonArray{}))if(matches(rule.GetObject(),attr.VendorID,attr.ProductID,caps.UsagePage,caps.Usage,product)){policy=rule.GetObject();break;}
   if(policy&&devices.size()<64){auto declaredFeatureLength=num(policy,L"implicitFeatureLength");bool implicitFeature=caps.FeatureReportByteLength==0&&declaredFeatureLength>=1&&declaredFeatureLength<=4097&&std::floor(declaredFeatureLength)==declaredFeatureLength;if(implicitFeature)caps.FeatureReportByteLength=(USHORT)declaredFeatureLength;JsonObject info;std::wstring cid;DeviceReader::physicalKey(node,cid);put(info,L"containerId",cid);put(info,L"implicitFeatureReport",implicitFeature);GUID nonce{};check_hresult(CoCreateGuid(&nonce));wchar_t token[64]{};StringFromGUID2(nonce,token,64);put(info,L"key",std::wstring(token));put(info,L"vendorId",(double)attr.VendorID);put(info,L"productId",(double)attr.ProductID);put(info,L"productName",std::wstring(product));put(info,L"usagePage",(double)caps.UsagePage);put(info,L"usage",(double)caps.Usage);put(info,L"inputLength",(double)caps.InputReportByteLength);put(info,L"outputLength",(double)caps.OutputReportByteLength);put(info,L"featureLength",(double)caps.FeatureReportByteLength);
    info.SetNamedValue(L"inputIds",reportIds(prep,HidP_Input,caps.NumberInputValueCaps,caps.NumberInputButtonCaps,caps.InputReportByteLength));info.SetNamedValue(L"outputIds",reportIds(prep,HidP_Output,caps.NumberOutputValueCaps,caps.NumberOutputButtonCaps,caps.OutputReportByteLength));info.SetNamedValue(L"featureIds",reportIds(prep,HidP_Feature,caps.NumberFeatureValueCaps,caps.NumberFeatureButtonCaps,caps.FeatureReportByteLength));
    JsonObject collection;put(collection,L"usagePage",(double)caps.UsagePage);put(collection,L"usage",(double)caps.Usage);collection.SetNamedValue(L"inputReports",reports(info.GetNamedArray(L"inputIds"),caps.InputReportByteLength));collection.SetNamedValue(L"outputReports",reports(info.GetNamedArray(L"outputIds"),caps.OutputReportByteLength));collection.SetNamedValue(L"featureReports",reports(info.GetNamedArray(L"featureIds"),caps.FeatureReportByteLength));collection.SetNamedValue(L"children",JsonArray{});JsonArray collections;collections.Append(collection);info.SetNamedValue(L"collections",collections);
    auto device=std::make_unique<Device>();device->path=detail->DevicePath;device->caps=caps;device->policy=policy;device->info=info;devices.emplace(token,std::move(device));result.Append(info);
   }HidD_FreePreparsedData(prep);CloseHandle(h);
  }}catch(...){SetupDiDestroyDeviceInfoList(set);throw;}SetupDiDestroyDeviceInfoList(set);JsonObject reply;reply.SetNamedValue(L"devices",result);put(reply,L"transportVersion",2.0);return reply;
 }
 void revoke(JsonObject state){for(auto it=batterySamples.begin();it!=batterySamples.end();){bool keep=false;for(auto const& value:state.GetNamedArray(L"plugins")){auto p=value.GetObject();if(str(p,L"id")==str(it->second,L"pluginId")&&flag(p,L"installed")&&!flag(p,L"forceStopped")&&str(p,L"mode")!=L"disabled")keep=true;}if(!keep||clockSeconds()-num(it->second,L"sampleTime")>60)it=batterySamples.erase(it);else ++it;}for(auto it=sessions.begin();it!=sessions.end();){bool keep=false;for(auto const& value:state.GetNamedArray(L"plugins")){auto p=value.GetObject();if(str(p,L"id")==it->first&&flag(p,L"installed")&&!flag(p,L"forceStopped")&&str(p,L"mode")!=L"disabled")keep=true;}if(!keep)it=sessions.erase(it);else ++it;}}
 JsonObject transact(JsonObject state,JsonObject body){
  auto id=str(body,L"id");auto plugin=find(state.GetNamedArray(L"plugins"),id);if(!flag(plugin,L"installed")||flag(plugin,L"forceStopped")||str(plugin,L"mode")==L"disabled")throw hresult_error(E_ACCESSDENIED,L"插件未安装或已禁用");bool allowed=false;for(auto const& permission:plugin.GetNamedArray(L"permissions",JsonArray{}))if(permission.GetString()==L"USB HID 配置")allowed=true;if(!allowed)throw hresult_error(E_ACCESSDENIED,L"插件没有 HID 配置权限");
  auto request=body.GetNamedObject(L"request");auto op=str(request,L"operation");JsonObject result;put(result,L"transportVersion",2.0);if(op==L"closeAll"){sessions.erase(id);return result;}if(op==L"enumerate")return enumerate(id,plugin);
  auto owner=sessions.find(id);if(owner==sessions.end())throw hresult_error(E_ACCESSDENIED,L"请先枚举并选择设备");auto found=owner->second.find(str(request,L"key"));if(found==owner->second.end())throw hresult_error(E_ACCESSDENIED,L"设备令牌不属于此插件");auto& device=*found->second;
  if(op==L"close"){device.close();return result;}
  if(op==L"open"){device.close();device.handle=CreateFileW(device.path.c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,nullptr);bool inputAvailable=device.handle!=INVALID_HANDLE_VALUE;if(device.handle==INVALID_HANDLE_VALUE&&device.caps.FeatureReportByteLength>0)device.handle=CreateFileW(device.path.c_str(),0,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,nullptr);if(device.handle==INVALID_HANDLE_VALUE)throw hresult_error(HRESULT_FROM_WIN32(GetLastError()),L"HID 通道无法打开，请退出原厂驱动");put(result,L"inputAvailable",inputAvailable);if(inputAvailable&&device.caps.InputReportByteLength>0&&device.caps.InputReportByteLength<=4097){device.event=CreateEventW(nullptr,TRUE,FALSE,nullptr);if(!device.event)throw hresult_error(HRESULT_FROM_WIN32(GetLastError()));device.beginRead();}return result;}
  if(device.handle==INVALID_HANDLE_VALUE)throw hresult_invalid_argument(L"设备尚未打开");
  if(op==L"publishBattery"){
   auto value=request.GetNamedValue(L"battery",JsonValue::CreateNullValue());if(value.ValueType()!=JsonValueType::Number||!std::isfinite(value.GetNumber())||value.GetNumber()<0||value.GetNumber()>100)throw hresult_invalid_argument(L"电量必须为 0–100 的实际回读值");
   auto sample=object(std::wstring(device.info.Stringify()));put(sample,L"pluginId",id);put(sample,L"battery",value.GetNumber());put(sample,L"batteryIsCharging",flag(request,L"batteryIsCharging"));put(sample,L"sampleTime",clockSeconds());batterySamples[id+device.path]=sample;return result;
  }
  if(op==L"poll"){JsonArray events;for(unsigned count=0;device.reading&&count<32;count++){DWORD read{};if(!GetOverlappedResult(device.handle,&device.read,&read,FALSE)){if(GetLastError()==ERROR_IO_INCOMPLETE)break;device.close();throw hresult_error(E_FAIL,L"设备断开连接");}device.reading=false;if(read>0&&contains(device.policy.GetNamedArray(L"inputReportIds",JsonArray{}),device.bytes[0])){JsonObject event;put(event,L"reportId",(double)device.bytes[0]);put(event,L"data",hex(device.bytes,1,read));events.Append(event);}device.beginRead();}result.SetNamedValue(L"events",events);return result;}
  auto rid=num(request,L"reportId",-1);if(rid<0||rid>255||std::floor(rid)!=rid)throw hresult_invalid_argument(L"报告 ID 无效");bool feature=op==L"sendFeature"||op==L"receiveFeature";if(!feature&&op!=L"sendOutput")throw hresult_invalid_argument(L"未知 HID 操作");auto policyKey=feature?L"featureReportIds":L"outputReportIds";auto actualKey=feature?L"featureIds":L"outputIds";
  if(!contains(device.policy.GetNamedArray(policyKey,JsonArray{}),rid)||!contains(device.info.GetNamedArray(actualKey),rid))throw hresult_error(E_ACCESSDENIED,L"报告 ID 不在插件和设备共同声明的范围内");unsigned length=feature?device.caps.FeatureReportByteLength:device.caps.OutputReportByteLength;if(length<1||length>4097)throw hresult_invalid_argument(L"报告长度不受支持");std::vector<unsigned char> packet(length);packet[0]=(unsigned char)rid;
  if(op==L"receiveFeature"){if(!HidD_GetFeature(device.handle,packet.data(),length))throw hresult_error(HRESULT_FROM_WIN32(GetLastError()),L"Feature 回读失败");put(result,L"data",hex(packet));return result;}
  auto payload=pluginPacket(str(request,L"data"));if(payload.size()+1>length||payload.size()>num(device.policy,L"maxPayload",1024))throw hresult_invalid_argument(L"报告数据超过声明长度");std::copy(payload.begin(),payload.end(),packet.begin()+1);
  if(feature){if(!HidD_SetFeature(device.handle,packet.data(),length))throw hresult_error(HRESULT_FROM_WIN32(GetLastError()),L"Feature 写入失败");}
  else {winrt::handle event(CreateEventW(nullptr,TRUE,FALSE,nullptr));OVERLAPPED write{};write.hEvent=event.get();DWORD sent{};BOOL ok=WriteFile(device.handle,packet.data(),length,&sent,&write);if(!ok&&GetLastError()==ERROR_IO_PENDING){if(WaitForSingleObject(event.get(),1000)!=WAIT_OBJECT_0){CancelIoEx(device.handle,&write);GetOverlappedResult(device.handle,&write,&sent,TRUE);throw hresult_error(E_FAIL,L"输出报告写入超时");}ok=GetOverlappedResult(device.handle,&write,&sent,FALSE);}if(!ok||sent!=length)throw hresult_error(E_FAIL,L"输出报告写入失败");}
  return result;
 }
};
}
