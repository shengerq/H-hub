#pragma once
#include "Common.h"
namespace hub {
struct WindowsWorker {
 struct Lease {
  HANDLE mutex{};
  Lease(std::wstring kind=L"Lifecycle"){auto name=L"Local\\Hhub.WindowsSettings."+kind+L"."+userSid()+instanceSuffix();mutex=CreateMutexW(nullptr,FALSE,name.c_str());if(!mutex)throw hresult_error(HRESULT_FROM_WIN32(GetLastError()));auto status=WaitForSingleObject(mutex,0);if(status!=WAIT_OBJECT_0&&status!=WAIT_ABANDONED){CloseHandle(mutex);mutex=nullptr;throw hresult_error(E_PENDING,L"另一个 Windows 设置操作正在执行，请稍后重试");}}
  ~Lease(){if(mutex){ReleaseMutex(mutex);CloseHandle(mutex);}}
 };
 static void uninstall(std::filesystem::path root=dataDir()){
  Lease lease;Lease execution(L"Execution");for(auto const& name:{L"windows-settings-undo.json",L"windows-settings-undo.json.tmp"}){auto file=root/name;if(std::filesystem::exists(file)&&!std::filesystem::remove(file))throw hresult_error(E_FAIL,L"Windows 设置回退记录清理失败");}
  auto folder=root/L"requests";if(std::filesystem::exists(folder)){
   for(auto const& item:std::filesystem::directory_iterator(folder)){if(!item.is_regular_file())continue;auto name=item.path().filename().wstring();auto suffix=name.ends_with(L".request.json")?std::wstring(L".request.json"):name.ends_with(L".response.json")?std::wstring(L".response.json"):std::wstring();if(suffix.empty())continue;auto prefix=name.substr(0,name.size()-suffix.size());if(prefix.empty()||!std::all_of(prefix.begin(),prefix.end(),[](wchar_t c){return c==L'-'||(c>=L'0'&&c<=L'9');}))continue;std::filesystem::remove(item.path());}
   if(std::filesystem::is_empty(folder))std::filesystem::remove(folder);
  }
 }
 static JsonObject run(std::wstring operation,JsonObject body=JsonObject{},bool elevate=false){
  Lease lease;auto folder=dataDir()/L"requests";std::filesystem::create_directories(folder);auto name=std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64());auto input=folder/(name+L".request.json"),output=folder/(name+L".response.json");
  struct Files{std::filesystem::path input,output,folder;~Files(){std::error_code ec;std::filesystem::remove(input,ec);std::filesystem::remove(output,ec);if(std::filesystem::is_empty(folder,ec))std::filesystem::remove(folder,ec);}}files{input,output,folder};
  JsonObject req;put(req,L"op",operation);req.SetNamedValue(L"body",body);write(input,std::wstring(req.Stringify()));auto exe=nativeComponentPath(L"windows",L"Hhub.Windows.exe");if(!std::filesystem::is_regular_file(exe))throw hresult_error(E_FAIL,L"Windows 设置插件缺少执行组件，请在插件市场更新或重新安装此插件");auto line=L"\""+exe.wstring()+L"\" \""+input.wstring()+L"\" \""+output.wstring()+L"\"";
  struct Process{HANDLE handle{};~Process(){if(handle){if(WaitForSingleObject(handle,0)!=WAIT_OBJECT_0){TerminateProcess(handle,1);WaitForSingleObject(handle,1000);}CloseHandle(handle);}}}process;
  if(elevate){SHELLEXECUTEINFOW launch{sizeof(launch)};launch.fMask=SEE_MASK_NOCLOSEPROCESS;launch.lpVerb=L"runas";launch.lpFile=exe.c_str();auto parameters=L"\""+input.wstring()+L"\" \""+output.wstring()+L"\"";launch.lpParameters=parameters.c_str();launch.nShow=SW_HIDE;if(!ShellExecuteExW(&launch))throw hresult_error(HRESULT_FROM_WIN32(GetLastError()),L"未完成管理员授权，设置未修改");process.handle=launch.hProcess;}
  else{STARTUPINFOW si{sizeof(si)};si.dwFlags=STARTF_USESHOWWINDOW;si.wShowWindow=SW_HIDE;PROCESS_INFORMATION pi{};if(!CreateProcessW(exe.c_str(),line.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,exe.parent_path().c_str(),&si,&pi))throw hresult_error(E_FAIL,L"Windows 设置模块启动失败");CloseHandle(pi.hThread);process.handle=pi.hProcess;}
  if(!process.handle||WaitForSingleObject(process.handle,elevate?60000:10000)!=WAIT_OBJECT_0)throw hresult_error(E_FAIL,L"Windows 设置模块响应超时");auto response=object(read(output));if(!flag(response,L"ok"))throw hresult_error(E_FAIL,str(response,L"error").c_str());return response.GetNamedObject(L"data");
 }
};
}
