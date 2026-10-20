#pragma once
// Included within namespace hub after PluginIntegration.h.
inline bool isPluginNavigationOperation(std::wstring const& op){return op==L"pluginNavigate"||op==L"pluginNavigationState"||op==L"pluginNavigationAck";}
inline JsonObject pluginNavigationRequest(JsonObject state,std::wstring const& op,JsonObject body){
 if(op==L"pluginNavigate"){
  auto id=str(body,L"id");auto plugin=pluginRecord(state,id);
  if(!plugin||!integrationEnabled(plugin)||!declarationContains(plugin.GetNamedArray(L"uiFeatures",JsonArray{}),L"navigate"))throw hresult_error(E_ACCESSDENIED,L"插件未声明页面导航能力");
  auto initial=body.GetNamedObject(L"initial",JsonObject{});if(initial.Stringify().size()>65536)throw hresult_invalid_argument();
  static unsigned long long sequence=0;JsonObject pending;put(pending,L"id",id);put(pending,L"token",id+L"-"+std::to_wstring(GetTickCount64())+L"-"+std::to_wstring(++sequence));pending.SetNamedValue(L"initial",initial);state.SetNamedValue(L"pluginNavigationRequest",pending);
 }else if(op==L"pluginNavigationAck"){
  if(state.HasKey(L"pluginNavigationRequest")&&str(state.GetNamedObject(L"pluginNavigationRequest"),L"token")==str(body,L"token"))state.Remove(L"pluginNavigationRequest");
 }else if(op!=L"pluginNavigationState")throw hresult_invalid_argument();
 JsonObject result,data;put(result,L"ok",true);if(state.HasKey(L"pluginNavigationRequest"))data.SetNamedValue(L"pending",state.GetNamedObject(L"pluginNavigationRequest"));result.SetNamedValue(L"state",data);return result;
}
