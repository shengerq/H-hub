#pragma once
#include <windows.h>
#include <unknwn.h>
#include <inspectable.h>
#include <winstring.h>
#include "Model.h"
#include <sddl.h>
#include <shellapi.h>
#include <shobjidl_core.h>
#include <psapi.h>
#include <vector>
#include <mutex>

namespace hub {
inline void initializeProcessIdentity(){check_hresult(SetCurrentProcessExplicitAppUserModelID(L"Hhub.Desktop"));}
inline std::filesystem::path binaryDir(){wchar_t p[32768]{};GetModuleFileNameW(nullptr,p,32768);auto directory=std::filesystem::path(p).parent_path();if(directory.filename()==L"core"&&std::filesystem::path(p).filename()==L"Hhub.exe")return directory.parent_path();return directory.filename()==L"runtime"&&std::filesystem::is_regular_file(directory/L"Hhub.runtime")?directory.parent_path():directory;}
inline std::filesystem::path nativeComponentPath(std::wstring const& id,std::wstring const& name,std::filesystem::path legacy={},std::filesystem::path root=dataDir()){auto definitions=nativeAssetDefinitions(id);if(std::none_of(definitions.begin(),definitions.end(),[&](auto const& d){return d.name==name;}))throw hresult_invalid_argument(L"未知的原生插件组件");auto installed=root/L"native-components"/id/name;if(std::filesystem::is_regular_file(installed))return installed;return binaryDir()/(legacy.empty()?std::filesystem::path(name):legacy);}
inline std::wstring userSid(){HANDLE token{};if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token))throw hresult_error(HRESULT_FROM_WIN32(GetLastError()));DWORD size=0;GetTokenInformation(token,TokenUser,nullptr,0,&size);std::vector<unsigned char> b(size);if(!GetTokenInformation(token,TokenUser,b.data(),size,&size)){CloseHandle(token);throw hresult_error(E_FAIL);}LPWSTR sid{};ConvertSidToStringSidW(((TOKEN_USER*)b.data())->User.Sid,&sid);std::wstring result(sid);LocalFree(sid);CloseHandle(token);return result;}
inline std::wstring pipeName(){return L"\\\\.\\pipe\\HhubPrototype.v1."+userSid()+instanceSuffix();}
inline double workingSet(){PROCESS_MEMORY_COUNTERS c{};GetProcessMemoryInfo(GetCurrentProcess(),&c,sizeof(c));return c.WorkingSetSize/1048576.0;}
inline bool transfer(HANDLE pipe,void* data,DWORD size,bool writing){DWORD done=0;auto deadline=GetTickCount64()+3000;while(done<size){DWORD n{};BOOL ok=writing?WriteFile(pipe,(char*)data+done,size-done,&n,nullptr):ReadFile(pipe,(char*)data+done,size-done,&n,nullptr);if(ok&&n){done+=n;continue;}auto error=GetLastError();if(!ok&&error!=ERROR_NO_DATA&&error!=ERROR_PIPE_LISTENING)return false;if(GetTickCount64()>deadline)return false;Sleep(5);}return true;}
inline bool sendFrame(HANDLE pipe,std::string text){DWORD size=(DWORD)text.size();return size<=33554432&&transfer(pipe,&size,sizeof(size),true)&&transfer(pipe,text.data(),size,true);}
inline bool receiveFrame(HANDLE pipe,std::string& text){DWORD size{};if(!transfer(pipe,&size,sizeof(size),false)||size==0||size>33554432)return false;text.resize(size);return transfer(pipe,text.data(),size,false);}
inline bool asyncTransfer(HANDLE pipe,void* data,DWORD size,bool writing){DWORD done=0;HANDLE event=CreateEventW(nullptr,TRUE,FALSE,nullptr);while(done<size){OVERLAPPED ov{};ov.hEvent=event;ResetEvent(event);DWORD n{};BOOL ok=writing?WriteFile(pipe,(char*)data+done,size-done,&n,&ov):ReadFile(pipe,(char*)data+done,size-done,&n,&ov);if(!ok&&GetLastError()==ERROR_IO_PENDING){if(WaitForSingleObject(event,3000)==WAIT_OBJECT_0)ok=GetOverlappedResult(pipe,&ov,&n,FALSE);else {CancelIoEx(pipe,&ov);WaitForSingleObject(event,INFINITE);CloseHandle(event);return false;}}if(!ok||n==0){CloseHandle(event);return false;}done+=n;}CloseHandle(event);return true;}
inline bool serverReceive(HANDLE pipe,std::string& text){DWORD size{};if(!asyncTransfer(pipe,&size,4,false)||size==0||size>33554432)return false;text.resize(size);return asyncTransfer(pipe,text.data(),size,false);}
inline bool serverSend(HANDLE pipe,std::string text){DWORD size=(DWORD)text.size();if(!asyncTransfer(pipe,&size,4,true)||!asyncTransfer(pipe,text.data(),size,true))return false;BYTE ack{};return asyncTransfer(pipe,&ack,1,false)&&ack==1;}
inline bool startCore(){auto exe=binaryDir()/L"core"/L"Hhub.exe";std::wstring line=L"\""+exe.wstring()+L"\"";STARTUPINFOW si{sizeof(si)};si.dwFlags=STARTF_USESHOWWINDOW;si.wShowWindow=SW_HIDE;PROCESS_INFORMATION pi{};if(!CreateProcessW(exe.c_str(),line.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,binaryDir().c_str(),&si,&pi))return false;CloseHandle(pi.hThread);CloseHandle(pi.hProcess);return true;}
inline bool coreAutoStartAllowed=true;
#ifdef HHUB_HOST_COMMON_TESTS
#include HHUB_HOST_COMMON_TESTS
#else
inline constexpr bool hostTestAutoConfirm(){return false;}
#endif
inline JsonObject call(std::wstring const& op,JsonObject body=JsonObject{}){
#ifdef HHUB_HOST_COMMON_TESTS
if(uiMemoryTestProfile())return testHostCall(op,body);
#endif
auto req=JsonObject{};put(req,L"op",op);req.SetNamedValue(L"body",body);HANDLE p=INVALID_HANDLE_VALUE;for(int i=0;i<40;++i){p=CreateFileW(pipeName().c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr);if(p!=INVALID_HANDLE_VALUE)break;auto error=GetLastError();if(error==ERROR_PIPE_BUSY){WaitNamedPipeW(pipeName().c_str(),25);continue;}if(i==0&&coreAutoStartAllowed&&!startCore())throw hresult_error(E_FAIL,L"无法启动后台核心");Sleep(50);}if(p==INVALID_HANDLE_VALUE)throw hresult_error(E_FAIL,L"无法连接后台核心");DWORD mode=PIPE_READMODE_BYTE|PIPE_NOWAIT;SetNamedPipeHandleState(p,&mode,nullptr,nullptr);std::string reply;auto ok=sendFrame(p,to_string(req.Stringify()))&&receiveFrame(p,reply);if(ok){BYTE ack=1;transfer(p,&ack,1,true);}CloseHandle(p);if(!ok)throw hresult_error(E_FAIL,L"后台通信超时");auto result=object(std::wstring(to_hstring(reply)));if(!flag(result,L"ok"))throw hresult_error(E_FAIL,str(result,L"error").c_str());return result.GetNamedObject(L"state");}
}
