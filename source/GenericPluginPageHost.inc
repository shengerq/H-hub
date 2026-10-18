// The host supplies a rectangle. All visible controls and behavior come from the plugin.
std::unique_ptr<NativePluginPage> nativeSurface;Border nativeSurfaceHost{nullptr};std::wstring surfaceOwner;
RECT surfaceRect{},surfaceToast{};int surfacePane=0;
bool isNativePluginRoute()const{return current.rfind(L"plugin:",0)==0;}
void positionNativeSurface(){
 if(!nativeSurface||!nativeSurfaceHost||!nativeSurfaceHost.IsLoaded()||!nativeSurface->window)return;
 double width=pageScroll.ActualWidth(),height=pageScroll.ActualHeight();if(width<=0||height<=0)return;
 if(std::abs(nativeSurfaceHost.Height()-height)>.5)nativeSurfaceHost.Height(height);
 auto at=pageScroll.TransformToVisual(shell).TransformPoint({0,0});double dpi=GetDpiForWindow(hwnd)/96.0;
 RECT rect{(LONG)std::lround(at.X*dpi),(LONG)std::lround(at.Y*dpi),(LONG)std::lround(width*dpi),(LONG)std::lround(height*dpi)};
 int covered=nav.IsPaneOpen()&&nav.DisplayMode()!=NavigationViewDisplayMode::Expanded?std::max<int>(0,(int)std::lround(nav.OpenPaneLength()*dpi)-rect.left):0;rect.left+=covered;rect.right=std::max<LONG>(1,rect.right-covered);
 if(!EqualRect(&rect,&surfaceRect)){surfaceRect=rect;SetWindowPos(nativeSurface->window,HWND_TOP,rect.left,rect.top,rect.right,rect.bottom,SWP_NOACTIVATE|SWP_SHOWWINDOW);}
 if(hasOpenDialog()){ShowWindow(nativeSurface->window,SW_HIDE);return;}if(!IsWindowVisible(nativeSurface->window))ShowWindow(nativeSurface->window,SW_SHOWNOACTIVATE);
 RECT toast{};if(feedback.IsOpen()&&feedback.root.IsLoaded()){auto point=feedback.root.TransformToVisual(shell).TransformPoint({0,0});RECT frame{rect.left,rect.top,rect.left+rect.right,rect.top+rect.bottom},notice{(LONG)(point.X*dpi),(LONG)(point.Y*dpi),(LONG)((point.X+feedback.root.ActualWidth())*dpi),(LONG)((point.Y+feedback.root.ActualHeight())*dpi)};if(IntersectRect(&toast,&frame,&notice))OffsetRect(&toast,-rect.left,-rect.top);}
 int pane=0;
 if(!EqualRect(&toast,&surfaceToast)||pane!=surfacePane){surfaceToast=toast;surfacePane=pane;auto region=CreateRectRgn(pane,0,rect.right,rect.bottom);if(!IsRectEmpty(&toast)){auto cut=CreateRectRgnIndirect(&toast);CombineRgn(region,region,cut,RGN_DIFF);DeleteObject(cut);}if(!SetWindowRgn(nativeSurface->window,region,TRUE))DeleteObject(region);}
}
void releaseNativeSurface(){nativeSurface.reset();nativeSurfaceHost=nullptr;surfaceOwner.clear();surfaceRect={};surfaceToast={};surfacePane=0;}
void nativePluginSurface(std::wstring const& owner){auto plugin=pluginRecord(state,owner);if(!plugin||!flag(plugin,L"installed")||str(plugin,L"mode")==L"disabled"||flag(plugin,L"forceStopped")){page.Children().Append(text(tr(L"插件已停止，请在插件管理中重新打开。"),14,true));return;}
 try{installedNativeManifests()[owner]=plugin;surfaceOwner=owner;nativeSurfaceHost=Border{};nativeSurfaceHost.Height(std::max(1.0,pageScroll.ActualHeight()));nativeSurfaceHost.HorizontalAlignment(HorizontalAlignment::Stretch);page.Children().Append(nativeSurfaceHost);JsonObject initial;put(initial,L"dark",dark);put(initial,L"app",eqApp);put(initial,L"output",eqOutput);initial.SetNamedValue(L"state",state);nativeSurface=std::make_unique<NativePluginPage>();nativeSurface->create(hwnd,initial,nullptr,owner);onLayoutUpdated(nativeSurfaceHost,[this](auto const&,auto const&){positionNativeSurface();});positionNativeSurface();}
 catch(hresult_error const& error){releaseNativeSurface();page.Children().Append(text(std::wstring(error.message())));}
}
// Legacy route aliases, kept for existing saved links only.
void positionNativeWindows(){positionNativeSurface();}
void releaseWindowsControls(){releaseNativeSurface();}
void resetWindowsPluginUi(){releaseNativeSurface();}
void windowsPage(){nativePluginSurface(L"windows");}
