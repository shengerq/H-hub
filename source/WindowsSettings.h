#pragma once
#include "Common.h"
#include <powrprof.h>
#include <functional>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <dxgi1_2.h>
#include <winternl.h>
#include <d3dkmthk.h>
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"uuid.lib")
#pragma comment(lib,"powrprof.lib")
#pragma comment(lib,"user32.lib")

namespace hub {
struct WindowsSettings {
 static JsonObject managed(std::wstring id,std::wstring name,std::wstring note,std::wstring uri){JsonObject o;put(o,L"id",id);put(o,L"name",name);put(o,L"note",note);put(o,L"uri",uri);put(o,L"available",false);put(o,L"confirmed",false);put(o,L"requiresNative",true);o.SetNamedValue(L"enabled",JsonValue::CreateNullValue());put(o,L"status",std::wstring(L"系统管理 · H hub 未获得可靠回传"));return o;}
 static bool preference(HKEY root,wchar_t const* path,wchar_t const* name,DWORD& value){DWORD size=sizeof(value);return RegGetValueW(root,path,name,RRF_RT_REG_DWORD,nullptr,&value,&size)==ERROR_SUCCESS&&value<=1;}
 static void observed(JsonObject option,bool enabled,std::wstring source){put(option,L"enabled",enabled);put(option,L"confirmed",true);put(option,L"readSucceeded",true);put(option,L"source",source);put(option,L"status",std::wstring(L"已读取系统状态"));}
 static int tokenPreference(std::wstring const& values,std::wstring const& key){
  int result=-1;for(size_t start=0;start<values.size();){auto end=values.find(L';',start);auto token=values.substr(start,end==std::wstring::npos?end:end-start);auto equal=token.find(L'=');if(equal!=std::wstring::npos&&token.substr(0,equal)==key){if(result!=-1||token.size()!=equal+2||(token.back()!=L'0'&&token.back()!=L'1'))return -2;result=token.back()-L'0';}if(end==std::wstring::npos)break;start=end+1;}return result;
 }
 static JsonObject captureOption(){
  auto option=managed(L"capture",L"游戏录制与后台捕获",L"读取录制总开关；后台捕获、音频和画质在 Windows 中调整。",L"ms-settings:gaming-gamedvr");DWORD value{},policy{};
  if(preference(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Policies\\Microsoft\\Windows\\GameDVR",L"AllowGameDVR",policy)&&policy==0)observed(option,false,L"Windows 录制策略");
  else if(preference(HKEY_CURRENT_USER,L"System\\GameConfigStore",L"GameDVR_Enabled",value))observed(option,value!=0,L"Windows 保存的录制总开关");
  else if(preference(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR",L"AppCaptureEnabled",value))observed(option,value!=0,L"Windows 保存的捕获偏好");
  DWORD historical{};if(preference(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR",L"HistoricalCaptureEnabled",historical))put(option,L"backgroundEnabled",historical!=0);
  if(!flag(option,L"confirmed")){put(option,L"readSucceeded",true);put(option,L"systemDefault",true);put(option,L"status",std::wstring(L"系统默认录制策略"));}return option;
 }
 static JsonObject schedulingOption(){
  auto option=managed(L"gpuScheduling",L"硬件加速 GPU 调度",L"读取显卡调度运行状态；修改后需要按 Windows 提示重启。",L"ms-settings:display-advancedgraphics-default");
  winrt::com_ptr<IDXGIFactory1> factory;if(FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1),factory.put_void())))return option;
  DXGI_ADAPTER_DESC1 selected{};bool found=false;
  for(UINT index=0;;index++){winrt::com_ptr<IDXGIAdapter1> adapter;if(factory->EnumAdapters1(index,adapter.put())==DXGI_ERROR_NOT_FOUND)break;if(!adapter)break;DXGI_ADAPTER_DESC1 desc{};if(FAILED(adapter->GetDesc1(&desc))||(desc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE))continue;if(!found||desc.DedicatedVideoMemory>selected.DedicatedVideoMemory){selected=desc;found=true;}}
  if(!found)return option;D3DKMT_OPENADAPTERFROMLUID opened{};opened.AdapterLuid=selected.AdapterLuid;if(D3DKMTOpenAdapterFromLuid(&opened)<0)return option;
  D3DKMT_WDDM_2_7_CAPS caps{};D3DKMT_QUERYADAPTERINFO query{};query.hAdapter=opened.hAdapter;query.Type=KMTQAITYPE_WDDM_2_7_CAPS;query.pPrivateDriverData=&caps;query.PrivateDriverDataSize=sizeof(caps);auto status=D3DKMTQueryAdapterInfo(&query);D3DKMT_CLOSEADAPTER close{};close.hAdapter=opened.hAdapter;D3DKMTCloseAdapter(&close);
  if(status>=0){observed(option,caps.HwSchEnabled!=0,L"WDDM 显卡运行能力");put(option,L"supported",caps.HwSchSupported!=0);put(option,L"adapter",std::wstring(selected.Description));}
  DWORD preference{},size=sizeof(preference);if(RegGetValueW(HKEY_LOCAL_MACHINE,L"SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers",L"HwSchMode",RRF_RT_REG_DWORD,nullptr,&preference,&size)==ERROR_SUCCESS&&(preference==1||preference==2)){put(option,L"savedEnabled",preference==2);if(flag(option,L"confirmed"))put(option,L"restartPending",flag(option,L"enabled")!=(preference==2));}return option;
 }
 static JsonObject windowedOption(){
  auto option=managed(L"windowedOptimizations",L"窗口化游戏优化与可变刷新率",L"读取全局图形偏好；单个游戏可覆盖全局设置。",L"ms-settings:display-advancedgraphics-default");DWORD bytes{};auto path=L"Software\\Microsoft\\DirectX\\UserGpuPreferences";
  auto status=RegGetValueW(HKEY_CURRENT_USER,path,L"DirectXUserGlobalSettings",RRF_RT_REG_SZ,nullptr,nullptr,&bytes);
  if(status==ERROR_FILE_NOT_FOUND){put(option,L"systemDefault",true);put(option,L"readSucceeded",true);put(option,L"status",std::wstring(L"Windows 默认图形策略"));return option;}
  if(status!=ERROR_SUCCESS||bytes>65536)return option;std::vector<wchar_t> data(bytes/sizeof(wchar_t)+1);if(RegGetValueW(HKEY_CURRENT_USER,path,L"DirectXUserGlobalSettings",RRF_RT_REG_SZ,nullptr,data.data(),&bytes)!=ERROR_SUCCESS)return option;
  std::wstring values(data.data());int optimization=tokenPreference(values,L"SwapEffectUpgradeEnable"),vrr=tokenPreference(values,L"VRROptimizeEnable");put(option,L"readSucceeded",true);if(optimization>=0)observed(option,optimization!=0,L"Windows 保存的窗口化优化偏好");else if(optimization==-1){put(option,L"systemDefault",true);put(option,L"status",std::wstring(L"窗口化优化使用 Windows 默认策略"));}else{put(option,L"readSucceeded",false);put(option,L"status",std::wstring(L"Windows 图形偏好格式无效"));}if(vrr>=0)put(option,L"vrrEnabled",vrr!=0);return option;
 }
 static JsonObject notificationsOption(){
  auto option=managed(L"notifications",L"通知与免打扰",L"读取通知偏好与当前提示许可；免打扰和自动规则在 Windows 中调整。",L"ms-settings:notifications");DWORD value{},policy{};bool known=false;
  for(auto root:{HKEY_LOCAL_MACHINE,HKEY_CURRENT_USER})if(preference(root,L"SOFTWARE\\Policies\\Microsoft\\Windows\\CurrentVersion\\PushNotifications",L"NoToastApplicationNotification",policy)&&policy==1){observed(option,false,L"Windows 通知策略");known=true;break;}
  if(!known&&preference(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\PushNotifications",L"ToastEnabled",value)){observed(option,value!=0,L"Windows 保存的通知总开关");known=true;}
  QUERY_USER_NOTIFICATION_STATE state{};if(SUCCEEDED(SHQueryUserNotificationState(&state))){put(option,L"notificationState",(double)state);put(option,L"canPresentNotification",state==QUNS_ACCEPTS_NOTIFICATIONS);put(option,L"readSucceeded",true);}
  if(!known){put(option,L"systemDefault",true);put(option,L"status",std::wstring(L"通知总开关使用 Windows 默认策略；已查询当前提示许可"));}return option;
 }
 static JsonArray systemManaged(){JsonArray options;options.Append(captureOption());options.Append(schedulingOption());options.Append(windowedOption());options.Append(notificationsOption());return options;}

 static JsonObject soundOption(){auto o=managed(L"systemSound",L"系统声音",L"控制当前默认播放设备是否静音，不改变其他设备与应用的音量。",L"ms-settings:apps-volume");put(o,L"requiresNative",false);try{winrt::com_ptr<IMMDeviceEnumerator> e;check_hresult(CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_INPROC_SERVER,__uuidof(IMMDeviceEnumerator),e.put_void()));winrt::com_ptr<IMMDevice> d;check_hresult(e->GetDefaultAudioEndpoint(eRender,eMultimedia,d.put()));winrt::com_ptr<IAudioEndpointVolume> volume;check_hresult(d->Activate(__uuidof(IAudioEndpointVolume),CLSCTX_ALL,nullptr,volume.put_void()));BOOL muted{};check_hresult(volume->GetMute(&muted));LPWSTR id{};check_hresult(d->GetId(&id));put(o,L"endpoint",std::wstring(id));CoTaskMemFree(id);put(o,L"enabled",muted==FALSE);put(o,L"available",true);put(o,L"confirmed",true);put(o,L"status",std::wstring(L"已读取默认播放设备的实际静音状态"));}catch(...){put(o,L"status",std::wstring(L"默认播放设备不可访问"));}return o;}
 // Public DisplayConfig 24H2 ABI (SDK 26100); older systems remain unconfirmed.
 struct HdrInfo {DISPLAYCONFIG_DEVICE_INFO_HEADER header;UINT32 value;DISPLAYCONFIG_COLOR_ENCODING encoding;UINT32 bitsPerChannel;UINT32 mode;};
 struct HdrSet {DISPLAYCONFIG_DEVICE_INFO_HEADER header;UINT32 enabled;};
 static JsonObject hdrOption(){auto o=managed(L"hdr",L"HDR",L"控制主显示器 HDR。显示器、连接方式和系统必须支持；更改可能使画面短暂闪烁。",L"ms-settings:display");put(o,L"requiresNative",false);UINT32 count{},modes{};auto status=GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS,&count,&modes);if(status!=ERROR_SUCCESS||!count){put(o,L"status",std::wstring(L"当前会话无法回传本地显示器 HDR"));return o;}std::vector<DISPLAYCONFIG_PATH_INFO> paths(count);std::vector<DISPLAYCONFIG_MODE_INFO> info(modes);if(QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS,&count,paths.data(),&modes,info.data(),nullptr)!=ERROR_SUCCESS)return o;std::wstring primary;for(DWORD i=0;;i++){DISPLAY_DEVICEW d{sizeof(d)};if(!EnumDisplayDevicesW(nullptr,i,&d,0))break;if(d.StateFlags&DISPLAY_DEVICE_PRIMARY_DEVICE){primary=d.DeviceName;break;}}for(auto const& path:paths){DISPLAYCONFIG_SOURCE_DEVICE_NAME source{};source.header.type=DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;source.header.size=sizeof(source);source.header.adapterId=path.sourceInfo.adapterId;source.header.id=path.sourceInfo.id;if(DisplayConfigGetDeviceInfo(&source.header)!=ERROR_SUCCESS||(!primary.empty()&&_wcsicmp(source.viewGdiDeviceName,primary.c_str())!=0))continue;HdrInfo hdr{};hdr.header.type=(DISPLAYCONFIG_DEVICE_INFO_TYPE)15;hdr.header.size=sizeof(hdr);hdr.header.adapterId=path.targetInfo.adapterId;hdr.header.id=path.targetInfo.id;if(DisplayConfigGetDeviceInfo(&hdr.header)!=ERROR_SUCCESS){put(o,L"status",std::wstring(L"系统未提供可靠 HDR 回传，请使用 Windows 设置"));return o;}bool supported=(hdr.value&(1<<4))!=0,limited=(hdr.value&(1<<3))!=0;if(!supported){put(o,L"status",std::wstring(L"当前主显示器未提供 HDR 支持"));return o;}put(o,L"enabled",hdr.mode==2);put(o,L"confirmed",true);put(o,L"available",!limited);put(o,L"targetLow",(double)hdr.header.adapterId.LowPart);put(o,L"targetHigh",(double)hdr.header.adapterId.HighPart);put(o,L"targetId",(double)hdr.header.id);put(o,L"status",limited?std::wstring(L"HDR 状态已回传，但受系统策略限制"):std::wstring(L"HDR 当前状态已回传"));return o;}return o;}
 static void setSound(JsonObject desired){winrt::com_ptr<IMMDeviceEnumerator> e;check_hresult(CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_INPROC_SERVER,__uuidof(IMMDeviceEnumerator),e.put_void()));winrt::com_ptr<IMMDevice> d;auto id=str(desired,L"endpoint");if(id.empty())check_hresult(e->GetDefaultAudioEndpoint(eRender,eMultimedia,d.put()));else check_hresult(e->GetDevice(id.c_str(),d.put()));winrt::com_ptr<IAudioEndpointVolume> volume;check_hresult(d->Activate(__uuidof(IAudioEndpointVolume),CLSCTX_ALL,nullptr,volume.put_void()));check_hresult(volume->SetMute(!flag(desired,L"enabled"),nullptr));BOOL actual{};check_hresult(volume->GetMute(&actual));if((actual==FALSE)!=flag(desired,L"enabled"))throw hresult_error(E_FAIL,L"静音状态未通过回读");}
 static void setHdr(JsonObject desired){auto current=hdrOption();if(!flag(current,L"available"))throw hresult_error(E_FAIL,L"当前显示器无法直接调整 HDR");if(desired.HasKey(L"targetId")&&(num(desired,L"targetId")!=num(current,L"targetId")||num(desired,L"targetLow")!=num(current,L"targetLow")||num(desired,L"targetHigh")!=num(current,L"targetHigh")))throw hresult_error(E_FAIL,L"显示器已经变化，请重新读取");HdrSet request{};request.header.type=(DISPLAYCONFIG_DEVICE_INFO_TYPE)16;request.header.size=sizeof(request);request.header.adapterId.LowPart=(DWORD)num(current,L"targetLow");request.header.adapterId.HighPart=(LONG)num(current,L"targetHigh");request.header.id=(UINT32)num(current,L"targetId");request.enabled=flag(desired,L"enabled")?1:0;auto result=DisplayConfigSetDeviceInfo(&request.header);if(result!=ERROR_SUCCESS)throw hresult_error(HRESULT_FROM_WIN32(result));auto actual=hdrOption();if(!flag(actual,L"confirmed")||flag(actual,L"enabled")!=flag(desired,L"enabled"))throw hresult_error(E_FAIL,L"HDR 尚未通过系统回传");}

 struct Entry{std::wstring id,name,note,uri,path,value;};
 static std::vector<Entry> entries(){return {
  {L"gameMode",L"游戏模式",L"Windows 保存的游戏模式偏好。重新进入游戏后由系统应用。",L"ms-settings:gaming-gamemode",L"Software\\Microsoft\\GameBar",L"AutoGameModeEnabled"},
  {L"controllerGameBar",L"手柄打开 Game Bar",L"允许手柄的 Xbox 按钮打开 Game Bar；不会卸载或禁用录制组件。",L"ms-settings:gaming-gamebar",L"Software\\Microsoft\\GameBar",L"UseNexusForGameBarEnabled"},
  {L"mousePrecision",L"提高指针精确度",L"Windows 鼠标加速。使用原始输入的游戏可能不受此开关影响。",L"ms-settings:mousetouchpad",L"",L""},
  {L"stickyShortcut",L"五次 Shift 启用粘滞键",L"只改变快捷键触发，保留粘滞键功能本身和其他辅助设置。",L"ms-settings:easeofaccess-keyboard",L"",L""},
  {L"filterShortcut",L"长按右 Shift 启用筛选键",L"只改变快捷键触发，保留按键筛选功能与时间参数。",L"ms-settings:easeofaccess-keyboard",L"",L""},
  {L"toggleShortcut",L"长按 Num Lock 启用切换键",L"只改变快捷键触发，保留切换键提示功能本身。",L"ms-settings:easeofaccess-keyboard",L"",L""}
 };}
 std::filesystem::path file;JsonObject undo;std::wstring registryPrefix;
 std::function<BOOL(UINT,UINT,PVOID,UINT)> spi=SystemParametersInfoW;
 WindowsSettings(std::filesystem::path path=dataDir()/L"windows-settings-undo.json",std::wstring prefix=L""):file(std::move(path)),registryPrefix(std::move(prefix)){undo=JsonObject{};try{if(std::filesystem::exists(file))undo=object(read(file));}catch(...) {}}
 static Entry entry(std::wstring const& id){for(auto const& e:entries())if(e.id==id)return e;throw hresult_invalid_argument(L"不支持的 Windows 开关");}
 std::wstring regPath(Entry const& e){return registryPrefix.empty()?e.path:registryPrefix+L"\\"+e.path;}
 struct PreferenceValue{bool machine;std::wstring path,name;};
 static std::vector<PreferenceValue> preferenceValues(std::wstring const& id){
  if(id==L"capture")return {{false,L"System\\GameConfigStore",L"GameDVR_Enabled"},{false,L"Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR",L"AppCaptureEnabled"}};
  if(id==L"notifications")return {{false,L"Software\\Microsoft\\Windows\\CurrentVersion\\PushNotifications",L"ToastEnabled"}};
  if(id==L"windowedOptimizations")return {{false,L"Software\\Microsoft\\DirectX\\UserGpuPreferences",L"DirectXUserGlobalSettings"}};
  if(id==L"gpuScheduling")return {{true,L"SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers",L"HwSchMode"}};
  return {};
 }
 HKEY preferenceRoot(PreferenceValue const& value){return value.machine&&registryPrefix.empty()?HKEY_LOCAL_MACHINE:HKEY_CURRENT_USER;}
 std::wstring preferencePath(PreferenceValue const& value){return registryPrefix.empty()?value.path:registryPrefix+(value.machine?L"\\Machine\\":L"\\User\\")+value.path;}
 JsonObject preferenceBackup(PreferenceValue const& value){
  JsonObject result;DWORD type{},bytes{};auto path=preferencePath(value);
  auto status=RegGetValueW(preferenceRoot(value),path.c_str(),value.name.c_str(),RRF_RT_ANY,nullptr,nullptr,&bytes);
  if(status==ERROR_FILE_NOT_FOUND){put(result,L"present",false);return result;}
  if(status!=ERROR_SUCCESS||bytes>65536)throw hresult_error(HRESULT_FROM_WIN32(status==ERROR_SUCCESS?ERROR_INVALID_DATA:status));
  std::vector<BYTE> buffer(bytes);status=RegGetValueW(preferenceRoot(value),path.c_str(),value.name.c_str(),RRF_RT_ANY,&type,buffer.data(),&bytes);
  if(status!=ERROR_SUCCESS)throw hresult_error(HRESULT_FROM_WIN32(status));
  put(result,L"present",true);put(result,L"type",(double)type);JsonArray data;for(DWORD i=0;i<bytes;i++)data.Append(JsonValue::CreateNumberValue(buffer[i]));result.SetNamedValue(L"data",data);return result;
 }
 static std::vector<BYTE> preferenceBytes(JsonObject stored){std::vector<BYTE> data;for(auto const& value:stored.GetNamedArray(L"data",JsonArray{}))data.push_back((BYTE)value.GetNumber());return data;}
 void savePreference(PreferenceValue const& value,JsonObject stored){
  auto path=preferencePath(value);HKEY key{};auto status=RegCreateKeyExW(preferenceRoot(value),path.c_str(),0,nullptr,0,KEY_SET_VALUE,nullptr,&key,nullptr);
  if(status!=ERROR_SUCCESS)throw hresult_error(HRESULT_FROM_WIN32(status));
  if(flag(stored,L"present")){auto bytes=preferenceBytes(stored);status=RegSetValueExW(key,value.name.c_str(),0,(DWORD)num(stored,L"type"),bytes.data(),(DWORD)bytes.size());}
  else status=RegDeleteValueW(key,value.name.c_str());RegCloseKey(key);
  if(status!=ERROR_SUCCESS&&status!=ERROR_FILE_NOT_FOUND)throw hresult_error(HRESULT_FROM_WIN32(status));
 }
 static JsonObject storedValue(DWORD type,void const* data,size_t bytes){JsonObject result;put(result,L"present",true);put(result,L"type",(double)type);JsonArray buffer;auto source=(BYTE const*)data;for(size_t i=0;i<bytes;i++)buffer.Append(JsonValue::CreateNumberValue(source[i]));result.SetNamedValue(L"data",buffer);return result;}
 static std::wstring replacePreferenceToken(std::wstring values,std::wstring const& key,bool enabled){
  std::wstring result;for(size_t start=0;start<values.size();){auto end=values.find(L';',start);auto token=values.substr(start,end==std::wstring::npos?end:end-start);if(!token.empty()&&token.substr(0,token.find(L'='))!=key)result+=token+L";";if(end==std::wstring::npos)break;start=end+1;}return result+key+(enabled?L"=1;":L"=0;");
 }
 JsonObject writablePreference(std::wstring const& id){
  auto result=id==L"capture"?captureOption():id==L"gpuScheduling"?schedulingOption():id==L"windowedOptimizations"?windowedOption():notificationsOption();
  auto values=preferenceValues(id);JsonArray originals;for(auto const& value:values)originals.Append(preferenceBackup(value));result.SetNamedValue(L"registryValues",originals);put(result,L"canUndo",undo.HasKey(id));put(result,L"requiresNative",false);
  bool writable=!registryPrefix.empty()||id!=L"gpuScheduling"||flag(result,L"supported");bool known=false,enabled=false;
  for(uint32_t i=0;i<originals.Size();i++){auto raw=originals.GetObjectAt(i);if(!flag(raw,L"present"))continue;auto bytes=preferenceBytes(raw);
   if(id==L"windowedOptimizations"){
    if(num(raw,L"type")!=REG_SZ||bytes.size()%sizeof(wchar_t))throw hresult_error(E_FAIL,L"图形偏好格式无效，未修改");
    std::wstring text((wchar_t*)bytes.data(),bytes.size()/sizeof(wchar_t));if(!text.empty()&&text.back()==0)text.pop_back();auto state=tokenPreference(text,L"SwapEffectUpgradeEnable");if(state==-2)throw hresult_error(E_FAIL,L"图形偏好存在重复或无效值");if(state>=0){known=true;enabled=state!=0;}put(result,L"note",std::wstring(L"控制窗口化游戏优化，保留可变刷新率及单个应用的图形偏好；重新启动游戏后生效。"));put(result,L"name",std::wstring(L"窗口化游戏优化"));break;
   }
   if(num(raw,L"type")!=REG_DWORD||bytes.size()!=sizeof(DWORD))throw hresult_error(E_FAIL,L"系统偏好类型无效，未修改");DWORD number{};memcpy(&number,bytes.data(),sizeof(number));
   if(id==L"gpuScheduling"){if(number!=1&&number!=2)continue;put(result,L"runtimeEnabled",flag(result,L"enabled"));enabled=number==2;known=true;break;}
   if(number>1)throw hresult_error(E_FAIL,L"系统偏好值无效，未修改");enabled=number!=0;known=true;break;
  }
  if(id==L"gpuScheduling"){if(!known&&flag(result,L"confirmed")){enabled=flag(result,L"enabled");known=true;}put(result,L"requiresElevation",true);put(result,L"note",std::wstring(L"更改需要管理员授权；保存后重启 Windows 生效。"));}
  if(registryPrefix.empty()&&id==L"capture"&&str(result,L"source")==L"Windows 录制策略")writable=false;
  if(registryPrefix.empty()&&id==L"notifications"&&str(result,L"source")==L"Windows 通知策略")writable=false;
  if(id==L"notifications"){put(result,L"name",std::wstring(L"系统通知"));put(result,L"note",std::wstring(L"控制通知总开关；免打扰与自动规则独立管理。保存后可能需要重新登录才能生效。"));}
  if(id==L"windowedOptimizations"){put(result,L"name",std::wstring(L"窗口化游戏优化"));put(result,L"note",std::wstring(L"控制窗口化游戏优化，保留可变刷新率和单个应用的设置；重新启动游戏后生效。"));}
  put(result,L"available",writable);if(known){put(result,L"enabled",enabled);put(result,L"confirmed",true);}
  if(!writable)put(result,L"status",std::wstring(id==L"gpuScheduling"?L"当前硬件或驱动不支持 GPU 调度":L"由组织策略锁定，无法直接修改"));
  else if(!known){put(result,L"confirmed",false);result.SetNamedValue(L"enabled",JsonValue::CreateNullValue());put(result,L"status",std::wstring(L"使用系统默认值；可直接选择开启或关闭"));}
  if(id==L"gpuScheduling"&&flag(result,L"restartPending"))put(result,L"status",std::wstring(L"设置已保存，重启 Windows 后生效"));return result;
 }
 void writePreference(std::wstring const& id,JsonObject desired){
  auto values=preferenceValues(id);auto backups=desired.GetNamedArray(L"registryValues",JsonArray{});
  if(backups.Size()){if(backups.Size()!=values.size())throw hresult_invalid_argument();for(uint32_t i=0;i<backups.Size();i++)savePreference(values[i],backups.GetObjectAt(i));return;}
  if(id==L"windowedOptimizations"){
   auto raw=preferenceBackup(values[0]);std::wstring text;if(flag(raw,L"present")){auto bytes=preferenceBytes(raw);if(num(raw,L"type")!=REG_SZ||bytes.size()%sizeof(wchar_t))throw hresult_error(E_FAIL,L"图形偏好类型无效");text.assign((wchar_t*)bytes.data(),bytes.size()/sizeof(wchar_t));if(!text.empty()&&text.back()==0)text.pop_back();}
   text=replacePreferenceToken(text,L"SwapEffectUpgradeEnable",flag(desired,L"enabled"));savePreference(values[0],storedValue(REG_SZ,text.c_str(),(text.size()+1)*sizeof(wchar_t)));
  }else{DWORD number=id==L"gpuScheduling"?(flag(desired,L"enabled")?2:1):(flag(desired,L"enabled")?1:0);for(auto const& value:values)savePreference(value,storedValue(REG_DWORD,&number,sizeof(number)));}
  DWORD_PTR response{};if(registryPrefix.empty())SendMessageTimeoutW(HWND_BROADCAST,WM_SETTINGCHANGE,0,reinterpret_cast<LPARAM>(L"WindowsSettings"),SMTO_ABORTIFHUNG,100,&response);
 }
 JsonObject readOption(std::wstring const& id){if(id==L"systemSound"||id==L"hdr"){auto o=id==L"hdr"?hdrOption():soundOption();put(o,L"canUndo",undo.HasKey(id));return o;}if(!preferenceValues(id).empty())return writablePreference(id);auto e=entry(id);JsonObject result;put(result,L"id",id);put(result,L"name",e.name);put(result,L"note",e.note);put(result,L"uri",e.uri);put(result,L"available",true);put(result,L"canUndo",undo.HasKey(id));result.SetNamedValue(L"enabled",JsonValue::CreateNullValue());
  if(!e.path.empty()){DWORD type{},value{},bytes=sizeof(value);auto status=RegGetValueW(HKEY_CURRENT_USER,regPath(e).c_str(),e.value.c_str(),RRF_RT_REG_DWORD,&type,&value,&bytes);if(status==ERROR_FILE_NOT_FOUND){put(result,L"present",false);put(result,L"status",std::wstring(L"未显式设置 · 由 Windows 默认值管理"));}else if(status!=ERROR_SUCCESS||value>1){put(result,L"available",false);put(result,L"status",std::wstring(L"无法读取有效的 Windows 偏好，请使用原生设置"));}else{put(result,L"present",true);put(result,L"raw",(double)value);put(result,L"enabled",value!=0);put(result,L"status",std::wstring(L"已读取 Windows 保存的偏好"));}put(result,L"confirmed",result.GetNamedValue(L"enabled").ValueType()==JsonValueType::Boolean&&flag(result,L"available"));return result;}
  bool ok=false,enabled=false;DWORD flags{};
  if(id==L"mousePrecision"){int values[3]{};ok=spi(SPI_GETMOUSE,0,values,0)!=FALSE;if(ok){enabled=values[2]!=0;JsonArray a;for(int n:values)a.Append(JsonValue::CreateNumberValue(n));result.SetNamedValue(L"mouse",a);}}
  else if(id==L"stickyShortcut"){STICKYKEYS s{sizeof(s)};ok=spi(SPI_GETSTICKYKEYS,sizeof(s),&s,0)!=FALSE;flags=s.dwFlags;enabled=(flags&SKF_HOTKEYACTIVE)!=0;}
  else if(id==L"filterShortcut"){FILTERKEYS s{sizeof(s)};ok=spi(SPI_GETFILTERKEYS,sizeof(s),&s,0)!=FALSE;flags=s.dwFlags;enabled=(flags&FKF_HOTKEYACTIVE)!=0;}
  else if(id==L"toggleShortcut"){TOGGLEKEYS s{sizeof(s)};ok=spi(SPI_GETTOGGLEKEYS,sizeof(s),&s,0)!=FALSE;flags=s.dwFlags;enabled=(flags&TKF_HOTKEYACTIVE)!=0;}
  put(result,L"available",ok);if(ok){put(result,L"enabled",enabled);put(result,L"flags",(double)flags);}put(result,L"confirmed",ok);put(result,L"status",ok?std::wstring(L"已读取 Windows 当前设置"):std::wstring(L"Windows 接口不可用"));return result;
 }
 static std::wstring guidString(GUID const& id){wchar_t value[64]{};StringFromGUID2(id,value,64);return value;}
 static std::wstring activePower(){GUID* id{};if(PowerGetActiveScheme(nullptr,&id)!=ERROR_SUCCESS)return L"";auto value=guidString(*id);LocalFree(id);return value;}
 static JsonArray powerPlans(){JsonArray plans;for(ULONG i=0;;i++){GUID id{};DWORD bytes=sizeof(id);auto result=PowerEnumerate(nullptr,nullptr,nullptr,ACCESS_SCHEME,i,(UCHAR*)&id,&bytes);if(result==ERROR_NO_MORE_ITEMS)break;if(result!=ERROR_SUCCESS)break;DWORD length{};PowerReadFriendlyName(nullptr,&id,nullptr,nullptr,nullptr,&length);std::wstring name=L"Windows 电源计划";if(length>0&&length<65536){std::vector<unsigned char> data(length);if(PowerReadFriendlyName(nullptr,&id,nullptr,nullptr,data.data(),&length)==ERROR_SUCCESS)name=(wchar_t*)data.data();}JsonObject p;put(p,L"id",guidString(id));put(p,L"name",name);plans.Append(p);}return plans;}
 JsonObject snapshot(){JsonObject result;JsonArray options;for(auto const& e:entries())options.Append(readOption(e.id));options.Append(readOption(L"systemSound"));options.Append(readOption(L"hdr"));for(auto id:{L"capture",L"gpuScheduling",L"windowedOptimizations",L"notifications"})options.Append(readOption(id));result.SetNamedValue(L"options",options);result.SetNamedValue(L"powerPlans",powerPlans());put(result,L"activePower",activePower());put(result,L"powerCanUndo",undo.HasKey(L"power"));put(result,L"real",true);put(result,L"sampleTime",clockSeconds());return result;}
 static DWORD hotkeyFlags(DWORD original,DWORD mask,bool enabled){return enabled?original|mask:original&~mask;}
 void writeOption(std::wstring const& id,JsonObject const& desired){if(id==L"systemSound"){setSound(desired);return;}if(id==L"hdr"){setHdr(desired);return;}if(!preferenceValues(id).empty()){writePreference(id,desired);return;}auto e=entry(id);
  if(!e.path.empty()){if(!flag(desired,L"present",true)){HKEY key{};auto status=RegOpenKeyExW(HKEY_CURRENT_USER,regPath(e).c_str(),0,KEY_SET_VALUE,&key);if(status==ERROR_FILE_NOT_FOUND)return;if(status!=ERROR_SUCCESS)throw hresult_error(HRESULT_FROM_WIN32(status));status=RegDeleteValueW(key,e.value.c_str());RegCloseKey(key);if(status!=ERROR_SUCCESS&&status!=ERROR_FILE_NOT_FOUND)throw hresult_error(HRESULT_FROM_WIN32(status));return;}
   HKEY key{};auto status=RegCreateKeyExW(HKEY_CURRENT_USER,regPath(e).c_str(),0,nullptr,0,KEY_SET_VALUE,nullptr,&key,nullptr);if(status!=ERROR_SUCCESS)throw hresult_error(HRESULT_FROM_WIN32(status));DWORD value=flag(desired,L"enabled")?1:0;status=RegSetValueExW(key,e.value.c_str(),0,REG_DWORD,(BYTE*)&value,sizeof(value));RegCloseKey(key);if(status!=ERROR_SUCCESS)throw hresult_error(HRESULT_FROM_WIN32(status));return;
  }
  bool enabled=flag(desired,L"enabled");BOOL ok=FALSE;UINT save=SPIF_UPDATEINIFILE|SPIF_SENDCHANGE;
  if(id==L"mousePrecision"){int values[3]{};if(!spi(SPI_GETMOUSE,0,values,0))throw hresult_error(E_FAIL,L"读取鼠标参数失败");auto previous=desired.GetNamedArray(L"mouse",JsonArray{});values[2]=previous.Size()==3?(int)previous.GetNumberAt(2):enabled?1:0;ok=spi(SPI_SETMOUSE,0,values,save);}
  else if(id==L"stickyShortcut"){STICKYKEYS s{sizeof(s)};if(!spi(SPI_GETSTICKYKEYS,sizeof(s),&s,0))throw hresult_error(E_FAIL);s.dwFlags=hotkeyFlags(s.dwFlags,SKF_HOTKEYACTIVE,enabled);ok=spi(SPI_SETSTICKYKEYS,sizeof(s),&s,save);}
  else if(id==L"filterShortcut"){FILTERKEYS s{sizeof(s)};if(!spi(SPI_GETFILTERKEYS,sizeof(s),&s,0))throw hresult_error(E_FAIL);s.dwFlags=hotkeyFlags(s.dwFlags,FKF_HOTKEYACTIVE,enabled);ok=spi(SPI_SETFILTERKEYS,sizeof(s),&s,save);}
  else if(id==L"toggleShortcut"){TOGGLEKEYS s{sizeof(s)};if(!spi(SPI_GETTOGGLEKEYS,sizeof(s),&s,0))throw hresult_error(E_FAIL);s.dwFlags=hotkeyFlags(s.dwFlags,TKF_HOTKEYACTIVE,enabled);ok=spi(SPI_SETTOGGLEKEYS,sizeof(s),&s,save);}
  if(!ok)throw hresult_error(E_FAIL,L"Windows 拒绝了设置，请在原生设置中检查权限或策略");
 }
 void set(std::wstring const& id,bool enabled){auto before=readOption(id);if(!flag(before,L"available"))throw hresult_error(E_FAIL,L"该设置当前不可写，请使用 Windows 原生设置");auto previous=object(std::wstring(undo.Stringify()));undo.SetNamedValue(id,before);write(file,std::wstring(undo.Stringify()));JsonObject desired=(id==L"systemSound"||id==L"hdr")?object(std::wstring(before.Stringify())):JsonObject{};put(desired,L"enabled",enabled);try{writeOption(id,desired);auto actual=readOption(id);if(!flag(actual,L"available")||actual.GetNamedValue(L"enabled").ValueType()!=JsonValueType::Boolean||flag(actual,L"enabled")!=enabled)throw hresult_error(E_FAIL,L"设置未通过回读验证");}catch(...){try{writeOption(id,before);undo=previous;write(file,std::wstring(undo.Stringify()));}catch(...){}throw;}}
 void setPower(std::wstring const& requested){GUID id{};if(FAILED(CLSIDFromString(requested.c_str(),&id)))throw hresult_invalid_argument(L"无效电源计划");bool found=false;auto plans=powerPlans();for(auto const& v:plans){GUID existing{};if(SUCCEEDED(CLSIDFromString(str(v.GetObject(),L"id").c_str(),&existing))&&IsEqualGUID(existing,id)){found=true;break;}}if(!found)throw hresult_invalid_argument(L"该电源计划不存在");auto old=activePower();if(old.empty())throw hresult_error(E_FAIL,L"无法读取当前电源计划");auto priorUndo=object(std::wstring(undo.Stringify()));JsonObject before;put(before,L"id",old);undo.SetNamedValue(L"power",before);write(file,std::wstring(undo.Stringify()));try{auto status=PowerSetActiveScheme(nullptr,&id);if(status!=ERROR_SUCCESS)throw hresult_error(HRESULT_FROM_WIN32(status));GUID actual{};if(FAILED(CLSIDFromString(activePower().c_str(),&actual))||!IsEqualGUID(actual,id))throw hresult_error(E_FAIL,L"电源计划未通过回读验证");}catch(...){GUID original{};if(SUCCEEDED(CLSIDFromString(old.c_str(),&original)))PowerSetActiveScheme(nullptr,&original);undo=priorUndo;try{write(file,std::wstring(undo.Stringify()));}catch(...){}throw;}}
 void restore(std::wstring const& id){if(!undo.HasKey(id))throw hresult_invalid_argument(L"没有该设置的恢复记录");auto before=undo.GetNamedObject(id);if(id==L"power"){GUID plan{};if(FAILED(CLSIDFromString(str(before,L"id").c_str(),&plan)))throw hresult_invalid_argument();auto status=PowerSetActiveScheme(nullptr,&plan);if(status!=ERROR_SUCCESS)throw hresult_error(HRESULT_FROM_WIN32(status));GUID actual{};if(FAILED(CLSIDFromString(activePower().c_str(),&actual))||!IsEqualGUID(actual,plan))throw hresult_error(E_FAIL,L"电源计划恢复未通过回读验证");}else{writeOption(id,before);auto actual=readOption(id);bool absent=!flag(before,L"present",true);bool registryRestored=before.HasKey(L"registryValues")&&std::wstring(before.GetNamedArray(L"registryValues").Stringify())==std::wstring(actual.GetNamedArray(L"registryValues").Stringify());if(before.HasKey(L"registryValues")?!registryRestored:(!flag(actual,L"available")||(absent?flag(actual,L"present"):flag(actual,L"enabled")!=flag(before,L"enabled"))))throw hresult_error(E_FAIL,L"恢复未通过回读验证");}undo.Remove(id);write(file,std::wstring(undo.Stringify()));}
};
}
