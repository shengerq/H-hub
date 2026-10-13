#pragma once
namespace hub {
inline bool integratedAudioName(std::wstring name){name=eqUpper(name);return name.find(L"FXSOUND AUDIO ENHANCER")!=std::wstring::npos||name.find(L"虚拟音频节点 (H HUB)")!=std::wstring::npos||name.find(L"H HUB AUDIO")!=std::wstring::npos;}
inline int renameIntegratedAudioNode(std::filesystem::path report){JsonObject result;try{auto e=audioEnumerator();winrt::com_ptr<IMMDeviceCollection> devices;check_hresult(e->EnumAudioEndpoints(eRender,DEVICE_STATE_ACTIVE,devices.put()));UINT count{};check_hresult(devices->GetCount(&count));int renamed=0;for(UINT i=0;i<count;i++){winrt::com_ptr<IMMDevice> device;check_hresult(devices->Item(i,device.put()));if(!integratedAudioName(audioName(device.get())))continue;if(audioName(device.get())==L"虚拟音频节点 (H hub)"){++renamed;continue;}auto id=audioId(device.get());auto at=id.rfind(L".{");if(at==std::wstring::npos)throw hresult_invalid_argument();auto path=L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\MMDevices\\Audio\\Render\\"+id.substr(at+1)+L"\\Properties";HKEY key{};auto opened=RegOpenKeyExW(HKEY_LOCAL_MACHINE,path.c_str(),0,KEY_SET_VALUE|KEY_WOW64_64KEY,&key);check_hresult(HRESULT_FROM_WIN32(opened));struct Close{HKEY key;~Close(){RegCloseKey(key);}}close{key};auto set=[&](wchar_t const* property,wchar_t const* value){check_hresult(HRESULT_FROM_WIN32(RegSetValueExW(key,property,0,REG_SZ,(BYTE const*)value,(DWORD)((wcslen(value)+1)*sizeof(wchar_t)))));};set(L"{a45c254e-df1c-4efd-8020-67d146a850e0},2",L"虚拟音频节点");set(L"{b3f8fa53-0004-438e-9003-51a46e139bfc},6",L"H hub");set(L"{a45c254e-df1c-4efd-8020-67d146a850e0},14",L"虚拟音频节点 (H hub)");++renamed;}put(result,L"ok",renamed>0);put(result,L"renamed",(double)renamed);if(!report.empty())write(report,std::wstring(result.Stringify()));return renamed?0:1;}catch(hresult_error const& error){put(result,L"ok",false);put(result,L"error",std::wstring(error.message()));if(!report.empty())write(report,std::wstring(result.Stringify()));return 1;}}
inline void restoreIntegratedDefaultOutput(std::wstring preferred){if(fxSoundIsRunning())return;auto devices=eqDevices();auto output=preferredEqOutput(devices.GetNamedArray(L"outputs"),preferred,str(devices,L"defaultOutput"));if(output.empty())return;auto e=audioEnumerator();GUID cls{};check_hresult(CLSIDFromString(L"{870AF99C-171D-4F9E-AF0D-E63DF40C2BC9}",&cls));winrt::com_ptr<EqDefaultPolicy> policy;check_hresult(CoCreateInstance(cls,nullptr,CLSCTX_ALL,__uuidof(EqDefaultPolicy),policy.put_void()));for(int role=0;role<3;role++){winrt::com_ptr<IMMDevice> current;if(SUCCEEDED(e->GetDefaultAudioEndpoint(eRender,(ERole)role,current.put()))&&integratedAudioName(audioName(current.get())))check_hresult(policy->SetDefaultEndpoint(output.c_str(),(ERole)role));}}
}









