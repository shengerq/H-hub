#include "SharedAudioDriverPolicy.h"
#include <iostream>
#include <stdexcept>
int main(){using namespace hub;auto check=[](bool ok,char const* name){if(!ok)throw std::runtime_error(name);};
 check(fxvadAdapterIdentity(L"ROOT\\FXVAD\\0000"),"root adapter");
 check(fxvadAdapterIdentity(L"SWD\\MMDEVAPI\\0.ROOT\\fxvad\\0000"),"endpoint adapter");
 check(!fxvadAdapterIdentity(L"ROOT\\HHUBVAD\\0000"),"dedicated experimental driver excluded");
 check(!fxvadAdapterIdentity(L"MYROOT\\FXVAD\\0000"),"lookalike prefix excluded");
 check(!fxvadAdapterIdentity(L"ROOT\\FXVAD2\\0000"),"lookalike hardware excluded");
 check(restoreLegacySharedName(L"ROOT\\FXVAD\\0000",L"Hhub Audio"),"old Hhub name eligible");
 check(restoreLegacySharedName(L"ROOT\\FXVAD\\0000",L"H hub Audio"),"old spaced name eligible");
 check(!restoreLegacySharedName(L"ROOT\\FXVAD\\0000",L"My headphones"),"user custom name preserved");
 check(!restoreLegacySharedName(L"ROOT\\FXVAD\\0000",L"FxSound Audio Enhancer"),"original name preserved");
 check(!restoreLegacySharedName(L"ROOT\\HHUBVAD\\0000",L"Hhub Audio"),"other driver preserved");
 check(reuseExistingSharedDriver(true,true,0),"working installed driver reused");
 check(!reuseExistingSharedDriver(false,true,0),"absent driver not reused");
 check(!reuseExistingSharedDriver(true,false,0),"stopped driver not healthy");
 check(!reuseExistingSharedDriver(true,true,28),"problem driver not healthy");
 std::cout<<"PASS 14 shared driver ownership, name migration and reuse checks\n";
}
