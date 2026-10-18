#pragma once
#include "Common.h"
namespace hub {
struct NativePluginPage {
 PluginProcess remote;std::wstring owner=L"eq";HWND window{};void* context{};bool xaml=false;
 void create(HWND parent,JsonObject initial,IUnknown* mount=nullptr,std::wstring plugin=L"eq"){owner=plugin;if(!nativeModuleLocal){remote.start(owner,L"page",initial,parent);JsonObject ready;put(ready,L"kind",std::wstring(L"window"));window=(HWND)(uintptr_t)num(remote.call(ready),L"window");xaml=false;return;}auto& module=nativeModule(owner);module.load(owner);using CreateXaml=HRESULT(WINAPI*)(HWND,wchar_t const*,IUnknown*,void**,HWND*);auto createXaml=reinterpret_cast<CreateXaml>(GetProcAddress(module.library,"HhubPluginCreateXamlPage"));xaml=mount&&createXaml;if(xaml)check_hresult(createXaml(parent,std::wstring(initial.Stringify()).c_str(),mount,&context,&window));else check_hresult(module.createPage(parent,std::wstring(initial.Stringify()).c_str(),&context,&window));}
 void command(JsonObject request){if(!nativeModuleLocal){if(remote.alive()){JsonObject cmd;put(cmd,L"kind",std::wstring(L"pageCommand"));cmd.SetNamedValue(L"request",request);remote.call(cmd);}return;}if(context)check_hresult(nativeModule(owner).pageCommand(context,std::wstring(request.Stringify()).c_str()));}
 ~NativePluginPage(){if(!nativeModuleLocal){remote.close();return;}if(context){auto& module=nativeModule(owner);module.destroyPage(context);context=nullptr;window=nullptr;module.unload();}}
};
}
