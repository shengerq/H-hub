#pragma once
#include "Common.h"
#include <map>
namespace hub {
// Generic JSON transport; audio devices, processing, routes and configuration
// validation are implemented exclusively by the installed native module.
struct EqAudio {
 JsonObject run(std::wstring const& operation,JsonObject body={},JsonObject state={}){auto& module=nativeModule();if(!module.load(L"eq",false,operation==L"uninstall"))return object(LR"({"apps":[],"outputs":[],"sources":[],"streams":[]})");JsonObject request;put(request,L"op",operation);request.SetNamedValue(L"body",body);request.SetNamedValue(L"state",state);return module.invoke(request).GetNamedObject(L"data",JsonObject{});}
 struct Stream{JsonObject data;JsonObject snapshot(){return data;}bool sameRoute(JsonObject config){auto old=object(str(data,L"configuration",L"{}"));for(auto key:{L"app",L"output",L"source"})if(str(old,key)!=str(config,key))return false;return true;}};
 struct Streams:std::map<std::wstring,std::unique_ptr<Stream>>{EqAudio* owner;explicit Streams(EqAudio* value):owner(value){}void clear(){owner->run(L"clear");std::map<std::wstring,std::unique_ptr<Stream>>::clear();}} streams{this};
 void refresh(JsonObject runtime){static_cast<std::map<std::wstring,std::unique_ptr<Stream>>&>(streams).clear();for(auto const& value:runtime.GetNamedArray(L"streams",JsonArray{})){auto data=value.GetObject();streams[str(data,L"app")]=std::make_unique<Stream>(Stream{data});}}
 JsonObject snapshot(JsonObject state){if(!pluginInstalled(state,L"eq"))return object(LR"({"apps":[],"outputs":[],"sources":[],"streams":[]})");auto result=run(L"snapshot",{},state);refresh(result);return result;}
 void nodeEnabled(bool enabled,std::wstring preferred=L""){JsonObject b;put(b,L"enabled",enabled);put(b,L"output",preferred);run(L"node",b);}
 void uninstall(std::wstring preferred){JsonObject b;put(b,L"output",preferred);run(L"uninstall",b);static_cast<std::map<std::wstring,std::unique_ptr<Stream>>&>(streams).clear();nativeModule().unload();}
 void start(JsonObject config){run(L"start",config);}
 void stop(std::wstring app){JsonObject b;put(b,L"app",app);run(L"stop",b);streams.erase(app);}
 void update(JsonObject config){run(L"update",config);}
 void validate(JsonObject config){run(L"validate",config);}
 void restore(std::wstring preferred){JsonObject b;put(b,L"output",preferred);run(L"restore",b);}
 void syncSceneAudio(JsonObject state){auto result=run(L"syncScene",{},state);if(result.HasKey(L"state")){auto next=result.GetNamedObject(L"state");state.Clear();for(auto const& v:next)state.SetNamedValue(v.Key(),v.Value());}refresh(result.GetNamedObject(L"runtime",JsonObject{}));}
 void syncGenericAudio(JsonObject state){auto result=run(L"syncGeneric",{},state);if(result.HasKey(L"state")){auto next=result.GetNamedObject(L"state");state.Clear();for(auto const& v:next)state.SetNamedValue(v.Key(),v.Value());}refresh(result.GetNamedObject(L"runtime",JsonObject{}));}
};
inline bool fxSoundIsRunning(){auto& module=nativeModule();if(!module.load(L"eq",false))return false;JsonObject request;put(request,L"op",std::wstring(L"conflict"));return flag(module.invoke(request).GetNamedObject(L"data"),L"busy");}
}
