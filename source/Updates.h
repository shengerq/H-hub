#pragma once
#include "Common.h"
#include "PluginCatalog.h"
namespace hub {
struct Updates {
 HANDLE process{};std::filesystem::path report=dataDir()/L"updates"/L"result.json";bool checking=false;std::wstring startupError;JsonObject cached{nullptr};std::filesystem::file_time_type cachedTime{};uintmax_t cachedSize=0;
 ~Updates(){if(process)CloseHandle(process);}
 void start(JsonObject state){startupError.clear();if(process&&WaitForSingleObject(process,0)==WAIT_TIMEOUT)return;if(process){CloseHandle(process);process=nullptr;}std::filesystem::create_directories(report.parent_path());JsonObject input;auto configFile=binaryDir()/L"update-source.json";JsonObject config=JsonObject{};try{if(std::filesystem::exists(configFile))config=object(read(configFile));}catch(...){}JsonObject app;put(app,L"id",std::wstring(L"hhub"));put(app,L"version",std::wstring(L"1.0"));put(app,L"updateUrl",mousePortTestProfile()?std::wstring():str(config,L"manifestUrl"));put(app,L"releaseEpoch",1.0);input.SetNamedValue(L"application",app);JsonArray plugins;if(flag(state.GetNamedObject(L"settings"),L"autoUpdate",true))for(auto const& v:state.GetNamedArray(L"plugins")){auto p=v.GetObject();if(flag(p,L"installed")&&!flag(p,L"builtin"))plugins.Append(p);}input.SetNamedValue(L"plugins",plugins);input.SetNamedValue(L"pluginCatalog",catalogSources());put(input,L"autoInstallApplication",flag(state.GetNamedObject(L"settings"),L"appAutoUpdate",true));put(input,L"installDirectory",binaryDir().wstring());auto file=report.parent_path()/L"request.json";write(file,std::wstring(input.Stringify()));std::error_code ec;std::filesystem::remove(report,ec);auto exe=binaryDir()/L"Hhub.Update.exe";auto line=L"\""+exe.wstring()+L"\" --check \""+file.wstring()+L"\" \""+report.wstring()+L"\"";STARTUPINFOW si{sizeof(si)};si.dwFlags=STARTF_USESHOWWINDOW;si.wShowWindow=SW_HIDE;PROCESS_INFORMATION pi{};if(!CreateProcessW(exe.c_str(),line.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,binaryDir().c_str(),&si,&pi)){startupError=std::filesystem::is_regular_file(exe)?L"更新服务启动失败（错误 "+std::to_wstring(GetLastError())+L"）":L"更新组件缺失，请重新安装完整安装包";throw hresult_error(E_FAIL,startupError.c_str());}CloseHandle(pi.hThread);process=pi.hProcess;checking=true;}
 void applyPlugin(JsonObject state,std::wstring id){
  if(isBuiltinFeature(id))throw hresult_invalid_argument(L"内置功能请更新完整应用");auto results=object(read(report));auto item=find(results.GetNamedArray(L"plugins"),id);
  if(!flag(item,L"ready"))throw hresult_invalid_argument(L"没有可安装的已验证更新");
  auto manifest=object(std::wstring(item.GetNamedObject(L"manifest").Stringify()));
  validatePluginUi(manifest);validatePluginModule(manifest);
  auto old=find(state.GetNamedArray(L"plugins"),id);
  if(str(manifest,L"id")!=id||str(manifest,L"author")!=str(old,L"author"))throw hresult_invalid_argument(L"插件身份不匹配");
  if(str(manifest,L"name").empty()||str(manifest,L"version").empty())throw hresult_invalid_argument(L"插件元信息不完整");
  auto permissions=manifest.GetNamedArray(L"permissions",JsonArray{});
  if(permissions.Size()>32)throw hresult_invalid_argument();for(auto const& permission:permissions)permission.GetString();
  auto source=str(manifest,L"updateUrl",str(old,L"updateUrl").c_str());
  if(!source.empty()&&source.rfind(L"https://",0)!=0)throw hresult_invalid_argument();
  put(manifest,L"updateUrl",source);
  auto assets=installNativePluginAssets(manifest,dataDir());
  // Replace the declaration completely: removed plugin fields must not survive an update.
  for(auto key:{L"installed",L"mode",L"isolated",L"forceStopped"})if(old.HasKey(key))manifest.SetNamedValue(key,old.GetNamedValue(key));
  put(manifest,L"runtime",std::wstring(L"stopped"));put(manifest,L"leaseUntil",0.0);
  put(manifest,L"connected",hasOfflineUi(manifest)||hasNativeModule(manifest));
  put(manifest,L"update",std::wstring(L"插件已更新"));
  auto entries=state.GetNamedArray(L"plugins");for(uint32_t i=0;i<entries.Size();++i)if(str(entries.GetObjectAt(i),L"id")==id){entries.SetAt(i,manifest);break;}
  put(item,L"ready",false);put(item,L"status",std::wstring(L"插件元信息已更新"));
  write(report,std::wstring(results.Stringify()));if(assets)assets->committed=true;
  installedNativeManifests()[id]=manifest;cached=nullptr;
 }
 JsonObject snapshot(JsonObject state){JsonObject value;if(!startupError.empty())put(value,L"error",startupError);put(value,L"checking",process&&WaitForSingleObject(process,0)==WAIT_TIMEOUT);if(std::filesystem::exists(report)){try{auto time=std::filesystem::last_write_time(report);auto size=std::filesystem::file_size(report);if(!cached||time!=cachedTime||size!=cachedSize){cached=object(read(report));for(auto const& v:cached.GetNamedArray(L"plugins",JsonArray{})){auto manifest=v.GetObject().GetNamedObject(L"manifest",JsonObject{});for(auto const& asset:manifest.GetNamedArray(L"nativeAssets",JsonArray{})){auto item=asset.GetObject();if(item.HasKey(L"data"))item.Remove(L"data");}}cachedTime=time;cachedSize=size;}value=cached;put(value,L"checking",false);for(auto const& v:value.GetNamedArray(L"plugins",JsonArray{})){auto item=v.GetObject();try{auto p=find(state.GetNamedArray(L"plugins"),str(item,L"id"));put(p,L"update",str(item,L"status"));}catch(...) {}}}catch(...) {}}return value;}
};
}


