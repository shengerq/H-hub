#pragma once
#include <windows.h>
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
inline JsonObject object(std::wstring const& text) { return JsonObject::Parse(text); }
inline std::wstring read(std::filesystem::path const& p) { std::ifstream in(p,std::ios::binary); std::string s((std::istreambuf_iterator<char>(in)),{}); return std::wstring(to_hstring(s)); }
inline void write(std::filesystem::path const& p, std::wstring const& text) { std::filesystem::create_directories(p.parent_path()); auto tmp=p;tmp+=L".tmp"; {std::ofstream out(tmp,std::ios::binary|std::ios::trunc); auto utf=to_string(text);out.write(utf.data(),utf.size());if(!out)throw hresult_error(E_FAIL,L"无法保存配置");} if(!MoveFileExW(tmp.c_str(),p.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw hresult_error(HRESULT_FROM_WIN32(GetLastError())); }
inline bool mousePortTestProfile(){wchar_t value[64]{};GetEnvironmentVariableW(L"HHUB_TEST_PROFILE",value,64);return std::wstring(value)==L"mouse-port-tests"||std::wstring(value)==L"mouse-per-device-tests";}
inline bool phoneTestProfile(){wchar_t value[64]{};GetEnvironmentVariableW(L"HHUB_TEST_PROFILE",value,64);return std::wstring(value)==L"phone-tests";}
inline bool uiMemoryTestProfile(){wchar_t value[64]{};GetEnvironmentVariableW(L"HHUB_TEST_PROFILE",value,64);return std::wstring(value)==L"ui-memory-tests";}
inline std::wstring instanceSuffix(){wchar_t value[80]{};GetEnvironmentVariableW(L"HHUB_INSTANCE",value,80);std::wstring id(value);if(id.size()>64||!std::all_of(id.begin(),id.end(),[](wchar_t c){return iswalnum(c)||c==L'-';}))throw hresult_invalid_argument(L"Invalid instance");return id.empty()?L"":L"."+id;}
inline std::filesystem::path dataDir() { wchar_t p[32768]{};GetEnvironmentVariableW(L"LOCALAPPDATA",p,32768);return std::filesystem::path(p)/(uiMemoryTestProfile()?L"HhubUiMemoryTests":phoneTestProfile()?L"HhubPhoneTests":mousePortTestProfile()?L"HhubMousePerDeviceTests":L"HhubPrototype"); }
inline double clockSeconds() { return (double)std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count(); }
inline std::filesystem::path binaryDir(){wchar_t p[32768]{};GetModuleFileNameW(nullptr,p,32768);auto directory=std::filesystem::path(p).parent_path();if(directory.filename()==L"core"&&std::filesystem::path(p).filename()==L"Hhub.exe")return directory.parent_path();return directory.filename()==L"runtime"&&std::filesystem::is_regular_file(directory/L"Hhub.runtime")?directory.parent_path():directory;}
inline std::wstring userSid(){HANDLE token{};check_bool(OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token));DWORD size{};GetTokenInformation(token,TokenUser,nullptr,0,&size);std::vector<unsigned char> buffer(size);check_bool(GetTokenInformation(token,TokenUser,buffer.data(),size,&size));CloseHandle(token);LPWSTR sid{};check_bool(ConvertSidToStringSidW(((TOKEN_USER*)buffer.data())->User.Sid,&sid));std::wstring value(sid);LocalFree(sid);return value;}
inline std::filesystem::path nativeComponentPath(std::wstring const& id,std::wstring const& name){if(id!=L"windows"||name!=L"Hhub.Windows.exe")throw hresult_invalid_argument();return dataDir()/L"native-components"/id/name;}
inline JsonObject pluginRecord(JsonObject const& state,std::wstring const& id){for(auto const& v:state.GetNamedArray(L"plugins",JsonArray{})){auto p=v.GetObject();if(str(p,L"id")==id)return p;}return nullptr;}
inline bool pluginInstalled(JsonObject const& state,std::wstring const& id){auto p=pluginRecord(state,id);return p&&flag(p,L"installed");}
}
