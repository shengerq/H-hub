#pragma once
#include "Common.h"
#include <commctrl.h>
#include <set>
#pragma comment(lib,"comctl32.lib")

namespace hub {
inline std::wstring trayDeviceName(JsonObject device,JsonArray plugins){
 for(auto const& value:plugins){
  auto plugin=value.GetObject();
  if(!flag(plugin,L"installed")||str(plugin,L"mode")==L"disabled"||str(plugin,L"id")==L"hhub.developer-tools")continue;
  for(auto const& target:plugin.GetNamedArray(L"hidTargets",JsonArray{})){
   auto match=target.GetObject();
   if(num(device,L"vid",-1)!=num(match,L"vid",-2)||num(device,L"pid",-1)!=num(match,L"pid",-2))continue;
   auto name=str(plugin,L"deviceName");if(name.empty())name=str(plugin,L"name");
   if(!name.empty())return name;
  }
 }
 return str(device,L"name");
}
inline std::wstring trayBatteryText(JsonObject detected,JsonArray plugins=JsonArray{}){
 std::wstring text;std::set<std::wstring> seen;
 for(auto key:{L"devices",L"peripherals"})for(auto const& value:detected.GetNamedArray(key,JsonArray{})){
  auto device=value.GetObject();
  auto battery=device.GetNamedValue(L"battery",JsonValue::CreateNullValue());
  if(battery.ValueType()!=JsonValueType::Number)continue;
  auto percent=battery.GetNumber();
  if(!(percent>=0&&percent<=100))continue;
  if(device.HasKey(L"connected")&&!flag(device,L"connected"))continue;
  auto name=trayDeviceName(device,plugins);if(name.empty())continue;
  auto id=str(device,L"containerId");if(id.empty())id=str(device,L"id");if(id.empty())id=name;
  if(!seen.insert(id).second)continue;
  if(!text.empty())text+=L"\n";
  text+=name+L"："+std::to_wstring((int)percent)+L"%";
 }
 return text;
}
// A tracking tooltip avoids the tray's 127-character text limit.
struct TrayBatteryTip {
 HWND window{};TOOLINFOW info{sizeof(info)};std::wstring text;
 ULONGLONG entered{};POINT anchor{};bool visible=false;std::wstring nativeText=L"H hub 1.1";
 explicit TrayBatteryTip(HWND owner){
  INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_WIN95_CLASSES};InitCommonControlsEx(&controls);
  window=CreateWindowExW(WS_EX_TOPMOST|WS_EX_NOACTIVATE,TOOLTIPS_CLASSW,nullptr,WS_POPUP|TTS_ALWAYSTIP|TTS_NOPREFIX,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,owner,nullptr,GetModuleHandleW(nullptr),nullptr);
  info.cbSize=TTTOOLINFOW_V2_SIZE;info.hwnd=owner;info.uId=1;info.uFlags=TTF_TRACK|TTF_ABSOLUTE;info.lpszText=const_cast<wchar_t*>(L"");
  SendMessageW(window,TTM_ADDTOOLW,0,reinterpret_cast<LPARAM>(&info));SendMessageW(window,TTM_SETMAXTIPWIDTH,0,480);
 }
 ~TrayBatteryTip(){if(window)DestroyWindow(window);}
 void hide(){if(visible)SendMessageW(window,TTM_TRACKACTIVATE,FALSE,reinterpret_cast<LPARAM>(&info));visible=false;entered=0;}
 void update(NOTIFYICONDATAW const& tray,JsonObject detected,JsonArray plugins=JsonArray{}){
  auto next=trayBatteryText(detected,plugins);
  bool extended=next.size()>=std::size(tray.szTip);
  auto tip=next.empty()?std::wstring(L"H hub"):extended?std::wstring():next;
  if(tip!=nativeText){
   auto iconData=tray;iconData.uFlags=NIF_TIP;
   wcscpy_s(iconData.szTip,tip.c_str());
   if(Shell_NotifyIconW(NIM_MODIFY,&iconData))nativeText=tip;
  }
  if(!extended){hide();return;}
  NOTIFYICONIDENTIFIER icon{sizeof(icon)};icon.hWnd=tray.hWnd;icon.uID=tray.uID;
  RECT rect{};POINT cursor{};GetCursorPos(&cursor);
  if(FAILED(Shell_NotifyIconGetRect(&icon,&rect))||!PtInRect(&rect,cursor)||(GetAsyncKeyState(VK_LBUTTON)&0x8000)||(GetAsyncKeyState(VK_RBUTTON)&0x8000)){hide();return;}
  if(!entered){entered=GetTickCount64();anchor=cursor;}
  if(abs(cursor.x-anchor.x)>4||abs(cursor.y-anchor.y)>4){hide();entered=GetTickCount64();anchor=cursor;}
  if(GetTickCount64()-entered<800)return;
  if(next.empty()){hide();return;}
  if(next!=text){text=std::move(next);info.lpszText=text.data();SendMessageW(window,TTM_UPDATETIPTEXTW,0,reinterpret_cast<LPARAM>(&info));}
  if(!visible){SendMessageW(window,TTM_TRACKPOSITION,0,MAKELPARAM(cursor.x+12,cursor.y+16));SendMessageW(window,TTM_TRACKACTIVATE,TRUE,reinterpret_cast<LPARAM>(&info));visible=true;}
 }
};
}
