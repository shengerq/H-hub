// The stable content host owns the handlers: page redraws must not remove drop support.
Border pluginDropZone{nullptr};
winrt::com_ptr<NativePluginDrop> nativePluginDrop;
RECT pluginDropRect{},pluginDropClip{};
void releasePluginDrop(){if(nativePluginDrop){nativePluginDrop->close();nativePluginDrop=nullptr;}pluginDropZone=nullptr;pluginDropRect={};pluginDropClip={};}
void positionNativePluginDrop(){
 if(!nativePluginDrop||!pluginDropZone||!pluginDropZone.IsLoaded()||!shell)return;
 auto h=nativePluginDrop->window;if(current!=L"plugins"||hasOpenDialog()){ShowWindow(h,SW_HIDE);return;}
 double dpi=GetDpiForWindow(hwnd)/96.0;auto point=pluginDropZone.TransformToVisual(shell).TransformPoint({0,0});
 RECT r{(LONG)std::lround(point.X*dpi),(LONG)std::lround(point.Y*dpi),(LONG)std::lround(pluginDropZone.ActualWidth()*dpi),(LONG)std::lround(pluginDropZone.ActualHeight()*dpi)};
 if(r.right<1||r.bottom<1){ShowWindow(h,SW_HIDE);return;}
 auto origin=pageScroll.TransformToVisual(shell).TransformPoint({0,0});RECT frame{r.left,r.top,r.left+r.right,r.top+r.bottom},viewport{(LONG)(origin.X*dpi),(LONG)(origin.Y*dpi),(LONG)((origin.X+pageScroll.ViewportWidth())*dpi),(LONG)((origin.Y+pageScroll.ViewportHeight())*dpi)},visible{};
 if(nav.IsPaneOpen()&&nav.DisplayMode()!=NavigationViewDisplayMode::Expanded)viewport.left=std::max<LONG>(viewport.left,(LONG)std::lround(nav.OpenPaneLength()*dpi));
 if(!IntersectRect(&visible,&frame,&viewport)){ShowWindow(h,SW_HIDE);return;}
 bool moved=!EqualRect(&r,&pluginDropRect);if(moved){pluginDropRect=r;SetWindowPos(h,HWND_TOP,r.left,r.top,r.right,r.bottom,SWP_NOACTIVATE|SWP_SHOWWINDOW);}else if(!IsWindowVisible(h))ShowWindow(h,SW_SHOWNOACTIVATE);
 OffsetRect(&visible,-r.left,-r.top);if(moved||!EqualRect(&visible,&pluginDropClip)){pluginDropClip=visible;auto region=CreateRoundRectRgn(0,0,r.right+1,r.bottom+1,(int)(12*dpi),(int)(12*dpi));auto clip=CreateRectRgnIndirect(&visible);CombineRgn(region,region,clip,RGN_AND);DeleteObject(clip);if(!SetWindowRgn(h,region,TRUE))DeleteObject(region);}
 if(nativePluginDrop->dark!=dark){nativePluginDrop->dark=dark;InvalidateRect(h,nullptr,FALSE);}
}
bool pluginDropReading=false;
bool canDropPlugin(){return current==L"plugins"&&!pluginImportBusy&&!pluginDropReading&&!hasOpenDialog();}
void paintPluginDrop(bool active=false){
 if(!pluginDropZone)return;
 pluginDropZone.BorderBrush(active?accent():(dark?brush(48,59,73):brush(221,227,235)));
 pluginDropZone.Background(active?(dark?brush(28,48,66):brush(230,242,255)):(dark?brush(30,37,47):brush(255,255,255)));
}
void pluginDropHint(){
 auto labels=stack(false,4);
 labels.Children().Append(title(tr(L"拖入插件包安装"),16));
 labels.Children().Append(text(tr(L"将一个 .hhub 或 .json 插件包拖到此页，松开后确认安装。"),13,true));
 pluginDropZone=card(labels,14);pluginDropZone.AllowDrop(true);
 Microsoft::UI::Xaml::Automation::AutomationProperties::SetName(pluginDropZone,tr(L"拖入插件包安装"));
 paintPluginDrop();page.Children().Append(pluginDropZone);
 nativePluginDrop=winrt::make_self<NativePluginDrop>();nativePluginDrop->dark=dark;
 nativePluginDrop->create(hwnd,tr(L"拖入插件包安装"),tr(L"将一个 .hhub 或 .json 插件包拖到此页，松开后确认安装。"));
 nativePluginDrop->enabled=[this]{return canDropPlugin();};
 nativePluginDrop->dropped=[this](std::vector<std::filesystem::path> paths){
  auto generation=renderGeneration;auto lifetime=get_strong();
  shell.DispatcherQueue().TryEnqueue([this,lifetime,generation,paths=std::move(paths)]{
   if(!canDropPlugin()||generation!=renderGeneration)return;
   if(paths.size()!=1){notice(tr(L"每次请拖入一个插件包"),true);return;}
   std::error_code error;if(!std::filesystem::is_regular_file(paths.front(),error)){notice(tr(L"请拖入文件，不能拖入文件夹"),true);return;}
   importPackage(paths.front());
  });
 };
 onLayoutUpdated(pluginDropZone,[this](auto const&,auto const&){positionNativePluginDrop();});
}
void installPluginDrop(){
 content.AllowDrop(true);
 auto enter=DragEventHandler{[this](IInspectable const&,DragEventArgs const& args){
  using namespace Windows::ApplicationModel::DataTransfer;
  if(current!=L"plugins"||!args.DataView().Contains(StandardDataFormats::StorageItems()))return;
  auto accept=canDropPlugin();args.AcceptedOperation(accept?DataPackageOperation::Copy:DataPackageOperation::None);
  if(accept){args.DragUIOverride().Caption(tr(L"松开以安装插件包"));args.DragUIOverride().IsCaptionVisible(true);}
  paintPluginDrop(accept);args.Handled(true);
 }};
 // ScrollViewer and child controls can mark routed drag events handled.
 // Observe them at the stable host as well, without intercepting scene/profile drags.
 content.AddHandler(UIElement::DragEnterEvent(),box_value(enter),true);
 content.AddHandler(UIElement::DragOverEvent(),box_value(enter),true);
 content.AddHandler(UIElement::DragLeaveEvent(),box_value(DragEventHandler{[this](auto const&,auto const&){paintPluginDrop();}}),true);
 content.AddHandler(UIElement::DropEvent(),box_value(DragEventHandler{[this](auto const&,DragEventArgs const& args){
  using namespace Windows::ApplicationModel::DataTransfer;
  if(current!=L"plugins"||!args.DataView().Contains(StandardDataFormats::StorageItems()))return;
  args.Handled(true);paintPluginDrop();
  if(!canDropPlugin()){args.AcceptedOperation(DataPackageOperation::None);return;}
  dropPlugin(args);
 }}),true);
}
Windows::Foundation::IAsyncOperation<hstring> droppedPluginPath(Windows::ApplicationModel::DataTransfer::DataPackageView data,uint64_t generation){
 using namespace Windows::ApplicationModel::DataTransfer;
 if(!data.Contains(StandardDataFormats::StorageItems()))throw hresult_invalid_argument(tr(L"请拖入本地插件包"));
 auto files=co_await data.GetStorageItemsAsync();
 if(current!=L"plugins"||generation!=renderGeneration)throw hresult_invalid_argument(tr(L"请在插件管理页面安装插件"));
 if(files.Size()!=1)throw hresult_invalid_argument(tr(L"每次请拖入一个插件包"));
 auto file=files.GetAt(0).try_as<Windows::Storage::StorageFile>();
 if(!file)throw hresult_invalid_argument(tr(L"请拖入文件，不能拖入文件夹"));
 co_return file.Path();
}
winrt::fire_and_forget dropPlugin(DragEventArgs args){
 auto lifetime=get_strong();auto deferral=args.GetDeferral();pluginDropReading=true;hstring path;
 try{path=co_await droppedPluginPath(args.DataView(),renderGeneration);args.AcceptedOperation(Windows::ApplicationModel::DataTransfer::DataPackageOperation::Copy);}
 catch(hresult_error const& e){args.AcceptedOperation(Windows::ApplicationModel::DataTransfer::DataPackageOperation::None);notice(std::wstring(e.message()),true);}
 catch(std::exception const& e){args.AcceptedOperation(Windows::ApplicationModel::DataTransfer::DataPackageOperation::None);notice(std::wstring(to_hstring(e.what())),true);}
 // Finish the OS drag session before opening the modal installation review.
 deferral.Complete();pluginDropReading=false;
 if(!path.empty())importPackage(std::filesystem::path(path.c_str()));
}
