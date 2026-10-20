#pragma once
#include "Common.h"
#include "SharedAudioDriverPolicy.h"
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
 static std::wstring property(IMMDevice* device,PROPERTYKEY const& key){winrt::com_ptr<IPropertyStore> properties;check_hresult(device->OpenPropertyStore(STGM_READ,properties.put()));PROPVARIANT value{};auto hr=properties->GetValue(key,&value);std::wstring result=SUCCEEDED(hr)&&value.vt==VT_LPWSTR&&value.pwszVal?value.pwszVal:L"";PropVariantClear(&value);return result;}
 static std::wstring name(IMMDevice* device){auto friendly=property(device,PKEY_Device_FriendlyName);if(!friendly.empty())return friendly;auto adapter=property(device,PKEY_DeviceInterface_FriendlyName);if(!adapter.empty())return adapter;return property(device,PKEY_Device_DeviceDesc);}
 static std::wstring id(IMMDevice* device){LPWSTR value{};check_hresult(device->GetId(&value));std::wstring result=value;CoTaskMemFree(value);return result;}
 static std::array<std::wstring,3> physicalRoleDefaults(){auto e=enumerator();std::array<std::wstring,3> roles;for(int i=0;i<3;i++){winrt::com_ptr<IMMDevice> item;if(SUCCEEDED(e->GetDefaultAudioEndpoint(eRender,(ERole)i,item.put()))&&!virtualOutput(name(item.get())))roles[i]=id(item.get());}return roles;}
 // Shared FXVAD belongs to both applications. Plugin lifecycle must never
 // change its visibility, its name, or the system default audio endpoint.
 static bool upstream(IMMDevice* device){
  PROPERTYKEY key{};check_hresult(CLSIDFromString(L"{b3f8fa53-0004-438e-9003-51a46e139bfc}",&key.fmtid));key.pid=2;
  return fxvadAdapterIdentity(property(device,key));
 }
 static int available(){auto e=enumerator();winrt::com_ptr<IMMDeviceCollection> endpoints;check_hresult(e->EnumAudioEndpoints(eRender,DEVICE_STATE_ACTIVE,endpoints.put()));UINT count{};endpoints->GetCount(&count);int found=0;for(UINT i=0;i<count;i++){winrt::com_ptr<IMMDevice> item;check_hresult(endpoints->Item(i,item.put()));if(upstream(item.get()))++found;}return found;}
 // Explicit installer migration only: undo our former branding on ROOT\\FXVAD.
 // Preserve user custom names and all non-FXVAD devices.
 static int restoreOriginalNames(){auto e=enumerator();auto p=policy();winrt::com_ptr<IMMDeviceCollection> endpoints;check_hresult(e->EnumAudioEndpoints(eAll,DEVICE_STATEMASK_ALL,endpoints.put()));UINT count{};endpoints->GetCount(&count);int restored=0;
  for(UINT i=0;i<count;i++){winrt::com_ptr<IMMDevice> item;check_hresult(endpoints->Item(i,item.put()));if(!upstream(item.get())||!legacyHhubEndpointName(property(item.get(),PKEY_Device_FriendlyName)))continue;auto endpoint=id(item.get());PROPVARIANT value{};check_hresult(InitPropVariantFromString(sharedAudioOriginalName,&value));auto hr=p->SetPropertyValue(endpoint.c_str(),PKEY_Device_FriendlyName,&value);PropVariantClear(&value);check_hresult(hr);++restored;}return restored;
 }
};
}
