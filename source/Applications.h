#pragma once
#include "Common.h"
#include <tlhelp32.h>
#include <map>
#include <algorithm>
#include <winver.h>
#pragma comment(lib,"version.lib")
namespace hub {
inline std::wstring applicationPath(DWORD pid){HANDLE h=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);if(!h)return L"";wchar_t path[32768]{};DWORD n=32768;BOOL ok=QueryFullProcessImageNameW(h,0,path,&n);CloseHandle(h);return ok?std::wstring(path):L"";}
inline std::wstring foregroundApplication(){DWORD pid{};GetWindowThreadProcessId(GetForegroundWindow(),&pid);return applicationPath(pid);}
inline std::wstring applicationLabel(std::wstring const& path){
 DWORD ignored{},size=GetFileVersionInfoSizeW(path.c_str(),&ignored);if(size&&size<1048576){std::vector<BYTE> bytes(size);if(GetFileVersionInfoW(path.c_str(),0,size,bytes.data())){struct Translation{WORD language,codepage;};Translation* translations{};UINT count{};if(VerQueryValueW(bytes.data(),L"\\VarFileInfo\\Translation",(void**)&translations,&count)){for(UINT i=0;i<count/sizeof(Translation);i++){wchar_t key[128]{};swprintf_s(key,L"\\StringFileInfo\\%04x%04x\\FileDescription",translations[i].language,translations[i].codepage);wchar_t* label{};UINT length{};if(VerQueryValueW(bytes.data(),key,(void**)&label,&length)&&label&&length>1)return label;}}}}
 return std::filesystem::path(path).filename().wstring();
}
struct Applications {
 JsonObject cached{nullptr};ULONGLONG last{};std::wstring lastForeground;std::map<std::wstring,std::wstring> labels;
 static std::wstring key(std::wstring path){std::transform(path.begin(),path.end(),path.begin(),towupper);return path;}
 JsonObject scan(bool force=false){
  if(cached&&!force&&GetTickCount64()-last<3000)return cached;last=GetTickCount64();
  auto foreground=foregroundApplication();if(!foreground.empty()&&key(std::filesystem::path(foreground).filename().wstring()).rfind(L"HHUB",0)!=0)lastForeground=foreground;
  std::map<DWORD,std::wstring> windows;
  EnumWindows([](HWND window,LPARAM data)->BOOL{if(!IsWindowVisible(window)||GetWindow(window,GW_OWNER))return TRUE;DWORD pid{};GetWindowThreadProcessId(window,&pid);wchar_t title[1024]{};GetWindowTextW(window,title,1024);if(title[0])(*reinterpret_cast<std::map<DWORD,std::wstring>*>(data))[pid]=title;return TRUE;},reinterpret_cast<LPARAM>(&windows));
  std::map<std::wstring,JsonObject> paths;auto snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
  if(snapshot!=INVALID_HANDLE_VALUE){PROCESSENTRY32W p{sizeof(p)};if(Process32FirstW(snapshot,&p))do{
   auto path=applicationPath(p.th32ProcessID);if(path.empty())continue;auto id=key(path);auto found=paths.find(id);
   if(found==paths.end()){JsonObject app;put(app,L"id",id);put(app,L"path",path);if(!labels.count(id))labels[id]=applicationLabel(path);put(app,L"name",labels[id]);put(app,L"executable",std::filesystem::path(path).filename().wstring());put(app,L"instances",1.0);put(app,L"pid",(double)p.th32ProcessID);put(app,L"foreground",id==key(lastForeground));put(app,L"hasWindow",windows.count(p.th32ProcessID)>0);if(windows.count(p.th32ProcessID))put(app,L"windowTitle",windows.at(p.th32ProcessID));paths.emplace(id,app);
   }else{put(found->second,L"instances",num(found->second,L"instances")+1);if(windows.count(p.th32ProcessID)){put(found->second,L"hasWindow",true);put(found->second,L"windowTitle",windows.at(p.th32ProcessID));}}
  }while(Process32NextW(snapshot,&p));CloseHandle(snapshot);}
  std::vector<JsonObject> sorted;for(auto const& [id,app]:paths)sorted.push_back(app);std::stable_sort(sorted.begin(),sorted.end(),[](auto a,auto b){if(flag(a,L"foreground")!=flag(b,L"foreground"))return flag(a,L"foreground");if(flag(a,L"hasWindow")!=flag(b,L"hasWindow"))return flag(a,L"hasWindow");return key(str(a,L"name"))<key(str(b,L"name"));});
  JsonArray running;for(auto const& app:sorted)running.Append(app);JsonObject result;result.SetNamedValue(L"running",running);put(result,L"foreground",lastForeground);put(result,L"sampleTime",clockSeconds());cached=result;return result;
 }
};
struct Lifecycle {
 static bool startup(){HKEY key{};if(RegOpenKeyExW(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,KEY_QUERY_VALUE,&key)!=ERROR_SUCCESS)return false;wchar_t data[32768]{};DWORD size=sizeof(data),type{};auto ok=RegQueryValueExW(key,L"Hhub",nullptr,&type,(BYTE*)data,&size)==ERROR_SUCCESS&&type==REG_SZ;RegCloseKey(key);return ok&&std::wstring(data).find((binaryDir()/L"Hhub.Core.exe").wstring())!=std::wstring::npos;}
 static void startup(bool enabled){HKEY key{};auto status=RegCreateKeyExW(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,nullptr,0,KEY_SET_VALUE,nullptr,&key,nullptr);if(status!=ERROR_SUCCESS)throw hresult_error(HRESULT_FROM_WIN32(status));if(enabled){auto command=L"\""+(binaryDir()/L"Hhub.Core.exe").wstring()+L"\"";status=RegSetValueExW(key,L"Hhub",0,REG_SZ,(BYTE*)command.c_str(),(DWORD)((command.size()+1)*sizeof(wchar_t)));}else{status=RegDeleteValueW(key,L"Hhub");if(status==ERROR_FILE_NOT_FOUND)status=ERROR_SUCCESS;}RegCloseKey(key);if(status!=ERROR_SUCCESS)throw hresult_error(HRESULT_FROM_WIN32(status));}
};
struct WindowsWorker {
 static JsonObject run(std::wstring operation,JsonObject body=JsonObject{},bool elevate=false){auto folder=dataDir()/L"requests";std::filesystem::create_directories(folder);auto name=std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64());auto input=folder/(name+L".request.json"),output=folder/(name+L".response.json");JsonObject req;put(req,L"op",operation);req.SetNamedValue(L"body",body);write(input,std::wstring(req.Stringify()));auto exe=binaryDir()/L"Hhub.Windows.exe";auto line=L"\""+exe.wstring()+L"\" \""+input.wstring()+L"\" \""+output.wstring()+L"\"";STARTUPINFOW si{sizeof(si)};si.dwFlags=STARTF_USESHOWWINDOW;si.wShowWindow=SW_HIDE;PROCESS_INFORMATION pi{};if(elevate){SHELLEXECUTEINFOW launch{sizeof(launch)};launch.fMask=SEE_MASK_NOCLOSEPROCESS;launch.lpVerb=L"runas";launch.lpFile=exe.c_str();auto parameters=L"\""+input.wstring()+L"\" \""+output.wstring()+L"\"";launch.lpParameters=parameters.c_str();launch.nShow=SW_HIDE;if(!ShellExecuteExW(&launch))throw hresult_error(HRESULT_FROM_WIN32(GetLastError()),L"未完成管理员授权，设置未修改");pi.hProcess=launch.hProcess;}else{if(!CreateProcessW(exe.c_str(),line.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,binaryDir().c_str(),&si,&pi))throw hresult_error(E_FAIL,L"Windows 设置模块启动失败");CloseHandle(pi.hThread);}auto waited=WaitForSingleObject(pi.hProcess,elevate?60000:10000);if(waited!=WAIT_OBJECT_0){TerminateProcess(pi.hProcess,1);CloseHandle(pi.hProcess);throw hresult_error(E_FAIL,L"Windows 设置模块响应超时");}CloseHandle(pi.hProcess);auto response=object(read(output));std::error_code ec;std::filesystem::remove(input,ec);std::filesystem::remove(output,ec);if(!flag(response,L"ok"))throw hresult_error(E_FAIL,str(response,L"error").c_str());return response.GetNamedObject(L"data");}
};
}
