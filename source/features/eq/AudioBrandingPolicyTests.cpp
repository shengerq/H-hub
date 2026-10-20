#include "AudioBrandingPolicy.h"
#include <iostream>
#include <stdexcept>
int main(){using namespace hub;int checks=0;auto check=[&](bool ok){if(!ok)throw std::runtime_error("audio branding policy failed");++checks;};
 check(brandFxvadEndpoint(L"ROOT\\FXVAD\\0000",L"FxSound Audio Enhancer"));
 check(brandFxvadEndpoint(L"SWD\\MMDEVAPI\\0.ROOT\\fxvad\\0000",L"Hhub Audio"));
 check(brandFxvadEndpoint(L"ROOT\\FXVAD\\0000",L"H hub Audio"));
 check(!brandFxvadEndpoint(L"ROOT\\FXVAD\\0000",L"My headphones"));
 check(!brandFxvadEndpoint(L"ROOT\\FXVAD2\\0000",L"FxSound Audio Enhancer"));
 check(!brandFxvadEndpoint(L"USB\\HEADSET",L"FxSound Audio Enhancer"));
 check(!brandFxvadEndpoint(L"MYROOT\\FXVAD\\0000",L"FxSound Audio Enhancer"));
 check(reuseExistingSharedDriver(true,true,0));check(!reuseExistingSharedDriver(true,false,0));check(!reuseExistingSharedDriver(true,true,28));
 std::cout<<"PASS "<<checks<<" driver branding scope, custom-name preservation and reuse checks\n";
}
