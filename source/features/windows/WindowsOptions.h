#pragma once
#include "Common.h"
namespace hub {
// Metadata only: creating the page never queries or changes Windows.
inline JsonArray windowsOptionCatalog(){
 JsonArray options;
 struct Item {wchar_t const* id;wchar_t const* name;wchar_t const* uri;};
 for(auto item:{Item{L"gameMode",L"游戏模式",L"gaming-gamemode"},
  Item{L"controllerGameBar",L"手柄打开 Game Bar",L"gaming-gamebar"},
  Item{L"capture",L"游戏录制与捕获",L"gaming-gamedvr"},
  Item{L"gpuScheduling",L"硬件加速 GPU 调度",L"display-advancedgraphics-default"},
  Item{L"windowedOptimizations",L"窗口化游戏优化",L"display-advancedgraphics-default"},
  Item{L"hdr",L"HDR",L"display"},
  Item{L"mousePrecision",L"提高指针精确度",L"mousetouchpad"},
  Item{L"stickyShortcut",L"五次 Shift 启用粘滞键",L"easeofaccess-keyboard"},
  Item{L"filterShortcut",L"长按右 Shift 启用筛选键",L"easeofaccess-keyboard"},
  Item{L"toggleShortcut",L"长按 Num Lock 启用切换键",L"easeofaccess-keyboard"},
  Item{L"notifications",L"系统通知",L"notifications"},
  Item{L"systemSound",L"系统声音",L"apps-volume"}}){
  JsonObject option;put(option,L"id",std::wstring(item.id));put(option,L"name",std::wstring(item.name));put(option,L"uri",std::wstring(L"ms-settings:")+item.uri);
  option.SetNamedValue(L"enabled",JsonValue::CreateNullValue());put(option,L"loading",true);options.Append(option);
 }
 return options;
}
}
