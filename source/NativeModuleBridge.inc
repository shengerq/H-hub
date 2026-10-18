// Included after JSON helpers and native asset validation, inside hub.
struct NativeModule {
 PluginProcess remote;bool isolated=false;HMODULE library{};bool deferredUnload=false;using CanUnload=BOOL(WINAPI*)();CanUnload canUnload{};
 using Request=HRESULT(WINAPI*)(wchar_t const*,wchar_t**);
 using CreatePage=HRESULT(WINAPI*)(HWND,wchar_t const*,void**,HWND*);
 using PageCommand=HRESULT(WINAPI*)(void*,wchar_t const*);
 using DestroyPage=void(WINAPI*)(void*);
 Request request{};CreatePage createPage{};PageCommand pageCommand{};DestroyPage destroyPage{};
 std::filesystem::path path;
 ~NativeModule(){unload();}
 bool busy()const{return !isolated&&library&&canUnload&&!canUnload();}
 void unload(){if(isolated){remote.close();isolated=false;library=nullptr;return;}if(busy()){deferredUnload=true;return;}deferredUnload=false;if(library){if(request){wchar_t* reply{};request(L"{\"op\":\"shutdown\"}",&reply);CoTaskMemFree(reply);}FreeLibrary(library);}library=nullptr;request=nullptr;createPage=nullptr;pageCommand=nullptr;destroyPage=nullptr;canUnload=nullptr;path.clear();}
 bool load(std::wstring const& owner,bool required=true,bool management=false){
  if(library){if(!isolated||remote.alive())return true;unload();}
  auto manifest=nativeManifest(owner);if(!nativeModuleLocal&&!management&&(flag(manifest,L"forceStopped")||str(manifest,L"mode")==L"disabled")){if(required)throw hresult_error(E_ACCESSDENIED,L"插件已停止或禁用");return false;}auto entry=str(manifest.GetNamedObject(L"implementation",JsonObject{}),L"library");auto assets=nativeAssetDefinitions(owner);auto definition=std::find_if(assets.begin(),assets.end(),[&](auto const& item){return entry.empty()?std::filesystem::path(item.name).extension()==L".dll":item.name==entry;});if(definition==assets.end()){if(required)throw hresult_error(E_NOTIMPL,L"请安装包含页面模块的新版插件包");return false;}auto candidate=dataDir()/L"native-components"/owner/definition->name;
  if(!std::filesystem::is_regular_file(candidate)){if(required)throw hresult_error(E_NOTIMPL,L"请安装包含独立功能组件的新版插件包");return false;}
  std::ifstream input(candidate,std::ios::binary);std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(input)),{});auto hash=nativeAssetDigest(bytes);auto definitions=nativeAssetDefinitions(owner);
  if(std::none_of(definitions.begin(),definitions.end(),[&](auto const& item){return item.name==definition->name&&nativeAssetHashApproved(owner,item,hash);})){if(!required)return false;throw hresult_error(E_ACCESSDENIED,L"插件模块摘要校验失败");}
  verifyNativeAssets(owner);if(!nativeModuleLocal){try{remote.start(owner,L"service");isolated=true;library=(HMODULE)1;return true;}catch(...){remote.close();throw;}}
  library=LoadLibraryExW(candidate.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);check_bool(library!=nullptr);path=candidate;
  request=reinterpret_cast<Request>(GetProcAddress(library,"HhubPluginRequest"));createPage=reinterpret_cast<CreatePage>(GetProcAddress(library,"HhubPluginCreatePage"));pageCommand=reinterpret_cast<PageCommand>(GetProcAddress(library,"HhubPluginPageCommand"));destroyPage=reinterpret_cast<DestroyPage>(GetProcAddress(library,"HhubPluginDestroyPage"));canUnload=reinterpret_cast<CanUnload>(GetProcAddress(library,"HhubPluginCanUnloadNow"));
  if(!request||!createPage||!pageCommand||!destroyPage){unload();throw hresult_error(E_NOINTERFACE,L"插件模块接口不兼容");}return true;
 }
 JsonObject invoke(JsonObject value){if(isolated){JsonObject command;put(command,L"kind",std::wstring(L"request"));command.SetNamedValue(L"request",value);auto reply=remote.call(command).GetNamedObject(L"reply");if(!flag(reply,L"ok"))throw hresult_error(E_FAIL,str(reply,L"error").c_str());return reply;}wchar_t* output{};auto hr=request(std::wstring(value.Stringify()).c_str(),&output);std::wstring text=output?output:L"{}";CoTaskMemFree(output);check_hresult(hr);auto reply=object(text);if(!flag(reply,L"ok"))throw hresult_error(E_FAIL,str(reply,L"error").c_str());return reply;}
};
inline std::map<std::wstring,NativeModule>& nativeModules(){static std::map<std::wstring,NativeModule> values;return values;}
inline NativeModule& nativeModule(std::wstring const& owner=L"eq"){return nativeModules()[owner];}
inline void collectNativeModules(){for(auto& [owner,module]:nativeModules())if(module.deferredUnload&&!module.busy())module.unload();}
inline void nativePluginModel(JsonObject state,std::wstring const& op,JsonObject body={},bool required=true){
 auto& module=nativeModule();if(!module.load(L"eq",required))return;JsonObject request;put(request,L"op",std::wstring(L"model"));put(request,L"operation",op);request.SetNamedValue(L"state",state);request.SetNamedValue(L"body",body);auto next=module.invoke(request).GetNamedObject(L"state");state.Clear();for(auto const& item:next)state.SetNamedValue(item.Key(),item.Value());
}
