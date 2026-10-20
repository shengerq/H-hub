#pragma once
#include "Common.h"
namespace hub {
struct NativePluginPage {
 PluginProcess remote;std::wstring owner;HWND window{};void* context{};bool xaml=false,local=false;
 bool alive()const{return local?context!=nullptr:remote.alive();}
 void create(HWND parent,JsonObject initial,IUnknown* mount=nullptr,std::wstring plugin=L""){if(plugin.empty())throw hresult_invalid_argument(L"必须指定插件页面所有者");owner=plugin;local=nativeModuleLocal||integratedFeatureModule(nativeManifest(owner));if(!local){remote.start(owner,L"page",initial,parent);JsonObject ready;put(ready,L"kind",std::wstring(L"window"));window=(HWND)(uintptr_t)num(remote.call(ready),L"window");xaml=false;return;}auto& module=nativeModule(owner);module.load(owner);using CreateXaml=HRESULT(WINAPI*)(HWND,wchar_t const*,IUnknown*,void**,HWND*);auto createXaml=reinterpret_cast<CreateXaml>(GetProcAddress(module.library,"HhubPluginCreateXamlPage"));xaml=mount&&createXaml;if(xaml)check_hresult(createXaml(parent,std::wstring(initial.Stringify()).c_str(),mount,&context,&window));else check_hresult(module.createPage(parent,std::wstring(initial.Stringify()).c_str(),&context,&window));}
 void command(JsonObject request){if(!local){if(remote.alive()){JsonObject cmd;put(cmd,L"kind",std::wstring(L"pageCommand"));cmd.SetNamedValue(L"request",request);remote.call(cmd);}return;}if(context)check_hresult(nativeModule(owner).pageCommand(context,std::wstring(request.Stringify()).c_str()));}
 ~NativePluginPage(){if(!local){remote.close();return;}if(context){auto& module=nativeModule(owner);module.destroyPage(context);context=nullptr;window=nullptr;module.unload();}}
};
}
