// Explicit, isolated review export. Normal application startup never runs this
// path. It renders one page at a time and releases it using the normal route.
void saveReviewWindow(std::filesystem::path file){
 RECT r{};GetClientRect(hwnd,&r);check_bool(r.right>0&&r.bottom>0&&r.right*r.bottom<16000000);
 struct Capture{HDC dc{};HBITMAP bitmap{},old{};~Capture(){if(dc){SelectObject(dc,old);DeleteDC(dc);}if(bitmap)DeleteObject(bitmap);}} capture;
 capture.dc=CreateCompatibleDC(nullptr);BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=r.right;info.bmiHeader.biHeight=-r.bottom;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;void* pixels{};
 capture.bitmap=CreateDIBSection(capture.dc,&info,DIB_RGB_COLORS,&pixels,nullptr,0);check_bool(capture.bitmap!=nullptr);capture.old=(HBITMAP)SelectObject(capture.dc,capture.bitmap);
 check_bool(GetForegroundWindow()==hwnd);POINT point{};ClientToScreen(hwnd,&point);DwmFlush();auto screen=GetDC(nullptr);auto copied=BitBlt(capture.dc,0,0,r.right,r.bottom,screen,point.x,point.y,SRCCOPY|CAPTUREBLT);ReleaseDC(nullptr,screen);check_bool(copied);GdiFlush();
 for(size_t i=0;i<(size_t)r.right*r.bottom;i++)((uint32_t*)pixels)[i]|=0xff000000;
 winrt::com_ptr<IWICImagingFactory> factory;check_hresult(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,__uuidof(IWICImagingFactory),factory.put_void()));winrt::com_ptr<IWICStream> stream;check_hresult(factory->CreateStream(stream.put()));check_hresult(stream->InitializeFromFilename(file.c_str(),GENERIC_WRITE));winrt::com_ptr<IWICBitmapEncoder> encoder;check_hresult(factory->CreateEncoder(GUID_ContainerFormatPng,nullptr,encoder.put()));check_hresult(encoder->Initialize(stream.get(),WICBitmapEncoderNoCache));winrt::com_ptr<IWICBitmapFrameEncode> frame;winrt::com_ptr<IPropertyBag2> props;check_hresult(encoder->CreateNewFrame(frame.put(),props.put()));check_hresult(frame->Initialize(props.get()));check_hresult(frame->SetSize(r.right,r.bottom));GUID format=GUID_WICPixelFormat32bppBGRA;check_hresult(frame->SetPixelFormat(&format));check_hresult(frame->WritePixels(r.bottom,r.right*4,r.right*r.bottom*4,(BYTE*)pixels));check_hresult(frame->Commit());check_hresult(encoder->Commit());
}
winrt::fire_and_forget exportUiReview(std::filesystem::path folder){
 auto lifetime=get_strong();winrt::apartment_context ui;if(!uiMemoryTestProfile())co_return;timer.Stop();profileWorkerTimer.Stop();
 
 std::filesystem::create_directories(folder/L"images");JsonObject report;JsonArray pages;bool ok=true;
 struct Route{std::wstring id,title,route,section;};
 std::vector<Route> routes={{L"P01",L"首页",L"home",L""},{L"P02",L"系统监测",L"system",L""},{L"P03",L"设备接口列表",L"devices",L""},{L"P04",L"设备详情",L"device-detail",L""},{L"P05",L"场景与设备配置",L"scenes",L"overview"},{L"P06",L"场景详情",L"scenes",L"detail"},{L"P07",L"插件市场",L"market",L""},{L"P08",L"插件库管理",L"market-sources",L""},{L"P09",L"已安装插件",L"plugins",L""},{L"P10",L"通用 EQ",L"eq",L""},{L"P11",L"EQ 设置",L"eq-settings",L""},{L"P12",L"Windows 游戏设置",L"windows",L""},{L"P13",L"手机遥控",L"phone",L""},{L"P14",L"开发者工具",L"developer",L""},{L"P15",L"应用设置",L"settings",L""}};
 catalogTried=true;wchar_t catalogPath[32768]{};GetEnvironmentVariableW(L"HHUB_UI_REVIEW_CATALOG",catalogPath,32768);if(catalogPath[0])try{catalogFeed=object(read(catalogPath));}catch(...){}
 auto initial=object(std::wstring(state.Stringify()));auto sceneList=state.GetNamedArray(L"scenes");sceneTargetId=sceneList.Size()?str(sceneList.GetObjectAt(0),L"id"):L"desktop";
 for(auto const& p:state.GetNamedArray(L"plugins",JsonArray{})){auto item=p.GetObject();if(str(item,L"kind")!=L"application"&&hasOfflineUi(item)&&str(item,L"id")!=L"hhub.developer-tools")routes.push_back({L"P"+std::to_wstring(16+routes.size()-15),str(item,L"name"),L"device-plugin",str(item,L"id")});}
 try{
  for(auto const& route:routes){JsonObject entry;put(entry,L"id",route.id);put(entry,L"title",route.title);put(entry,L"route",route.route);JsonArray views;
   bool installed=(route.route!=L"eq"&&route.route!=L"eq-settings"||pluginInstalled(state,L"eq"))&&(route.route!=L"windows"||pluginInstalled(state,L"windows"))&&(route.route!=L"phone"||pluginInstalled(state,L"phone"))&&(route.route!=L"developer"||developerPlugin());
   if(!installed){put(entry,L"note",std::wstring(L"该可选插件当前未安装，页面按安装状态显示。"));entry.SetNamedValue(L"views",views);pages.Append(entry);continue;}
   for(int variant=0;variant<2;variant++){
    auto dpi=GetDpiForWindow(hwnd)/96.0;MONITORINFO monitor{sizeof(monitor)};GetMonitorInfoW(MonitorFromWindow(hwnd,MONITOR_DEFAULTTONEAREST),&monitor);int w=std::min<int>((int)((variant==1?980:1240)*dpi),monitor.rcWork.right-monitor.rcWork.left-24),h=std::min<int>((int)((variant==2?256:variant==1?680:900)*dpi),monitor.rcWork.bottom-monitor.rcWork.top-24);
    SetWindowPos(hwnd,HWND_TOP,monitor.rcWork.left+12,monitor.rcWork.top+12,w,h,SWP_SHOWWINDOW);window.Activate();SetForegroundWindow(hwnd);for(int attempt=0;attempt<150&&GetForegroundWindow()!=hwnd;attempt++){co_await winrt::resume_after(std::chrono::milliseconds(200));co_await ui;}co_await winrt::resume_after(std::chrono::milliseconds(200));co_await ui;
    current=route.route;sceneSection=route.section;if(current==L"device-plugin")devicePluginId=route.section;if(current==L"eq-settings")eqSettingsDraft=object(std::wstring(state.GetNamedObject(L"eqTemplate",JsonObject{}).Stringify()));feedback.IsOpen(false);render();nav.IsPaneOpen(false);
    auto menuId=current==L"eq-settings"?L"eq":current==L"market-sources"?L"market":current==L"devices"||current==L"device-detail"||current==L"system"?L"home":current.c_str();if(current==L"settings")nav.SelectedItem(nav.SettingsItem());else for(auto const& item:nav.MenuItems())if(strTag(item)==menuId)nav.SelectedItem(item);
    co_await winrt::resume_after(std::chrono::milliseconds(current==L"developer"||current==L"device-plugin"?2000:450));co_await ui;page.UpdateLayout();positionNativeEq();
    auto range=pageScroll.ScrollableHeight();int parts=std::clamp((int)std::ceil((range+pageScroll.ViewportHeight())/std::max(1.0,pageScroll.ViewportHeight())),1,10);
    for(int part=0;part<parts;part++){double offset=std::min(range,part*pageScroll.ViewportHeight());pageScroll.ChangeView(nullptr,box_value(offset).as<Windows::Foundation::IReference<double>>(),nullptr,true);co_await winrt::resume_after(std::chrono::milliseconds(150));co_await ui;positionNativeEq();
     std::wstring file=route.id+(variant==2?L"-short-":variant==1?L"-compact-":L"-wide-")+std::to_wstring(part+1)+L".png";saveReviewWindow(folder/L"images"/file);JsonObject view;put(view,L"image",L"images/"+file);put(view,L"variant",std::wstring(variant==2?L"short":variant==1?L"compact":L"wide"));put(view,L"part",(double)part+1);put(view,L"width",pageScroll.ViewportWidth());put(view,L"height",pageScroll.ViewportHeight());put(view,L"overflowY",range);put(view,L"overflowX",pageScroll.ScrollableWidth());put(view,L"compactLayout",compactUi);put(view,L"shortLayout",shortUi);put(view,L"offset",pageScroll.VerticalOffset());views.Append(view);
    }
   }entry.SetNamedValue(L"views",views);auto previewPlugin=pluginRecord(state,route.route==L"developer"?L"hhub.developer-tools":route.route);if(previewPlugin&&flag(previewPlugin,L"reviewPreviewOnly"))put(entry,L"note",std::wstring(L"可选插件的隔离演示；不会安装到主程序。"));if(route.route==L"device-detail"&&visibleDevices().Size()==0)put(entry,L"note",std::wstring(L"当前没有可读取外设，展示真实空状态；具体型号界面由已安装插件和已连接设备提供。"));pages.Append(entry);
  }
  current=L"scenes";sceneSection=L"detail";render();co_await winrt::resume_after(std::chrono::milliseconds(200));co_await ui;
  for(int kind=0;kind<7;kind++){JsonObject entry;std::wstring id=L"D0"+std::to_wstring(kind+1),title;
   if(kind==0){title=L"编辑场景";sceneDialog(find(state.GetNamedArray(L"scenes"),sceneTargetId));}else if(kind==1){title=L"新建场景";sceneDialog(JsonObject{});}else if(kind==2){title=L"添加插件库";editCatalogSource();}else if(kind==3){title=L"设备概览";deviceOverviewDialog();}else if(kind==4){title=L"绑定后台应用 EQ";audioBindingDialog();}else if(kind==5){title=L"实时 EQ 连接与驱动提示";audioConnectionDialog();}else{title=L"保存 EQ 预设";saveEqPreset();}
   co_await winrt::resume_after(std::chrono::milliseconds(350));co_await ui;put(entry,L"id",id);put(entry,L"title",title);put(entry,L"route",std::wstring(L"dialog"));JsonArray views;
   if(hasOpenDialog()){auto file=id+L".png";saveReviewWindow(folder/L"images"/file);JsonObject view;put(view,L"image",L"images/"+file);put(view,L"variant",std::wstring(L"dialog"));put(view,L"part",1.0);views.Append(view);}else put(entry,L"note",std::wstring(L"当前缺少该操作需要的应用或设备，未显示对话框。"));entry.SetNamedValue(L"views",views);pages.Append(entry);for(auto const& weak:openDialogs)if(auto dialog=weak.get())dialog.Hide();co_await winrt::resume_after(std::chrono::milliseconds(200));co_await ui;
  }
 }catch(hresult_error const& error){ok=false;put(report,L"error",std::wstring(error.message()));}
 report.SetNamedValue(L"pages",pages);put(report,L"ok",ok);put(report,L"version",std::wstring(L"Hhub 1.0 · UI review"));put(report,L"privacy",std::wstring(L"实际程序界面 · 隔离脱敏数据；可选插件仅在预览实例展示，不安装到主程序。"));write(folder/L"manifest.json",std::wstring(report.Stringify()));Exit();
}
std::wstring strTag(Windows::Foundation::IInspectable const& item){return std::wstring(unbox_value<hstring>(item.as<NavigationViewItem>().Tag()));}

