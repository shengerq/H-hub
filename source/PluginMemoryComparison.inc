// Diagnostic only: the caller requires both the isolated UI profile and the
// reproduction model. It never contacts the user's Core or changes plugins.
winrt::fire_and_forget runBuiltinPluginComparison(){
 auto lifetime=get_strong();winrt::apartment_context ui;
 try{
  timer.Stop();profileWorkerTimer.Stop();
  write(dataDir()/L"eq-install-repro.csv",L"phase,privateMB,workingMB,retiredPages,eqCanvases,callbacks,privateWorkingMB,gdiObjects,userObjects,nativeWindows,currentPage,openDialogs,nativeWidth,nativeHeight,nativeVisible\n");
  go(L"home");shell.UpdateLayout();
  for(int i=0;i<4;i++){
   co_await winrt::resume_after(std::chrono::seconds(3));co_await ui;
   co_await refreshComparisonTelemetry();co_await ui;
   shell.UpdateLayout();sampleEqInstallRepro("home_"+std::to_string((i+1)*3)+"s");
  }
  go(L"settings");co_await winrt::resume_after(std::chrono::seconds(3));co_await ui;sampleEqInstallRepro("settings_3s");
  go(L"home");co_await winrt::resume_after(std::chrono::seconds(3));co_await ui;sampleEqInstallRepro("home_after_settings");
  JsonObject report;put(report,L"eqInstalled",pluginInstalled(state,L"eq"));put(report,L"phoneInstalled",pluginInstalled(state,L"phone"));put(report,L"windowsInstalled",pluginInstalled(state,L"windows"));put(report,L"navigationRows",(double)nav.rows.size());put(report,L"eqPageCreated",nativeEq!=nullptr);put(report,L"monitorBindings",(double)monitorBindings.size());put(report,L"historyFrames",(double)monitorHistory.size());put(report,L"stateChars",(double)state.Stringify().size());put(report,L"visible",IsWindowVisible(hwnd)&&!IsIconic(hwnd));write(dataDir()/L"plugin-comparison.json",std::wstring(report.Stringify()));
 }catch(hresult_error const& error){write(dataDir()/L"eq-install-repro-error.txt",std::wstring(error.message()));}
 Exit();
}
Windows::Foundation::IAsyncAction refreshComparisonTelemetry(){
 // Same rendering/update path in both variants, with the exact same snapshot.
 refreshMonitoring();co_await winrt::resume_after(std::chrono::milliseconds(120));
}
