#pragma once
#include "EqNode.h"
#include "AudioBrandingPolicy.h"
namespace hub {
struct AudioBrandKey{HKEY value{};~AudioBrandKey(){if(value)RegCloseKey(value);}};
inline int brandVisibleAudioEndpoints(){
 auto enumerator=EqNode::enumerator();auto policy=EqNode::policy();winrt::com_ptr<IMMDeviceCollection> endpoints;
 check_hresult(enumerator->EnumAudioEndpoints(eAll,DEVICE_STATEMASK_ALL,endpoints.put()));UINT count{};check_hresult(endpoints->GetCount(&count));int verified=0;
 for(UINT i=0;i<count;++i){
  winrt::com_ptr<IMMDevice> endpoint;check_hresult(endpoints->Item(i,endpoint.put()));if(!EqNode::upstream(endpoint.get()))continue;
  auto current=EqNode::property(endpoint.get(),PKEY_Device_FriendlyName);if(!brandFxvadEndpoint(L"ROOT\\FXVAD\\0000",current))continue;
  auto id=EqNode::id(endpoint.get());PROPVARIANT value{};check_hresult(InitPropVariantFromString(hhubAudioDisplayName,&value));
  auto result=policy->SetPropertyValue(id.c_str(),PKEY_Device_FriendlyName,&value);PropVariantClear(&value);check_hresult(result);
  winrt::com_ptr<IMMDevice> refreshed;check_hresult(enumerator->GetDevice(id.c_str(),refreshed.put()));
  if(EqNode::property(refreshed.get(),PKEY_Device_FriendlyName)!=hhubAudioDisplayName)throw hresult_error(E_FAIL,L"音频端点名称回读校验失败");++verified;
 }return verified;
}
inline int brandStoredAudioEndpoints(){
 int verified=0;
 for(auto flow:{L"Render",L"Capture"}){
  auto base=std::wstring(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\MMDevices\\Audio\\")+flow;
  HKEY root{};auto status=RegOpenKeyExW(HKEY_LOCAL_MACHINE,base.c_str(),0,KEY_READ|KEY_WOW64_64KEY,&root);
  if(status==ERROR_FILE_NOT_FOUND)continue;check_hresult(HRESULT_FROM_WIN32(status));AudioBrandKey owner{root};
  for(DWORD index=0;;++index){
   wchar_t child[256]{};DWORD length=256;status=RegEnumKeyExW(root,index,child,&length,nullptr,nullptr,nullptr,nullptr);
   if(status==ERROR_NO_MORE_ITEMS)break;check_hresult(HRESULT_FROM_WIN32(status));auto path=std::wstring(child)+L"\\Properties";
   HKEY properties{};status=RegOpenKeyExW(root,path.c_str(),0,KEY_QUERY_VALUE|KEY_WOW64_64KEY,&properties);
   if(status==ERROR_FILE_NOT_FOUND)continue;check_hresult(HRESULT_FROM_WIN32(status));AudioBrandKey readOwner{properties};
   auto get=[&](wchar_t const* key){wchar_t text[4096]{};DWORD bytes=sizeof(text);return RegGetValueW(properties,nullptr,key,RRF_RT_REG_SZ,nullptr,text,&bytes)==ERROR_SUCCESS?std::wstring(text):std::wstring{};};
   wchar_t const* friendly=L"{a45c254e-df1c-4efd-8020-67d146a850e0},14";
   if(!brandFxvadEndpoint(get(L"{b3f8fa53-0004-438e-9003-51a46e139bfc},2"),get(friendly)))continue;
   HKEY writable{};check_hresult(HRESULT_FROM_WIN32(RegOpenKeyExW(root,path.c_str(),0,KEY_SET_VALUE|KEY_WOW64_64KEY,&writable)));AudioBrandKey writeOwner{writable};
   check_hresult(HRESULT_FROM_WIN32(RegSetValueExW(writable,friendly,0,REG_SZ,reinterpret_cast<BYTE const*>(hhubAudioDisplayName),sizeof(hhubAudioDisplayName))));
   if(get(friendly)!=hhubAudioDisplayName)throw hresult_error(E_FAIL,L"音频端点持久名称回读校验失败");++verified;
  }
 }return verified;
}
}
