#pragma once
#include "Common.h"
namespace hub {
inline JsonObject catalogTask(std::wstring mode,JsonObject input){auto folder=dataDir()/L"plugin-library";std::filesystem::create_directories(folder);static LONG sequence=0;auto nonce=std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64())+L"-"+std::to_wstring(InterlockedIncrement(&sequence));auto request=folder/(nonce+L"-request.json"),reply=folder/(nonce+L"-result.json");write(request,std::wstring(input.Stringify()));auto exe=binaryDir()/L"Hhub.Catalog.exe";auto command=L"\""+exe.wstring()+L"\" "+mode+L" \""+request.wstring()+L"\" \""+reply.wstring()+L"\"";STARTUPINFOW si{sizeof(si)};si.dwFlags=STARTF_USESHOWWINDOW;si.wShowWindow=SW_HIDE;PROCESS_INFORMATION pi{};if(!CreateProcessW(exe.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,binaryDir().c_str(),&si,&pi))throw hresult_error(E_FAIL,L"插件库服务无法启动");CloseHandle(pi.hThread);auto status=WaitForSingleObject(pi.hProcess,mode==L"--sync-many"?900000:60000);if(status!=WAIT_OBJECT_0){TerminateProcess(pi.hProcess,1);CloseHandle(pi.hProcess);throw hresult_error(E_FAIL,L"插件库连接超时");}CloseHandle(pi.hProcess);auto result=object(read(reply));std::error_code ec;std::filesystem::remove(request,ec);std::filesystem::remove(reply,ec);return result;}
inline JsonObject catalogConfig(){auto file=dataDir()/L"plugin-library"/L"source.json";if(std::filesystem::exists(file)){auto config=object(read(file));if(!str(config,L"indexUrl").empty())return config;}file=binaryDir()/L"plugin-library-source.json";return std::filesystem::exists(file)?object(read(file)):object(LR"({"repository":"shengerq/H-hub-plugins","indexUrl":"https://raw.githubusercontent.com/shengerq/H-hub-plugins/main/index.json"})");}
inline std::wstring catalogFileUrl(std::filesystem::path path){return std::wstring(Windows::Foundation::Uri(L"file:///"+path.generic_wstring()).AbsoluteUri());}
inline JsonObject catalogSources(){
 auto file=dataDir()/L"plugin-library"/L"sources.json";JsonObject config;JsonArray sources;
 if(std::filesystem::exists(file)){config=object(read(file));sources=config.GetNamedArray(L"sources");}
 else {auto legacy=catalogConfig();JsonObject source;put(source,L"id",std::wstring(L"source-default"));put(source,L"name",str(legacy,L"repository").empty()?L"默认插件库":str(legacy,L"repository"));put(source,L"address",str(legacy,L"indexUrl"));put(source,L"indexUrl",str(legacy,L"indexUrl"));put(source,L"kind",std::wstring(L"json"));put(source,L"enabled",true);sources.Append(source);put(config,L"schemaVersion",2.0);config.SetNamedValue(L"sources",sources);}
 // Replace only the automatically registered local library, preserving custom sources.
 const std::wstring official=L"https://raw.githubusercontent.com/shengerq/H-hub-plugins/main/index.json";
 JsonArray migrated;bool removed=false,hasOfficial=false,enabled=false;
 for(auto const& value:sources){auto source=value.GetObject();auto url=str(source,L"indexUrl");
  bool generated=str(source,L"id")==L"native-builtins"&&(url==catalogFileUrl(binaryDir()/L"native-plugin-library"/L"index.json")||(!str(config,L"nativeLibraryUrl").empty()&&url==str(config,L"nativeLibraryUrl")));
  if(generated){removed=true;enabled=flag(source,L"enabled",true);continue;}
  hasOfficial=hasOfficial||url==official;migrated.Append(source);
 }
 if(removed&&!hasOfficial){JsonObject source;put(source,L"id",std::wstring(L"hhub-official"));put(source,L"name",std::wstring(L"Hhub GitHub 插件库"));put(source,L"address",std::wstring(L"shengerq/H-hub-plugins"));put(source,L"indexUrl",official);put(source,L"kind",std::wstring(L"github"));put(source,L"enabled",enabled);migrated.Append(source);}

 const std::wstring applications=L"https://raw.githubusercontent.com/shengerq/H-hub-app-plugins/main/index.json";
 bool hasApplications=false;for(auto const& value:migrated){auto item=value.GetObject();auto url=str(item,L"indexUrl");if(url==official){put(item,L"name",std::wstring(L"Hhub 外设插件库"));put(item,L"scope",std::wstring(L"peripheral"));}if(url==applications)hasApplications=true;}
 if(num(config,L"officialLibrariesRevision")<1){if(!hasApplications){JsonObject item;put(item,L"id",std::wstring(L"hhub-applications"));put(item,L"name",std::wstring(L"Hhub 应用插件库"));put(item,L"address",std::wstring(L"shengerq/H-hub-app-plugins"));put(item,L"indexUrl",applications);put(item,L"kind",std::wstring(L"github"));put(item,L"scope",std::wstring(L"application"));put(item,L"enabled",true);migrated.Append(item);}put(config,L"officialLibrariesRevision",1.0);}
 config.SetNamedValue(L"sources",migrated);
 for(auto key:{L"nativeLibraryRegistered",L"nativeLibraryUrl"})if(config.HasKey(key))config.Remove(key);

 return config;
}
inline void saveCatalogSources(JsonObject config){auto folder=dataDir()/L"plugin-library";std::filesystem::create_directories(folder);write(folder/L"sources.json",std::wstring(config.Stringify()));}
// Only the UI owns migration writes. Core readers use the normalized view above.
inline void migrateCatalogSources(){auto file=dataDir()/L"plugin-library"/L"sources.json";if(!std::filesystem::exists(file))return;auto before=read(file);auto config=catalogSources();if(before!=std::wstring(config.Stringify())){saveCatalogSources(config);auto old=object(before);if(flag(old,L"nativeLibraryRegistered")){bool retained=false;for(auto const& v:config.GetNamedArray(L"sources"))retained=retained||str(v.GetObject(),L"id")==L"native-builtins";if(!retained){std::error_code ec;std::filesystem::remove(file.parent_path()/L"cache"/L"native-builtins.json",ec);}}}}
inline JsonObject cachedCatalog(){
 auto folder=dataDir()/L"plugin-library";JsonArray plugins,statuses;
 for(auto const& value:catalogSources().GetNamedArray(L"sources",JsonArray{})){
  auto source=value.GetObject();if(!flag(source,L"enabled",true))continue;auto file=folder/L"cache"/(str(source,L"id")+L".json");

  if(!std::filesystem::exists(file)&&str(source,L"id")==L"source-default"){file=folder/L"catalog.json";if(!std::filesystem::exists(file))file=binaryDir()/L"plugin-library-cache.json";}
  try{if(!std::filesystem::exists(file))continue;auto feed=object(read(file));if(str(feed,L"indexUrl")!=str(source,L"indexUrl"))continue;for(auto const& p:feed.GetNamedArray(L"plugins",JsonArray{})){auto entry=object(std::wstring(p.Stringify()));if(str(source,L"scope")==L"peripheral"&&(str(entry,L"kind")==L"application"||str(entry,L"group")==L"applications"))continue;put(entry,L"sourceId",str(source,L"id"));put(entry,L"sourceName",str(source,L"name"));put(entry,L"sourceIndexUrl",str(source,L"indexUrl"));plugins.Append(entry);}
   // Preserve the legacy cache once so a failed first refresh can still use it.
   auto target=folder/L"cache"/(str(source,L"id")+L".json");if(file!=target){std::filesystem::create_directories(target.parent_path());write(target,std::wstring(feed.Stringify()));}
  }catch(...){}
 }
 JsonObject result;result.SetNamedValue(L"plugins",plugins);result.SetNamedValue(L"sources",statuses);return result;
}
}
