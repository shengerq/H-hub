#ifdef HHUB_SERVICE_ONLY
#include "Localization.h"
#include "WindowsOptions.h"
#include "WindowsWorkerClient.h"
#else
#include "WindowsPage.h"
#endif
using namespace hub;
extern "C" __declspec(dllexport) HRESULT WINAPI HhubPluginRequest(wchar_t const* input,wchar_t** output){if(!output)return E_POINTER;*output=nullptr;JsonObject reply;try{auto request=object(input);auto op=str(request,L"op");JsonObject data;if(op==L"host"){auto state=request.GetNamedObject(L"state");auto operation=str(request,L"operation");auto body=request.GetNamedObject(L"body",JsonObject{});auto value=WindowsWorker::run(operation==L"windowsSet"?L"set":operation==L"windowsRestore"?L"restore":operation==L"windowsPower"?L"power":L"snapshot",body);state.SetNamedValue(L"windowsSettings",value);reply.SetNamedValue(L"state",state);}else if(op==L"shutdown"){}else if(op==L"catalog")data.SetNamedValue(L"options",windowsOptionCatalog());else if(op==L"cleanup")WindowsWorker::uninstall();else if(op==L"worker")data=WindowsWorker::run(str(request,L"operation"),request.GetNamedObject(L"body",JsonObject{}),flag(request,L"elevate"));else throw hresult_invalid_argument(L"未知插件调用");put(reply,L"ok",true);reply.SetNamedValue(L"data",data);}catch(hresult_error const& e){put(reply,L"ok",false);put(reply,L"error",std::wstring(e.message()));}catch(...){put(reply,L"ok",false);put(reply,L"error",std::wstring(L"Windows 设置插件调用失败"));}auto text=std::wstring(reply.Stringify());auto memory=(wchar_t*)CoTaskMemAlloc((text.size()+1)*sizeof(wchar_t));if(!memory)return E_OUTOFMEMORY;memcpy(memory,text.c_str(),(text.size()+1)*sizeof(wchar_t));*output=memory;return S_OK;}
#ifndef HHUB_SERVICE_ONLY
extern "C" __declspec(dllexport) HRESULT WINAPI HhubPluginCreateXamlPage(HWND,wchar_t const* input,IUnknown* mount,void** context,HWND* window){if(!context||!window||!mount)return E_POINTER;*context=nullptr;*window=nullptr;try{auto page=std::make_shared<windowsUI::Page>();mount->AddRef();page->attach(mount,object(input));*context=new std::shared_ptr<windowsUI::Page>(std::move(page));return S_OK;}catch(hresult_error const& e){return FAILED(e.code())?e.code().value:E_FAIL;}catch(...){return E_FAIL;}}
extern "C" __declspec(dllexport) HRESULT WINAPI HhubPluginCreatePage(HWND,wchar_t const*,void**,HWND*){return E_NOTIMPL;}
extern "C" __declspec(dllexport) HRESULT WINAPI HhubPluginPageCommand(void* context,wchar_t const* input){try{(*static_cast<std::shared_ptr<windowsUI::Page>*>(context))->command(object(input));return S_OK;}catch(...){return E_FAIL;}}
extern "C" __declspec(dllexport) void WINAPI HhubPluginDestroyPage(void* context){auto page=static_cast<std::shared_ptr<windowsUI::Page>*>(context);(*page)->detach();delete page;}
extern "C" __declspec(dllexport) BOOL WINAPI HhubPluginCanUnloadNow(){return windowsUI::livePages.load()==0;}

#endif
