#pragma once
#include "Common.h"
#include <map>
namespace hub {
inline std::wstring& languageLocale(){static std::wstring locale=L"zh-CN";return locale;}
inline std::map<std::wstring,std::wstring>& languageStrings(){static std::map<std::wstring,std::wstring> strings;return strings;}
inline std::wstring resolveLanguage(std::wstring requested){if(requested==L"zh-CN"||requested==L"en-US")return requested;return PRIMARYLANGID(GetUserDefaultUILanguage())==LANG_CHINESE?L"zh-CN":L"en-US";}
inline std::filesystem::path pluginLanguageRoot(){HMODULE module{};check_bool(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&pluginLanguageRoot),&module));wchar_t path[32768]{};check_bool(GetModuleFileNameW(module,path,32768)!=0);return std::filesystem::path(path).parent_path()/L"languages";}
inline void initializeLanguage(std::wstring requested){
 languageLocale()=resolveLanguage(requested);auto& strings=languageStrings();strings.clear();
 try{auto file=pluginLanguageRoot()/(languageLocale()+L".json");if(std::filesystem::file_size(file)>1024*1024)return;auto pack=object(read(file));if(num(pack,L"schemaVersion")!=1||str(pack,L"locale")!=languageLocale())return;for(auto const& item:pack.GetNamedObject(L"strings"))if(item.Value().ValueType()==winrt::Windows::Data::Json::JsonValueType::String)strings.emplace(std::wstring(item.Key()),std::wstring(item.Value().GetString()));}catch(...){}
}
// Only source-owned literals go through this function. Device/plugin names,
// user profiles, paths, and entered text remain untouched. No per-page cache.
inline wchar_t const* tr(wchar_t const* source){auto const& strings=languageStrings();auto found=strings.find(source);return found==strings.end()?source:found->second.c_str();}
inline std::wstring translated(std::wstring const& value){return tr(value.c_str());}
}
