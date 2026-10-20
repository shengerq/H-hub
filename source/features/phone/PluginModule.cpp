#ifndef HHUB_SERVICE_ONLY
#include "PhonePage.h"
#endif
#include "PhoneBackend.h"
using namespace hub;
extern "C" __declspec(dllexport) HRESULT WINAPI HhubPluginRequest(wchar_t const* text,wchar_t** output){if(!output)return E_POINTER;*output=nullptr;JsonObject reply;try{reply=phoneBackend().request(object(text));}catch(hresult_error const& e){put(reply,L"ok",false);put(reply,L"error",std::wstring(e.message()));}catch(...){put(reply,L"ok",false);put(reply,L"error",std::wstring(L"手机插件调用失败"));}auto value=std::wstring(reply.Stringify());*output=(wchar_t*)CoTaskMemAlloc((value.size()+1)*sizeof(wchar_t));if(!*output)return E_OUTOFMEMORY;memcpy(*output,value.c_str(),(value.size()+1)*sizeof(wchar_t));return S_OK;}
#ifndef HHUB_SERVICE_ONLY
extern "C" __declspec(dllexport) HRESULT WINAPI HhubPluginCreateXamlPage(HWND,wchar_t const* initial,IUnknown* mount,void** context,HWND* window){if(!mount||!context||!window)return E_POINTER;*context=nullptr;*window=nullptr;try{auto page=std::make_shared<phoneUI::Page>();mount->AddRef();page->attach(mount,object(initial));*context=new std::shared_ptr<phoneUI::Page>(std::move(page));return S_OK;}catch(hresult_error const& e){return e.code().value;}catch(...){return E_FAIL;}}
extern "C" __declspec(dllexport) HRESULT WINAPI HhubPluginCreatePage(HWND,wchar_t const*,void**,HWND*){return E_NOTIMPL;}
extern "C" __declspec(dllexport) HRESULT WINAPI HhubPluginPageCommand(void* context,wchar_t const* input){try{(*static_cast<std::shared_ptr<phoneUI::Page>*>(context))->command(object(input));return S_OK;}catch(...){return E_FAIL;}}
extern "C" __declspec(dllexport) void WINAPI HhubPluginDestroyPage(void* context){auto page=static_cast<std::shared_ptr<phoneUI::Page>*>(context);(*page)->detach();delete page;}
extern "C" __declspec(dllexport) BOOL WINAPI HhubPluginCanUnloadNow(){return phoneUI::livePages.load()==0;}

#endif
