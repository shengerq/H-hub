#pragma once
#include "PhoneHost.h"
#include "EqAudio.h"
namespace hub {
inline void phoneRequest(Model& model,std::wstring op,JsonObject body){JsonObject req;put(req,L"op",op);req.SetNamedValue(L"body",body);model.request(req);}
inline JsonObject phoneEqConfig(Model& model,JsonObject body){for(auto const& v:model.state.GetNamedArray(L"eqRealPrefs",JsonArray{}))if(phoneEqKey(v.GetObject())==str(body,L"id"))return object(std::wstring(v.GetObject().Stringify()));throw hresult_invalid_argument(L"EQ 配置不存在");}
inline void phoneProfileSelect(Model& model,JsonObject body){
 auto p=find(model.state.GetNamedArray(L"deviceProfiles",JsonArray{}),str(body,L"id"));if(!p.HasKey(L"payload"))throw hresult_invalid_argument(L"此配置尚未读取保存");
 auto scene=find(model.state.GetNamedArray(L"scenes"),str(model.state,L"currentScene"));bool bound=false;for(auto const& b:scene.GetNamedArray(L"bindings",JsonArray{}))if(str(b.GetObject(),L"device")==str(p,L"device"))bound=true;
 if(!bound)throw hresult_invalid_argument(L"配置所属设备未绑定当前场景");
 auto overrides=model.state.GetNamedObject(L"deviceProfileOverrides",JsonObject{});put(overrides,str(p,L"device").c_str(),str(p,L"id"));model.state.SetNamedValue(L"deviceProfileOverrides",overrides);model.requestSceneProfiles(scene);model.save();
}
inline void phoneEqEdit(Model& model,JsonObject selected,JsonObject body){
 auto kind=str(body,L"kind");if(kind==L"eqSave"){selected.SetNamedValue(L"bands",body.GetNamedArray(L"bands"));put(selected,L"preamp",body.GetNamedNumber(L"preamp"));put(selected,L"autoPreamp",body.GetNamedBoolean(L"autoPreamp"));put(selected,L"enabled",body.GetNamedBoolean(L"enabled"));put(selected,L"preset",std::wstring(L"custom"));}
 else if(kind==L"eqPreset"){auto preset=find(model.state.GetNamedArray(L"presets"),str(body,L"preset"));for(auto key:{L"bands",L"frequencies"})selected.SetNamedValue(key,preset.GetNamedArray(key));put(selected,L"preset",str(preset,L"id"));}
 else if(kind==L"eqBackground")put(selected,L"backgroundAutoStart",body.GetNamedBoolean(L"enabled"));
 else if(kind==L"eqBypass")put(selected,L"enabled",body.GetNamedBoolean(L"enabled"));
 phoneRequest(model,L"eqRealSave",selected);
}
inline void phoneAction(Model& model,EqAudio& audio,JsonObject body){
 auto kind=str(body,L"kind");
 if(kind==L"deviceRead"||kind==L"deviceWrite"){model.requestPhoneDevice(body);return;}
 if(kind==L"scene"){JsonObject b;put(b,L"id",str(body,L"id"));put(b,L"locked",true);phoneRequest(model,L"applyScene",b);audio.syncSceneAudio(model.state);return;}
 if(kind==L"sceneAuto"){phoneRequest(model,L"unlockScene",JsonObject{});audio.syncSceneAudio(model.state);return;}
 if(kind==L"profileSelect"){phoneProfileSelect(model,body);audio.syncSceneAudio(model.state);return;}
 if(kind==L"profileRetry"){JsonObject b;put(b,L"profile",str(body,L"id"));phoneRequest(model,L"deviceProfileRetry",b);audio.syncSceneAudio(model.state);return;}
 if(kind==L"deviceVibrate"){auto id=str(body,L"id");bool found=false;for(auto const& v:model.state.GetNamedObject(L"detected",JsonObject{}).GetNamedArray(L"peripherals",JsonArray{}))if(str(v.GetObject(),L"id")==id&&id.rfind(L"xinput:",0)==0)found=true;if(!found)throw hresult_invalid_argument(L"此设备不支持振动");auto slot=id.substr(7);if(slot.size()!=1||slot[0]<L'0'||slot[0]>L'3')throw hresult_invalid_argument();XINPUT_VIBRATION vibration{12000,12000};auto index=(DWORD)(slot[0]-L'0');if(XInputSetState(index,&vibration)!=ERROR_SUCCESS)throw hresult_error(E_FAIL,L"手柄已断开");Sleep(300);vibration={};XInputSetState(index,&vibration);return;}
 if(kind!=L"eqBypass"&&kind!=L"eqSave"&&kind!=L"eqStart"&&kind!=L"eqStop"&&kind!=L"eqPreset"&&kind!=L"eqBackground")throw hresult_invalid_argument(L"未授权的手机操作");
 auto selected=phoneEqConfig(model,body);auto app=str(selected,L"app");
 if(kind==L"eqStop"){
  // Disable only this app/output binding; stopping an idle output must not affect another output.
  phoneRequest(model,L"audioBindingDisable",selected);auto found=audio.streams.find(app);if(found!=audio.streams.end()&&str(found->second->snapshot(),L"output")==str(selected,L"output")&&str(found->second->snapshot(),L"sceneProfile").empty())audio.stop(app);return;
 }
 if(!flag(model.state.GetNamedObject(L"settings"),L"eqEnabled",true))throw hresult_error(E_ACCESSDENIED,L"电脑端通用 EQ 已关闭");
 if(kind==L"eqStart"){
  auto plugin=find(model.state.GetNamedArray(L"plugins"),L"eq");if(!flag(plugin,L"installed")||str(plugin,L"mode")==L"disabled")throw hresult_error(E_ACCESSDENIED,L"请先启用 EQ 音频处理器");
  put(selected,L"backgroundAutoStart",true);put(selected,L"enabled",true);phoneRequest(model,L"eqRealSave",selected);audio.start(selected);phoneRequest(model,L"eqStart",selected);return;
 }
 phoneEqEdit(model,selected,body);if(kind==L"eqBackground"&&flag(selected,L"backgroundAutoStart")){auto found=audio.streams.find(app);if(found!=audio.streams.end()&&!found->second->sameRoute(selected))audio.stop(app);}audio.update(selected);
}
}
