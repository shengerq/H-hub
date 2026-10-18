#pragma once
#include <windows.h>
#include <unknwn.h>
// Hhub native module ABI 1. UTF-16 JSON; reply memory uses CoTaskMemAlloc.
// All exports execute inside Hhub.PluginHost.exe, never inside Hhub.exe/Core.
extern "C" {
__declspec(dllexport) HRESULT WINAPI HhubPluginRequest(wchar_t const* request,wchar_t** reply);
__declspec(dllexport) HRESULT WINAPI HhubPluginCreatePage(HWND parent,wchar_t const* initial,void** context,HWND* window);
__declspec(dllexport) HRESULT WINAPI HhubPluginPageCommand(void* context,wchar_t const* request);
__declspec(dllexport) void WINAPI HhubPluginDestroyPage(void* context);
// Optional: build controls into the supplied WinUI Border, inside the plugin helper.
__declspec(dllexport) HRESULT WINAPI HhubPluginCreateXamlPage(HWND parent,wchar_t const* initial,IUnknown* mount,void** context,HWND* window);
__declspec(dllexport) BOOL WINAPI HhubPluginCanUnloadNow();
}
