#pragma once
namespace hub {
inline void validateLegacyProfileDevice(JsonObject detected,JsonObject device,JsonObject plugin){std::set<std::wstring> containers;for(auto const& v:detected.GetNamedArray(L"devices",JsonArray{})){auto d=v.GetObject();if(!flag(d,L"connected",true)||str(d,L"containerId").empty())continue;for(auto const& value:plugin.GetNamedArray(L"hidTargets",JsonArray{})){auto t=value.GetObject();if(num(d,L"vid",-1)==num(t,L"vid",-2)&&num(d,L"pid",-1)==num(t,L"pid",-2))containers.insert(str(d,L"containerId"));}}if(containers.size()!=1||str(device,L"containerId").empty()||*containers.begin()!=str(device,L"containerId"))throw hresult_invalid_argument(L"请连接配置对应的唯一鼠标；同型号多个设备时不自动写入。");}
}
