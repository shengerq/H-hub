bool eqDriverInstalling=false;
winrt::fire_and_forget offerEqDriverInstall(){
 if(uiMemoryTestProfile()||eqDriverInstalling)co_return;
 auto lifetime=get_strong();winrt::apartment_context ui;JsonObject next{nullptr};
 try{co_await winrt::resume_background();next=hub::call(L"eqRuntime");}catch(...){}co_await ui;
 if(next)acceptState(next);if(flag(eqRuntime(),L"driverInstalled"))co_return;
 openEqDriver();
}
winrt::fire_and_forget openEqDriver(){
 if(eqDriverInstalling||uiMemoryTestProfile())co_return;
 auto lifetime=get_strong();winrt::apartment_context ui;
 try{
  auto installer=nativeComponentPath(L"eq",L"HhubAudioDriverSetup.exe",L"drivers/audio/HhubAudioDriverSetup.exe");
  if(!std::filesystem::is_regular_file(installer)){notice(tr(L"EQ 插件缺少驱动安装组件，请在插件市场更新通用 EQ 插件。"),true);co_return;}
  eqDriverInstalling=true;ContentDialog dialog;dialog.Title(box_value(tr(L"安装 EQ 虚拟音频驱动")));dialog.Content(text(tr(L"插件包包含 FxSound 官方签名虚拟音频驱动。\n\n实时音效需要安装此驱动，Windows 将请求管理员授权，部分电脑需要重启。请先退出 FxSound。\n\nHhub 仅处理你绑定的应用，保留物理输出设备。选择“稍后”仍可编辑和保存曲线。")));dialog.PrimaryButtonText(tr(L"安装驱动"));dialog.CloseButtonText(tr(L"稍后"));
  if(co_await showDialog(dialog)!=ContentDialogResult::Primary){eqDriverInstalling=false;co_return;}
  eqDriverInstalling=true;auto report=dataDir()/(L"eq-driver-plugin-"+std::to_wstring(GetTickCount64())+L".json");auto args=L"--from-plugin \""+report.wstring()+L"\"";
  SHELLEXECUTEINFOW launch{sizeof(launch)};launch.fMask=SEE_MASK_NOCLOSEPROCESS;launch.hwnd=hwnd;launch.lpVerb=L"runas";launch.lpFile=installer.c_str();launch.lpParameters=args.c_str();launch.nShow=SW_HIDE;
  if(!ShellExecuteExW(&launch)){eqDriverInstalling=false;auto code=GetLastError();if(code==ERROR_CANCELLED){notice(tr(L"已取消管理员授权，曲线编辑仍可使用"));co_return;}throw hresult_error(HRESULT_FROM_WIN32(code));}
  struct Process{HANDLE handle{};~Process(){if(handle)CloseHandle(handle);}} process{launch.hProcess};if(!process.handle)throw hresult_error(E_FAIL,tr(L"无法取得驱动安装结果"));notice(tr(L"正在安装虚拟音频驱动，请等待安装回传…"));
  DWORD code{};JsonObject result{nullptr},next{nullptr};co_await winrt::resume_background();WaitForSingleObject(process.handle,INFINITE);GetExitCodeProcess(process.handle,&code);try{if(std::filesystem::exists(report))result=object(read(report));next=hub::call(L"eqRuntime");}catch(...){}co_await ui;eqDriverInstalling=false;if(next)acceptState(next);if(current==L"eq")updateEqStatus();
  if(!result||!flag(result,L"ok")){notice(result?str(result,L"error",tr(L"驱动安装未完成")):tr(L"驱动安装未返回有效结果，请重试"),true);co_return;}
  if(code==3010||flag(result,L"rebootRequired")){notice(tr(L"驱动已安装，请重启 Windows 后使用实时 EQ"));co_return;}
  if(flag(eqRuntime(),L"driverReady")){notice(tr(L"驱动与音频端点已就绪，请选择输出设备并启用 EQ"));if(current==L"market"||current==L"plugins")go(L"eq");}
  else notice(tr(L"驱动已安装，但音频端点尚未就绪；请在本地桌面检查 Windows 声音设置"),true);
 }catch(hresult_error const& e){eqDriverInstalling=false;notice(std::wstring(e.message()),true);}catch(...){eqDriverInstalling=false;notice(tr(L"驱动安装未完成，请重试"),true);}
}
