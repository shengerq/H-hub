// Isolated native layout regression: deliberately exercise the widths at which
// card columns, action rows, sidebar and compact-height modes change.
bool homeResizeTestEnabled(){wchar_t value[8]{};GetEnvironmentVariableW(L"HHUB_HOME_RESIZE_TEST",value,8);return value[0]==L'1';}
winrt::fire_and_forget runHomeResizeCheck(){
 auto lifetime=get_strong();winrt::apartment_context ui;if(!uiMemoryTestProfile())co_return;timer.Stop();profileWorkerTimer.Stop();
 JsonObject report;JsonArray checks;put(report,L"ok",false);report.SetNamedValue(L"checks",checks);auto output=dataDir()/L"home-resize-check.json";
 auto save=[&]{write(output,std::wstring(report.Stringify()));};save();
 try{
  current=L"home";nav.IsPaneOpen(false);render();ShowWindow(hwnd,SW_RESTORE);
  struct Size{int width,height;};std::vector<Size> sizes={{1240,900},{686,874},{680,874},{670,874},{660,874},{650,874},{640,874},{630,874},{620,874},{610,874},{600,874},{590,874},{580,874},{740,874},{750,874},{760,874},{900,900},{910,900},{920,900},{1100,900},{1090,900},{1240,680},{1240,256},{686,874}};
  for(int cycle=0;cycle<3;cycle++)for(auto const& size:sizes){
   put(report,L"pendingWidth",(double)size.width);put(report,L"pendingHeight",(double)size.height);put(report,L"cycle",(double)cycle);save();
   auto dpi=GetDpiForWindow(hwnd)/96.0;SetWindowPos(hwnd,nullptr,12,12,(int)(size.width*dpi),(int)(size.height*dpi),SWP_NOZORDER|SWP_NOACTIVATE|SWP_SHOWWINDOW);
   co_await winrt::resume_after(std::chrono::milliseconds(160));co_await ui;page.UpdateLayout();
   JsonObject check;put(check,L"width",(double)size.width);put(check,L"height",(double)size.height);put(check,L"viewportWidth",pageScroll.ViewportWidth());put(check,L"viewportHeight",pageScroll.ViewportHeight());put(check,L"overflowX",pageScroll.ScrollableWidth());put(check,L"overflowY",pageScroll.ScrollableHeight());
   auto before=pageScroll.ExtentHeight();co_await winrt::resume_after(std::chrono::milliseconds(80));co_await ui;put(check,L"settled",std::abs(pageScroll.ExtentHeight()-before)<1);
   RECT bounds{};GetWindowRect(hwnd,&bounds);auto minimum=minimumUiWindowSize(hwnd);MINMAXINFO limits{};SendMessageW(hwnd,WM_GETMINMAXINFO,0,reinterpret_cast<LPARAM>(&limits));
   bool floorRespected=unboundedUiResizeTest()||(bounds.right-bounds.left>=minimum.cx&&bounds.bottom-bounds.top>=minimum.cy&&limits.ptMinTrackSize.x==minimum.cx&&limits.ptMinTrackSize.y==minimum.cy);
   put(check,L"minimumEnforced",!unboundedUiResizeTest());put(check,L"minimumRespected",floorRespected);put(check,L"actualWidth",(double)(bounds.right-bounds.left));put(check,L"actualHeight",(double)(bounds.bottom-bounds.top));
   put(check,L"passed",floorRespected&&pageScroll.ViewportHeight()>0&&pageScroll.ScrollableWidth()<1&&std::abs(pageScroll.ExtentHeight()-before)<1);checks.Append(check);save();
  }
  bool passed=true;for(auto const& value:checks)passed=passed&&flag(value.GetObject(),L"passed");put(report,L"ok",passed);put(report,L"completed",true);save();
 }catch(hresult_error const& error){put(report,L"error",std::wstring(error.message()));put(report,L"hresult",(double)(uint32_t)error.code().value);save();}
 // Let the test coroutine release its state before normal window shutdown.
 window.DispatcherQueue().TryEnqueue(Microsoft::UI::Dispatching::DispatcherQueuePriority::Low,[this]{window.Close();});
}
