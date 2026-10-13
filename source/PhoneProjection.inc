// Included inside namespace hub. Only public mobile metadata crosses the network.
inline void phoneFields(JsonObject target,JsonObject source,std::initializer_list<wchar_t const*> keys){for(auto key:keys)if(source.HasKey(key))target.SetNamedValue(key,source.GetNamedValue(key));}
inline JsonObject phoneView(JsonObject state){
 JsonObject data;put(data,L"computer",std::wstring(L"H hub"));put(data,L"protocol",2.0);
 JsonObject monitor;phoneFields(monitor,state.GetNamedObject(L"monitor",JsonObject{}),{L"cpu",L"memory",L"memoryUsedGB",L"memoryTotalGB",L"gpu",L"cpuTemp",L"gpuTemp"});data.SetNamedValue(L"monitor",monitor);
 JsonArray scenes;for(auto const& v:state.GetNamedArray(L"scenes")){auto s=v.GetObject();JsonObject item;phoneFields(item,s,{L"id",L"name",L"priority"});put(item,L"appCount",(double)s.GetNamedArray(L"apps",JsonArray{}).Size());put(item,L"bindingCount",(double)s.GetNamedArray(L"bindings",JsonArray{}).Size());scenes.Append(item);}
 data.SetNamedValue(L"scenes",scenes);put(data,L"scene",str(state,L"currentScene"));put(data,L"sceneLocked",flag(state,L"sceneLocked"));put(data,L"sceneDemo",false);
 JsonArray profiles;auto current=find(state.GetNamedArray(L"scenes"),str(state,L"currentScene"));
 for(auto const& v:state.GetNamedArray(L"deviceProfiles",JsonArray{})){auto p=v.GetObject();JsonObject item;phoneFields(item,p,{L"id",L"name",L"device",L"kind",L"provider",L"revision"});put(item,L"saved",p.HasKey(L"payload"));
  for(auto const& d:state.GetNamedArray(L"profileDevices",JsonArray{}))if(str(d.GetObject(),L"id")==str(p,L"device"))put(item,L"deviceName",str(d.GetObject(),L"name"));
  bool bound=false;for(auto const& b:current.GetNamedArray(L"bindings",JsonArray{}))if(str(b.GetObject(),L"device")==str(p,L"device"))bound=true;put(item,L"inCurrentScene",bound);
  for(auto const& r:state.GetNamedArray(L"profileRuntime",JsonArray{}))if(str(r.GetObject(),L"profile")==str(p,L"id")){phoneFields(item,r.GetObject(),{L"state",L"error"});put(item,L"selected",true);}
  profiles.Append(item);
 }
 data.SetNamedValue(L"profiles",profiles);
 auto runtime=state.GetNamedObject(L"eqRuntime",JsonObject{});put(data,L"eqEngine",str(runtime,L"engine"));JsonArray eq;
 for(auto const& v:state.GetNamedArray(L"eqRealPrefs",JsonArray{})){auto p=v.GetObject();JsonObject item;put(item,L"id",phoneEqKey(p));auto app=str(p,L"app");put(item,L"name",std::filesystem::path(app.rfind(L"app:",0)==0?app.substr(4):app).filename().wstring());
  phoneFields(item,p,{L"enabled",L"bands",L"frequencies",L"preamp",L"autoPreamp",L"backgroundAutoStart",L"preset"});put(item,L"outputName",std::wstring(L"已保存的输出设备"));
  for(auto const& output:runtime.GetNamedArray(L"outputs",JsonArray{}))if(str(output.GetObject(),L"id")==str(p,L"output"))put(item,L"outputName",str(output.GetObject(),L"name"));
  bool active=false;for(auto const& stream:runtime.GetNamedArray(L"streams",JsonArray{})){auto s=stream.GetObject();if(str(s,L"app")==app&&str(s,L"output")==str(p,L"output")&&str(s,L"sceneProfile").empty()){active=true;phoneFields(item,s,{L"state",L"error",L"acknowledged",L"frames"});}}
  put(item,L"active",active);if(!active)put(item,L"state",flag(p,L"backgroundAutoStart")?std::wstring(L"等待后台音频链路"):std::wstring(L"未启动"));eq.Append(item);
 }
 data.SetNamedValue(L"eq",eq);JsonArray presets;for(auto const& v:state.GetNamedArray(L"presets",JsonArray{})){auto p=v.GetObject();if(p.GetNamedArray(L"bands",JsonArray{}).Size()!=12||p.GetNamedArray(L"frequencies",JsonArray{}).Size()!=12)continue;JsonObject item;phoneFields(item,p,{L"id",L"name"});presets.Append(item);}data.SetNamedValue(L"presets",presets);
 JsonArray devices;for(auto const& v:state.GetNamedObject(L"detected",JsonObject{}).GetNamedArray(L"peripherals",JsonArray{})){auto d=v.GetObject();JsonObject item;phoneFields(item,d,{L"id",L"name",L"kind",L"connection",L"connected",L"status",L"battery",L"batteryLevel",L"batterySource"});put(item,L"canVibrate",str(d,L"id").rfind(L"xinput:",0)==0);try{auto plugin=phoneControlPlugin(state,d);put(item,L"canConfigure",true);put(item,L"pluginName",str(plugin,L"name"));put(item,L"name",str(plugin,L"deviceName",str(plugin,L"name").c_str()));}catch(hresult_error const& e){put(item,L"canConfigure",false);put(item,L"controlReason",std::wstring(e.message()));}for(auto const& job:state.GetNamedArray(L"phoneDeviceControls",JsonArray{}))if(str(job.GetObject(),L"id")==str(d,L"id")){JsonObject status;phoneFields(status,job.GetObject(),{L"state",L"error",L"values",L"sampleTime"});item.SetNamedValue(L"configuration",status);}devices.Append(item);}data.SetNamedValue(L"devices",devices);put(data,L"sampleTime",clockSeconds());return data;
}
