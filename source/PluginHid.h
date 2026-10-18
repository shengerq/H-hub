#pragma once
#include "HidProbe.h"
namespace hub {
inline void validatePluginHidRequest(JsonObject plugin,JsonObject request){
 if(!flag(plugin,L"installed")||flag(plugin,L"forceStopped")||str(plugin,L"mode")==L"disabled")throw hresult_error(E_ACCESSDENIED,L"设备插件尚未启用");
 bool granted=false;for(auto const& value:plugin.GetNamedArray(L"permissions",JsonArray{}))if(value.GetString()==L"USB HID 配置")granted=true;
 if(!granted)throw hresult_error(E_ACCESSDENIED,L"插件未声明 USB HID 配置权限");
 auto match=request.GetNamedObject(L"match");JsonObject target{nullptr};
 for(auto const& value:plugin.GetNamedArray(L"hidTargets",JsonArray{})){auto t=value.GetObject();bool same=true;for(auto key:{L"vid",L"pid",L"usagePage",L"outputUsage",L"featureUsage"})if(num(t,key,-1)!=num(match,key,-2))same=false;if(same)target=t;}
 if(!target)throw hresult_error(E_ACCESSDENIED,L"设备不在插件声明的范围内");
 auto queries=request.GetNamedArray(L"queries");if(!queries.Size()||queries.Size()>8)throw hresult_invalid_argument(L"每次事务需要 1 至 8 条命令");
 for(auto const& value:queries){auto q=value.GetObject();if(num(q,L"reportId",-1)!=num(target,L"outputReportId",-2)||num(q,L"featureReportId",-1)!=num(target,L"featureReportId",-2))throw hresult_error(E_ACCESSDENIED,L"报告 ID 不在插件声明的范围内");auto bytes=pluginPacket(str(q,L"payload"));bool allowed=false;
  for(auto const& item:target.GetNamedArray(L"commands")){auto rule=item.GetObject();auto prefix=pluginPacket(str(rule,L"prefix"));if(bytes.size()>=prefix.size()&&bytes.size()<=num(rule,L"maxLength")&&std::equal(prefix.begin(),prefix.end(),bytes.begin())){if(flag(q,L"readOnly")&&!flag(rule,L"readOnly"))continue;allowed=true;}}
  if(!allowed)throw hresult_error(E_ACCESSDENIED,L"命令不在插件声明的范围内");
 }
}
inline JsonObject pluginHidTransaction(JsonObject state,JsonObject body){auto plugin=find(state.GetNamedArray(L"plugins"),str(body,L"id"));auto request=body.GetNamedObject(L"request");validatePluginHidRequest(plugin,request);return hidTransaction(request);}
}
