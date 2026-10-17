#pragma once
#include "Common.h"
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <propvarutil.h>
#include <array>
#pragma comment(lib,"propsys.lib")
namespace hub {
// PolicyConfig ABI used by FxSound itself for endpoint properties and enable/disable.
MIDL_INTERFACE("f8679f50-850a-41cf-9c72-430f290290c8") EqNodePolicy : public IUnknown {
 virtual HRESULT STDMETHODCALLTYPE slot0()=0;virtual HRESULT STDMETHODCALLTYPE slot1()=0;virtual HRESULT STDMETHODCALLTYPE slot2()=0;virtual HRESULT STDMETHODCALLTYPE slot3()=0;virtual HRESULT STDMETHODCALLTYPE slot4()=0;virtual HRESULT STDMETHODCALLTYPE slot5()=0;virtual HRESULT STDMETHODCALLTYPE slot6()=0;virtual HRESULT STDMETHODCALLTYPE slot7()=0;virtual HRESULT STDMETHODCALLTYPE slot8()=0;
 virtual HRESULT STDMETHODCALLTYPE SetPropertyValue(LPCWSTR,PROPERTYKEY const&,PROPVARIANT*)=0;
 virtual HRESULT STDMETHODCALLTYPE SetDefaultEndpoint(LPCWSTR,ERole)=0;
 virtual HRESULT STDMETHODCALLTYPE SetEndpointVisibility(LPCWSTR,INT)=0;
};
struct EqNode {
 static std::wstring upper(std::wstring name){std::transform(name.begin(),name.end(),name.begin(),towupper);return name;}
 static bool integrated(std::wstring name){name=upper(name);return name.find(L"FXSOUND AUDIO ENHANCER")!=std::wstring::npos||name.find(L"H HUB AUDIO")!=std::wstring::npos||name.find(L"HHUB AUDIO")!=std::wstring::npos||name.find(L"虚拟音频节点 (H HUB)")!=std::wstring::npos;}
 static bool virtualOutput(std::wstring name){name=upper(name);return integrated(name)||name.find(L"CABLE")!=std::wstring::npos||name.find(L"VOICEMEETER")!=std::wstring::npos;}
 static winrt::com_ptr<IMMDeviceEnumerator> enumerator(){winrt::com_ptr<IMMDeviceEnumerator> value;check_hresult(CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_ALL,__uuidof(IMMDeviceEnumerator),value.put_void()));return value;}
 static winrt::com_ptr<EqNodePolicy> policy(){GUID cls{};check_hresult(CLSIDFromString(L"{870AF99C-171D-4F9E-AF0D-E63DF40C2BC9}",&cls));winrt::com_ptr<EqNodePolicy> value;check_hresult(CoCreateInstance(cls,nullptr,CLSCTX_ALL,__uuidof(EqNodePolicy),value.put_void()));return value;}
 static std::wstring name(IMMDevice* device){winrt::com_ptr<IPropertyStore> properties;check_hresult(device->OpenPropertyStore(STGM_READ,properties.put()));PROPVARIANT value{};check_hresult(properties->GetValue(PKEY_Device_FriendlyName,&value));std::wstring result=value.vt==VT_LPWSTR?value.pwszVal:L"";PropVariantClear(&value);return result;}
 static std::wstring id(IMMDevice* device){LPWSTR value{};check_hresult(device->GetId(&value));std::wstring result=value;CoTaskMemFree(value);return result;}
 static std::array<std::wstring,3> physicalRoleDefaults(){auto e=enumerator();std::array<std::wstring,3> roles;for(int i=0;i<3;i++){winrt::com_ptr<IMMDevice> item;if(SUCCEEDED(e->GetDefaultAudioEndpoint(eRender,(ERole)i,item.put()))&&!virtualOutput(name(item.get())))roles[i]=id(item.get());}return roles;}
 static void physicalDefaults(std::wstring preferred=L"",std::array<std::wstring,3> originals={}){
  auto e=enumerator();auto p=policy();winrt::com_ptr<IMMDeviceCollection> outputs;check_hresult(e->EnumAudioEndpoints(eRender,DEVICE_STATE_ACTIVE,outputs.put()));UINT count{};outputs->GetCount(&count);std::wstring physical;
  for(UINT i=0;i<count;i++){winrt::com_ptr<IMMDevice> item;outputs->Item(i,item.put());if(virtualOutput(name(item.get())))continue;auto candidate=id(item.get());if(physical.empty()||candidate==preferred)physical=candidate;}
  for(int role=0;role<3;role++){winrt::com_ptr<IMMDevice> current;if(SUCCEEDED(e->GetDefaultAudioEndpoint(eRender,(ERole)role,current.put()))&&integrated(name(current.get()))){auto destination=physical;if(!originals[role].empty()){winrt::com_ptr<IMMDevice> saved;DWORD state{};if(SUCCEEDED(e->GetDevice(originals[role].c_str(),saved.put()))&&SUCCEEDED(saved->GetState(&state))&&(state&DEVICE_STATE_ACTIVE)&&!virtualOutput(name(saved.get())))destination=originals[role];}if(destination.empty())throw hresult_error(E_FAIL,L"没有可恢复的物理音频输出");check_hresult(p->SetDefaultEndpoint(destination.c_str(),(ERole)role));}}
 }
 static int rename(){auto e=enumerator();auto p=policy();winrt::com_ptr<IMMDeviceCollection> endpoints;check_hresult(e->EnumAudioEndpoints(eAll,DEVICE_STATEMASK_ALL,endpoints.put()));UINT count{};endpoints->GetCount(&count);int renamed=0;
  for(UINT i=0;i<count;i++){winrt::com_ptr<IMMDevice> item;endpoints->Item(i,item.put());auto current=name(item.get());if(!integrated(current))continue;auto endpoint=id(item.get());if(current!=L"Hhub Audio"){PROPVARIANT value{};check_hresult(InitPropVariantFromString(L"Hhub Audio",&value));auto hr=p->SetPropertyValue(endpoint.c_str(),PKEY_Device_FriendlyName,&value);PropVariantClear(&value);check_hresult(hr);if(name(item.get())!=L"Hhub Audio")throw hresult_error(E_FAIL,L"虚拟音频节点名称尚未回传");}++renamed;}return renamed;
 }
 static void active(bool enabled,std::wstring preferred=L""){
  physicalDefaults(preferred);auto originals=physicalRoleDefaults();auto e=enumerator();auto p=policy();winrt::com_ptr<IMMDeviceCollection> endpoints;check_hresult(e->EnumAudioEndpoints(eAll,DEVICE_STATEMASK_ALL,endpoints.put()));UINT count{};endpoints->GetCount(&count);
  for(UINT i=0;i<count;i++){winrt::com_ptr<IMMDevice> item;endpoints->Item(i,item.put());if(!integrated(name(item.get())))continue;DWORD state{};item->GetState(&state);if(state&DEVICE_STATE_NOTPRESENT)continue;
   if(enabled?(state&DEVICE_STATE_DISABLED):(state&DEVICE_STATE_ACTIVE)){auto endpoint=id(item.get());check_hresult(p->SetEndpointVisibility(endpoint.c_str(),enabled?1:0));DWORD actual{};check_hresult(item->GetState(&actual));if(enabled?!(actual&DEVICE_STATE_ACTIVE):!(actual&DEVICE_STATE_DISABLED))throw hresult_error(E_FAIL,L"虚拟音频节点启停尚未回传");}
  }
  // Enabling a virtual endpoint must never promote it to the system default.
  physicalDefaults(preferred,originals);
 }
};
}
