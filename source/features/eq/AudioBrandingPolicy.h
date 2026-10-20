#pragma once
#include "SharedAudioDriverPolicy.h"
namespace hub {
inline constexpr wchar_t hhubAudioDisplayName[]=L"H hub Audio";
inline bool brandFxvadEndpoint(std::wstring const& adapter,std::wstring const& current){return fxvadAdapterIdentity(adapter)&&(sharedAudioUpper(current)==sharedAudioUpper(sharedAudioOriginalName)||legacyHhubEndpointName(current));}
}
