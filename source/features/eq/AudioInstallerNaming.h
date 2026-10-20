#pragma once
#include "EqNode.h"
namespace hub {
struct AudioNameRegistryKey{HKEY value{};~AudioNameRegistryKey(){if(value)RegCloseKey(value);}};
// A remote desktop session may enumerate only Remote Audio. Persist the name in
// the machine endpoint store as well, without changing any signed driver file.
inline int restoreInstalledFxSoundNames(){
 auto driverId=std::wstring(L"FXVAD");
 int renamed=0;
 for(auto flow:{L"Render",L"Capture"}){
  auto base=std::wstring(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\MMDevices\\Audio\\")+flow;
  HKEY root{};auto status=RegOpenKeyExW(HKEY_LOCAL_MACHINE,base.c_str(),0,KEY_READ|KEY_WOW64_64KEY,&root);
  if(status==ERROR_FILE_NOT_FOUND)continue;check_hresult(HRESULT_FROM_WIN32(status));
  AudioNameRegistryKey rootOwner{root};
  for(DWORD index=0;;index++){
   wchar_t child[256]{};DWORD length=256;status=RegEnumKeyExW(root,index,child,&length,nullptr,nullptr,nullptr,nullptr);
   if(status==ERROR_NO_MORE_ITEMS)break;check_hresult(HRESULT_FROM_WIN32(status));
   auto path=std::wstring(child)+L"\\Properties";HKEY properties{};
   status=RegOpenKeyExW(root,path.c_str(),0,KEY_QUERY_VALUE|KEY_WOW64_64KEY,&properties);
   if(status==ERROR_FILE_NOT_FOUND)continue;check_hresult(HRESULT_FROM_WIN32(status));
   AudioNameRegistryKey propertyOwner{properties};
   auto get=[&](wchar_t const* key){wchar_t text[4096]{};DWORD bytes=sizeof(text);auto error=RegGetValueW(properties,nullptr,key,RRF_RT_REG_SZ,nullptr,text,&bytes);return error==ERROR_SUCCESS?std::wstring(text):std::wstring{};};
   // This property identifies the backing adapter, independently of any user name.
   auto adapter=EqNode::upper(get(L"{b3f8fa53-0004-438e-9003-51a46e139bfc},2"));
   auto hardware=std::wstring(L"ROOT\\")+EqNode::upper(driverId)+L"\\";
   if(!restoreLegacySharedName(adapter,get(L"{a45c254e-df1c-4efd-8020-67d146a850e0},14")))continue;
   HKEY writable{};check_hresult(HRESULT_FROM_WIN32(RegOpenKeyExW(root,path.c_str(),0,KEY_SET_VALUE|KEY_WOW64_64KEY,&writable)));AudioNameRegistryKey writableOwner{writable};
   wchar_t const* key=L"{a45c254e-df1c-4efd-8020-67d146a850e0},14";
   wchar_t const* display=sharedAudioOriginalName;
   check_hresult(HRESULT_FROM_WIN32(RegSetValueExW(writable,key,0,REG_SZ,reinterpret_cast<BYTE const*>(display),static_cast<DWORD>((wcslen(display)+1)*sizeof(wchar_t)))));
   if(get(key)!=display)throw hresult_error(E_FAIL,L"FxSound 原始节点名称恢复后校验失败");
   ++renamed;
  }
 }
 return renamed;
}
}
