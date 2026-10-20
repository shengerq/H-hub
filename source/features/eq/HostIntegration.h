#pragma once
// Compatibility operations, state migration and scheduling are owned by this plugin.
inline JsonObject eqHostIntegration(JsonObject request,EqAudio& engine){
 auto state=request.GetNamedObject(L"state",JsonObject{});auto body=request.GetNamedObject(L"body",JsonObject{});auto event=str(request,L"event"),op=str(request,L"operation");Model model({},false);model.state=state;
 auto output=[&]{return str(state.GetNamedObject(L"eqTemplate",JsonObject{}),L"output");};
 auto initialize=[&]{if(!state.HasKey(L"eq"))state.SetNamedValue(L"eq",JsonArray{});if(!state.HasKey(L"eqRealPrefs"))state.SetNamedValue(L"eqRealPrefs",JsonArray{});if(!state.HasKey(L"presets"))state.SetNamedValue(L"presets",JsonArray{});auto settings=state.GetNamedObject(L"settings");if(!settings.HasKey(L"eqEnabled"))put(settings,L"eqEnabled",false);if(str(state,L"eqSchema")!=L"12-band"){for(auto key:{L"presets",L"eqRealPrefs",L"eq"})for(auto const& v:state.GetNamedArray(key,JsonArray{})){auto p=v.GetObject();auto gains=p.GetNamedArray(L"bands",JsonArray{});if(gains.Size()==10){gains.Append(JsonValue::CreateNumberValue(0));gains.Append(JsonValue::CreateNumberValue(0));}JsonArray centers;for(double f:{31.25,62.5,125.0,250.0,500.0,1000.0,2000.0,4000.0,8000.0,16000.0,40.0,12000.0})centers.Append(JsonValue::CreateNumberValue(f));if(str(p,L"id")==L"flat"){centers.Clear();for(double f:{31.25,62.5,125.0,250.0,500.0,1000.0,2000.0,4000.0,8000.0,12000.0,16000.0,20000.0})centers.Append(JsonValue::CreateNumberValue(f));}if(!p.HasKey(L"frequencies"))p.SetNamedValue(L"frequencies",centers);}put(state,L"eqSchema",std::wstring(L"12-band"));}model.migrateEqPresets();
  for(auto const& v:state.GetNamedArray(L"deviceProfiles",JsonArray{})){auto p=v.GetObject();if(str(p,L"kind")==L"audio"&&str(p,L"provider")==L"generic")put(p,L"plugin",std::wstring(L"eq"));}
 };
 auto snapshot=[&]{state.SetNamedValue(L"eqRuntime",engine.snapshot(state));};
 if(event==L"initialize")initialize();
 else if(event==L"startup"){initialize();engine.nodeEnabled(flag(state.GetNamedObject(L"settings"),L"eqEnabled"),output());snapshot();}
 else if(event==L"shutdown"||event==L"stop"){engine.streams.clear();engine.nodeEnabled(false,output());for(auto const& v:state.GetNamedArray(L"eqRealPrefs",JsonArray{}))put(v.GetObject(),L"active",false);}
 else if(event==L"uninstall"){engine.streams.clear();engine.uninstall(output());model.clearNativePluginState(L"eq");}
 else if(event==L"before"){
  if(op==L"settings"&&body.HasKey(L"eqEnabled")){bool enabled=flag(body,L"eqEnabled");if(enabled&&fxSoundIsRunning())throw hresult_error(E_ABORT,L"请先退出 FxSound，再启用 Hhub 音频节点");if(!enabled){engine.streams.clear();for(auto const& v:state.GetNamedArray(L"eqRealPrefs",JsonArray{}))put(v.GetObject(),L"active",false);}engine.nodeEnabled(enabled,output());}
 }
 else if(event==L"after"){
  if(op==L"open"||op==L"snapshot"||op==L"monitor")snapshot();
  if(op==L"pluginMode"&&str(body,L"id")==L"eq"&&str(body,L"mode")!=L"disabled")engine.nodeEnabled(flag(state.GetNamedObject(L"settings"),L"eqEnabled"),output());
 }
 else if(event==L"tick"){
  bool armed=flag(state.GetNamedObject(L"settings"),L"eqEnabled");if(armed){snapshot();engine.syncSceneAudio(state);engine.syncGenericAudio(state);}
 }
 else if(event==L"request"){
  initialize();
  if(op==L"eqRuntime")snapshot();
  else if(op==L"eqPreview"){EqProcessor p;p.configure(body);engine.update(body);}
  else{
   if(op==L"eqStart"){
    if(!flag(state.GetNamedObject(L"settings"),L"eqEnabled"))throw hresult_error(E_ACCESSDENIED,L"通用 EQ 已关闭，请在插件页面启用");JsonObject config{nullptr};for(auto const& v:state.GetNamedArray(L"eqRealPrefs")){auto c=v.GetObject();if(str(c,L"app")==str(body,L"app")&&str(c,L"output")==str(body,L"output"))config=c;}if(!config)throw hresult_invalid_argument(L"请先保存 EQ 配置");engine.start(config);
   }
   if(op==L"eqStop"||op==L"eqForget")engine.stop(str(body,L"app"));
   if(op==L"audioBindingRemove"||op==L"audioBindingDisable"){auto found=engine.streams.find(str(body,L"app"));if(found!=engine.streams.end()&&str(found->second->snapshot(),L"output")==str(body,L"output"))engine.stop(str(body,L"app"));}
   if(op==L"eqRealSave"&&flag(body,L"backgroundAutoStart")){auto found=engine.streams.find(str(body,L"app"));if(found!=engine.streams.end()&&!found->second->sameRoute(body))engine.stop(str(body,L"app"));}
   JsonObject req;put(req,L"op",op);req.SetNamedValue(L"body",body);model.request(req);
   if(op==L"eqRealSave")for(auto const& v:state.GetNamedArray(L"eqRealPrefs",JsonArray{})){auto c=v.GetObject();if(str(c,L"app")==str(body,L"app")&&str(c,L"output")==str(body,L"output"))engine.update(c);}
   snapshot();
  }
 }else throw hresult_invalid_argument(L"未知 EQ 宿主事件");
 JsonObject reply;put(reply,L"retainService",flag(state.GetNamedObject(L"settings"),L"eqEnabled")&&!((event==L"shutdown")||(event==L"stop")));put(reply,L"ok",true);reply.SetNamedValue(L"state",state);put(reply,L"leaseSeconds",engine.streams.empty()?0.0:30.0);return reply;
}
