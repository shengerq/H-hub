bool pluginDropTestEnabled(){wchar_t value[8]{};GetEnvironmentVariableW(L"HHUB_PLUGIN_DROP_TEST",value,8);return value[0]==L'1';}
winrt::fire_and_forget runPluginDropCheck(){
 auto lifetime=get_strong();winrt::apartment_context ui;timer.Stop();profileWorkerTimer.Stop();JsonObject report;JsonArray checks;bool ok=true;
 auto check=[&](bool pass,std::wstring name){JsonObject item;put(item,L"passed",pass);put(item,L"name",name);checks.Append(item);ok=ok&&pass;};
 try{
  using namespace Windows::ApplicationModel::DataTransfer;
  using namespace Windows::Storage;
  auto folder=dataDir()/L"拖放测试";std::filesystem::create_directories(folder);
  auto path=folder/L"鼠标插件.HHUB";write(path,LR"({"id":"drop-regression","name":"Drop regression","author":"H hub tests","version":"1.0.0","permissions":[]})");
  auto file=co_await StorageFile::GetFileFromPathAsync(path.wstring());co_await ui;
  auto package=[&](std::vector<IStorageItem> items){DataPackage data;data.SetStorageItems(items);return data;};
  auto data=package({file});go(L"plugins");window.Activate();co_await winrt::resume_after(std::chrono::milliseconds(300));co_await ui;
  check(pluginDropZone&&pluginDropZone.AllowDrop()&&content.AllowDrop()&&canDropPlugin(),L"Plugin manager has a visible, hit-testable drop area");
  check(pluginDropZone.ActualWidth()>200&&pluginDropZone.ActualHeight()>30&&pluginDropZone.Background(),L"Drop area remains hit-testable across its full bounds");
  positionNativePluginDrop();check(nativePluginDrop&&nativePluginDrop->registered&&IsWindowVisible(nativePluginDrop->window),L"Explorer-compatible native file target is registered and visible");
  wchar_t interactive[8]{};GetEnvironmentVariableW(L"HHUB_PLUGIN_DROP_INTERACTIVE",interactive,8);
  if(interactive[0]==L'1'){
   auto source=card(text(L"Drag this test package into the drop area below"),14);
   page.Children().InsertAt(1,source);
   page.UpdateLayout();co_await winrt::resume_after(std::chrono::milliseconds(200));co_await ui;positionNativePluginDrop();auto point=source.TransformToVisual(shell).TransformPoint({0,0});auto dpi=GetDpiForWindow(hwnd)/96.0;
   PluginDropProbe probe;probe.create(hwnd,path,RECT{(LONG)(point.X*dpi),(LONG)(point.Y*dpi),(LONG)(source.ActualWidth()*dpi),(LONG)(source.ActualHeight()*dpi)});
   auto format=NativePluginDrop::fileFormat();check(probe.data->QueryGetData(&format)==S_OK,L"The pointer drag source supplies CF_HDROP file data");
   stage("Plugin drag probe ready");bool opened=false;
   for(int i=0;i<1200&&!opened;i++){co_await winrt::resume_after(std::chrono::milliseconds(100));co_await ui;opened=pluginImportBusy&&hasOpenDialog();}
   check(opened,L"Actual pointer drag with Explorer OLE file data opens installation review");
   put(report,L"pointerDragResult",(double)(uint32_t)probe.lastResult);put(report,L"pointerDragEffect",(double)probe.effect);
   if(opened){saveReviewWindow(dataDir()/L"plugin-drop-pointer-confirmation.png");for(auto const& weak:openDialogs)if(auto dialog=weak.get())dialog.Hide();co_await winrt::resume_after(std::chrono::milliseconds(200));co_await ui;}
   render();
  }
  auto resolved=co_await droppedPluginPath(data.GetView(),renderGeneration);co_await ui;
  check(resolved==file.Path(),L"Storage-item drop preserves Unicode paths and uppercase package extensions");
  auto count=state.GetNamedArray(L"plugins").Size();importPackage(std::filesystem::path(resolved.c_str()));
  co_await winrt::resume_after(std::chrono::milliseconds(200));co_await ui;
  check(pluginImportBusy&&hasOpenDialog()&&!canDropPlugin(),L"A valid dropped package opens installation review and blocks duplicate drops");
  check(!IsWindowVisible(nativePluginDrop->window),L"The native drop area cannot cover the installation review dialog");
  check(state.GetNamedArray(L"plugins").Size()==count,L"Dropping a package never installs it before confirmation");
  for(auto const& weak:openDialogs)if(auto dialog=weak.get())dialog.Hide();
  co_await winrt::resume_after(std::chrono::milliseconds(200));co_await ui;
  check(!pluginImportBusy&&!hasOpenDialog()&&canDropPlugin()&&state.GetNamedArray(L"plugins").Size()==count,L"Cancelling review restores drag installation and leaves plugins unchanged");
  auto rejects=[&](DataPackageView view,uint64_t generation)->Windows::Foundation::IAsyncOperation<bool>{try{co_await droppedPluginPath(view,generation);}catch(hresult_error const&){co_return true;}co_return false;};
  DataPackage textData;textData.SetText(L"not a file");check(co_await rejects(textData.GetView(),renderGeneration),L"Text drops are rejected");
  auto multiple=package({file,file});check(co_await rejects(multiple.GetView(),renderGeneration),L"Multiple-file drops are rejected");
  DataPackage empty;check(co_await rejects(empty.GetView(),renderGeneration),L"Empty drops are rejected");
  auto storageFolder=co_await StorageFolder::GetFolderFromPathAsync(folder.wstring());co_await ui;auto directory=package({storageFolder});check(co_await rejects(directory.GetView(),renderGeneration),L"Folder drops are rejected");
  check(co_await rejects(data.GetView(),renderGeneration-1),L"A drop from a retired page cannot open an installation dialog");
  auto retired=nativePluginDrop->window;go(L"settings");check(!pluginDropZone&&!nativePluginDrop&&!IsWindow(retired)&&!canDropPlugin(),L"Leaving plugin manager destroys its native drop area and disables plugin imports by drag");
  check(co_await rejects(data.GetView(),renderGeneration),L"File drops on other pages cannot install plugins");
  go(L"plugins");check(pluginDropZone&&canDropPlugin(),L"Returning to plugin manager restores drag support");
  auto saved=state;state=object(std::wstring(state.Stringify()));state.SetNamedValue(L"plugins",JsonArray{});render();check(pluginDropZone&&canDropPlugin()&&installedPluginTiles.empty(),L"Drop installation is also available with no installed plugins");
  if(GetForegroundWindow()==hwnd)saveReviewWindow(dataDir()/L"plugin-drop-empty.png");
  auto bad=folder/L"invalid.json";write(bad,L"not valid json");importPackage(bad);check(!pluginImportBusy&&!hasOpenDialog()&&feedback.IsOpen(),L"Invalid packages show an error without disabling subsequent drops");
  state=saved;render();check(pluginDropZone&&canDropPlugin(),L"Rebuilding the page retains a usable drop target");
  if(GetForegroundWindow()==hwnd)saveReviewWindow(dataDir()/L"plugin-drop-installed.png");
 }catch(hresult_error const& e){ok=false;put(report,L"error",std::wstring(e.message()));}catch(std::exception const& e){ok=false;put(report,L"error",std::wstring(to_hstring(e.what())));}
 put(report,L"ok",ok);report.SetNamedValue(L"checks",checks);write(dataDir()/L"plugin-drop-check.json",std::wstring(report.Stringify()));Exit();
}
