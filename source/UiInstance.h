#pragma once
#include "Common.h"

namespace hub {
inline wchar_t const* uiWindowProperty=uiWindowPropertyName();
inline HWND findUiWindow(){
 HWND found{};
 EnumWindows([](HWND window,LPARAM context)->BOOL{
  if(!GetPropW(window,uiWindowProperty))return TRUE;
  *reinterpret_cast<HWND*>(context)=window;return FALSE;
 },reinterpret_cast<LPARAM>(&found));
 return found;
}
inline void restoreUiWindow(HWND window){
 ShowWindow(window,IsIconic(window)?SW_RESTORE:SW_SHOW);
 SetForegroundWindow(window);
}
// The mutex covers initialization too, before the XAML window exists.
struct UiInstance {
 HANDLE mutex{},activation{};bool primary=false;
 UiInstance(){
  auto name=L"Local\\Hhub.UI."+userSid()+instanceSuffix();
  activation=CreateEventW(nullptr,FALSE,FALSE,(name+L".Activate").c_str());
  if(!activation)throw hresult_error(HRESULT_FROM_WIN32(GetLastError()));
  mutex=CreateMutexW(nullptr,FALSE,(name+L".Instance").c_str());
  auto error=GetLastError();
  if(!mutex){CloseHandle(activation);throw hresult_error(HRESULT_FROM_WIN32(error));}
  primary=error!=ERROR_ALREADY_EXISTS;
 }
 ~UiInstance(){CloseHandle(mutex);CloseHandle(activation);}
 void activateExisting(){SetEvent(activation);if(auto window=findUiWindow())restoreUiWindow(window);}
};
inline HANDLE uiActivationEvent{};
}
