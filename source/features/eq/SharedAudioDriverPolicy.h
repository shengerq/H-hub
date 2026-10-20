#pragma once
#include <string>
#include <algorithm>
#include <cwctype>
namespace hub {
inline std::wstring sharedAudioUpper(std::wstring value){std::transform(value.begin(),value.end(),value.begin(),towupper);return value;}
inline bool legacyHhubEndpointName(std::wstring value){value=sharedAudioUpper(value);return value==L"HHUB AUDIO"||value==L"H HUB AUDIO"||value==L"虚拟音频节点 (H HUB)";}
inline bool fxvadAdapterIdentity(std::wstring value){value=sharedAudioUpper(value);auto start=value.find(L"ROOT\\FXVAD\\");return start!=std::wstring::npos&&(start==0||value[start-1]==L'.');}
inline bool restoreLegacySharedName(std::wstring const& adapter,std::wstring const& name){return fxvadAdapterIdentity(adapter)&&legacyHhubEndpointName(name);}
inline bool reuseExistingSharedDriver(bool present,bool started,unsigned long problem){return present&&started&&problem==0;}
inline constexpr wchar_t sharedAudioOriginalName[]=L"FxSound Audio Enhancer";
}
