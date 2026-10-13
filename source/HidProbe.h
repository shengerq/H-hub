#pragma once
#include "DeviceReader.h"

namespace hub {
// On-demand transport diagnostic. Protocol requests live in the developer plugin, not in this host.
inline JsonObject hidTransaction(JsonObject request){
 JsonObject report;JsonArray transactions;report.SetNamedValue(L"transactions",transactions);put(report,L"settingsWriteRequested",false);
 HANDLE out=INVALID_HANDLE_VALUE,in=INVALID_HANDLE_VALUE;try{
  auto match=request.GetNamedObject(L"match");unsigned vid=(unsigned)num(match,L"vid"),pid=(unsigned)num(match,L"pid"),page=(unsigned)num(match,L"usagePage"),outUsage=(unsigned)num(match,L"outputUsage"),inUsage=(unsigned)num(match,L"featureUsage");
  GUID hid{};HidD_GetHidGuid(&hid);HDEVINFO devices=SetupDiGetClassDevsW(&hid,nullptr,nullptr,DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);if(devices==INVALID_HANDLE_VALUE)throw hresult_error(HRESULT_FROM_WIN32(GetLastError()));
  std::wstring outputPath,featurePath;HIDP_CAPS outputCaps{},featureCaps{};
  for(DWORD i=0;;i++){SP_DEVICE_INTERFACE_DATA item{sizeof(item)};if(!SetupDiEnumDeviceInterfaces(devices,nullptr,&hid,i,&item))break;DWORD size{};SetupDiGetDeviceInterfaceDetailW(devices,&item,nullptr,0,&size,nullptr);std::vector<unsigned char> storage(size);if(size<sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W))continue;auto detail=(SP_DEVICE_INTERFACE_DETAIL_DATA_W*)storage.data();detail->cbSize=sizeof(*detail);if(!SetupDiGetDeviceInterfaceDetailW(devices,&item,detail,size,nullptr,nullptr))continue;
   HANDLE handle=CreateFileW(detail->DevicePath,0,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);if(handle==INVALID_HANDLE_VALUE)continue;HIDD_ATTRIBUTES attributes{sizeof(attributes)};PHIDP_PREPARSED_DATA prep{};
   if(HidD_GetAttributes(handle,&attributes)&&attributes.VendorID==vid&&attributes.ProductID==pid&&HidD_GetPreparsedData(handle,&prep)){HIDP_CAPS caps{};if(HidP_GetCaps(prep,&caps)==HIDP_STATUS_SUCCESS&&caps.UsagePage==page){if(caps.Usage==outUsage&&caps.OutputReportByteLength){if(!outputPath.empty()){HidD_FreePreparsedData(prep);CloseHandle(handle);SetupDiDestroyDeviceInfoList(devices);throw hresult_invalid_argument(L"多个候选输出设备，请缩小匹配条件");}outputPath=detail->DevicePath;outputCaps=caps;}if(caps.Usage==inUsage&&caps.FeatureReportByteLength){if(!featurePath.empty()){HidD_FreePreparsedData(prep);CloseHandle(handle);SetupDiDestroyDeviceInfoList(devices);throw hresult_invalid_argument(L"多个候选回复设备，请缩小匹配条件");}featurePath=detail->DevicePath;featureCaps=caps;}}HidD_FreePreparsedData(prep);}CloseHandle(handle);
  }SetupDiDestroyDeviceInfoList(devices);if(outputPath.empty()||featurePath.empty())throw hresult_invalid_argument(L"没有找到指定 HID 输出和回复通道");
  out=CreateFileW(outputPath.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);in=CreateFileW(featurePath.c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);if(out==INVALID_HANDLE_VALUE||in==INVALID_HANDLE_VALUE)throw hresult_error(HRESULT_FROM_WIN32(GetLastError()),L"HID 查询通道无法打开");
  put(report,L"outputReportLength",(double)outputCaps.OutputReportByteLength);put(report,L"featureReportLength",(double)featureCaps.FeatureReportByteLength);
  auto hex=[](std::vector<unsigned char> const& bytes){std::wstring result;wchar_t const* digits=L"0123456789ABCDEF";for(auto value:bytes){if(!result.empty())result+=L' ';result+=digits[value>>4];result+=digits[value&15];}return result;};
  auto queries=request.GetNamedArray(L"queries");if(queries.Size()>32)throw hresult_invalid_argument(L"诊断查询过多");
  for(auto const& v:queries){auto query=v.GetObject();auto body=pluginPacket(str(query,L"payload"));auto rid=(unsigned)num(query,L"reportId"),replyRid=(unsigned)num(query,L"featureReportId");if(rid>255||replyRid>255||body.size()+1>outputCaps.OutputReportByteLength||outputCaps.OutputReportByteLength>4097||featureCaps.FeatureReportByteLength>4097)throw hresult_invalid_argument(L"查询报告 ID 或长度无效");
   if(!flag(query,L"readOnly"))put(report,L"settingsWriteRequested",true);
   JsonObject result;put(result,L"name",str(query,L"name"));put(result,L"payload",hex(body));transactions.Append(result);std::vector<unsigned char> packet(outputCaps.OutputReportByteLength);packet[0]=(unsigned char)rid;std::copy(body.begin(),body.end(),packet.begin()+1);DWORD written{};auto start=GetTickCount64();if(!WriteFile(out,packet.data(),(DWORD)packet.size(),&written,nullptr)||written!=packet.size())throw hresult_error(E_FAIL,std::wstring(L"输出查询失败：").append(std::to_wstring(GetLastError())).c_str());
   auto wait=(unsigned)num(query,L"waitMs",20);if(wait>200)throw hresult_invalid_argument(L"等待时间过长");Sleep(wait);std::vector<unsigned char> reply(featureCaps.FeatureReportByteLength);reply[0]=(unsigned char)replyRid;if(!HidD_GetFeature(in,reply.data(),(ULONG)reply.size()))throw hresult_error(E_FAIL,L"读取 HID 回复失败");
   for(unsigned attempt=0;reply[0]==2&&attempt<5;attempt++){Sleep(20);reply.assign(featureCaps.FeatureReportByteLength,0);reply[0]=(unsigned char)replyRid;if(!HidD_GetFeature(in,reply.data(),(ULONG)reply.size()))throw hresult_error(E_FAIL,L"读取 HID 忙状态失败");}
   put(result,L"response",hex(reply));put(result,L"elapsedMs",(double)(GetTickCount64()-start));
   auto ack=num(query,L"ack",-1);if(ack>=0&&reply[0]!=ack)throw hresult_error(E_FAIL,L"设备拒绝命令或回复仍为忙状态");
   if(query.HasKey(L"expected")){auto expected=pluginPacket(str(query,L"expected"));auto offset=(unsigned)num(query,L"responseOffset",4);if(offset+expected.size()>reply.size()||!std::equal(expected.begin(),expected.end(),reply.begin()+offset))throw hresult_error(E_FAIL,L"设备回读值与请求值不一致");}
  }put(report,L"ok",true);
 }catch(hresult_error const& error){put(report,L"ok",false);put(report,L"error",std::wstring(error.message()));}catch(...){put(report,L"ok",false);put(report,L"error",std::wstring(L"HID 诊断失败"));}
 if(out!=INVALID_HANDLE_VALUE)CloseHandle(out);if(in!=INVALID_HANDLE_VALUE)CloseHandle(in);return report;
}
inline int hidTransactionProbe(std::filesystem::path requestFile,std::filesystem::path reportFile){
 auto report=hidTransaction(object(read(requestFile)));write(reportFile,std::wstring(report.Stringify()));return flag(report,L"ok")?0:1;
}
}
