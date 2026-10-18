#pragma once
#include <commctrl.h>
#pragma comment(lib,"comctl32.lib")

namespace hub {
inline bool unboundedUiResizeTest(){wchar_t value[8]{};GetEnvironmentVariableW(L"HHUB_ALLOW_UNBOUNDED_RESIZE",value,8);return uiMemoryTestProfile()&&value[0]==L'1';}
inline SIZE minimumUiWindowSize(HWND window){
 auto dpi=GetDpiForWindow(window);if(!dpi)dpi=96;
 SIZE minimum{MulDiv(900,dpi,96),MulDiv(640,dpi,96)};
 MONITORINFO monitor{sizeof(monitor)};
 if(GetMonitorInfoW(MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST),&monitor)){
  // Smaller work areas must still leave the whole window reachable.
  minimum.cx=std::min<LONG>(minimum.cx,monitor.rcWork.right-monitor.rcWork.left);
  minimum.cy=std::min<LONG>(minimum.cy,monitor.rcWork.bottom-monitor.rcWork.top);
 }
 return minimum;
}
inline LRESULT CALLBACK uiWindowSizePolicy(HWND window,UINT message,WPARAM wparam,LPARAM lparam,UINT_PTR id,DWORD_PTR){
 if(message==WM_NCDESTROY){RemoveWindowSubclass(window,uiWindowSizePolicy,id);return DefSubclassProc(window,message,wparam,lparam);}
 if(message==WM_GETMINMAXINFO){
  auto result=DefSubclassProc(window,message,wparam,lparam);
  if(!unboundedUiResizeTest()){auto minimum=minimumUiWindowSize(window);auto info=reinterpret_cast<MINMAXINFO*>(lparam);info->ptMinTrackSize.x=minimum.cx;info->ptMinTrackSize.y=minimum.cy;}
  return result;
 }
 if(message==WM_WINDOWPOSCHANGING&&!unboundedUiResizeTest()&&!IsIconic(window)){
  auto position=reinterpret_cast<WINDOWPOS*>(lparam);
  if(!(position->flags&(SWP_NOSIZE|SWP_HIDEWINDOW))){auto minimum=minimumUiWindowSize(window);position->cx=std::max<int>(position->cx,minimum.cx);position->cy=std::max<int>(position->cy,minimum.cy);}
 }
 return DefSubclassProc(window,message,wparam,lparam);
}
inline void installUiWindowSizePolicy(HWND window){check_bool(SetWindowSubclass(window,uiWindowSizePolicy,0x48485542,0));}
}
