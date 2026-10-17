// Included inside selfTest. No physical device or audio writes.
test(L"Phone scene/profile projection exposes confirmation and hides device payloads",[&]{
 Model m({},false);auto devices=object(LR"({"items":[{"id":"keyboard","name":"Test keyboard"}]})").GetNamedArray(L"items");m.state.SetNamedValue(L"profileDevices",devices);
 m.state.SetNamedValue(L"deviceProfiles",object(LR"({"items":[{"id":"a","name":"A","device":"keyboard","plugin":"rog","kind":"keyboard","revision":1,"payload":{"privateMacro":"do-not-export"}},{"id":"b","name":"B","device":"keyboard","plugin":"rog","kind":"keyboard","revision":1,"payload":{"privateMacro":"do-not-export"}}]})").GetNamedArray(L"items"));
 auto scene=find(m.state.GetNamedArray(L"scenes"),L"desktop");scene.SetNamedValue(L"bindings",object(LR"({"items":[{"device":"keyboard","profile":"a"}]})").GetNamedArray(L"items"));m.requestSceneProfiles(scene);
 phoneProfileSelect(m,object(LR"({"id":"b"})"));auto r=m.state.GetNamedArray(L"profileRuntime").GetObjectAt(0);ensure(str(r,L"profile")==L"b"&&str(r,L"state")==L"waitingPlugin");
 auto view=phoneView(m.state);ensure(!flag(view,L"sceneDemo")&&view.GetNamedArray(L"profiles").Size()==2);ensure(std::wstring(view.Stringify()).find(L"do-not-export")==std::wstring::npos);ensure(!view.GetNamedArray(L"profiles").GetObjectAt(1).HasKey(L"payload"));
 bool denied=false;try{phoneProfileSelect(m,object(LR"({"id":"missing"})"));}catch(...){denied=true;}ensure(denied);
});
test(L"Phone EQ stop disables only the selected output and does not claim saved active state",[&]{
 auto m=pluginFixtureModel();m.state.SetNamedValue(L"eqRuntime",object(LR"({"apps":[{"id":"app:music"}],"outputs":[{"id":"headphones","name":"Headphones"},{"id":"speakers","name":"Speakers"}],"sources":[],"streams":[]})"));
 auto config=object(std::wstring(JsonArray::Parse(fxSoundPresetData).GetObjectAt(0).Stringify()));put(config,L"source",std::wstring());put(config,L"app",std::wstring(L"app:music"));put(config,L"output",std::wstring(L"headphones"));put(config,L"preamp",0.0);put(config,L"enabled",true);put(config,L"autoPreamp",true);put(config,L"backgroundAutoStart",false);phoneRequest(m,L"eqRealSave",config);
 auto second=object(std::wstring(config.Stringify()));put(second,L"output",std::wstring(L"speakers"));put(second,L"backgroundAutoStart",true);phoneRequest(m,L"eqRealSave",second);
 EqAudio audio;JsonObject body;put(body,L"kind",std::wstring(L"eqStop"));put(body,L"id",phoneEqKey(config));phoneAction(m,audio,body);auto prefs=m.state.GetNamedArray(L"eqRealPrefs");ensure(!flag(prefs.GetObjectAt(0),L"backgroundAutoStart")&&flag(prefs.GetObjectAt(1),L"backgroundAutoStart"));
 put(prefs.GetObjectAt(1),L"active",true);auto view=phoneView(m.state);ensure(!flag(view.GetNamedArray(L"eq").GetObjectAt(1),L"active"));ensure(str(view.GetNamedArray(L"eq").GetObjectAt(1),L"outputName")==L"Speakers");
 body=JsonObject{};put(body,L"kind",std::wstring(L"eqPreset"));put(body,L"preset",std::wstring(L"fxsound-2"));phoneEqEdit(m,second,body);auto saved=m.state.GetNamedArray(L"eqRealPrefs").GetObjectAt(1);ensure(str(saved,L"preset")==L"fxsound-2"&&saved.GetNamedArray(L"frequencies").GetNumberAt(2)==110.0);
});
