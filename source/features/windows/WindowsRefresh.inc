 winrt::fire_and_forget refreshWindows(){if(!windowsPluginReady())co_return;
  if(windowsWriteBusy||windowsReadBusy){windowsRefreshQueued=true;co_return;}
  windowsReadBusy=true;windowsRefreshQueued=false;auto revision=windowsMutationGeneration;
  auto lifetime=get_strong();winrt::apartment_context ui;JsonObject result{nullptr};std::wstring error;
  try{co_await resume_background();JsonObject body;put(body,L"switchesOnly",true);result=WindowsWorker::run(L"snapshot",body);}
  catch(hresult_error const& e){error=std::wstring(e.message());}catch(...){error=tr(L"读取 Windows 设置失败");}
  co_await ui;windowsReadBusy=false;
  if(windowsPluginReady()&&revision==windowsMutationGeneration&&!windowsWriteBusy){
   windowsLoaded=true;
   if(result){state.SetNamedValue(L"windowsSettings",result);windowsIssues.erase(L"page");}
   else windowsIssues[L"page"]=error;
   updateWindowsControls();
  }
  if(windowsRefreshQueued&&current==L"windows"&&!windowsWriteBusy)refreshWindows();
 }
