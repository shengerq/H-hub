#pragma once

#include <windows.h>
#include <bcrypt.h>
#include <wincrypt.h>
#include <memory>
#include <map>
#include <mutex>
#include <thread>
#include <cwctype>
#include <winrt/Windows.Data.Json.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <chrono>
#include <cmath>
#include <vector>
#include <algorithm>
#include <cstdint>

namespace hub {
using namespace winrt;
using namespace Windows::Data::Json;
inline std::wstring str(JsonObject const& o, wchar_t const* k, wchar_t const* fallback=L"") { return std::wstring(o.GetNamedString(k, fallback)); }
inline double num(JsonObject const& o, wchar_t const* k, double fallback=0) { return o.GetNamedNumber(k, fallback); }
inline bool flag(JsonObject const& o, wchar_t const* k, bool fallback=false) { return o.GetNamedBoolean(k, fallback); }
inline void put(JsonObject const& o, wchar_t const* k, std::wstring const& v) { o.SetNamedValue(k, JsonValue::CreateStringValue(v)); }
inline void put(JsonObject const& o, wchar_t const* k, double v) { o.SetNamedValue(k, JsonValue::CreateNumberValue(v)); }
inline void put(JsonObject const& o, wchar_t const* k, bool v) { o.SetNamedValue(k, JsonValue::CreateBooleanValue(v)); }
} // namespace hub

namespace hub {
inline JsonObject find(JsonArray const& a, std::wstring const& id) { for(auto const& v:a) { auto o=v.GetObject(); if(str(o,L"id")==id) return o; } throw hresult_invalid_argument(L"找不到记录"); }
inline JsonObject object(std::wstring const& text) { return JsonObject::Parse(text); }
inline std::wstring read(std::filesystem::path const& p) { std::ifstream in(p,std::ios::binary); std::string s((std::istreambuf_iterator<char>(in)),{}); return std::wstring(to_hstring(s)); }
inline void write(std::filesystem::path const& p, std::wstring const& text) { std::filesystem::create_directories(p.parent_path()); auto tmp=p;tmp+=L".tmp"; {std::ofstream out(tmp,std::ios::binary|std::ios::trunc); auto utf=to_string(text);out.write(utf.data(),utf.size());if(!out)throw hresult_error(E_FAIL,L"无法保存配置");} if(!MoveFileExW(tmp.c_str(),p.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw hresult_error(HRESULT_FROM_WIN32(GetLastError())); }
#ifdef HHUB_HOST_MODEL_TESTS
#include HHUB_HOST_MODEL_TESTS
#else
inline constexpr bool mousePortTestProfile(){return false;}
inline constexpr bool uiMemoryTestProfile(){return false;}
inline std::wstring instanceSuffix(){wchar_t value[80]{};GetEnvironmentVariableW(L"HHUB_INSTANCE",value,80);std::wstring id(value);if(id.size()>64||!std::all_of(id.begin(),id.end(),[](wchar_t c){return iswalnum(c)||c==L'-';}))throw hresult_invalid_argument(L"Invalid instance");return id.empty()?L"":L"."+id;}
inline constexpr bool unboundedUiResizeTest(){return false;}
inline wchar_t const* uiWindowPropertyName(){static std::wstring name=L"Hhub.UI.MainWindow"+instanceSuffix();return name.c_str();}
inline std::filesystem::path dataDir(){wchar_t p[32768]{};GetEnvironmentVariableW(L"LOCALAPPDATA",p,32768);return std::filesystem::path(p)/L"HhubPrototype";}
#endif
inline double clockSeconds() { return (double)std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count(); }
inline bool hasOfflineUi(JsonObject const& plugin){auto value=plugin.GetNamedValue(L"ui",JsonValue::CreateNullValue());return value.ValueType()==JsonValueType::Object&&str(value.GetObject(),L"type")==L"offline-html"&&!str(value.GetObject(),L"content").empty();}
inline JsonObject pluginRecord(JsonObject const& state,std::wstring const& id){for(auto const& v:state.GetNamedArray(L"plugins",JsonArray{})){auto p=v.GetObject();if(str(p,L"id")==id)return p;}return nullptr;}
inline bool pluginInstalled(JsonObject const& state,std::wstring const& id){auto p=pluginRecord(state,id);return p&&flag(p,L"installed");}
inline bool hasNativeModule(JsonObject const& plugin){return str(plugin.GetNamedObject(L"implementation",JsonObject{}),L"type")==L"native-module";}
inline std::wstring nativePluginPage(JsonObject const& plugin){return hasNativeModule(plugin)?L"plugin:"+str(plugin,L"id"):std::wstring();}
inline void validateIntegrationManifest(JsonObject plugin);
inline void validatePluginUi(JsonObject const& plugin){
 validateIntegrationManifest(plugin);
 if(plugin.HasKey(L"nativePage")||hasNativeModule(plugin)){auto host=plugin.GetNamedObject(L"requiredHost",JsonObject{});auto api=str(host,L"apiVersion");if(nativePluginPage(plugin).empty()||str(host,L"id")!=L"hhub"||(api!=L"1.0"&&api!=L"1.1"&&api!=L"1.2"&&api!=L"1.3"&&api!=L"1.4")||str(host,L"execution")!=L"host-only"||flag(host,L"standalone",true))throw hresult_invalid_argument(L"原生功能插件的宿主契约无效");if(hasNativeModule(plugin)){auto capabilities=plugin.GetNamedArray(L"capabilities",JsonArray{});if(capabilities.Size()>64)throw hresult_invalid_argument(L"插件能力声明过多");for(auto const& value:capabilities){auto name=value.GetString();if(name.empty()||name.size()>80)throw hresult_invalid_argument(L"插件能力声明无效");}auto impl=plugin.GetNamedObject(L"implementation");if(num(impl,L"abiVersion")!=1||str(impl,L"library").empty())throw hresult_invalid_argument(L"插件 ABI 或入口无效");}}

 if(!plugin.HasKey(L"ui"))return;auto ui=plugin.GetNamedObject(L"ui");auto content=str(ui,L"content");
 if(str(ui,L"type")!=L"offline-html"||content.empty()||content.size()>8*1024*1024||str(plugin,L"apiVersion")!=L"1.0")throw hresult_invalid_argument(L"离线插件页面格式或 API 版本不受支持");


}
inline std::vector<unsigned char> pluginPacket(std::wstring const& text){
 std::vector<unsigned char> bytes;int high=-1;for(wchar_t c:text){if(c==L' '||c==L'\t'||c==L'\r'||c==L'\n')continue;int digit=c>=L'0'&&c<=L'9'?c-L'0':c>=L'a'&&c<=L'f'?c-L'a'+10:c>=L'A'&&c<=L'F'?c-L'A'+10:-1;if(digit<0)throw hresult_invalid_argument(L"插件报文不是十六进制字节");if(high<0)high=digit;else{bytes.push_back((unsigned char)(high*16+digit));high=-1;}if(bytes.size()>4096)throw hresult_invalid_argument(L"插件报文过大");}if(high>=0||bytes.empty())throw hresult_invalid_argument(L"插件报文为空或不完整");return bytes;
}
inline void validatePluginModule(JsonObject const& plugin){
 if(plugin.HasKey(L"module"))throw hresult_invalid_argument(L"请将协议与参数编码放在插件实现中，主程序不解释内嵌设备协议");
}
#include "ManifestNativeAssets.inc"
#include "PluginProcess.inc"
#include "NativeModuleBridge.inc"
#include "PluginNavigation.h"
#include "BuiltinFeatures.inc"
inline wchar_t const* initialState=LR"JSON({"schema":1,"demo":false,"selectedDevice":"","currentScene":"desktop","sceneLocked":false,"frontendApp":"","checks":0,"settings":{"language":"system","theme":"light","startup":false,"autoUpdate":true,"appAutoUpdate":true,"appAutoDetect":true,"compact":false,"reduceMotion":false},"devices":[],"plugins":[],"scenes":[{"id":"desktop","name":"日常桌面","app":"","apps":[],"priority":0,"bindings":[]}],"events":[]})JSON";


struct Model {
 JsonObject state{nullptr};std::filesystem::path file;bool persistent{};
 void clearNativePluginState(std::wstring const& id){if(auto plugin=pluginRecord(state,id))clearOwnedPluginState(state,plugin);}

 explicit Model(std::filesystem::path p={},bool save=true):file(std::move(p)),persistent(save) {
  state=object(initialState);
  if(persistent&&std::filesystem::exists(file)) { try{auto loaded=object(read(file));if(num(loaded,L"schema")==1)state=loaded;}catch(...){} }
  if(state.HasKey(L"monitor"))state.Remove(L"monitor");if(state.HasKey(L"detected"))state.Remove(L"detected");stripPluginTransientState(state);
  if(persistent&&!file.empty())provisionBuiltinFeatures(state,file.parent_path());
  auto installedPlugins=state.GetNamedArray(L"plugins");
  for(auto const& v:state.GetNamedArray(L"plugins")) {auto p=v.GetObject();put(p,L"runtime",std::wstring(L"stopped"));put(p,L"leaseUntil",0.0);}
  for(auto const& v:installedPlugins){auto plugin=v.GetObject();if(flag(plugin,L"installed")&&plugin.HasKey(L"nativeAssets"))installedNativeManifests()[str(plugin,L"id")]=plugin;}
  if(persistent)this->save();
  for(auto const& v:installedPlugins){auto plugin=v.GetObject();if(str(plugin,L"mode")==L"manual")continue;try{initializePluginIntegration(state,plugin);}catch(hresult_error const& e){pluginFailures()[str(plugin,L"id")]=std::wstring(e.message());}}
  if(mousePortTestProfile()){auto settings=state.GetNamedObject(L"settings");put(settings,L"autoUpdate",false);put(settings,L"appAutoUpdate",false);put(settings,L"startup",false);}
  migrateSceneConfigurations();requestSceneProfiles(find(state.GetNamedArray(L"scenes"),str(state,L"currentScene")));reconcile();
 }
 void save(){registerNativeManifests(state);if(persistent){auto saved=object(std::wstring(state.Stringify()));if(saved.HasKey(L"pluginNavigationRequest"))saved.Remove(L"pluginNavigationRequest");if(saved.HasKey(L"pluginTasks"))saved.Remove(L"pluginTasks");if(saved.HasKey(L"deviceProfileOverrides"))saved.Remove(L"deviceProfileOverrides");if(saved.HasKey(L"profileRuntime"))saved.Remove(L"profileRuntime");if(saved.HasKey(L"monitor"))saved.Remove(L"monitor");if(saved.HasKey(L"detected"))saved.Remove(L"detected");stripPluginTransientState(saved);if(saved.HasKey(L"applications"))saved.Remove(L"applications");if(saved.HasKey(L"updates"))saved.Remove(L"updates");write(file,std::wstring(saved.Stringify()));}}
 void log(std::wstring const& text){auto a=state.GetNamedArray(L"events");a.InsertAt(0,JsonValue::CreateStringValue(text));while(a.Size()>30)a.RemoveAtEnd();}
 void reconcile(){for(auto const& v:state.GetNamedArray(L"plugins")){auto p=v.GetObject();auto mode=str(p,L"mode");bool running=flag(p,L"installed")&&!flag(p,L"forceStopped")&&flag(p,L"connected")&&mode!=L"disabled"&&(mode==L"resident"||num(p,L"leaseUntil")>clockSeconds());put(p,L"runtime",running?std::wstring(L"ready"):std::wstring(L"stopped"));}}
#include "SceneConfigurationModel.inc"
 #include "PluginTaskModel.inc"
 void applyScene(JsonObject const& scene){if(str(state,L"currentScene")!=str(scene,L"id"))state.SetNamedValue(L"deviceProfileOverrides",JsonObject{});put(state,L"currentScene",str(scene,L"id"));requestSceneProfiles(scene);log(L"已选择场景："+str(scene,L"name"));}
 JsonObject request(JsonObject const& req){auto op=str(req,L"op");auto b=req.GetNamedObject(L"body",JsonObject{});reconcile();
  if(isPluginNavigationOperation(op))return pluginNavigationRequest(state,op,b);
  if(op==L"pluginTaskValidate"||op==L"pluginTaskAck")return pluginTaskWorkerRequest(op,b);
  if(op==L"pluginTaskSubmit"){submitPluginTask(b);JsonObject result;put(result,L"ok",true);result.SetNamedValue(L"state",state);return result;}
  if(op==L"deviceProfileWorkerState"){JsonArray plugins;std::vector<std::wstring> seen;for(auto const& v:state.GetNamedArray(L"profileRuntime",JsonArray{})){auto item=v.GetObject();if(str(item,L"state")!=L"waitingPlugin"&&str(item,L"state")!=L"confirmed")continue;try{auto p=find(state.GetNamedArray(L"deviceProfiles"),str(item,L"profile"));auto id=str(p,L"plugin");if(id.empty()||!p.HasKey(L"payload"))continue;auto plugin=find(state.GetNamedArray(L"plugins"),id);if(!flag(plugin,L"installed")||flag(plugin,L"forceStopped")||str(plugin,L"mode")==L"disabled"||str(plugin,L"mode")==L"manual")continue;auto html=str(plugin.GetNamedObject(L"ui",JsonObject{}),L"content");if(num(plugin.GetNamedObject(L"ui",JsonObject{}),L"profileAdapterVersion")!=1)continue;if(std::find(seen.begin(),seen.end(),id)==seen.end()){plugins.Append(JsonValue::CreateStringValue(id));seen.push_back(id);}}catch(...) {}}bool registered=false;expirePluginTasks();for(auto const& value:state.GetNamedArray(L"pluginTasks",JsonArray{})){auto job=value.GetObject();if(str(job,L"state")!=L"waiting"&&str(job,L"state")!=L"running")continue;auto id=str(job,L"plugin");if(std::find(seen.begin(),seen.end(),id)==seen.end()){plugins.Append(JsonValue::CreateStringValue(id));seen.push_back(id);}registered=true;}for(auto const& v:state.GetNamedArray(L"deviceProfiles",JsonArray{})){auto p=v.GetObject();if(!p.HasKey(L"payload")||str(p,L"plugin").empty()||nativeProfileProvider(state,p))continue;for(auto const& scene:state.GetNamedArray(L"scenes"))for(auto const& binding:scene.GetObject().GetNamedArray(L"bindings",JsonArray{}))if(str(binding.GetObject(),L"profile")==str(p,L"id"))registered=true;}JsonObject data,result;put(data,L"registered",registered);data.SetNamedValue(L"plugins",plugins);data.SetNamedValue(L"manifestRecords",state.GetNamedArray(L"plugins"));std::wstring revision;for(auto const& value:state.GetNamedArray(L"plugins")){auto p=value.GetObject();if(p.HasKey(L"workerExtensions")&&flag(p,L"installed")&&!flag(p,L"forceStopped")&&str(p,L"mode")!=L"disabled")revision+=str(p,L"id")+str(p,L"version")+std::wstring(p.GetNamedArray(L"nativeAssets",JsonArray{}).Stringify());}put(data,L"extensionRevision",revision);put(result,L"ok",true);result.SetNamedValue(L"state",data);return result;}
  if(op==L"deviceProfileManual"){auto p=find(state.GetNamedArray(L"deviceProfiles"),str(b,L"profile"));if(str(p,L"plugin")!=str(b,L"plugin"))throw hresult_invalid_argument();auto overrides=state.GetNamedObject(L"deviceProfileOverrides",JsonObject{});put(overrides,str(p,L"device").c_str(),str(p,L"id"));state.SetNamedValue(L"deviceProfileOverrides",overrides);requestSceneProfiles(find(state.GetNamedArray(L"scenes"),str(state,L"currentScene")));for(auto const& v:state.GetNamedArray(L"profileRuntime")){auto item=v.GetObject();if(str(item,L"profile")==str(p,L"id"))put(item,L"state",std::wstring(L"confirmed"));}save();JsonObject result;put(result,L"ok",true);result.SetNamedValue(L"state",state);return result;}
  if(op==L"deviceProfileValidate"){auto p=find(state.GetNamedArray(L"deviceProfiles"),str(b,L"profile"));if(str(p,L"plugin")!=str(b,L"plugin"))throw hresult_invalid_argument();auto plugin=find(state.GetNamedArray(L"plugins"),str(b,L"plugin"));if(!flag(plugin,L"installed")||flag(plugin,L"forceStopped")||str(plugin,L"mode")==L"disabled")throw hresult_invalid_argument(L"插件未启用");JsonObject result,data;put(result,L"ok",true);put(data,L"valid",true);result.SetNamedValue(L"state",data);return result;}
  if(op==L"deviceProfileState"){auto plugin=find(state.GetNamedArray(L"plugins"),str(b,L"plugin"));if(!flag(plugin,L"installed")||flag(plugin,L"forceStopped")||str(plugin,L"mode")==L"disabled")throw hresult_invalid_argument(L"插件未启用");JsonObject data;JsonArray profiles,pending;for(auto const& v:state.GetNamedArray(L"deviceProfiles",JsonArray{})){auto p=v.GetObject();if(str(p,L"plugin")==str(b,L"plugin")){auto copy=object(std::wstring(p.Stringify()));put(copy,L"deviceName",str(find(state.GetNamedArray(L"profileDevices"),str(p,L"device")),L"name"));JsonObject identity;auto device=find(state.GetNamedArray(L"profileDevices"),str(p,L"device"));put(identity,L"id",str(device,L"id"));put(identity,L"containerId",str(device,L"containerId"));copy.SetNamedValue(L"deviceIdentity",identity);profiles.Append(copy);}}for(auto const& v:state.GetNamedArray(L"profileRuntime",JsonArray{}))if(str(v.GetObject(),L"plugin")==str(b,L"plugin"))pending.Append(v);data.SetNamedValue(L"profiles",profiles);data.SetNamedValue(L"pending",pending);data.SetNamedValue(L"remote",pluginTasks(str(b,L"plugin")));JsonObject result;put(result,L"ok",true);result.SetNamedValue(L"state",data);return result;}
  if(op==L"monitorPause"||op==L"monitorElevate"||op==L"applicationScan"||op==L"updateState"||op==L"pluginUpdate"||op==L"snapshot"||op==L"monitor"||op==L"monitorStop"||op==L"deviceScan"||op==L"deviceSnapshot"){}
  else if(op==L"open"||op==L"checkUpdates"){if(op==L"checkUpdates"||flag(state.GetNamedObject(L"settings"),L"autoUpdate")){put(state,L"checks",num(state,L"checks")+1);log(L"检查更新（仅界面打开/手动触发）");}}
  else if(op==L"deviceProfileSelect"){auto p=find(state.GetNamedArray(L"deviceProfiles"),str(b,L"id"));if(!p.HasKey(L"payload"))throw hresult_invalid_argument(L"此配置尚未读取保存");auto scene=find(state.GetNamedArray(L"scenes"),str(state,L"currentScene"));bool bound=false;for(auto const& v:scene.GetNamedArray(L"bindings",JsonArray{}))if(str(v.GetObject(),L"device")==str(p,L"device"))bound=true;if(!bound)throw hresult_invalid_argument(L"设备未绑定当前场景");auto overrides=state.GetNamedObject(L"deviceProfileOverrides",JsonObject{});put(overrides,str(p,L"device").c_str(),str(p,L"id"));state.SetNamedValue(L"deviceProfileOverrides",overrides);requestSceneProfiles(scene);}
  else if(op==L"settings"){auto settings=state.GetNamedObject(L"settings");for(auto const& kv:b){if(kv.Key()!=L"language"&&kv.Key()!=L"theme"&&kv.Key()!=L"startup"&&kv.Key()!=L"autoUpdate"&&kv.Key()!=L"compact"&&kv.Key()!=L"appAutoUpdate"&&kv.Key()!=L"appAutoDetect"&&!pluginSettingDeclared(state,std::wstring(kv.Key()))&&kv.Key()!=L"reduceMotion")throw hresult_invalid_argument();if(kv.Key()==L"language"){auto value=kv.Value().GetString();if(value!=L"system"&&value!=L"zh-CN"&&value!=L"en-US")throw hresult_invalid_argument(L"Unsupported language");}settings.SetNamedValue(kv.Key(),kv.Value());}}
  else if(op==L"selectDetectedDevice"){find(state.GetNamedObject(L"detected").GetNamedArray(L"devices"),str(b,L"id"));put(state,L"selectedDetectedDevice",str(b,L"id"));}
  else if(op==L"selectDevice"){find(state.GetNamedArray(L"devices"),str(b,L"id"));put(state,L"selectedDevice",str(b,L"id"));}
  else if((op==L"pluginRemove"||op==L"pluginMode")&&isBuiltinFeature(str(b,L"id")))throw hresult_invalid_argument(L"内置功能请在对应页面启用或关闭");
  else if(op==L"pluginMode"){auto p=find(state.GetNamedArray(L"plugins"),str(b,L"id"));auto mode=str(b,L"mode");if(mode!=L"auto"&&mode!=L"resident"&&mode!=L"manual"&&mode!=L"disabled")throw hresult_invalid_argument();put(p,L"mode",mode);put(p,L"leaseUntil",0.0);log(L"已改变插件策略："+str(p,L"name"));}
  else if(op==L"pluginRemove"&&(find(state.GetNamedArray(L"plugins"),str(b,L"id")).HasKey(L"nativeAssets")||!nativePluginPage(find(state.GetNamedArray(L"plugins"),str(b,L"id"))).empty())){auto id=str(b,L"id");if(persistent&&!file.empty())removeNativePluginAssets(file.parent_path(),id);clearNativePluginState(id);auto entries=state.GetNamedArray(L"plugins");for(uint32_t i=entries.Size();i>0;--i)if(str(entries.GetObjectAt(i-1),L"id")==id)entries.RemoveAt(i-1);log(L"已卸载插件并清理配置："+id);}
  else if(op==L"pluginStart"||op==L"pluginStop"||op==L"pluginInstall"||op==L"pluginRemove"||op==L"pluginIsolation"){auto p=find(state.GetNamedArray(L"plugins"),str(b,L"id"));if(op==L"pluginStart"){if(str(p,L"mode")==L"disabled"||!flag(p,L"installed"))throw hresult_error(E_ACCESSDENIED,L"插件不可启动");put(p,L"forceStopped",false);installedNativeManifests()[str(p,L"id")]=p;put(p,L"leaseUntil",clockSeconds()+300);}else if(op==L"pluginStop") {put(p,L"leaseUntil",0.0);put(p,L"mode",std::wstring(L"manual"));}else if(op==L"pluginIsolation")put(p,L"isolated",flag(b,L"isolated",true));else {put(p,L"installed",op==L"pluginInstall");put(p,L"leaseUntil",0.0);if(op==L"pluginRemove"&&p.HasKey(L"ui"))p.Remove(L"ui");}log(L"插件操作："+str(p,L"name"));}
  else if(op==L"importPlugin"){auto p=object(std::wstring(b.GetNamedObject(L"manifest").Stringify()));validatePluginUi(p);validatePluginModule(p);auto id=str(p,L"id");if(isBuiltinFeature(id))throw hresult_invalid_argument(L"此功能随应用提供，请更新完整应用");if(id.empty()||id.size()>80||str(p,L"name").empty()||str(p,L"version").empty()||str(p,L"author").empty())throw hresult_invalid_argument(L"插件必须包含 id/name/version/author");auto url=str(p,L"updateUrl");if(!url.empty()&&url.rfind(L"https://",0)!=0)throw hresult_invalid_argument(L"更新地址必须使用 HTTPS");auto existing=state.GetNamedArray(L"plugins");int replace=-1;bool wasInstalled=false;for(uint32_t i=0;i<existing.Size();i++)if(str(existing.GetObjectAt(i),L"id")==id){wasInstalled=flag(existing.GetObjectAt(i),L"installed");if(wasInstalled&&(!flag(b,L"replaceInstalled")||str(existing.GetObjectAt(i),L"author")!=str(p,L"author")))throw hresult_invalid_argument(L"插件 ID 已存在；更新来源变更不能静默导入");replace=(int)i;}if(!p.HasKey(L"permissions"))p.SetNamedValue(L"permissions",JsonArray{});auto perms=p.GetNamedArray(L"permissions");if(perms.Size()>32)throw hresult_invalid_argument();for(auto const& v:perms) v.GetString();if(p.HasKey(L"nativeAssets")){auto& module=nativeModule(id);module.unload();if(module.library)throw hresult_error(E_PENDING,L"插件操作仍在完成，请稍后重试");}auto nativeAssets=installNativePluginAssets(p,persistent&&!file.empty()?file.parent_path():std::filesystem::path{});put(p,L"mode",wasInstalled?str(existing.GetObjectAt(replace),L"mode",L"auto"):std::wstring(L"auto"));put(p,L"runtime",std::wstring(L"stopped"));put(p,L"isolated",nativePluginPage(p).empty());put(p,L"installed",true);put(p,L"connected",hasOfflineUi(p)||!nativePluginPage(p).empty());put(p,L"leaseUntil",0.0);put(p,L"update",!nativePluginPage(p).empty()?std::wstring(L"使用 Hhub 原生页面与后台服务"):hasOfflineUi(p)?std::wstring(L"离线页面插件，未运行硬件 Worker"):std::wstring(L"仅登记元信息，未执行代码"));if(replace>=0)existing.SetAt(replace,p);else existing.Append(p);if(!nativePluginPage(p).empty()){if(p.HasKey(L"nativeAssets"))installedNativeManifests()[id]=p;initializePluginIntegration(state,p);}log(L"已安装本地插件："+str(p,L"name"));if(nativeAssets)nativeAssets->committed=true;}

  else if(op==L"deviceProfileSave"||op==L"deviceProfileDelete"||op==L"sceneBindProfile"||op==L"sceneUnbindProfile"||op==L"deviceProfileCapture"||op==L"deviceProfileAck"||op==L"deviceProfileRetry")sceneConfigurationRequest(op,b);
  else if(op==L"saveScene"){auto scene=b.GetNamedObject(L"scene");auto id=str(scene,L"id");if(id.empty()||str(scene,L"name").empty())throw hresult_invalid_argument(L"场景名称不能为空");if(!scene.HasKey(L"apps")){JsonArray apps;if(!str(scene,L"app").empty())apps.Append(JsonValue::CreateStringValue(str(scene,L"app")));scene.SetNamedValue(L"apps",apps);}if(scene.GetNamedArray(L"apps").Size()>64)throw hresult_invalid_argument(L"单个场景最多关联 64 个应用");for(auto const& value:scene.GetNamedArray(L"apps"))if(value.ValueType()!=JsonValueType::String||value.GetString().empty())throw hresult_invalid_argument(L"无效应用路径");if(!std::isfinite(num(scene,L"priority"))||num(scene,L"priority")<0||num(scene,L"priority")>1000)throw hresult_invalid_argument(L"优先级范围 0–1000");if(!scene.HasKey(L"bindings"))scene.SetNamedValue(L"bindings",JsonArray{});std::vector<std::wstring> seen;for(auto const& v:scene.GetNamedArray(L"bindings")){auto b=v.GetObject();auto profile=find(state.GetNamedArray(L"deviceProfiles"),str(b,L"profile"));auto device=str(profile,L"device");if(device!=str(b,L"device")||std::find(seen.begin(),seen.end(),device)!=seen.end())throw hresult_invalid_argument(L"一个场景内每个外设只能绑定一套配置");seen.push_back(device);}auto a=state.GetNamedArray(L"scenes");bool replaced=false;for(uint32_t i=0;i<a.Size();++i)if(str(a.GetObjectAt(i),L"id")==id){a.SetAt(i,scene);replaced=true;break;}if(!replaced)a.Append(scene);if(str(scene,L"id")==str(state,L"currentScene"))requestSceneProfiles(scene);log(L"已保存场景："+str(scene,L"name"));}
  else if(op==L"applyScene"){applyScene(find(state.GetNamedArray(L"scenes"),str(b,L"id")));put(state,L"sceneLocked",flag(b,L"locked",true));}
  else if(op==L"foregroundApp"){auto app=str(b,L"path");put(state,L"frontendApp",app);if(!flag(state,L"sceneLocked")){auto target=matchingScene(app);applyScene(target);}log(app.empty()?L"前台程序退出":L"前台程序："+app);}
  else if(op==L"lockScene"){put(state,L"sceneLocked",true);}
  else if(op==L"unlockScene"){put(state,L"sceneLocked",false);auto r=object(L"{\"op\":\"foregroundApp\",\"body\":{}}");put(r.GetNamedObject(L"body"),L"path",str(state,L"frontendApp"));return request(r);}
  else if(op==L"pluginData"){auto id=str(b,L"id");auto plugin=find(state.GetNamedArray(L"plugins"),id);if(!flag(plugin,L"installed")||flag(plugin,L"forceStopped")||str(plugin,L"mode")==L"disabled")throw hresult_error(E_ACCESSDENIED,L"插件未运行");auto data=state.GetNamedObject(L"pluginData",JsonObject{});if(b.HasKey(L"value")){auto value=b.GetNamedObject(L"value");if(value.Stringify().size()>512*1024)throw hresult_invalid_argument(L"插件配置过大");data.SetNamedValue(id,object(std::wstring(value.Stringify())));state.SetNamedValue(L"pluginData",data);}}
  else if(op==L"quit"||op==L"pluginInvoke"||op==L"pluginRuntime"||op==L"pluginForceStop"){}
  else throw hresult_invalid_argument(L"未知操作");
  reconcile();if(op!=L"monitorPause"&&op!=L"monitorElevate"&&op!=L"applicationScan"&&op!=L"updateState"&&op!=L"snapshot"&&op!=L"monitor"&&op!=L"monitorStop"&&op!=L"deviceScan"&&op!=L"deviceSnapshot")save();auto result=JsonObject{};put(result,L"ok",true);result.SetNamedValue(L"state",state);return result;
 }
};
}
