#pragma once
#include <windows.h>
#include <psapi.h>
#include <unknwn.h>
#include <objbase.h>
#include <sddl.h>
#include <shellapi.h>
#include <map>
#include <deque>
#include <functional>
#include <atomic>
#include <bcrypt.h>
#include <wincrypt.h>
#include <memory>
#include <winrt/Windows.Data.Json.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <chrono>
#include <cmath>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <set>
#include <mutex>
#include <thread>

namespace hub {
using namespace winrt;
using namespace Windows::Data::Json;
inline std::wstring str(JsonObject const& o, wchar_t const* k, wchar_t const* fallback=L"") { return std::wstring(o.GetNamedString(k, fallback)); }
inline double num(JsonObject const& o, wchar_t const* k, double fallback=0) { return o.GetNamedNumber(k, fallback); }
inline bool flag(JsonObject const& o, wchar_t const* k, bool fallback=false) { return o.GetNamedBoolean(k, fallback); }
inline void put(JsonObject const& o, wchar_t const* k, std::wstring const& v) { o.SetNamedValue(k, JsonValue::CreateStringValue(v)); }
inline void put(JsonObject const& o, wchar_t const* k, double v) { o.SetNamedValue(k, JsonValue::CreateNumberValue(v)); }
inline void put(JsonObject const& o, wchar_t const* k, bool v) { o.SetNamedValue(k, JsonValue::CreateBooleanValue(v)); }
} // namespace hub

namespace hub {
inline JsonObject find(JsonArray const& a, std::wstring const& id) { for(auto const& v:a) { auto o=v.GetObject(); if(str(o,L"id")==id) return o; } throw hresult_invalid_argument(L"找不到记录"); }
inline bool hasOfflineUi(JsonObject const& p){return p.HasKey(L"ui")&&str(p.GetNamedObject(L"ui"),L"type")==L"offline-html";}
inline JsonObject object(std::wstring const& text) { return JsonObject::Parse(text); }
inline std::wstring read(std::filesystem::path const& p) { std::ifstream in(p,std::ios::binary); std::string s((std::istreambuf_iterator<char>(in)),{}); return std::wstring(to_hstring(s)); }
inline void write(std::filesystem::path const& p, std::wstring const& text) { std::filesystem::create_directories(p.parent_path()); auto tmp=p;tmp+=L".tmp"; {std::ofstream out(tmp,std::ios::binary|std::ios::trunc); auto utf=to_string(text);out.write(utf.data(),utf.size());if(!out)throw hresult_error(E_FAIL,L"无法保存配置");} if(!MoveFileExW(tmp.c_str(),p.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw hresult_error(HRESULT_FROM_WIN32(GetLastError())); }
inline bool mousePortTestProfile(){wchar_t value[64]{};GetEnvironmentVariableW(L"HHUB_TEST_PROFILE",value,64);return std::wstring(value)==L"mouse-port-tests"||std::wstring(value)==L"mouse-per-device-tests";}
inline bool phoneTestProfile(){wchar_t value[64]{};GetEnvironmentVariableW(L"HHUB_TEST_PROFILE",value,64);return std::wstring(value)==L"phone-tests";}
inline bool uiMemoryTestProfile(){wchar_t value[64]{};GetEnvironmentVariableW(L"HHUB_TEST_PROFILE",value,64);return std::wstring(value)==L"ui-memory-tests";}
inline std::wstring instanceSuffix(){wchar_t value[80]{};GetEnvironmentVariableW(L"HHUB_INSTANCE",value,80);std::wstring id(value);if(id.size()>64||!std::all_of(id.begin(),id.end(),[](wchar_t c){return iswalnum(c)||c==L'-';}))throw hresult_invalid_argument(L"Invalid instance");return id.empty()?L"":L"."+id;}
inline std::filesystem::path dataDir() { wchar_t p[32768]{};GetEnvironmentVariableW(L"LOCALAPPDATA",p,32768);return std::filesystem::path(p)/(uiMemoryTestProfile()?L"HhubUiMemoryTests":phoneTestProfile()?L"HhubPhoneTests":mousePortTestProfile()?L"HhubMousePerDeviceTests":L"HhubPrototype"); }
inline double clockSeconds() { return (double)std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count(); }
inline std::filesystem::path pluginFolder(){HMODULE module{};check_bool(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&pluginFolder),&module));wchar_t path[32768]{};check_bool(GetModuleFileNameW(module,path,32768)!=0);return std::filesystem::path(path).parent_path();}
inline std::filesystem::path binaryDir(){wchar_t p[32768]{};GetModuleFileNameW(nullptr,p,32768);auto directory=std::filesystem::path(p).parent_path();if(directory.filename()==L"core"&&std::filesystem::path(p).filename()==L"Hhub.exe")return directory.parent_path();return directory.filename()==L"runtime"&&std::filesystem::is_regular_file(directory/L"Hhub.runtime")?directory.parent_path():directory;}
inline std::wstring userSid(){HANDLE token{};check_bool(OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token));DWORD size{};GetTokenInformation(token,TokenUser,nullptr,0,&size);std::vector<unsigned char> buffer(size);check_bool(GetTokenInformation(token,TokenUser,buffer.data(),size,&size));CloseHandle(token);LPWSTR sid{};check_bool(ConvertSidToStringSidW(((TOKEN_USER*)buffer.data())->User.Sid,&sid));std::wstring value(sid);LocalFree(sid);return value;}
inline JsonObject pluginRecord(JsonObject const& state,std::wstring const& id){for(auto const& v:state.GetNamedArray(L"plugins",JsonArray{})){auto p=v.GetObject();if(str(p,L"id")==id)return p;}return nullptr;}
inline bool pluginInstalled(JsonObject const& state,std::wstring const& id){auto p=pluginRecord(state,id);return p&&flag(p,L"installed");}
inline std::wstring pipeName(){return L"\\\\.\\pipe\\HhubPrototype.v1."+userSid()+instanceSuffix();}
inline double workingSet(){PROCESS_MEMORY_COUNTERS c{};GetProcessMemoryInfo(GetCurrentProcess(),&c,sizeof(c));return c.WorkingSetSize/1048576.0;}
inline bool transfer(HANDLE pipe,void* data,DWORD size,bool writing){DWORD done=0;auto deadline=GetTickCount64()+3000;while(done<size){DWORD n{};BOOL ok=writing?WriteFile(pipe,(char*)data+done,size-done,&n,nullptr):ReadFile(pipe,(char*)data+done,size-done,&n,nullptr);if(ok&&n){done+=n;continue;}auto error=GetLastError();if(!ok&&error!=ERROR_NO_DATA&&error!=ERROR_PIPE_LISTENING)return false;if(GetTickCount64()>deadline)return false;Sleep(5);}return true;}
inline bool sendFrame(HANDLE pipe,std::string text){DWORD size=(DWORD)text.size();return size<=33554432&&transfer(pipe,&size,sizeof(size),true)&&transfer(pipe,text.data(),size,true);}
inline bool receiveFrame(HANDLE pipe,std::string& text){DWORD size{};if(!transfer(pipe,&size,sizeof(size),false)||size==0||size>33554432)return false;text.resize(size);return transfer(pipe,text.data(),size,false);}
inline bool asyncTransfer(HANDLE pipe,void* data,DWORD size,bool writing){DWORD done=0;HANDLE event=CreateEventW(nullptr,TRUE,FALSE,nullptr);while(done<size){OVERLAPPED ov{};ov.hEvent=event;ResetEvent(event);DWORD n{};BOOL ok=writing?WriteFile(pipe,(char*)data+done,size-done,&n,&ov):ReadFile(pipe,(char*)data+done,size-done,&n,&ov);if(!ok&&GetLastError()==ERROR_IO_PENDING){if(WaitForSingleObject(event,3000)==WAIT_OBJECT_0)ok=GetOverlappedResult(pipe,&ov,&n,FALSE);else {CancelIoEx(pipe,&ov);WaitForSingleObject(event,INFINITE);CloseHandle(event);return false;}}if(!ok||n==0){CloseHandle(event);return false;}done+=n;}CloseHandle(event);return true;}
inline bool serverReceive(HANDLE pipe,std::string& text){DWORD size{};if(!asyncTransfer(pipe,&size,4,false)||size==0||size>33554432)return false;text.resize(size);return asyncTransfer(pipe,text.data(),size,false);}
inline bool serverSend(HANDLE pipe,std::string text){DWORD size=(DWORD)text.size();if(!asyncTransfer(pipe,&size,4,true)||!asyncTransfer(pipe,text.data(),size,true))return false;BYTE ack{};return asyncTransfer(pipe,&ack,1,false)&&ack==1;}

inline JsonObject call(std::wstring const& op,JsonObject body=JsonObject{}){auto req=JsonObject{};put(req,L"op",op);req.SetNamedValue(L"body",body);HANDLE p=INVALID_HANDLE_VALUE;for(int i=0;i<40;++i){p=CreateFileW(pipeName().c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr);if(p!=INVALID_HANDLE_VALUE)break;auto error=GetLastError();if(error==ERROR_PIPE_BUSY){WaitNamedPipeW(pipeName().c_str(),25);continue;}Sleep(50);}if(p==INVALID_HANDLE_VALUE)throw hresult_error(E_FAIL,L"无法连接后台核心");DWORD mode=PIPE_READMODE_BYTE|PIPE_NOWAIT;SetNamedPipeHandleState(p,&mode,nullptr,nullptr);std::string reply;auto ok=sendFrame(p,to_string(req.Stringify()))&&receiveFrame(p,reply);if(ok){BYTE ack=1;transfer(p,&ack,1,true);}CloseHandle(p);if(!ok)throw hresult_error(E_FAIL,L"后台通信超时");auto result=object(std::wstring(to_hstring(reply)));if(!flag(result,L"ok"))throw hresult_error(E_FAIL,str(result,L"error").c_str());return result.GetNamedObject(L"state");}
}
