#pragma once
#include "PhoneIdentity.h"
namespace hub {
#include "PhoneDeviceSupport.inc"
#include "PhoneProjection.inc"
inline std::filesystem::path phoneModuleFolder(){HMODULE module{};check_bool(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&phoneModuleFolder),&module));wchar_t file[32768]{};check_bool(GetModuleFileNameW(module,file,32768)!=0);return std::filesystem::path(file).parent_path();}
struct PhoneBackend {
 HANDLE process{},job{};JsonObject config;std::mutex mutex;
 PhoneBackend(){auto file=dataDir()/L"phone"/L"settings.json";try{config=object(read(file));}catch(...){config=JsonObject{};} }
 ~PhoneBackend(){shutdown();}
 void save(){write(dataDir()/L"phone"/L"settings.json",std::wstring(config.Stringify()));}
 bool alive(){return process&&WaitForSingleObject(process,0)==WAIT_TIMEOUT;}
 static std::wstring environment(){std::wstring result;auto block=GetEnvironmentStringsW();if(!block)throw hresult_error(E_OUTOFMEMORY);for(auto entry=block;*entry;entry+=wcslen(entry)+1){if(_wcsnicmp(entry,L"HHUB_PHONE_LEGACY_OWNER=",24)==0)continue;result.append(entry,wcslen(entry)+1);}FreeEnvironmentStringsW(block);result+=L"HHUB_PHONE_LEGACY_OWNER="+(binaryDir()/L"Hhub.Phone.exe").wstring();result.push_back(L'\0');result.push_back(L'\0');return result;}
 void ensure(){if(alive())return;if(process){CloseHandle(process);process=nullptr;}if(job){CloseHandle(job);job=nullptr;}auto exe=phoneModuleFolder()/L"Hhub.Phone.exe";auto line=L"\""+exe.wstring()+L"\"";auto env=environment();STARTUPINFOW si{sizeof(si)};si.dwFlags=STARTF_USESHOWWINDOW;si.wShowWindow=SW_HIDE;PROCESS_INFORMATION pi{};if(!CreateProcessW(exe.c_str(),line.data(),nullptr,nullptr,FALSE,CREATE_SUSPENDED|CREATE_NO_WINDOW|CREATE_UNICODE_ENVIRONMENT,env.data(),exe.parent_path().c_str(),&si,&pi))throw hresult_error(E_FAIL,L"无法启动手机内置后台");
  job=CreateJobObjectW(nullptr,nullptr);JOBOBJECT_EXTENDED_LIMIT_INFORMATION limit{};limit.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
  if(!job||!SetInformationJobObject(job,JobObjectExtendedLimitInformation,&limit,sizeof(limit))||!AssignProcessToJobObject(job,pi.hProcess)){TerminateProcess(pi.hProcess,1);CloseHandle(pi.hThread);CloseHandle(pi.hProcess);if(job)CloseHandle(job);job=nullptr;throw hresult_error(E_ACCESSDENIED,L"无法管理手机后台生命周期");}
  process=pi.hProcess;ResumeThread(pi.hThread);CloseHandle(pi.hThread);
 }
 JsonObject call(std::wstring operation,JsonObject body={}){ensure();auto name=L"\\\\.\\pipe\\Hhub.Phone.ctrl."+userSid()+instanceSuffix();HANDLE pipe=INVALID_HANDLE_VALUE;for(int i=0;i<50;i++){pipe=CreateFileW(name.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr);if(pipe!=INVALID_HANDLE_VALUE)break;if(!alive())break;Sleep(30);}if(pipe==INVALID_HANDLE_VALUE)throw hresult_error(E_FAIL,L"手机插件后台未响应");DWORD mode=PIPE_READMODE_BYTE|PIPE_NOWAIT;SetNamedPipeHandleState(pipe,&mode,nullptr,nullptr);JsonObject request;put(request,L"op",operation);request.SetNamedValue(L"body",body);std::string reply;bool ok=sendFrame(pipe,to_string(request.Stringify()))&&receiveFrame(pipe,reply);if(ok){BYTE ack=1;transfer(pipe,&ack,1,true);}CloseHandle(pipe);if(!ok)throw hresult_error(E_FAIL,L"手机插件后台通信超时");auto result=object(std::wstring(to_hstring(reply)));if(!flag(result,L"ok"))throw hresult_error(E_ACCESSDENIED,str(result,L"error").c_str());return result.GetNamedObject(L"data");}
 void shutdown(){if(alive()){try{call(L"quit");}catch(...){}if(WaitForSingleObject(process,3000)==WAIT_TIMEOUT)TerminateProcess(process,ERROR_PROCESS_ABORTED);}if(job){CloseHandle(job);job=nullptr;}if(process){CloseHandle(process);process=nullptr;}}
 void authorize(JsonObject body,bool control){if(!alive())throw hresult_error(E_ACCESSDENIED,L"手机连接已停止");JsonObject check;put(check,L"deviceId",str(body,L"deviceId"));put(check,L"control",control);call(L"authorize",check);}
 void uninstall(){shutdown();auto exe=phoneModuleFolder()/L"Hhub.Phone.exe";auto line=L"\""+exe.wstring()+L"\" --uninstall";auto env=environment();STARTUPINFOW si{sizeof(si)};si.dwFlags=STARTF_USESHOWWINDOW;si.wShowWindow=SW_HIDE;PROCESS_INFORMATION pi{};if(!CreateProcessW(exe.c_str(),line.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW|CREATE_UNICODE_ENVIRONMENT,env.data(),exe.parent_path().c_str(),&si,&pi))throw hresult_error(E_FAIL,L"手机插件清理程序未启动");CloseHandle(pi.hThread);auto waited=WaitForSingleObject(pi.hProcess,10000);DWORD code=1;GetExitCodeProcess(pi.hProcess,&code);if(waited!=WAIT_OBJECT_0)TerminateProcess(pi.hProcess,1);CloseHandle(pi.hProcess);if(waited!=WAIT_OBJECT_0||code)throw hresult_error(E_FAIL,L"手机授权或防火墙规则未能清理，卸载尚未完成");config=JsonObject{};}
 JsonArray actions(JsonObject body,JsonObject state){
  authorize(body,true);auto kind=str(body,L"kind");JsonArray requests;
  auto append=[&](std::wstring op,JsonObject data){JsonObject request;put(request,L"op",op);request.SetNamedValue(L"body",data);requests.Append(request);};
  auto idBody=[&]{JsonObject b;put(b,L"id",str(body,L"id"));return b;};
  if(kind==L"scene"){auto b=idBody();put(b,L"locked",true);append(L"applyScene",b);}
  else if(kind==L"sceneAuto")append(L"unlockScene",{});
  else if(kind==L"profileSelect")append(L"deviceProfileSelect",idBody());
  else if(kind==L"profileRetry"){JsonObject b;put(b,L"profile",str(body,L"id"));append(L"deviceProfileRetry",b);}
  else if(kind==L"deviceRead"||kind==L"deviceWrite"){
   auto device=find(state.GetNamedObject(L"detected").GetNamedArray(L"peripherals"),str(body,L"id"));auto plugin=phoneControlPlugin(state,device);if(kind==L"deviceWrite")phoneValidateControlPatch(body.GetNamedObject(L"patch"),plugin);
   auto job=object(std::wstring(body.Stringify()));put(job,L"plugin",str(plugin,L"id"));put(job,L"operation",kind==L"deviceWrite"?std::wstring(L"write"):std::wstring(L"read"));job.SetNamedValue(L"deviceIdentity",device);append(L"pluginTaskSubmit",job);
  }else{
   if(kind!=L"eqBypass"&&kind!=L"eqSave"&&kind!=L"eqStart"&&kind!=L"eqStop"&&kind!=L"eqPreset"&&kind!=L"eqBackground")throw hresult_invalid_argument(L"不支持的手机操作");
   JsonObject selected{nullptr};for(auto const& value:state.GetNamedArray(L"eqRealPrefs",JsonArray{}))if(phoneEqKey(value.GetObject())==str(body,L"id"))selected=object(std::wstring(value.Stringify()));if(!selected)throw hresult_invalid_argument(L"EQ 配置不存在");
   if(kind==L"eqStop")append(L"audioBindingDisable",selected);
   else{
    if(!flag(state.GetNamedObject(L"settings"),L"eqEnabled"))throw hresult_error(E_ACCESSDENIED,L"电脑端 EQ 已关闭");
    if(kind==L"eqSave"){for(auto key:{L"bands",L"preamp",L"autoPreamp",L"enabled"})selected.SetNamedValue(key,body.GetNamedValue(key));put(selected,L"preset",std::wstring(L"custom"));}
    if(kind==L"eqPreset"){auto preset=find(state.GetNamedArray(L"presets"),str(body,L"preset"));for(auto key:{L"bands",L"frequencies"})selected.SetNamedValue(key,preset.GetNamedValue(key));put(selected,L"preset",str(preset,L"id"));}
    if(kind==L"eqBypass")put(selected,L"enabled",body.GetNamedBoolean(L"enabled"));
    if(kind==L"eqBackground")put(selected,L"backgroundAutoStart",body.GetNamedBoolean(L"enabled"));
    if(kind==L"eqStart"){put(selected,L"backgroundAutoStart",true);put(selected,L"enabled",true);}
    append(L"eqRealSave",selected);if(kind==L"eqStart")append(L"eqStart",selected);
   }
  }return requests;
 }
 JsonObject request(JsonObject request){std::scoped_lock lock(mutex);auto op=str(request,L"op");auto body=request.GetNamedObject(L"body",JsonObject{});auto state=request.GetNamedObject(L"state",JsonObject{});JsonObject result,data;put(result,L"ok",true);
  if(op==L"shutdown")shutdown();
  else if(op==L"uninstall")uninstall();
  else if(op==L"resume"){if(flag(config,L"enabled")){data=call(L"start",config);}}
  else if(op==L"remoteStatus"){authorize(body,false);data=phoneView(state);}
  else if(op==L"remoteAction")result.SetNamedValue(L"hostRequests",actions(body,state));
  else if(op==L"authorize"||op==L"jobValidate"||op==L"jobAck"){
   auto job=body.GetNamedObject(L"job");authorize(job,true);
   if(op!=L"authorize"){auto device=find(state.GetNamedObject(L"detected").GetNamedArray(L"peripherals"),str(job,L"id"));auto plugin=phoneControlPlugin(state,device);if(str(plugin,L"id")!=str(job,L"plugin"))throw hresult_invalid_argument(L"设备插件已改变");}
   if(op==L"jobAck"&&flag(body,L"confirmed")){auto payload=body.GetNamedObject(L"payload");if(payload.Stringify().size()>65536)throw hresult_invalid_argument();auto values=phoneControlValues(payload);if(str(job,L"operation")==L"write")for(auto const& pair:job.GetNamedObject(L"patch")){if(!values.HasKey(pair.Key())||values.GetNamedValue(pair.Key()).Stringify()!=pair.Value().Stringify())throw hresult_invalid_argument(L"设备回读与请求不一致");}data.SetNamedValue(L"values",values);}
  }
  else if(op==L"status"){put(data,L"workerAlive",alive());}
  else if(op==L"state")data=call(L"state");
  else if(op==L"start"){data=call(op,body);config=object(std::wstring(body.Stringify()));put(config,L"enabled",true);save();}
  else if(op==L"stop"){data=call(op);put(config,L"enabled",false);save();}
  else if(op==L"rotate"||op==L"approve"||op==L"reject"||op==L"revoke"||op==L"permission")data=call(op,body);
  else throw hresult_invalid_argument(L"手机插件未提供此操作");
  put(result,L"retainService",flag(config,L"enabled")&&op!=L"shutdown");result.SetNamedValue(L"data",data);return result;
 }
};
inline PhoneBackend& phoneBackend(){static PhoneBackend backend;return backend;}
}
