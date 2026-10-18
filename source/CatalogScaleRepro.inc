winrt::fire_and_forget runCatalogScaleRepro(){
 auto lifetime=get_strong();winrt::apartment_context ui;
 try{
  timer.Stop();profileWorkerTimer.Stop();
  write(dataDir()/L"eq-install-repro.csv",L"phase,privateMB,workingMB,retiredPages,eqCanvases,callbacks,privateWorkingMB,gdiObjects,userObjects,nativeWindows,currentPage,openDialogs,nativeWidth,nativeHeight,nativeVisible\n");
  catalogKindFilter=L"all";pluginQuery.clear();catalogPageIndex=0;go(L"market");co_await winrt::resume_after(std::chrono::seconds(3));co_await ui;window.AppWindow().Show(false);if(IsIconic(hwnd))ShowWindow(hwnd,SW_SHOWNOACTIVATE);sampleEqInstallRepro("catalog_335_first");
  for(int i=0;i<5;i++){go(L"home");co_await winrt::resume_after(std::chrono::milliseconds(650));co_await ui;go(L"market");co_await winrt::resume_after(std::chrono::milliseconds(650));co_await ui;sampleEqInstallRepro("catalog_return_"+std::to_string(i));}
  co_await winrt::resume_after(std::chrono::seconds(3));co_await ui;window.AppWindow().Show(false);if(IsIconic(hwnd))ShowWindow(hwnd,SW_SHOWNOACTIVATE);sampleEqInstallRepro("catalog_335_idle");
  JsonObject report;put(report,L"entries",(double)catalogFeed.GetNamedArray(L"plugins").Size());put(report,L"kindFilter",catalogKindFilter);put(report,L"query",pluginQuery);put(report,L"pageIndex",(double)catalogPageIndex);put(report,L"mountedChildren",(double)catalogItems.Children().Size());put(report,L"visible",IsWindowVisible(hwnd)&&!IsIconic(hwnd));write(dataDir()/L"catalog-scale.json",std::wstring(report.Stringify()));
  wchar_t image[8]{};GetEnvironmentVariableW(L"HHUB_CATALOG_SCALE_IMAGE",image,8);if(image[0]==L'1'){co_await saveThemeCheckImage(shell,dataDir()/L"catalog-scale.png");co_await ui;}wchar_t verify[8]{};GetEnvironmentVariableW(L"HHUB_CATALOG_INTERACTION_TEST",verify,8);if(verify[0]==L'1'){co_await checkCatalogPageInteractions();co_await ui;}
 }catch(hresult_error const& error){write(dataDir()/L"eq-install-repro-error.txt",std::wstring(error.message()));}
 Exit();
}
