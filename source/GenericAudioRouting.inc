// Included inside EqAudio. Independent application and scene bindings override
// the general EQ; generic ownership never changes persisted application prefs.
std::set<std::wstring> genericAudioOwned;
void syncGenericAudio(JsonObject state){
 auto runtime=state.GetNamedObject(L"eqRuntime",JsonObject{});auto settings=state.GetNamedObject(L"settings");auto plugin=find(state.GetNamedArray(L"plugins"),L"eq");auto config=state.GetNamedObject(L"eqTemplate",JsonObject{});
 bool ready=flag(settings,L"eqEnabled",true)&&flag(plugin,L"installed")&&str(plugin,L"mode")!=L"disabled"&&str(plugin,L"mode")!=L"manual"&&flag(runtime,L"driverReady")&&flag(runtime,L"routingAvailable")&&config.HasKey(L"bands");
 auto outputs=runtime.GetNamedArray(L"outputs",JsonArray{}),sources=runtime.GetNamedArray(L"sources",JsonArray{});std::wstring output=preferredEqOutput(outputs,str(config,L"output"),str(runtime,L"defaultOutput"));
 ready=ready&&outputs.Size()&&sources.Size();std::map<std::wstring,JsonObject> desired;
 if(ready)for(auto const& v:runtime.GetNamedArray(L"apps",JsonArray{})){auto app=v.GetObject();auto id=str(app,L"id");if(upstreamFxSoundProcess(str(app,L"path")))continue;if(id==eqAppId((binaryDir()/L"Hhub.exe").wstring())||id==eqAppId((binaryDir()/L"Hhub.Core.exe").wstring()))continue;bool explicitConfig=sceneAudioOwned.count(id)>0;for(auto const& pref:state.GetNamedArray(L"eqRealPrefs",JsonArray{})){auto c=pref.GetObject();if(str(c,L"app")==id&&(flag(c,L"active")||flag(c,L"backgroundAutoStart")))explicitConfig=true;}if(explicitConfig)continue;
 auto c=object(std::wstring(config.Stringify()));put(c,L"app",id);put(c,L"output",output);put(c,L"source",str(sources.GetObjectAt(0),L"id"));put(c,L"enabled",true);put(c,L"genericAudio",true);desired[id]=c;}
 for(auto it=genericAudioOwned.begin();it!=genericAudioOwned.end();){if(!desired.count(*it)){auto stream=streams.find(*it);if(stream!=streams.end()){auto old=object(str(stream->second->snapshot(),L"configuration"));if(flag(old,L"genericAudio"))stop(*it);}it=genericAudioOwned.erase(it);}else ++it;}
 for(auto const& [id,c]:desired){auto stream=streams.find(id);if(stream!=streams.end()&&!genericAudioOwned.count(id))continue;if(stream==streams.end()||str(stream->second->snapshot(),L"configuration")!=std::wstring(c.Stringify()))try{start(c);genericAudioOwned.insert(id);}catch(...) {}}
}

