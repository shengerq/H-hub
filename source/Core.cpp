#include "Common.h"
#include "Localization.h"
#include "UiInstance.h"
#include "TrayBattery.h"
#include "Monitor.h"
#include "SensorAuthorizationCheck.h"
#include "MonitoringDemand.h"
#include "HomeMonitoring.h"

#include "DeviceReader.h"
#include "HidTransaction.h"
#include "PluginHid.h"
#include "PluginHidV2.h"
#include "LegacyProfileIdentity.h"
#include "PluginPackage.h"










#include "Applications.h"
#include "Updates.h"
#include <cassert>
#include <functional>
#include <sstream>
#include <limits>

using namespace hub;
static UINT const trayMessage=WM_APP+1;
static bool alive=true;static bool deviceTopologyDirty=false;
static NOTIFYICONDATAW tray{};static bool foregroundDirty=true;static void CALLBACK ForegroundEvent(HWINEVENTHOOK,DWORD,HWND,LONG,LONG,DWORD,DWORD){foregroundDirty=true;PostMessageW(tray.hWnd,WM_APP+2,0,0);}
static std::filesystem::path uiPath;
static void openUI(){if(auto window=findUiWindow()){restoreUiWindow(window);return;}std::wstring line=L"\""+uiPath.wstring()+L"\"";STARTUPINFOW si{sizeof(si)};PROCESS_INFORMATION pi{};if(CreateProcessW(uiPath.c_str(),line.data(),nullptr,nullptr,FALSE,0,nullptr,uiPath.parent_path().c_str(),&si,&pi)){CloseHandle(pi.hThread);CloseHandle(pi.hProcess);}}
static void ensurePluginWorkerHost(){if(findUiWindow())return;static ULONGLONG launched=0;if(GetTickCount64()-launched<10000)return;launched=GetTickCount64();std::wstring line=L"\""+uiPath.wstring()+L"\" --background-plugins";STARTUPINFOW si{sizeof(si)};si.dwFlags=STARTF_USESHOWWINDOW;si.wShowWindow=SW_HIDE;PROCESS_INFORMATION pi{};if(CreateProcessW(uiPath.c_str(),line.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,uiPath.parent_path().c_str(),&si,&pi)){CloseHandle(pi.hThread);CloseHandle(pi.hProcess);}}
static bool trayEnglish=false;
static LRESULT CALLBACK TrayProc(HWND w,UINT m,WPARAM wp,LPARAM lp){if(m==WM_DEVICECHANGE){deviceTopologyDirty=true;return 0;}if(m==trayMessage){if(LOWORD(lp)==WM_LBUTTONDBLCLK)openUI();if(LOWORD(lp)==WM_RBUTTONUP){POINT p{};GetCursorPos(&p);auto menu=CreatePopupMenu();AppendMenuW(menu,MF_STRING,1,trayEnglish?L"Open H hub":L"打开 H hub");AppendMenuW(menu,MF_STRING,2,trayEnglish?L"Exit H hub":L"完全退出");SetForegroundWindow(w);auto selected=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_NONOTIFY,p.x,p.y,0,w,nullptr);DestroyMenu(menu);if(selected==1)openUI();if(selected==2)alive=false;}return 0;}if(m==WM_DESTROY){alive=false;return 0;}return DefWindowProcW(w,m,wp,lp);}

static hub::PluginHidV2 nativeHid;
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR,int){initializeProcessIdentity();winrt::init_apartment(winrt::apartment_type::multi_threaded);int argc{};auto args=CommandLineToArgvW(GetCommandLineW(),&argc);if(argc>=2&&std::wstring(args[1])==L"--verify-sensor-authorization"){LocalFree(args);return verifySensorAuthorization();}if(argc>=2&&std::wstring(args[1])==L"--shutdown"){LocalFree(args);auto client=CreateFileW(pipeName().c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr);if(client==INVALID_HANDLE_VALUE)return 0;DWORD mode=PIPE_READMODE_BYTE|PIPE_NOWAIT;SetNamedPipeHandleState(client,&mode,nullptr,nullptr);std::string response;bool ok=sendFrame(client,"{\"op\":\"quit\",\"body\":{}}")&&receiveFrame(client,response);HANDLE core{};if(ok){try{auto reply=object(std::wstring(to_hstring(response)));core=OpenProcess(SYNCHRONIZE,FALSE,(DWORD)num(reply.GetNamedObject(L"state"),L"corePid"));}catch(...){}BYTE ack=1;transfer(client,&ack,1,true);}CloseHandle(client);if(core){WaitForSingleObject(core,6000);CloseHandle(core);}return ok?0:1;}LocalFree(args);
 HANDLE mutex=CreateMutexW(nullptr,FALSE,(L"Local\\HhubPrototype.Core."+userSid()+instanceSuffix()).c_str());if(GetLastError()==ERROR_ALREADY_EXISTS){CloseHandle(mutex);return 0;}
 auto sddl=L"D:P(A;;GA;;;"+userSid()+L")";PSECURITY_DESCRIPTOR sd{};ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(),SDDL_REVISION_1,&sd,nullptr);SECURITY_ATTRIBUTES sa{sizeof(sa),sd,FALSE};auto pipe=CreateNamedPipeW(pipeName().c_str(),PIPE_ACCESS_DUPLEX|FILE_FLAG_FIRST_PIPE_INSTANCE|FILE_FLAG_OVERLAPPED,PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_REJECT_REMOTE_CLIENTS,1,1048576,1048576,0,&sa);LocalFree(sd);if(pipe==INVALID_HANDLE_VALUE){CloseHandle(mutex);return 2;}
 uiPath=binaryDir()/L"Hhub.exe";WNDCLASSW wc{};wc.hInstance=instance;wc.lpfnWndProc=TrayProc;wc.lpszClassName=L"HhubPrototype.Tray";RegisterClassW(&wc);auto window=CreateWindowExW(0,wc.lpszClassName,L"H hub 1.0 内置功能版 Core",0,0,0,0,0,nullptr,nullptr,instance,nullptr);tray.cbSize=sizeof(tray);tray.hWnd=window;tray.uID=1;tray.uFlags=NIF_ICON|NIF_TIP|NIF_MESSAGE;tray.uCallbackMessage=trayMessage;tray.hIcon=reinterpret_cast<HICON>(LoadImageW(instance,MAKEINTRESOURCEW(101),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_SHARED));wcscpy_s(tray.szTip,L"H hub 1.0 内置功能版");Shell_NotifyIconW(NIM_ADD,&tray);
 Model model(dataDir()/L"state.json",true);Monitor monitor;MonitoringDemand monitoringDemand;DeviceReader deviceReader;Applications applications;Updates updates;HANDLE acceptEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);OVERLAPPED accept{};accept.hEvent=acceptEvent;
 put(model.state.GetNamedObject(L"settings"),L"startup",Lifecycle::startup());auto hook=SetWinEventHook(EVENT_SYSTEM_FOREGROUND,EVENT_SYSTEM_FOREGROUND,nullptr,ForegroundEvent,0,0,WINEVENT_OUTOFCONTEXT|WINEVENT_SKIPOWNPROCESS);auto detect=[&]{if(foregroundDirty&&flag(model.state.GetNamedObject(L"settings"),L"appAutoDetect",true)){foregroundDirty=false;auto path=foregroundApplication();if(path!=str(model.state,L"frontendApp")){JsonObject req;put(req,L"op",std::wstring(L"foregroundApp"));JsonObject b;put(b,L"path",path);req.SetNamedValue(L"body",b);model.request(req);}}};auto listen=[&]{ResetEvent(acceptEvent);BOOL ok=ConnectNamedPipe(pipe,&accept);if(ok||GetLastError()==ERROR_PIPE_CONNECTED)SetEvent(acceptEvent);};listen();MSG msg{};TrayBatteryTip batteryTip(window);ULONGLONG batteryRefresh{};
auto monitoringUiVisible=[](){auto ui=findUiWindow();return ui&&IsWindowVisible(ui)&&!IsIconic(ui);};
auto releaseMonitoring=[&]{monitoringDemand.stop();monitor.stopSensors(true);for(auto key:{L"monitor",L"homeMonitoring"})if(model.state.HasKey(key))model.state.Remove(key);};
std::function<JsonObject(JsonObject,unsigned)> dispatch;
dispatch=[&](JsonObject req,unsigned depth)->JsonObject{
 if(depth>8)throw hresult_invalid_argument(L"插件调用层级过深");auto backup=std::wstring(model.state.Stringify());JsonObject reply;
 try{
  auto op=str(req,L"op");auto body=req.GetNamedObject(L"body",JsonObject{});
  auto owner=pluginOperationOwner(model.state,op);
  if(!owner.empty()){auto plugin=pluginRecord(model.state,owner);invokePluginIntegration(model.state,plugin,L"request",op,body);model.save();put(reply,L"ok",true);reply.SetNamedValue(L"state",model.state);return reply;}
  if(op==L"pluginInvoke"){
   auto id=str(body,L"id");auto plugin=pluginRecord(model.state,id);if(!plugin||!integrationEnabled(plugin))throw hresult_error(E_ACCESSDENIED,L"插件未运行或已禁用");auto operation=str(body,L"operation");if(!declarationContains(plugin.GetNamedArray(L"capabilities",JsonArray{}),operation))throw hresult_error(E_ACCESSDENIED,L"插件未声明该能力");
   JsonObject request;put(request,L"op",operation);request.SetNamedValue(L"body",body.GetNamedObject(L"body",JsonObject{}));request.SetNamedValue(L"state",model.state);auto& module=nativeModule(id);module.load(id);auto result=module.invoke(request);if(result.HasKey(L"state"))mergePluginState(model.state,plugin,result.GetNamedObject(L"state"));auto actions=result.GetNamedArray(L"hostRequests",JsonArray{});if(actions.Size()>16)throw hresult_invalid_argument(L"插件请求过多");
   std::set<std::wstring> safe{L"applyScene",L"unlockScene",L"deviceProfileSelect",L"deviceProfileRetry",L"pluginTaskSubmit"};auto grants=plugin.GetNamedArray(L"hostOperations",JsonArray{});
   for(auto const& v:actions){auto action=v.GetObject();auto name=str(action,L"op");if((!safe.count(name)&&pluginOperationOwner(model.state,name).empty())||!declarationContains(grants,name))throw hresult_error(E_ACCESSDENIED,L"插件未声明此宿主操作");if(name==L"pluginTaskSubmit")put(action.GetNamedObject(L"body"),L"provider",id);auto answer=dispatch(action,depth+1);if(!flag(answer,L"ok"))throw hresult_error(E_FAIL,str(answer,L"error").c_str());}
   JsonObject data;data.SetNamedValue(L"pluginResult",result);put(reply,L"ok",true);reply.SetNamedValue(L"state",data);model.save();return reply;
  }
  observePluginIntegrations(model.state,L"before",op,body);
  if(op==L"pluginRemove"||op==L"pluginUpdate"||op==L"pluginStop"||(op==L"pluginMode"&&str(body,L"mode")==L"disabled")){
   auto id=str(body,L"id");auto plugin=pluginRecord(model.state,id);if(plugin&&hasNativeModule(plugin)){auto lifecycle=plugin.GetNamedObject(L"lifecycle",JsonObject{});auto cleanup=str(lifecycle,op==L"pluginRemove"?L"uninstall":L"stop");if(!cleanup.empty()){JsonObject request;put(request,L"op",cleanup);request.SetNamedValue(L"state",model.state);auto& module=nativeModule(id);module.load(id,true,true);auto result=module.invoke(request);if(result.HasKey(L"state"))mergePluginState(model.state,plugin,result.GetNamedObject(L"state"));}forceStopPlugin(id);nativeModule(id).unload();}
  }
  if(op==L"pluginForceStop"){auto id=str(body,L"id");forceStopPlugin(id);nativeModule(id).unload();nativeHid.sessions.erase(id);auto plugin=find(model.state.GetNamedArray(L"plugins"),id);put(plugin,L"forceStopped",true);put(plugin,L"leaseUntil",0.0);}
  if(op==L"importPlugin"){auto manifest=body.GetNamedObject(L"manifest");validatePluginUi(manifest);validatePluginModule(manifest);auto id=str(manifest,L"id");if(auto prior=pluginRecord(model.state,id);prior&&hasNativeModule(prior)){forceStopPlugin(id);nativeModule(id).unload();}}
  if(op==L"settings"){if(body.HasKey(L"startup"))Lifecycle::startup(flag(body,L"startup"));if(body.HasKey(L"appAutoDetect"))foregroundDirty=true;}
  if(op==L"applicationScan"||op==L"open")model.state.SetNamedValue(L"applications",applications.scan(op==L"applicationScan"));
  auto settings=model.state.GetNamedObject(L"settings");if(op==L"checkUpdates")updates.start(model.state);else if(op==L"open"&&(flag(settings,L"autoUpdate",true)||flag(settings,L"appAutoUpdate",true)))try{updates.start(model.state);}catch(hresult_error const& e){updates.startupError=std::wstring(e.message());}
  if(op==L"updateState"||op==L"checkUpdates"||op==L"open"||op==L"pluginUpdate")model.state.SetNamedValue(L"updates",updates.snapshot(model.state));
  if(op==L"pluginUpdate"){updates.applyPlugin(model.state,str(body,L"id"));registerNativeManifests(model.state);initializePluginIntegration(model.state,find(model.state.GetNamedArray(L"plugins"),str(body,L"id")));model.save();}
  if(op==L"deviceProfileValidate"||op==L"deviceProfileManual"){auto p=find(model.state.GetNamedArray(L"deviceProfiles"),str(body,L"profile"));if(str(p,L"plugin")!=str(body,L"plugin"))throw hresult_invalid_argument();auto payload=p.GetNamedObject(L"payload");if(num(payload,L"transportVersion")==1)validateLegacyProfileDevice(deviceReader.scan(true),find(model.state.GetNamedArray(L"profileDevices"),str(p,L"device")),find(model.state.GetNamedArray(L"plugins"),str(p,L"plugin")));}if(op==L"deviceProfileCapture"){auto p=find(model.state.GetNamedArray(L"deviceProfiles"),str(body,L"profile"));auto d=find(model.state.GetNamedArray(L"profileDevices"),str(p,L"device"));auto payload=body.GetNamedObject(L"payload");if(num(payload,L"transportVersion")==1){validateLegacyProfileDevice(deviceReader.scan(true),d,find(model.state.GetNamedArray(L"plugins"),str(p,L"plugin")));put(payload,L"containerId",str(d,L"containerId"));}else{auto owner=nativeHid.sessions.find(str(body,L"plugin"));if(owner==nativeHid.sessions.end())throw hresult_invalid_argument(L"请先连接外设");auto device=owner->second.find(str(payload,L"transportKey"));if(device==owner->second.end())throw hresult_invalid_argument(L"配置接口不属于此插件");auto info=device->second->info;if(str(d,L"containerId").empty()||str(info,L"containerId")!=str(d,L"containerId"))throw hresult_invalid_argument(L"当前连接不是配置所属的外设");}}
  if(op==L"pluginTaskSubmit")ensurePluginWorkerHost();
  JsonObject runtime;for(auto const& v:model.state.GetNamedArray(L"plugins")){auto p=v.GetObject();if(!flag(p,L"installed")||!hasNativeModule(p))continue;JsonObject status;put(status,L"processes",(double)pluginProcessCount(str(p,L"id")));put(status,L"forcedStop",flag(p,L"forceStopped"));put(status,L"error",pluginFailures()[str(p,L"id")]);runtime.SetNamedValue(str(p,L"id"),status);}model.state.SetNamedValue(L"pluginRuntime",runtime);put(model.state,L"coreMemoryMB",workingSet());put(model.state,L"corePid",(double)GetCurrentProcessId());
  if(op==L"monitorElevate"){if(monitoringUiVisible()){monitoringDemand.request(GetTickCount64());monitor.startElevated();model.state.SetNamedValue(L"monitor",monitor.sample());}else releaseMonitoring();}
  if(op==L"monitorPause"||op==L"monitorStop")releaseMonitoring();

  if(op==L"open"||op==L"snapshot"||op==L"monitor"||op==L"deviceScan"||op==L"deviceSnapshot")model.state.SetNamedValue(L"detected",deviceReader.scan(op==L"deviceScan"||std::exchange(deviceTopologyDirty,false)));nativeHid.decorate(model.state.GetNamedObject(L"detected",JsonObject{}));
  if(op==L"monitor"){if(monitoringUiVisible()){monitoringDemand.request(GetTickCount64());model.state.SetNamedValue(L"monitor",monitor.sample(true));}else releaseMonitoring();}
  if(op==L"pluginHidV2"||op==L"pluginHid"){auto transport=op==L"pluginHidV2"?nativeHid.transact(model.state,body):pluginHidTransaction(model.state,body);JsonObject data;data.SetNamedValue(L"pluginHidResult",transport);put(reply,L"ok",true);reply.SetNamedValue(L"state",data);}
  else{if(monitoringDemand.active&&(op==L"monitor"||op==L"monitorElevate"))model.state.SetNamedValue(L"homeMonitoring",HomeMonitoring::build(model.state,clockSeconds()));reply=model.request(req);if(op==L"pluginStart"){auto p=find(model.state.GetNamedArray(L"plugins"),str(body,L"id"));auto startup=str(p.GetNamedObject(L"lifecycle",JsonObject{}),L"startup");if(!startup.empty()){auto& module=nativeModule(str(p,L"id"));module.load(str(p,L"id"));JsonObject request;put(request,L"op",startup);request.SetNamedValue(L"state",model.state);auto result=module.invoke(request);if(result.HasKey(L"state"))mergePluginState(model.state,p,result.GetNamedObject(L"state"));}}}
  observePluginIntegrations(model.state,L"after",op,body);
 }catch(hresult_error const& e){model.state=object(backup);put(reply,L"ok",false);put(reply,L"error",std::wstring(e.message()));}catch(...){model.state=object(backup);reply=object(L"{\"ok\":false,\"error\":\"请求处理失败\"}");}return reply;
};

for(auto const& v:model.state.GetNamedArray(L"plugins")){auto p=v.GetObject();if(!hasNativeModule(p)||!flag(p,L"installed")||flag(p,L"forceStopped")||(str(p,L"mode")!=L"auto"&&str(p,L"mode")!=L"resident"))continue;auto operation=str(p.GetNamedObject(L"lifecycle",JsonObject{}),L"startup");if(operation.empty())continue;try{auto& module=nativeModule(str(p,L"id"));module.load(str(p,L"id"));JsonObject request;put(request,L"op",operation);request.SetNamedValue(L"state",model.state);auto reply=module.invoke(request);if(reply.HasKey(L"state"))mergePluginState(model.state,p,reply.GetNamedObject(L"state"));}catch(hresult_error const& e){pluginFailures()[str(p,L"id")]=std::wstring(e.message());}}
 while(alive){collectNativeModules();if(monitoringDemand.expired(GetTickCount64())||(!monitoringUiVisible()&&monitoringDemand.active))releaseMonitoring();trayEnglish=resolveLanguage(str(model.state.GetNamedObject(L"settings"),L"language",L"system"))==L"en-US";tickPluginIntegrations(model.state);detect();if(GetTickCount64()-batteryRefresh>=2000){batteryRefresh=GetTickCount64();model.state.SetNamedValue(L"detected",deviceReader.scan(std::exchange(deviceTopologyDirty,false)));nativeHid.decorate(model.state.GetNamedObject(L"detected",JsonObject{}));}model.refreshProfileConnections();batteryTip.update(tray,model.state.GetNamedObject(L"detected",JsonObject{}),model.state.GetNamedArray(L"plugins",JsonArray{}));auto wait=MsgWaitForMultipleObjects(1,&acceptEvent,FALSE,200,QS_ALLINPUT);if(wait==WAIT_TIMEOUT)continue;if(wait==WAIT_OBJECT_0+1){while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);}continue;}if(wait!=WAIT_OBJECT_0)break;
  nativeHid.revoke(model.state);std::string input;bool quit=false;if(serverReceive(pipe,input)){
 JsonObject fastRequest{nullptr};try{fastRequest=object(std::wstring(to_hstring(input)));}catch(...){}
 if(fastRequest&&(str(fastRequest,L"op")==L"pluginHid"||str(fastRequest,L"op")==L"pluginHidV2")){
  JsonObject reply;try{auto body=fastRequest.GetNamedObject(L"body");JsonObject result;
   if(str(fastRequest,L"op")==L"pluginHidV2")result=nativeHid.transact(model.state,body);
   else{auto request=body.GetNamedObject(L"request");result=pluginHidTransaction(model.state,body);}
   JsonObject data;data.SetNamedValue(L"pluginHidResult",result);put(reply,L"ok",true);reply.SetNamedValue(L"state",data);
  }catch(hresult_error const& error){put(reply,L"ok",false);put(reply,L"error",std::wstring(error.message()));}catch(...){put(reply,L"ok",false);put(reply,L"error",std::wstring(L"设备通信失败"));}
  serverSend(pipe,to_string(reply.Stringify()));DisconnectNamedPipe(pipe);listen();continue;
 }
 JsonObject reply;try{if(!fastRequest)throw hresult_invalid_argument();quit=str(fastRequest,L"op")==L"quit";reply=dispatch(fastRequest,0);}catch(...){reply=object(LR"({"ok":false,"error":"请求格式无效"})");}if(quit)put(reply,L"profileDirectory",dataDir().wstring());serverSend(pipe,to_string(reply.Stringify()));}DisconnectNamedPipe(pipe);if(quit)alive=false;else listen();
 }
 for(auto const& v:model.state.GetNamedArray(L"plugins")){auto p=v.GetObject();auto op=str(p.GetNamedObject(L"lifecycle",JsonObject{}),L"shutdown");if(op.empty()||!nativeModule(str(p,L"id")).library)continue;try{JsonObject request;put(request,L"op",op);request.SetNamedValue(L"state",model.state);nativeModule(str(p,L"id")).invoke(request);}catch(...) {}}
 if(hook)UnhookWinEvent(hook);CancelIoEx(pipe,nullptr);model.save();Shell_NotifyIconW(NIM_DELETE,&tray);CloseHandle(pipe);CloseHandle(acceptEvent);DestroyWindow(window);CloseHandle(mutex);return 0;
}






















