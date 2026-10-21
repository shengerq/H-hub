#include <limits>
#include <wincodec.h>
#include <shlwapi.h>
#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"windowscodecs.lib")
#include "Common.h"
#include "PluginDocumentUri.h"
#include "Localization.h"
#include <winrt/Windows.Globalization.h>
#include "HomeMonitoring.h"
#include "MonitoringDemand.h"
#include "UiPageState.h"
#include "PluginPackage.h"
#include <winrt/Windows.ApplicationModel.DataTransfer.h>
#include <winrt/Windows.Storage.h>
#include "UiInstance.h"
#include "WindowSizePolicy.h"
#include "Applications.h"

#include <deque>
#include "PluginCatalog.h"
#include "CatalogBrowse.h"



#include "NativePluginPage.h"
#include <functional>
#include <commctrl.h>
#include <windowsx.h>
#include <array>
#include "NativePluginDrop.h"
#include <shlobj.h>
#include "PluginDropProbe.h"
#include "PluginProfiles.h"
#undef GetCurrentTime
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.Web.WebView2.Core.h>
#include <winrt/Microsoft.UI.Xaml.Controls.Primitives.h>
#include <winrt/Microsoft.UI.Xaml.Automation.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Xaml.Media.Animation.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>
#include <winrt/Microsoft.UI.Xaml.Input.h>
#include <winrt/Microsoft.UI.Input.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Microsoft.UI.Xaml.Shapes.h>
#include <winrt/Microsoft.UI.Xaml.XamlTypeInfo.h>
#include <winrt/Microsoft.UI.Xaml.Markup.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Security.Cryptography.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.FileProperties.h>
#include <memory>
#include <winrt/Windows.System.h>

#include <winrt/Microsoft.UI.Windowing.h>
#include <winrt/Windows.UI.ViewManagement.h>
#include <winrt/Windows.UI.Text.h>
#include <microsoft.ui.xaml.window.h>
#include <commdlg.h>
#include <dwmapi.h>
#include <functional>
#include <iomanip>
#include <sstream>
#include <map>
#include <set>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace hub;
#include "LightFeedback.h"
#include "LeanNavigation.h"
#ifdef FindText
#undef FindText
#endif
#include <winrt/Microsoft.UI.Xaml.Automation.Provider.h>

static std::wstring digits(double n,int precision=0){std::wostringstream s;s<<std::fixed<<std::setprecision(precision)<<n;return s.str();}
static Thickness margin(double all){return Thickness{all,all,all,all};}
// UI-thread-only, bounded immutable palette. Controls share brushes, never mutate them.
static SolidColorBrush brush(unsigned r,unsigned g,unsigned b,unsigned a=255){
 thread_local std::map<uint32_t,SolidColorBrush> palette;
 uint32_t key=(a<<24)|(r<<16)|(g<<8)|b;
 if(auto it=palette.find(key);it!=palette.end())return it->second;
 SolidColorBrush value(Windows::UI::Color{(uint8_t)a,(uint8_t)r,(uint8_t)g,(uint8_t)b});
 if(palette.size()<96)palette.emplace(key,value);
 return value;
}
static void stage(char const* label){std::filesystem::create_directories(dataDir());std::ofstream out(dataDir()/L"startup.log",std::ios::app);out<<label<<"\n";}

struct App:ApplicationT<App, Microsoft::UI::Xaml::Markup::IXamlMetadataProvider> {
 std::wstring pluginQuery,catalogMessage,catalogRenderSignature,catalogKindFilter=L"mouse";uint32_t catalogPageIndex=0;uint32_t catalogPageSize=5;bool catalogFitQueued=false;DispatcherTimer catalogSearchDelay{nullptr};Grid catalogFilterHost{nullptr};Border catalogFooterSpacer{nullptr};JsonObject catalogFeed{nullptr};bool catalogBusy=false,catalogTried=false;TextBox catalogSearch{nullptr};StackPanel catalogItems{nullptr};std::vector<std::pair<Border,JsonObject>> installedPluginTiles;
 Canvas profileWorkerLayer{nullptr};DispatcherTimer profileWorkerTimer{nullptr};std::map<std::wstring,WebView2> profileWorkers;bool profileWorkersBusy=false;HANDLE profileCoreWatch{};Microsoft::Web::WebView2::Core::CoreWebView2Environment pluginEnvironment{nullptr};WebView2 pluginView{nullptr};bool developerNavVisible=false;bool pluginImportBusy=false;std::wstring devicePluginId;
 Microsoft::UI::Xaml::XamlTypeInfo::XamlControlsXamlMetaDataProvider metadata;
 Microsoft::UI::Xaml::Markup::IXamlType GetXamlType(Windows::UI::Xaml::Interop::TypeName const& name){return metadata.GetXamlType(name);}
 Microsoft::UI::Xaml::Markup::IXamlType GetXamlType(hstring const& name){return metadata.GetXamlType(name);}
 com_array<Microsoft::UI::Xaml::Markup::XmlnsDefinition> GetXmlnsDefinitions(){return metadata.GetXmlnsDefinitions();}
 Window window{nullptr};Grid shell{nullptr};TitleBar chrome{nullptr};LeanNavigation nav{nullptr};StackPanel page{nullptr};ScrollViewer pageScroll{nullptr};std::wstring lastRenderedPage;uint64_t renderGeneration=0,settingsRevision=0;TextBlock settingsUpdateStatus{nullptr};Grid content{nullptr},deviceArea{nullptr};LightFeedback feedback{nullptr};JsonObject state{nullptr};std::wstring current=L"home";bool dark=false,compactUi=false,shortUi=false,responsiveResetOffset=false,responsiveQueued=false;bool updatesBusy=false,applicationsBusy=false;bool monitorBusy=false;unsigned timerTicks=0,monitorFastTicks=0;std::vector<JsonObject> monitorHistory;std::vector<std::function<void(JsonObject)>> monitorBindings;std::vector<std::function<void()>> deviceBindings;DispatcherTimer timer{nullptr},feedbackTimer{nullptr};HWND hwnd{};
 struct AppChoiceMemory{std::wstring query,selected;bool advanced=false;double offset=0;};AppChoiceMemory sceneChoices;std::wstring sceneTargetId,sceneSection=L"overview",scenePreviewPath,reviewExportPath;std::vector<std::function<void(JsonObject)>> applicationBindings;
 JsonObject pluginPageInitial;bool showAllDevices=false;

 std::vector<weak_ref<ListView>> sensorLists;bool monitoringFixture=false;
 std::atomic<uint64_t> monitorEpoch{0};std::mutex monitoringIo;bool monitoringActive=false;
 winrt::fire_and_forget stopMonitoringInBackground(uint64_t epoch){auto lifetime=get_strong();co_await resume_background();std::lock_guard lock(monitoringIo);if(epoch!=monitorEpoch.load())co_return;try{hub::call(L"monitorStop");}catch(...) {}}
 void syncMonitoringDemand(){bool wanted=!monitoringFixture&&monitoringPage(current)&&hwnd&&IsWindowVisible(hwnd)&&!IsIconic(hwnd);if(wanted==monitoringActive)return;monitoringActive=wanted;auto epoch=++monitorEpoch;if(!wanted){monitorHistory.clear();monitorHistory.shrink_to_fit();for(auto key:{L"monitor",L"homeMonitoring"})if(state&&state.HasKey(key))state.Remove(key);stopMonitoringInBackground(epoch);}}
 App() {initializeProcessIdentity();integratedFeatureUi=true;stage("App constructor");RequestedTheme(ApplicationTheme::Light);UnhandledException([](auto const&,UnhandledExceptionEventArgs const& e){stage(to_string(e.Message()).c_str());});}
 bool preferredDarkTheme(){auto selected=str(state.GetNamedObject(L"settings"),L"theme",L"system");if(selected!=L"system")return selected==L"dark";auto c=Windows::UI::ViewManagement::UISettings{}.GetColorValue(Windows::UI::ViewManagement::UIColorType::Background);return (c.R+c.G+c.B)<384;}
 void refreshSystemTheme(){bool next=preferredDarkTheme();if(next!=dark){dark=next;if(current==L"settings"){applyNavigationTheme();applySettingsTheme();}else if(current==L"device-plugin"||current==L"developer")applyNavigationTheme();else render();}syncPluginTheme();}
 void OnLaunched(LaunchActivatedEventArgs const&) {
  stage("OnLaunched");
#ifdef HHUB_HOST_UI_TESTS
 initializeExternalUiTests();
#endif
  try{migrateCatalogSources();}catch(...){}
  try{state=hub::call(L"open");}catch(hresult_error const& e){MessageBoxW(hwnd,e.message().c_str(),tr(L"H hub 启动失败"),MB_ICONERROR);Exit();return;}
  dark=preferredDarkTheme();coreAutoStartAllowed=false;profileCoreWatch=OpenProcess(SYNCHRONIZE,FALSE,(DWORD)num(state,L"corePid"));stage("Core connected");initializeLanguage(str(state.GetNamedObject(L"settings"),L"language",L"system"));try{Windows::Globalization::ApplicationLanguages::PrimaryLanguageOverride(languageLocale());}catch(...){}
  ResourceDictionary resources;resources.MergedDictionaries().Append(XamlControlsResources{});Resources(resources);stage("Resources ready");
  window=Window{};window.AppWindow().Closing([](auto const&,Microsoft::UI::Windowing::AppWindowClosingEventArgs const&){coreAutoStartAllowed=false;});window.Title(mousePortTestProfile()?tr(L"H hub · 鼠标插件测试"):L"H hub 1.0");auto native=window.as<IWindowNative>();check_hresult(native->get_WindowHandle(&hwnd));check_bool(SetPropW(hwnd,uiWindowProperty,reinterpret_cast<HANDLE>(1)));installWindowIcon();installUiWindowSizePolicy(hwnd);double dpi=GetDpiForWindow(hwnd)/96.0;
  MONITORINFO monitorInfo{sizeof(monitorInfo)};RECT workArea{0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN)};
  if(GetMonitorInfoW(MonitorFromWindow(hwnd,MONITOR_DEFAULTTONEAREST),&monitorInfo))workArea=monitorInfo.rcWork;
  int workWidth=workArea.right-workArea.left,workHeight=workArea.bottom-workArea.top;
  int width=std::min((int)(1240*dpi),workWidth-std::min((int)(48*dpi),workWidth/10));
  int height=std::min((int)(900*dpi),workHeight-std::min((int)(48*dpi),workHeight/10));
  SetWindowPos(hwnd,nullptr,workArea.left+(workWidth-width)/2,workArea.top+(workHeight-height)/2,width,height,SWP_NOZORDER);
nav.initialize();nav.RequestedTheme(dark?ElementTheme::Dark:ElementTheme::Light);nav.Background(dark?brush(19,24,32):brush(247,248,250));nav.Foreground(dark?brush(245,245,245):brush(27,27,27));nav.IsTitleBarAutoPaddingEnabled(false);nav.Resources(ResourceDictionary{});nav.PaneTitle(L"");nav.IsBackButtonVisible(NavigationViewBackButtonVisible::Collapsed);nav.PaneDisplayMode(NavigationViewPaneDisplayMode::Auto);nav.CompactPaneLength(56);nav.OpenPaneLength(216);nav.CompactModeThresholdWidth(700);nav.ExpandedModeThresholdWidth(1100);nav.IsPaneOpen(!flag(state.GetNamedObject(L"settings"),L"compact"));nav.IsSettingsVisible(true);nav.IsPaneToggleButtonVisible(false);
  addNav(L"home",tr(L"首页"),L"\uE80F");addNav(L"scenes",tr(L"场景与配置"),L"\uE945");addNav(L"market",tr(L"插件市场"),L"\uE719");addNav(L"plugins",tr(L"插件管理"),L"\uEA86");syncNativeNav();
  nav.navigate=[this](std::wstring target){go(target);};
  content=Grid{};RowDefinition r0;r0.Height({1,GridUnitType::Star});RowDefinition r1;r1.Height({0,GridUnitType::Auto});content.RowDefinitions().Append(r0);content.RowDefinitions().Append(r1);
  pageScroll=ScrollViewer{};pageScroll.SizeChanged([this](auto const&,SizeChangedEventArgs const& e){positionNativeSurface();positionNativeSurface();auto height=e.NewSize().Height;bool compact=height>0&&height<720,shortLayout=height>0&&height<430;if(compact==compactUi&&shortLayout==shortUi)return;compactUi=compact;shortUi=shortLayout;if(isNativePluginRoute())return;responsiveResetOffset=true;if(responsiveQueued)return;responsiveQueued=true;pageScroll.DispatcherQueue().TryEnqueue([this]{responsiveQueued=false;if(page.Children().Size())render();});});pageScroll.ViewChanged([this](auto const&,auto const&){positionNativePluginDrop();positionNativeSurface();});auto scroll=pageScroll;scroll.VerticalScrollBarVisibility(ScrollBarVisibility::Auto);scroll.HorizontalScrollBarVisibility(ScrollBarVisibility::Disabled);scroll.HorizontalScrollMode(ScrollMode::Disabled);scroll.ZoomMode(ZoomMode::Disabled);page=StackPanel{};page.Spacing(20);page.Padding(Thickness{12,12,28,20});page.LayoutUpdated([this](auto const&,auto const&){layoutDeviceArea();});scroll.Content(page);content.Children().Append(scroll);
  feedback.initialize();feedback.changed=[this]{positionNativeSurface();positionNativeSurface();};feedback.IsOpen(false);feedback.IsClosable(true);feedback.Margin(Thickness{30,0,30,12});Grid::SetRow(feedback,0);feedback.VerticalAlignment(VerticalAlignment::Bottom);feedback.HorizontalAlignment(HorizontalAlignment::Right);feedback.MaxWidth(520);feedback.Margin(Thickness{20,0,20,20});Canvas::SetZIndex(feedback,10);content.Children().Append(feedback);nav.Content(content);installPluginDrop();nav.BackRequested([this](auto const&,auto const&){go(L"home");});installChrome();nav.SelectedItem(nav.MenuItems().GetAt(0));profileWorkerLayer=Canvas{};Grid::SetRow(profileWorkerLayer,1);profileWorkerLayer.Width(1);profileWorkerLayer.Height(1);profileWorkerLayer.IsHitTestVisible(false);TranslateTransform offscreen;offscreen.X(-20000);profileWorkerLayer.RenderTransform(offscreen);shell.Children().Append(profileWorkerLayer);profileWorkerTimer=DispatcherTimer{};profileWorkerTimer.Interval(std::chrono::seconds(2));profileWorkerTimer.Tick([this](auto const&,auto const&){if(profileCoreWatch&&WaitForSingleObject(profileCoreWatch,0)==WAIT_OBJECT_0){profileWorkerTimer.Stop();timer.Stop();for(auto const& [id,view]:profileWorkers)try{releasePluginView(view);}catch(...){}profileWorkers.clear();CloseHandle(profileCoreWatch);profileCoreWatch=nullptr;Exit();return;}refreshProfileWorkers();});profileWorkerTimer.Start();
  timer=DispatcherTimer{};timer.Interval(std::chrono::milliseconds(250));timer.Tick([this](auto const&,auto const&){syncMonitoringDemand();collectNativeModules();if(nativeSurface&&!nativeSurface->alive()){releaseNativeSurface();page.Children().Append(text(tr(L"插件已退出或被停止，请在插件管理中重新打开。")));}if(uiActivationEvent && WaitForSingleObject(uiActivationEvent,0)==WAIT_OBJECT_0)restoreUiWindow(hwnd);if(IsWindowVisible(hwnd)&&!IsIconic(hwnd)){++timerTicks;if(timerTicks%2==0&&(!pageContributions.empty()||nativeSurface))refreshPluginNavigation();bool monitorPage=current==L"home"||current==L"system";auto m=monitoring();bool waiting=monitorPage&&(!flag(m,L"sensorReady")||flag(m,L"sensorElevationPending")||clockSeconds()-num(m,L"sensorSampleTime")>=5);if(!waiting)monitorFastTicks=0;bool fast=waiting&&monitorFastTicks++<24&&timerTicks%4==0;if(timerTicks%8!=0&&!fast)return;refreshSystemTheme();if(current==L"home"||current==L"system"||current==L"devices"||current==L"device-detail")refreshMonitoring();else if(current==L"device-plugin")updateDevicePluginHeading();else if(current==L"scenes")refreshSceneState();else if(current==L"plugins"||current==L"market"||current==L"settings")refreshUpdates();}});timer.Start();window.Closed([this](auto const&,WindowEventArgs const&){monitoringActive=false;++monitorEpoch;stopMonitoringInBackground(monitorEpoch.load());releasePageEvents();releaseNativeSurface();pageContributions.clear();monitorBindings.clear();deviceBindings.clear();applicationBindings.clear();if(pluginView){releasePluginView(pluginView);pluginView=nullptr;}releasePluginDrop();RemovePropW(hwnd,uiWindowProperty);if(profileCoreWatch){CloseHandle(profileCoreWatch);profileCoreWatch=nullptr;}timer.Stop();if(profileWorkerTimer)profileWorkerTimer.Stop();std::vector<std::wstring> workers;for(auto const& [id,view]:profileWorkers)workers.push_back(id);for(auto const& id:workers)stopProfileWorker(id);Exit();});window.Activated([this](auto const&,WindowActivatedEventArgs const& e){applyWindowTheme();if(e.WindowActivationState()!=WindowActivationState::Deactivated){monitorFastTicks=0;if(current==L"home"||current==L"system")refreshMonitoring();}});int count{};auto args=CommandLineToArgvW(GetCommandLineW(),&count);std::wstring layoutPage;bool backgroundPluginStart=false;bool sceneStartPage=false,pluginStartPage=false;if(args){for(int i=1;i<count;i++)if(std::wstring(args[i])==L"--background-plugins")backgroundPluginStart=true;else if(std::wstring(args[i]).rfind(L"--page=",0)==0)layoutPage=std::wstring(args[i]).substr(7);else if(std::wstring(args[i])==L"--page=scenes")sceneStartPage=true;else if(std::wstring(args[i]).rfind(L"--plugin=",0)==0){devicePluginId=std::wstring(args[i]).substr(9);pluginStartPage=true;}else if(std::wstring(args[i]).rfind(L"--scene=",0)==0){sceneTargetId=std::wstring(args[i]).substr(8);sceneSection=L"detail";sceneStartPage=true;}else if(std::wstring(args[i]).rfind(L"--layout-page=",0)==0)layoutPage=std::wstring(args[i]).substr(14);LocalFree(args);}if(sceneStartPage)go(L"scenes");if(pluginStartPage)openInstalledPlugin(devicePluginId);if(!layoutPage.empty())go(layoutPage);if(page.Children().Size()==0)render();if(std::wstring(GetCommandLineW()).find(L"--verify-builtin-lifecycle")!=std::wstring::npos)runBuiltinLifecycleCheck();
#ifdef HHUB_HOST_UI_TESTS
if(uiMemoryTestProfile())window.AppWindow().Show(false);
#else
window.Activate();
#endif

#ifdef HHUB_HOST_UI_TESTS
 if(uiMemoryTestProfile()&&catalogBrowseTestEnabled())runCatalogBrowseCheck();else if(uiMemoryTestProfile()&&devicePluginHeadingTestEnabled())checkDevicePluginHeading();
#endif
 if(backgroundPluginStart)ShowWindow(hwnd,SW_HIDE);
 if(std::wstring(GetCommandLineW()).find(L"--verify-installed-memory")!=std::wstring::npos)runInstalledMemoryCheck();
 if(std::wstring(GetCommandLineW()).find(L"--verify-eq-rendering")!=std::wstring::npos)runEqRenderingCheck();
 if(std::wstring(GetCommandLineW()).find(L"--verify-sensor-capacity")!=std::wstring::npos)runSensorCapacityCheck();
 }
 void installWindowIcon(){
  auto module=GetModuleHandleW(nullptr);
  auto set=[&](WPARAM kind,int x,int y){auto icon=LoadImageW(module,MAKEINTRESOURCEW(101),IMAGE_ICON,GetSystemMetrics(x),GetSystemMetrics(y),LR_SHARED);if(icon)SendMessageW(hwnd,WM_SETICON,kind,reinterpret_cast<LPARAM>(icon));};
  set(ICON_BIG,SM_CXICON,SM_CYICON);set(ICON_SMALL,SM_CXSMICON,SM_CYSMICON);
 }
 winrt::fire_and_forget loadBrand(Image image){
  try{auto lifetime=get_strong();auto file=co_await Windows::Storage::StorageFile::GetFileFromPathAsync((binaryDir()/L"assets"/L"hhub.png").wstring());auto stream=co_await file.OpenAsync(Windows::Storage::FileAccessMode::Read);Microsoft::UI::Xaml::Media::Imaging::BitmapImage bitmap;bitmap.DecodePixelHeight(64);co_await bitmap.SetSourceAsync(stream);image.Source(bitmap);}catch(...){}
 }
 void installChrome(){
  // The original H button remains the sole pane toggle; its position stays fixed.
  auto brand=stack(true,0);brand.Margin(margin(0));brand.VerticalAlignment(VerticalAlignment::Center);
  Image logo;logo.Width(28);logo.Height(28);logo.Stretch(Stretch::Uniform);loadBrand(logo);
  Button toggle;toggle.Width(56);toggle.Height(48);toggle.HorizontalContentAlignment(HorizontalAlignment::Center);toggle.Padding(margin(0));toggle.Background(brush(0,0,0,0));toggle.BorderThickness(margin(0));toggle.Content(logo);Microsoft::UI::Xaml::Automation::AutomationProperties::SetName(toggle,tr(L"展开或收起侧栏"));brand.Children().Append(toggle);
  auto name=title(L"H hub",20);name.Margin(Thickness{4,0,0,0});name.TextWrapping(TextWrapping::NoWrap);name.VerticalAlignment(VerticalAlignment::Center);brand.Children().Append(name);
  auto syncBrand=[this,name,toggle]{bool expanded=nav.IsPaneOpen();name.Visibility(expanded?Visibility::Visible:Visibility::Collapsed);ToolTipService::SetToolTip(toggle,box_value(expanded?tr(L"收起侧栏"):tr(L"展开侧栏")));};
  toggle.Click([this,syncBrand](auto const&,auto const&){nav.IsPaneOpen(!nav.IsPaneOpen());syncBrand();});nav.PaneOpened([this,syncBrand](auto const&,auto const&){syncBrand();positionNativeSurface();positionNativeSurface();positionNativePluginDrop();});nav.PaneClosed([this,syncBrand](auto const&,auto const&){syncBrand();positionNativeSurface();positionNativeSurface();positionNativePluginDrop();});nav.PaneOpening([this](auto const&,auto const&){positionNativeSurface();});syncBrand();
  chrome=TitleBar{};chrome.Resources(ResourceDictionary{});chrome.Resources().Insert(box_value(L"TitleBarCompactHeight"),box_value(48.0));chrome.Resources().Insert(box_value(L"TitleBarExpandedHeight"),box_value(48.0));chrome.Resources().Insert(box_value(L"TitleBarLeftPaddingWidth"),box_value(0.0));chrome.Padding(margin(0));chrome.MinHeight(48);chrome.Height(48);chrome.IsPaneToggleButtonVisible(false);chrome.LeftHeader(brand);chrome.BackRequested([this](auto const&,auto const&){go(L"home");});
  shell=Grid{};RowDefinition header;header.Height({48,GridUnitType::Pixel});RowDefinition body;body.Height({1,GridUnitType::Star});shell.RowDefinitions().Append(header);shell.RowDefinitions().Append(body);shell.Children().Append(chrome);Grid::SetRow(nav,1);shell.Children().Append(nav);
  window.ExtendsContentIntoTitleBar(true);window.Content(shell);window.SetTitleBar(chrome);
  shell.Loaded([this](auto const&,auto const&){shell.DispatcherQueue().TryEnqueue(Microsoft::UI::Dispatching::DispatcherQueuePriority::Low,[this]{shell.UpdateLayout();stage("Host chrome layout ready");});});window.AppWindow().TitleBar().PreferredHeightOption(Microsoft::UI::Windowing::TitleBarHeightOption::Tall);
 }
 void addNav(std::wstring const& id,std::wstring const& label,std::wstring const& glyph){NavigationViewItem item;item.Content(box_value(label));item.Tag(box_value(id));item.MinHeight(44);item.Height(44);item.Margin(margin(0));FontIcon icon;icon.Glyph(glyph);icon.FontSize(20);item.Icon(icon);Microsoft::UI::Xaml::Automation::AutomationProperties::SetName(item,label);nav.MenuItems().Append(item);}
 bool pageEventsEnabled=false;std::vector<std::function<void()>> pageEventCleanup,catalogEventCleanup;
 void releaseCatalogEvents(){auto callbacks=std::move(catalogEventCleanup);catalogEventCleanup.clear();for(auto const& revoke:callbacks)try{revoke();}catch(...) {}}
 void releasePageEvents(){clearDevicePluginHeading();releasePluginDrop();settingsUpdateStatus=nullptr;if(catalogSearchDelay){catalogSearchDelay.Stop();catalogSearchDelay=nullptr;}catalogFilterHost=nullptr;releaseCatalogEvents();auto callbacks=std::move(pageEventCleanup);pageEventCleanup.clear();for(auto const& revoke:callbacks)try{revoke();}catch(...){}installedPluginTiles.clear();catalogItems=nullptr;catalogFooterSpacer=nullptr;catalogRenderSignature.clear();catalogSearch=nullptr;}
 template<class Element,class Handler> void onClick(Element const& element,Handler handler){auto token=element.Click(handler);if(pageEventsEnabled)pageEventCleanup.push_back([weak=make_weak(element),token]{if(auto value=weak.get())value.Click(token);});}
 template<class Element,class Handler> void onToggled(Element const& element,Handler handler){auto token=element.Toggled(handler);if(pageEventsEnabled)pageEventCleanup.push_back([weak=make_weak(element),token]{if(auto value=weak.get())value.Toggled(token);});}
 template<class Element,class Handler> void onSelectionChanged(Element const& element,Handler handler){auto token=element.SelectionChanged(handler);if(pageEventsEnabled)pageEventCleanup.push_back([weak=make_weak(element),token]{if(auto value=weak.get())value.SelectionChanged(token);});}
 template<class Element,class Handler> void onSizeChanged(Element const& element,Handler handler){auto token=element.SizeChanged(handler);if(pageEventsEnabled)pageEventCleanup.push_back([weak=make_weak(element),token]{if(auto value=weak.get())value.SizeChanged(token);});}
 template<class Element,class Handler> void onTextChanged(Element const& element,Handler handler){auto token=element.TextChanged(handler);if(pageEventsEnabled)pageEventCleanup.push_back([weak=make_weak(element),token]{if(auto value=weak.get())value.TextChanged(token);});}
 template<class Element,class Handler> void onValueChanged(Element const& element,Handler handler){auto token=element.ValueChanged(handler);if(pageEventsEnabled)pageEventCleanup.push_back([weak=make_weak(element),token]{if(auto value=weak.get())value.ValueChanged(token);});}
 template<class Element,class Handler> void onLoaded(Element const& element,Handler handler){auto token=element.Loaded(handler);if(pageEventsEnabled)pageEventCleanup.push_back([weak=make_weak(element),token]{if(auto value=weak.get())value.Loaded(token);});}
 template<class Element,class Handler> void onLostFocus(Element const& element,Handler handler){auto token=element.LostFocus(handler);if(pageEventsEnabled)pageEventCleanup.push_back([weak=make_weak(element),token]{if(auto value=weak.get())value.LostFocus(token);});}
 template<class Element,class Handler> void onKeyUp(Element const& element,Handler handler){auto token=element.KeyUp(handler);if(pageEventsEnabled)pageEventCleanup.push_back([weak=make_weak(element),token]{if(auto value=weak.get())value.KeyUp(token);});}
 template<class Element,class Handler> void onKeyDown(Element const& element,Handler handler){auto token=element.KeyDown(handler);if(pageEventsEnabled)pageEventCleanup.push_back([weak=make_weak(element),token]{if(auto value=weak.get())value.KeyDown(token);});}
 template<class Element,class Handler> void onPointerPressed(Element const& element,Handler handler){auto token=element.PointerPressed(handler);if(pageEventsEnabled)pageEventCleanup.push_back([weak=make_weak(element),token]{if(auto value=weak.get())value.PointerPressed(token);});}
 template<class Element,class Handler> void onPointerMoved(Element const& element,Handler handler){auto token=element.PointerMoved(handler);if(pageEventsEnabled)pageEventCleanup.push_back([weak=make_weak(element),token]{if(auto value=weak.get())value.PointerMoved(token);});}
 template<class Element,class Handler> void onPointerReleased(Element const& element,Handler handler){auto token=element.PointerReleased(handler);if(pageEventsEnabled)pageEventCleanup.push_back([weak=make_weak(element),token]{if(auto value=weak.get())value.PointerReleased(token);});}
 template<class Element,class Handler> void onPointerCaptureLost(Element const& element,Handler handler){auto token=element.PointerCaptureLost(handler);if(pageEventsEnabled)pageEventCleanup.push_back([weak=make_weak(element),token]{if(auto value=weak.get())value.PointerCaptureLost(token);});}
 template<class Element,class Handler> void onPointerWheelChanged(Element const& element,Handler handler){auto token=element.PointerWheelChanged(handler);if(pageEventsEnabled)pageEventCleanup.push_back([weak=make_weak(element),token]{if(auto value=weak.get())value.PointerWheelChanged(token);});}
 template<class Element,class Handler> void onChecked(Element const& element,Handler handler){auto token=element.Checked(handler);if(pageEventsEnabled)pageEventCleanup.push_back([weak=make_weak(element),token]{if(auto value=weak.get())value.Checked(token);});}
 template<class Element,class Handler> void onUnchecked(Element const& element,Handler handler){auto token=element.Unchecked(handler);if(pageEventsEnabled)pageEventCleanup.push_back([weak=make_weak(element),token]{if(auto value=weak.get())value.Unchecked(token);});}
 template<class Element,class Handler> void onDoubleTapped(Element const& element,Handler handler){auto token=element.DoubleTapped(handler);if(pageEventsEnabled)pageEventCleanup.push_back([weak=make_weak(element),token]{if(auto value=weak.get())value.DoubleTapped(token);});}
 template<class Element,class Handler> void onDragEnter(Element const& element,Handler handler){auto token=element.DragEnter(handler);if(pageEventsEnabled)pageEventCleanup.push_back([weak=make_weak(element),token]{if(auto value=weak.get())value.DragEnter(token);});}
 template<class Element,class Handler> void onDragOver(Element const& element,Handler handler){auto token=element.DragOver(handler);if(pageEventsEnabled)pageEventCleanup.push_back([weak=make_weak(element),token]{if(auto value=weak.get())value.DragOver(token);});}
 template<class Element,class Handler> void onDrop(Element const& element,Handler handler){auto token=element.Drop(handler);if(pageEventsEnabled)pageEventCleanup.push_back([weak=make_weak(element),token]{if(auto value=weak.get())value.Drop(token);});}
 template<class Element,class Handler> void onDragStarting(Element const& element,Handler handler){auto token=element.DragStarting(handler);if(pageEventsEnabled)pageEventCleanup.push_back([weak=make_weak(element),token]{if(auto value=weak.get())value.DragStarting(token);});}
 template<class Element,class Handler> void onViewChanged(Element const& element,Handler handler){auto token=element.ViewChanged(handler);if(pageEventsEnabled)pageEventCleanup.push_back([weak=make_weak(element),token]{if(auto value=weak.get())value.ViewChanged(token);});}
 template<class Element,class Handler> void onLayoutUpdated(Element const& element,Handler handler){auto token=element.LayoutUpdated(handler);if(pageEventsEnabled)pageEventCleanup.push_back([weak=make_weak(element),token]{if(auto value=weak.get())value.LayoutUpdated(token);});}
 std::map<void*,std::vector<std::function<void()>>> pluginEventCleanup;
 void releasePluginView(WebView2 view){auto found=pluginEventCleanup.find(::winrt::get_unknown(view));if(found!=pluginEventCleanup.end()){auto callbacks=std::move(found->second);pluginEventCleanup.erase(found);for(auto const& revoke:callbacks)try{revoke();}catch(...) {}}view.Close();}












 TextBlock text(std::wstring const& value,double size=14,bool muted=false){TextBlock t;t.Text(value);t.FontSize(size);t.TextWrapping(TextWrapping::Wrap);if(muted){t.Tag(box_value(L"hhub-muted"));t.Foreground(dark?brush(164,178,195):brush(95,105,120));}return t;}
 TextBlock title(std::wstring const& value,double size=22){auto t=text(value,size);t.FontWeight(Windows::UI::Text::FontWeights::SemiBold());return t;}
 StackPanel stack(bool horizontal=false,double spacing=12){StackPanel p;p.Spacing(spacing);if(horizontal)p.Orientation(Orientation::Horizontal);return p;}
 Grid actionHeader(UIElement label,UIElement action){
  Grid header;ColumnDefinition info;info.Width({1,GridUnitType::Star});ColumnDefinition buttons;buttons.Width({0,GridUnitType::Auto});header.ColumnDefinitions().Append(info);header.ColumnDefinitions().Append(buttons);RowDefinition first;first.Height({0,GridUnitType::Auto});RowDefinition second;second.Height({0,GridUnitType::Auto});header.RowDefinitions().Append(first);header.RowDefinitions().Append(second);
  if(auto name=label.try_as<TextBlock>())name.TextWrapping(TextWrapping::Wrap);label.as<FrameworkElement>().VerticalAlignment(VerticalAlignment::Center);action.as<FrameworkElement>().VerticalAlignment(VerticalAlignment::Center);action.as<FrameworkElement>().HorizontalAlignment(HorizontalAlignment::Right);action.as<FrameworkElement>().Margin(Thickness{16,0,0,0});Grid::SetColumn(action.as<FrameworkElement>(),1);header.Children().Append(label);header.Children().Append(action);
  // DesiredSize includes the current margin and MaxWidth. Using that value
  // after moving the action between rows makes the breakpoint oscillate.
  struct HeaderLayout{double naturalWidth=0;bool initialized=false,stacked=false;};auto layout=std::make_shared<HeaderLayout>();
  auto weak=make_weak(header);onSizeChanged(header,[weak,label,action,layout](auto const&,SizeChangedEventArgs const& e){
   if(!weak.get()||e.NewSize().Width<=0)return;auto element=action.as<FrameworkElement>();
   if(layout->naturalWidth<=0){auto inset=element.Margin();layout->naturalWidth=std::max(0.0,(double)action.DesiredSize().Width-inset.Left-inset.Right);}
   bool narrow=e.NewSize().Width<420||layout->naturalWidth>e.NewSize().Width*.7;
   double maximum=std::max(104.0,(double)e.NewSize().Width-(narrow?0.0:160.0));if(std::abs(element.MaxWidth()-maximum)>.5)element.MaxWidth(maximum);
   if(layout->initialized&&layout->stacked==narrow)return;layout->initialized=true;layout->stacked=narrow;
   Grid::SetColumnSpan(label.as<FrameworkElement>(),narrow?2:1);Grid::SetColumn(element,narrow?0:1);Grid::SetRow(element,narrow?1:0);Grid::SetColumnSpan(element,narrow?2:1);element.Margin(narrow?Thickness{0,8,0,0}:Thickness{16,0,0,0});
  });return header;
 }
 std::vector<weak_ref<StackPanel>> retiredPages;
 void verifyButtonLayout(){
  wchar_t enabled[8]{};GetEnvironmentVariableW(L"HHUB_LAYOUT_DIAGNOSTICS",enabled,8);if(enabled[0]!=L'1')return;retiredPages.erase(std::remove_if(retiredPages.begin(),retiredPages.end(),[](auto const& weak){return !weak.get();}),retiredPages.end());stage(("Retained old pages="+std::to_string(retiredPages.size())).c_str());if(current==L"home")stage(("Home viewport="+std::to_string(pageScroll.ViewportHeight())+" extent="+std::to_string(page.ActualHeight())+" scroll="+std::to_string(pageScroll.ScrollableHeight())).c_str());unsigned checked=0,clipped=0;
  std::function<void(DependencyObject)> visit=[&](DependencyObject node){if(auto action=node.try_as<Button>()){if(auto label=action.Content().try_as<TextBlock>();label&&action.ActualWidth()>0){TextBlock probe;probe.Text(label.Text());probe.FontFamily(label.FontFamily());probe.FontSize(label.FontSize());probe.FontWeight(label.FontWeight());probe.Measure(Windows::Foundation::Size{10000,10000});checked++;if(action.ActualWidth()+1<probe.DesiredSize().Width+action.Padding().Left+action.Padding().Right){clipped++;stage(("Clipped button: "+to_string(label.Text())).c_str());}}}for(int i=0;i<VisualTreeHelper::GetChildrenCount(node);i++)visit(VisualTreeHelper::GetChild(node,i));};visit(page);stage(("Button layout checked="+std::to_string(checked)+" clipped="+std::to_string(clipped)).c_str());
 }
 double actionWidth(UIElement item){
  if(auto action=item.try_as<Button>()){
   if(auto label=action.Content().try_as<TextBlock>()){label.Measure(Windows::Foundation::Size{10000,10000});return std::max(104.0,(double)label.DesiredSize().Width*Windows::UI::ViewManagement::UISettings{}.TextScaleFactor()+32);}
   if(auto value=action.Content().try_as<Windows::Foundation::IPropertyValue>()){if(value.Type()==Windows::Foundation::PropertyType::String){TextBlock label;label.FontSize(14);label.Text(value.GetString());label.Measure(Windows::Foundation::Size{10000,10000});return std::max(104.0,(double)label.DesiredSize().Width*Windows::UI::ViewManagement::UISettings{}.TextScaleFactor()+32);}}
  }
  auto element=item.as<FrameworkElement>();return std::isnan(element.Width())?156:element.Width();
 }
 VariableSizedWrapGrid buttonBar(std::vector<UIElement> items){
  VariableSizedWrapGrid bar;bar.Orientation(Orientation::Horizontal);double width=104,height=36;for(auto const& item:items){width=std::max(width,actionWidth(item));if(auto action=item.try_as<Button>())if(auto caption=action.Content().try_as<TextBlock>())height=std::max(height,(double)caption.DesiredSize().Height*Windows::UI::ViewManagement::UISettings{}.TextScaleFactor()+16);}bar.ItemWidth(width+8);bar.ItemHeight(height+8);
  for(auto const& item:items){auto element=item.as<FrameworkElement>();element.Width(width);element.MinHeight(height);element.Height(NAN);element.Margin(Thickness{0,0,8,8});element.HorizontalAlignment(HorizontalAlignment::Stretch);bar.Children().Append(item);}return bar;
 }
 void alignInlineButtons(UIElement child){
  if(auto panel=child.try_as<StackPanel>()){
   if(panel.Orientation()==Orientation::Horizontal){std::vector<Button> actions;double width=104;for(auto const& item:panel.Children())if(auto action=item.try_as<Button>()){width=std::max(width,actionWidth(action));actions.push_back(action);}if(actions.size()>1)for(auto const& action:actions){action.Width(NAN);action.MinWidth(width);action.MinHeight(36);action.Height(NAN);}}
   for(auto const& item:panel.Children())alignInlineButtons(item);
  }else if(auto grid=child.try_as<Grid>())for(auto const& item:grid.Children())alignInlineButtons(item);
 }
 void compactCardActions(UIElement const& child){
  auto body=child.try_as<StackPanel>();if(!body||body.Orientation()!=Orientation::Vertical)return;std::vector<UIElement> items;for(auto const& item:body.Children())items.push_back(item);if(items.size()<2)return;
  std::vector<UIElement> actions,remaining;
  if(items.front().try_as<TextBlock>()){
   for(size_t i=1;i<items.size();i++){if(items[i].try_as<Button>()){actions.push_back(items[i]);continue;}auto row=items[i].try_as<StackPanel>();bool buttons=row&&row.Orientation()==Orientation::Horizontal&&row.Children().Size();if(buttons)for(auto const& item:row.Children())if(!item.try_as<Button>())buttons=false;
    if(buttons){for(auto const& item:row.Children())actions.push_back(item);row.Children().Clear();}else remaining.push_back(items[i]);}
   if(!actions.empty()){body.Children().Clear();body.Children().Append(actionHeader(items.front(),actions.size()==1?actions[0]:buttonBar(actions)));for(auto const& item:remaining)body.Children().Append(item);return;}
  }
  body.Children().Clear();for(size_t i=0;i<items.size();){if(i+1<items.size()&&items[i].try_as<Button>()&&items[i+1].try_as<TextBlock>()){body.Children().Append(actionHeader(items[i+1],items[i]));i+=2;continue;}if(items[i].try_as<Button>()){std::vector<UIElement> group;while(i<items.size()&&items[i].try_as<Button>())group.push_back(items[i++]);body.Children().Append(group.size()==1?group[0]:buttonBar(group));}else body.Children().Append(items[i++]);}
 }
 Border card(UIElement const& child,double padding=20){compactCardActions(child);alignInlineButtons(child);Border b;b.Background(dark?brush(30,37,47):brush(255,255,255));b.BorderBrush(dark?brush(48,59,73):brush(225,230,237));b.BorderThickness(margin(1));b.CornerRadius(CornerRadius{10,10,10,10});b.Padding(margin(compactUi?std::min(padding,12.0):padding));b.Child(child);return b;}
 std::vector<weak_ref<ContentDialog>> openDialogs;bool dialogRenderPending=false;
 bool hasOpenDialog(){for(auto const& weak:openDialogs)if(weak.get())return true;return false;}
 void styleAccentButton(Button const& b){
  auto color=accent().Color();auto shade=[color](double factor){return brush((uint8_t)(color.R*factor),(uint8_t)(color.G*factor),(uint8_t)(color.B*factor));};
  auto foreground=(.2126*color.R+.7152*color.G+.0722*color.B)>175?brush(20,20,20):brush(255,255,255);
  b.Resources(ResourceDictionary{});auto resources=b.Resources();
  resources.Insert(box_value(L"ButtonBackground"),accent());resources.Insert(box_value(L"ButtonBackgroundPointerOver"),shade(.92));resources.Insert(box_value(L"ButtonBackgroundPressed"),shade(.80));
  for(auto key:{L"ButtonForeground",L"ButtonForegroundPointerOver",L"ButtonForegroundPressed"})resources.Insert(box_value(key),foreground);
  resources.Insert(box_value(L"ButtonBorderBrushPointerOver"),shade(.92));resources.Insert(box_value(L"ButtonBorderBrushPressed"),shade(.80));
  b.Background(accent());b.Foreground(foreground);b.Tag(box_value(L"hhub-accent"));
 }
 void applyDialogTheme(ContentDialog const& dialog){
  auto theme=dark?ElementTheme::Dark:ElementTheme::Light;dialog.RequestedTheme(theme);
  std::function<void(DependencyObject)> visit=[&](DependencyObject node){
   if(auto e=node.try_as<FrameworkElement>())e.RequestedTheme(theme);
   if(auto b=node.try_as<Button>()){auto marker=unbox_value_or<hstring>(b.Tag(),hstring{});if(marker==L"hhub-accent")styleAccentButton(b);else if(marker==L"hhub-button"){b.Background(dark?brush(41,49,61):brush(242,244,248));b.Foreground(dark?brush(245,245,245):brush(27,27,27));}}
   if(auto t=node.try_as<TextBlock>();t&&unbox_value_or<hstring>(t.Tag(),hstring{})==L"hhub-muted")t.Foreground(dark?brush(164,178,195):brush(95,105,120));
   if(auto panel=node.try_as<Panel>()){for(auto const& child:panel.Children())visit(child);}else for(int i=0;i<VisualTreeHelper::GetChildrenCount(node);++i)visit(VisualTreeHelper::GetChild(node,i));
  };if(auto body=dialog.Content().try_as<UIElement>())visit(body);
 }
 void syncDialogThemes(){openDialogs.erase(std::remove_if(openDialogs.begin(),openDialogs.end(),[](auto const& weak){return !weak.get();}),openDialogs.end());for(auto const& weak:openDialogs)if(auto dialog=weak.get())applyDialogTheme(dialog);}
 Windows::Foundation::IAsyncOperation<ContentDialogResult> showDialog(ContentDialog dialog){
  auto lifetime=get_strong();if(nativeSurface)ShowWindow(nativeSurface->window,SW_HIDE);if(nativePluginDrop)ShowWindow(nativePluginDrop->window,SW_HIDE);dialog.XamlRoot(shell.XamlRoot());applyDialogTheme(dialog);openDialogs.push_back(make_weak(dialog));
  auto identity=::winrt::get_unknown(dialog);auto retire=[this,identity]{openDialogs.erase(std::remove_if(openDialogs.begin(),openDialogs.end(),[identity](auto const& weak){auto value=weak.get();return !value||::winrt::get_unknown(value)==identity;}),openDialogs.end());if(dialogRenderPending&&!hasOpenDialog()){dialogRenderPending=false;render();}if(nativeSurface)ShowWindow(nativeSurface->window,SW_SHOWNOACTIVATE);positionNativePluginDrop();};
  ContentDialogResult result;try{result=co_await dialog.ShowAsync();if(hostTestAutoConfirm())result=ContentDialogResult::Primary;}catch(...){retire();throw;}retire();co_return result;
 }
 Button button(std::wstring const& label,std::function<void()> fn,bool primary=false){Button b;b.HorizontalContentAlignment(HorizontalAlignment::Center);b.VerticalContentAlignment(VerticalAlignment::Center);TextBlock caption;caption.TextAlignment(TextAlignment::Center);caption.VerticalAlignment(VerticalAlignment::Center);caption.Text(label);caption.FontSize(14);caption.TextWrapping(TextWrapping::NoWrap);caption.TextTrimming(TextTrimming::None);b.Content(caption);b.MinWidth(104);b.MinHeight(36);b.Padding(Thickness{12,6,12,6});b.VerticalAlignment(VerticalAlignment::Center);b.RequestedTheme(dark?ElementTheme::Dark:ElementTheme::Light);b.Background(dark?brush(41,49,61):brush(242,244,248));b.Foreground(dark?brush(245,245,245):brush(27,27,27));b.Tag(box_value(L"hhub-button"));if(primary)styleAccentButton(b);onClick(b,[this,fn,label](auto const&,auto const&){try{fn();}catch(hresult_error const& e){notice(label+L"："+std::wstring(e.message()),true);}catch(std::exception const& e){notice(label+L"："+std::wstring(to_hstring(e.what())),true);}});return b;}
 ComboBox combo(std::vector<std::wstring> const& items,int selected=0,double width=220){ComboBox c;c.MinHeight(36);c.VerticalAlignment(VerticalAlignment::Center);c.MinWidth(width);for(auto const& i:items)c.Items().Append(box_value(i));c.SelectedIndex(selected);return c;}
 TextBox field(std::wstring const& label,std::wstring const& value=L""){TextBox f;f.MinHeight(36);f.Header(box_value(label));f.Text(value);f.MinWidth(240);return f;}

 void notice(std::wstring const& message,bool error=false){
  if(message.empty())return;
  if(!feedbackTimer){feedbackTimer=DispatcherTimer{};feedbackTimer.Interval(std::chrono::seconds(4));feedbackTimer.Tick([this](auto const&,auto const&){feedbackTimer.Stop();feedback.IsOpen(false);});}
  feedbackTimer.Stop();feedback.Title(error?tr(L"操作未完成"):L"");feedback.Message(translated(message));
  feedback.Severity(error?InfoBarSeverity::Error:message.find(L"正在")!=std::wstring::npos?InfoBarSeverity::Informational:InfoBarSeverity::Success);
  feedback.IsOpen(true);if(!error)feedbackTimer.Start();
 }
 void acceptState(JsonObject next){releaseInactiveUiState(next,current);state=next;}
 void invoke(std::wstring const& op,JsonObject b=JsonObject{},bool redraw=false){auto before=current;if(op==L"pluginForceStop"||op==L"pluginRemove"){auto id=str(b,L"id");stopProfileWorker(id);if(surfaceOwner==id)releaseNativeSurface();}auto next=hub::call(op,b);if(op==L"settings")++settingsRevision;acceptState(next);if(op==L"importPlugin"||op==L"pluginRemove"||op==L"pluginMode"||op==L"pluginUpdate"){syncNativeNav();syncDeveloperNav();}if(redraw||current!=before)render();}
 JsonObject idBody(std::wstring const& id){JsonObject b;put(b,L"id",id);return b;}
 std::wstring deviceGlyph(std::wstring const& kind){if(kind==L"mouse")return L"\uE962";if(kind==L"keyboard")return L"\uE765";if(kind==L"gamepad")return L"\uE967";return L"\uE7F6";}
 std::wstring modeLabel(std::wstring const& mode){if(mode==L"resident")return tr(L"常驻");if(mode==L"manual")return tr(L"手动");if(mode==L"disabled")return tr(L"禁用");return tr(L"自动");}
 void syncNativeNav(){
 std::map<std::wstring,JsonObject> pages;for(auto const& value:state.GetNamedArray(L"plugins")){auto plugin=value.GetObject();if(!flag(plugin,L"installed"))continue;auto route=nativePluginPage(plugin);if(!route.empty())pages[route]=plugin;}
 for(uint32_t i=nav.MenuItems().Size();i>0;--i){auto item=nav.MenuItems().GetAt(i-1).as<NavigationViewItem>();auto tag=std::wstring(unbox_value<hstring>(item.Tag()));if(tag.rfind(L"plugin:",0)==0){auto it=pages.find(tag);if(it==pages.end()){nav.MenuItems().RemoveAt(i-1);if(current==tag)current=L"market";}else{auto declaration=it->second.GetNamedObject(L"navigation",JsonObject{});item.Content(box_value(str(declaration.GetNamedObject(L"localizedTitles",JsonObject{}),languageLocale().c_str(),str(declaration,L"title",str(it->second,L"name").c_str()).c_str())));pages.erase(it);}}}
 for(auto const& [route,plugin]:pages){auto declaration=plugin.GetNamedObject(L"navigation",JsonObject{});addNav(route,str(declaration.GetNamedObject(L"localizedTitles",JsonObject{}),languageLocale().c_str(),str(declaration,L"title",str(plugin,L"name").c_str()).c_str()),str(declaration,L"glyph",L"\uE74C"));if(declaration.HasKey(L"iconGrid")){auto index=nav.MenuItems().Size()-1;auto item=nav.MenuItems().GetAt(index).as<NavigationViewItem>();item.Resources().Insert(box_value(L"Hhub.IconGrid"),box_value(num(declaration,L"iconGrid")));nav.MenuItems().SetAt(index,item);}}
 }
 void go(std::wstring const& requested,bool unused=false){auto id=requested;pluginPageInitial=JsonObject{};for(auto const& value:state.GetNamedArray(L"plugins")){auto p=value.GetObject();if(!flag(p,L"installed"))continue;for(auto const& value:p.GetNamedArray(L"routeAliases",JsonArray{})){auto alias=value.GetObject();if(str(alias,L"route")==id){pluginPageInitial=alias.GetNamedObject(L"initial",JsonObject{});id=nativePluginPage(p);break;}}}if(id.rfind(L"plugin:",0)==0){auto p=pluginRecord(state,id.substr(7));if(!p||!flag(p,L"installed")){go(L"market");return;}}if(id==current&&page.Children().Size())return;feedback.IsOpen(false);current=id;monitorFastTicks=0;render();auto menuId=(id==L"devices"||id==L"device-detail"||id==L"system")?std::wstring(L"home"):id==L"market-sources"?std::wstring(L"market"):id;if(id==L"settings")nav.SelectedItem(nav.SettingsItem());else for(auto const& item:nav.MenuItems()){auto n=item.as<NavigationViewItem>();if(std::wstring(unbox_value<hstring>(n.Tag()))==menuId){nav.SelectedItem(n);break;}}if(id==L"system")refreshMonitoring();if(id==L"scenes")refreshSceneState();}
 bool windowThemeApplied=false,windowThemeDark=false,navThemeApplied=false,navThemeDark=false;
 void applyWindowTheme(){if(!hwnd||(windowThemeApplied&&windowThemeDark==dark))return;windowThemeApplied=true;windowThemeDark=dark;BOOL immersive=dark?TRUE:FALSE;COLORREF caption=dark?RGB(23,29,38):RGB(243,245,248);COLORREF titleText=dark?RGB(245,245,245):RGB(27,27,27);COLORREF border=dark?RGB(48,59,73):RGB(225,230,237);auto modeResult=DwmSetWindowAttribute(hwnd,DWMWA_USE_IMMERSIVE_DARK_MODE,&immersive,sizeof(immersive));auto capResult=DwmSetWindowAttribute(hwnd,DWMWA_CAPTION_COLOR,&caption,sizeof(caption));auto textResult=DwmSetWindowAttribute(hwnd,DWMWA_TEXT_COLOR,&titleText,sizeof(titleText));auto borderResult=DwmSetWindowAttribute(hwnd,DWMWA_BORDER_COLOR,&border,sizeof(border));if(SUCCEEDED(modeResult)&&SUCCEEDED(capResult)&&SUCCEEDED(textResult)&&SUCCEEDED(borderResult))stage(dark?"Native title attributes applied: dark":"Native title attributes applied: light");else{auto result=std::string("Native title attribute HRESULTs: ")+std::to_string(modeResult)+","+std::to_string(capResult)+","+std::to_string(textResult)+","+std::to_string(borderResult);stage(result.c_str());}}
 void applyNavigationTheme(){syncDialogThemes();if(navThemeApplied&&navThemeDark==dark)return;navThemeApplied=true;navThemeDark=dark;feedback.theme(dark);auto background=dark?brush(19,24,32):brush(247,248,250);auto foreground=dark?brush(245,245,245):brush(27,27,27);nav.Resources().Insert(box_value(L"NavigationViewDefaultPaneBackground"),background);nav.Resources().Insert(box_value(L"NavigationViewExpandedPaneBackground"),background);for(auto key:{L"NavigationViewItemForeground",L"NavigationViewItemForegroundPointerOver",L"NavigationViewItemForegroundPressed",L"NavigationViewItemForegroundSelected",L"NavigationViewItemForegroundSelectedPointerOver",L"NavigationViewItemForegroundSelectedPressed",L"NavigationViewPaneTitleForeground"})nav.Resources().Insert(box_value(key),foreground);nav.Resources().Insert(box_value(L"NavigationViewContentBackground"),background);nav.Resources().Insert(box_value(L"NavigationViewContentGridBorderBrush"),brush(0,0,0,0));nav.Resources().Insert(box_value(L"NavigationViewContentGridCornerRadius"),box_value(CornerRadius{0,0,0,0}));nav.Resources().Insert(box_value(L"NavigationViewContentGridBorderThickness"),box_value(Thickness{0,0,0,0}));nav.Background(background);nav.Foreground(foreground);nav.RequestedTheme(dark?ElementTheme::Dark:ElementTheme::Light);content.Background(background);page.Background(background);page.RequestedTheme(dark?ElementTheme::Dark:ElementTheme::Light);if(chrome){shell.RequestedTheme(dark?ElementTheme::Dark:ElementTheme::Light);chrome.RequestedTheme(dark?ElementTheme::Dark:ElementTheme::Light);chrome.Background(background);shell.Background(background);auto titlebar=window.AppWindow().TitleBar();titlebar.ButtonBackgroundColor(Windows::UI::Color{0,0,0,0});titlebar.ButtonInactiveBackgroundColor(Windows::UI::Color{0,0,0,0});titlebar.ButtonForegroundColor(dark?Windows::UI::Color{255,245,245,245}:Windows::UI::Color{255,27,27,27});}applyWindowTheme();}


#include "BuiltinLifecycleCheck.inc"
#include "SensorCapacityCheck.inc"
#include "InstalledMemoryCheck.inc"
#include "EqRenderingCheck.inc"
 void render(){syncMonitoringDemand();bool restoreCatalogFocus=current==L"market"&&catalogSearch&&catalogSearch.FocusState()!=FocusState::Unfocused;int searchStart=restoreCatalogFocus?catalogSearch.SelectionStart():0,searchLength=restoreCatalogFocus?catalogSearch.SelectionLength():0;if(hasOpenDialog()){dialogRenderPending=true;applyNavigationTheme();return;}auto renderStarted=GetTickCount64();syncNativeNav();pageScroll.VerticalScrollBarVisibility((isNativePluginRoute()||current==L"market")?ScrollBarVisibility::Disabled:ScrollBarVisibility::Auto);pageScroll.VerticalScrollMode((isNativePluginRoute()||current==L"market")?ScrollMode::Disabled:ScrollMode::Enabled);if(current==L"device-plugin")stopProfileWorker(devicePluginId);syncDeveloperNav();if(pluginView){try{auto p=find(state.GetNamedArray(L"plugins"),devicePluginId);if(p.HasKey(L"hidV2Targets")){auto body=idBody(devicePluginId);body.SetNamedValue(L"request",object(LR"({"transportVersion":2,"operation":"closeAll"})"));hub::call(L"pluginHidV2",body);}}catch(...){}releasePluginView(pluginView);pluginView=nullptr;if(profileWorkers.empty())pluginEnvironment=nullptr;}page.ChildrenTransitions(Microsoft::UI::Xaml::Media::Animation::TransitionCollection{});double savedOffset=!responsiveResetOffset&&pageScroll&&lastRenderedPage==current?pageScroll.VerticalOffset():0;responsiveResetOffset=false;lastRenderedPage=current;auto generation=++renderGeneration;auto rendered=current;releasePageEvents();releaseNativeSurface();pageContributions.clear();pageEventsEnabled=true;auto settings=state.GetNamedObject(L"settings");auto theme=str(settings,L"theme",L"system");if(theme==L"system"){Windows::UI::ViewManagement::UISettings u;auto background=u.GetColorValue(Windows::UI::ViewManagement::UIColorType::Background);dark=(background.R+background.G+background.B)<384;}else dark=theme==L"dark";applyNavigationTheme();monitorBindings.clear();deviceBindings.clear();applicationBindings.clear();deviceArea=nullptr;auto previousPage=page;retiredPages.erase(std::remove_if(retiredPages.begin(),retiredPages.end(),[](auto const& weak){return !weak.get();}),retiredPages.end());retiredPages.push_back(make_weak(previousPage));previousPage.Children().Clear();page=StackPanel{};page.Spacing(shortUi?8:compactUi?12:20);page.Padding((isNativePluginRoute())?Thickness{0,0,0,0}:compactUi?Thickness{12,8,20,12}:Thickness{12,12,28,12});page.Background(previousPage.Background());page.RequestedTheme(previousPage.RequestedTheme());onLayoutUpdated(page,[this](auto const&,auto const&){layoutDeviceArea();if(current==L"market")queueCatalogFit();});
  nav.IsBackButtonVisible(NavigationViewBackButtonVisible::Collapsed);chrome.IsBackButtonVisible(false);chrome.IsBackButtonEnabled(false);std::wstring heading=current==L"home"?tr(L"首页"):current==L"devices"?tr(L"我的设备"):current==L"device-detail"?tr(L"设备详情"):current==L"system"?tr(L"系统监测"):current==L"scenes"?tr(L"场景与配置"):current==L"market"?tr(L"插件市场"):current==L"market-sources"?tr(L"插件库管理"):current==L"plugins"?tr(L"插件管理"):current==L"developer"?tr(L"开发者工具"):current==L"device-plugin"?tr(L"设备插件"):tr(L"设置");if(current!=L"home"&&current!=L"device-plugin"&&current!=L"scenes"&&current!=L"plugins")pageHeading(heading);
  if(isNativePluginRoute())nativePluginSurface(current.substr(7));else if(current==L"home")home();else if(current==L"devices"){deviceListPage();}else if(current==L"device-detail"){devices();}else if(current==L"system")system();else if(current==L"scenes")scenes();else if(current==L"market")pluginCatalogPanel();else if(current==L"market-sources")catalogSourcesPage();else if(current==L"plugins")plugins();else if(current==L"developer")developerPage();else if(current==L"device-plugin")devicePluginPage();else settingsPage();if(pageScroll){pageScroll.Content(page);pageScroll.UpdateLayout();if(restoreCatalogFocus&&catalogSearch){catalogSearch.Focus(FocusState::Programmatic);auto length=(int)catalogSearch.Text().size();catalogSearch.Select(std::min(searchStart,length),std::min(searchLength,length-std::min(searchStart,length)));}pageScroll.ChangeView(nullptr,box_value(savedOffset).as<Windows::Foundation::IReference<double>>(),nullptr,true);page.DispatcherQueue().TryEnqueue(Microsoft::UI::Dispatching::DispatcherQueuePriority::Low,[this,generation]{if(generation!=renderGeneration)return;verifyButtonLayout();});wchar_t diagnostic[8]{};GetEnvironmentVariableW(L"HHUB_LAYOUT_DIAGNOSTICS",diagnostic,8);if(diagnostic[0]==L'1')stage(("Render page="+to_string(current)+" ms="+std::to_string(GetTickCount64()-renderStarted)).c_str());}
 }
#ifdef HHUB_HOST_UI_TESTS
#include HHUB_HOST_UI_TESTS
#endif
#include "CompactPluginProfiles.inc"
#include "DevicePluginHeading.inc"
#include "ScenePluginWorkers.inc"
#include "PluginPage.inc"
#include "NativeHomePage.inc"
 SolidColorBrush accent(){auto c=Windows::UI::ViewManagement::UISettings{}.GetColorValue(Windows::UI::ViewManagement::UIColorType::Accent);return brush(c.R,c.G,c.B);}
 Grid section(std::wstring const& heading,std::wstring const& caption,std::function<void()> action){Grid g;ColumnDefinition a;a.Width({1,GridUnitType::Star});ColumnDefinition b;b.Width({0,GridUnitType::Auto});g.ColumnDefinitions().Append(a);g.ColumnDefinitions().Append(b);auto link=button(heading+L"  ›",action);link.FontSize(23);link.Content().as<TextBlock>().FontSize(23);link.Content().as<TextBlock>().FontWeight(Windows::UI::Text::FontWeights::SemiBold());link.FontWeight(Windows::UI::Text::FontWeights::SemiBold());link.Padding(Thickness{0,4,0,4});link.Background(brush(0,0,0,0));link.BorderThickness(margin(0));link.HorizontalAlignment(HorizontalAlignment::Left);g.Children().Append(link);auto t=text(caption,12,true);t.VerticalAlignment(VerticalAlignment::Center);Grid::SetColumn(t,1);g.Children().Append(t);return g;}
 Grid metric(std::wstring const& name,std::wstring const& value,double percent=-1){
  Grid row;row.Height(28);ColumnDefinition labelColumn;labelColumn.Width({52,GridUnitType::Pixel});ColumnDefinition valueColumn;valueColumn.Width({1,GridUnitType::Star});row.ColumnDefinitions().Append(labelColumn);row.ColumnDefinitions().Append(valueColumn);
  auto label=text(name,12,true);label.TextWrapping(TextWrapping::NoWrap);label.TextTrimming(TextTrimming::CharacterEllipsis);label.as<FrameworkElement>().VerticalAlignment(VerticalAlignment::Center);row.Children().Append(label);
  auto number=text(value,13);number.FontWeight(Windows::UI::Text::FontWeights::SemiBold());number.TextWrapping(TextWrapping::NoWrap);number.TextTrimming(TextTrimming::CharacterEllipsis);number.TextAlignment(TextAlignment::Right);number.VerticalAlignment(VerticalAlignment::Center);number.Margin(Thickness{6,0,0,0});Grid::SetColumn(number,1);row.Children().Append(number);return row;
 }
 static std::wstring compactModel(std::wstring name,bool gpu){
  for(auto token:{L"(R)",L"(TM)"})for(auto position=name.find(token);position!=std::wstring::npos;position=name.find(token))name.erase(position,wcslen(token));
  while(name.find(L"  ")!=std::wstring::npos)name.replace(name.find(L"  "),2,L" ");
  if(gpu){for(auto prefix:{L"NVIDIA GeForce ",L"NVIDIA ",L"AMD "})if(name.rfind(prefix,0)==0){name.erase(0,wcslen(prefix));break;}if(name.size()>=4&&name.substr(name.size()-4)==L" GPU")name.resize(name.size()-4);}
  else for(auto prefix:{L"Intel Core ",L"Intel ",L"AMD "})if(name.rfind(prefix,0)==0){name.erase(0,wcslen(prefix));break;}
  return name;
 }
 Grid hardwareHeading(std::wstring label,std::wstring glyph,std::wstring key,bool gpu=false){
  Grid heading;heading.Height(30);ColumnDefinition iconColumn;iconColumn.Width({28,GridUnitType::Pixel});ColumnDefinition labelColumn;labelColumn.Width({0,GridUnitType::Auto});ColumnDefinition modelColumn;modelColumn.Width({1,GridUnitType::Star});heading.ColumnDefinitions().Append(iconColumn);heading.ColumnDefinitions().Append(labelColumn);heading.ColumnDefinitions().Append(modelColumn);
  FontIcon icon;icon.Glyph(glyph);icon.FontSize(20);icon.Foreground(accent());icon.HorizontalAlignment(HorizontalAlignment::Left);heading.Children().Append(icon);auto name=title(label,15);name.VerticalAlignment(VerticalAlignment::Center);Grid::SetColumn(name,1);heading.Children().Append(name);
  auto model=text(L"",12,true);model.Margin(Thickness{8,0,0,0});model.VerticalAlignment(VerticalAlignment::Center);model.TextWrapping(TextWrapping::NoWrap);model.TextTrimming(TextTrimming::CharacterEllipsis);Grid::SetColumn(model,2);heading.Children().Append(model);
  auto update=[model,key,gpu](JsonObject m){auto raw=str(m,key.c_str());model.Text(raw.empty()?tr(L"读取中…"):compactModel(raw,gpu));ToolTipService::SetToolTip(model,box_value(raw));};update(monitoring());monitorBindings.push_back(update);return heading;
 }
 JsonObject physicalDisk(JsonObject sample,std::wstring const& id){for(auto const& value:sample.GetNamedArray(L"disks",JsonArray{})){auto disk=value.GetObject();if(str(disk,L"id")==id)return disk;}return nullptr;}
 Grid diskRow(std::wstring label,std::wstring id,bool capacity){
  auto row=metric(label,tr(L"读取中…"));auto value=row.Children().GetAt(1).as<TextBlock>();
  auto update=[this,value,id,capacity](JsonObject sample){auto disk=physicalDisk(sample,id);if(!disk){value.Text(flag(sample,L"sensorReady")?tr(L"未回传"):tr(L"读取中…"));return;}
   auto number=[disk](wchar_t const* key,int precision){auto v=disk.GetNamedValue(key,JsonValue::CreateNullValue());return v.ValueType()==JsonValueType::Number?digits(v.GetNumber(),precision):std::wstring(L"—");};
   value.Text(capacity?number(L"usedGB",0)+L" / "+number(L"totalGB",0)+L" GB":number(L"readMB",1)+L" / "+number(L"writeMB",1)+L" MB/s");
   ToolTipService::SetToolTip(value,box_value(capacity?tr(L"已用 / 总容量：")+number(L"usedGB",1)+L" / "+number(L"totalGB",1)+L" GB":tr(L"读取：")+number(L"readMB",1)+tr(L" MB/s；写入：")+number(L"writeMB",1)+L" MB/s"));
  };update(monitoring());monitorBindings.push_back(update);return row;
 }
 StackPanel physicalDisks(){
  auto panel=stack(false,4);panel.Children().Append(metricTitle(tr(L"磁盘"),L"\uEDA2"));auto disks=monitoring().GetNamedArray(L"disks",JsonArray{});
  if(disks.Size()==0){panel.Children().Append(text(tr(L"Windows 磁盘统计"),12,true));panel.Children().Append(liveText([](JsonObject m){return tr(L"已用 / 总量  ")+amount(m,L"diskUsedGB",1)+L" / "+amount(m,L"diskTotalGB",1)+L" GB";},13));panel.Children().Append(liveText([](JsonObject m){return tr(L"读 / 写  ")+amount(m,L"diskReadMB",1)+L" / "+amount(m,L"diskWriteMB",1)+L" MB/s";},13));panel.Children().Append(liveText([](JsonObject m){std::wstring out;for(auto const& v:m.GetNamedArray(L"volumes",JsonArray{})){auto d=v.GetObject();out+=str(d,L"name")+L"  "+amount(d,L"usedGB",1)+L" / "+amount(d,L"totalGB",1)+L" GB\n";}return out;},11));return panel;}
  for(uint32_t index=0;index<disks.Size();index++){
   auto disk=disks.GetObjectAt(index);auto id=str(disk,L"id");
   if(index){Border divider;divider.Height(1);divider.Margin(Thickness{0,4,0,4});divider.Background(dark?brush(48,59,73):brush(225,230,237));panel.Children().Append(divider);}
   auto name=text(tr(L"磁盘 ")+std::to_wstring(index+1)+L" · "+str(disk,L"name"),12,true);name.TextWrapping(TextWrapping::NoWrap);name.TextTrimming(TextTrimming::CharacterEllipsis);ToolTipService::SetToolTip(name,box_value(str(disk,L"name")+L"\n"+id));panel.Children().Append(name);
   panel.Children().Append(diskRow(tr(L"已用/总量"),id,true));panel.Children().Append(diskRow(tr(L"读 / 写"),id,false));
  }return panel;
 }
 std::wstring diskSignature(){std::wstring result;for(auto const& value:monitoring().GetNamedArray(L"disks",JsonArray{})){auto disk=value.GetObject();result+=str(disk,L"id")+L":"+str(disk,L"name")+L";";}return result;}

 Grid metricTitle(std::wstring const& label,std::wstring const& glyph){Grid g;g.Height(30);ColumnDefinition c0;c0.Width({28,GridUnitType::Pixel});ColumnDefinition c1;c1.Width({1,GridUnitType::Star});g.ColumnDefinitions().Append(c0);g.ColumnDefinitions().Append(c1);FontIcon icon;icon.Glyph(glyph);icon.FontSize(20);icon.Foreground(accent());icon.HorizontalAlignment(HorizontalAlignment::Left);g.Children().Append(icon);auto t=title(label,15);t.TextWrapping(TextWrapping::NoWrap);t.TextTrimming(TextTrimming::CharacterEllipsis);t.VerticalAlignment(VerticalAlignment::Center);Grid::SetColumn(t,1);g.Children().Append(t);return g;}
 Border separator(){Border line;line.Height(1);line.Margin(Thickness{0,6,0,6});line.Background(dark?brush(62,62,62):brush(229,229,229));return line;}
 static std::wstring amount(JsonObject m,wchar_t const* key,int precision=0){auto v=m.GetNamedValue(key,JsonValue::CreateNullValue());return v.ValueType()==JsonValueType::Number?digits(v.GetNumber(),precision):std::wstring(tr(L"不可用"));}
 JsonObject monitoring(){return state.GetNamedObject(L"monitor",JsonObject{});}
 TextBlock liveText(std::function<std::wstring(JsonObject)> format,double size=12){auto t=text(format(monitoring()),size,true);monitorBindings.push_back([t,format](JsonObject m){auto next=format(m);if(t.Text()!=next)t.Text(next);});return t;}
 Grid liveRow(std::wstring label,std::function<std::wstring(JsonObject)> format){auto row=metric(label,format(monitoring()));auto value=row.Children().GetAt(1).as<TextBlock>();monitorBindings.push_back([value,format](JsonObject m){auto next=format(m);if(value.Text()!=next)value.Text(next);});return row;}
 Grid liveMetric(std::wstring label,std::wstring key,std::wstring unit,int precision=0,bool percent=false){auto row=metric(label,tr(L"读取中"));auto value=row.Children().GetAt(1).as<TextBlock>();auto update=[this,value,key,unit,precision](JsonObject m){auto v=m.GetNamedValue(key,JsonValue::CreateNullValue());bool available=v.ValueType()==JsonValueType::Number;auto kind=str(m.GetNamedObject(L"fieldAvailability",JsonObject{}),key.c_str());value.Text(available?digits(v.GetNumber(),precision)+unit:kind==L"initializing"?tr(L"读取中…"):kind==L"needsDriver"?tr(L"需驱动"):kind==L"authorizing"?tr(L"授权中…"):kind==L"needsElevation"?tr(L"需授权"):kind==L"unsupported"?tr(L"未提供"):flag(m,L"virtualMachine")?tr(L"VM 未回传"):tr(L"未回传"));bool neutral=kind==L"authorizing"||kind==L"initializing"||kind==L"unsupported";value.Foreground(available?(dark?brush(245,245,245):brush(27,27,27)):neutral?brush(140,140,140):brush(196,43,28));ToolTipService::SetToolTip(value,box_value(str(m.GetNamedObject(L"unavailableReasons",JsonObject{}),key.c_str())));};update(monitoring());monitorBindings.push_back(update);return row;}
 winrt::fire_and_forget elevateMonitoring(){if(flag(monitoring(),L"sensorElevationPending"))co_return;auto lifetime=get_strong();winrt::apartment_context ui;JsonObject next{nullptr};std::wstring error;try{co_await resume_background();next=hub::call(L"monitorElevate");}catch(hresult_error const& e){error=std::wstring(e.message());}co_await ui;if(next){acceptState(next);for(auto const& update:monitorBindings)update(monitoring());notice(tr(L"等待 Windows 授权；仅提升只读传感器采集进程"));}else notice(error.empty()?tr(L"无法启动授权采集，请重试"):error,true);}

 winrt::fire_and_forget refreshMonitoring(){
  if(monitoringFixture)co_return;
  syncMonitoringDemand();if(monitorBusy||(monitoringPage(current)&&!monitoringActive))co_return;monitorBusy=true;auto lifetime=get_strong();
  auto oldDevices=deviceSignature(),oldDisks=diskSignature();auto requestedPage=current;auto generation=renderGeneration;auto epoch=monitorEpoch.load();
  auto operation=(current==L"devices"||current==L"device-detail")?L"deviceSnapshot":L"monitor";
  winrt::apartment_context ui;JsonObject next{nullptr};
  try{co_await resume_background();std::lock_guard lock(monitoringIo);if(epoch==monitorEpoch.load())next=hub::call(operation);}catch(...){}
  co_await ui;finishBusy();
  if(current!=requestedPage||generation!=renderGeneration||epoch!=monitorEpoch.load())co_return;
  if(next){acceptState(next);if(operation==L"monitor"){
   auto sample=HomeMonitoring::trendSample(monitoring(),homeSnapshot());
   if(monitorHistory.empty()||num(sample,L"sampleTime")>num(monitorHistory.back(),L"sampleTime"))monitorHistory.push_back(sample);
   while(!monitorHistory.empty()&&(clockSeconds()-num(monitorHistory.front(),L"sampleTime")>60||monitorHistory.size()>90))monitorHistory.erase(monitorHistory.begin());
  }}
  if(current!=requestedPage||generation!=renderGeneration)co_return;
  if(next&&((current==L"home"||current==L"devices")&&oldDevices!=deviceSignature()||(current==L"home"||current==L"system")&&oldDisks!=diskSignature())){render();co_return;}
  if(!next&&(current==L"home"||current==L"system")){
   auto unavailable=HomeMonitoring::build(JsonObject{},clockSeconds());put(unavailable,L"status",std::wstring(L"stale"));
   auto fields=unavailable.GetNamedObject(L"fields");for(auto const& pair:fields){auto field=pair.Value().GetObject();put(field,L"status",std::wstring(L"stale"));put(field,L"reason",std::wstring(tr(L"无法连接核心服务，正在重试")));}
   unavailable.SetNamedValue(L"devices",homeSnapshot().GetNamedArray(L"devices",JsonArray{}));state.SetNamedValue(L"homeMonitoring",unavailable);
  }
  for(auto const& update:deviceBindings)update();
  if(current==L"home"||current==L"system")for(auto const& update:monitorBindings)update(next?monitoring():JsonObject{});
 }

#include "CpuGpuSensors.inc"
 void monitorDetails(){if(compactUi){Expander details;details.Header(box_value(tr(L"传感器详情 · CPU / GPU")));details.HorizontalAlignment(HorizontalAlignment::Stretch);details.HorizontalContentAlignment(HorizontalAlignment::Stretch);details.Content(cpuGpuSensorColumns());details.IsExpanded(false);page.Children().Append(details);}else{page.Children().Append(title(tr(L"传感器详情"),20));page.Children().Append(cpuGpuSensorColumns());}}
 void monitorChart(std::wstring label,std::wstring key){auto panel=stack(false,6);panel.Children().Append(title(label,15));Canvas canvas;canvas.Height(70);canvas.Background(dark?brush(32,32,32):brush(248,248,248));Microsoft::UI::Xaml::Shapes::Polyline line;line.Stroke(accent());line.StrokeThickness(2);canvas.Children().Append(line);auto draw=[this,canvas,line,key](JsonObject){line.Points().Clear();auto now=clockSeconds();double width=std::max(1.0,canvas.ActualWidth());for(auto const& m:monitorHistory){auto v=m.GetNamedValue(key,JsonValue::CreateNullValue());if(v.ValueType()!=JsonValueType::Number){line.Points().Clear();continue;}double x=std::clamp(1.0-(now-num(m,L"sampleTime"))/60.0,0.0,1.0)*width;double y=66-std::clamp(v.GetNumber(),0.0,100.0)*0.62;line.Points().Append(Windows::Foundation::Point{(float)x,(float)y});}};onSizeChanged(canvas,[draw](auto const&,auto const&){draw(JsonObject{});});monitorBindings.push_back(draw);draw(monitoring());panel.Children().Append(canvas);panel.Children().Append(text(tr(L"0–100% · 2 秒采样 · 不可用时留空"),10,true));page.Children().Append(card(panel,12));}
 void summary(){Grid grid;for(int i=0;i<2;i++){ColumnDefinition c;c.Width({1,GridUnitType::Star});grid.ColumnDefinitions().Append(c);}for(int i=0;i<2;i++){RowDefinition r;r.Height({0,GridUnitType::Auto});grid.RowDefinitions().Append(r);}
  auto cpu=stack(false,4);cpu.Children().Append(hardwareHeading(L"CPU",L"\uE950",L"cpuName"));cpu.Children().Append(liveMetric(tr(L"利用率"),L"cpu",L"%",0,true));cpu.Children().Append(liveMetric(tr(L"频率"),L"cpuClock",L" MHz"));cpu.Children().Append(liveMetric(tr(L"温度"),L"cpuTemp",L" °C"));cpu.Children().Append(liveMetric(tr(L"功耗"),L"cpuPower",L" W",1));
  auto gpu=stack(false,4);gpu.Children().Append(hardwareHeading(L"GPU",L"\uE7F4",L"gpuName",true));gpu.Children().Append(liveMetric(tr(L"利用率"),L"gpu",L"%",0,true));gpu.Children().Append(liveRow(tr(L"独立显存"),[](JsonObject m){return amount(m,L"gpuUsedGB",1)+L" / "+amount(m,L"gpuTotalGB",1)+L" GB";}));gpu.Children().Append(liveMetric(tr(L"频率"),L"gpuClock",L" MHz"));gpu.Children().Append(liveMetric(tr(L"温度"),L"gpuTemp",L" °C"));gpu.Children().Append(liveMetric(tr(L"功耗"),L"gpuPower",L" W",1));
  auto memory=stack(false,4);memory.Children().Append(metricTitle(tr(L"内存"),L"\uE964"));memory.Children().Append(liveRow(tr(L"已使用"),[](JsonObject m){return amount(m,L"memoryUsedGB",1)+L" / "+amount(m,L"memoryTotalGB",1)+L" GB";}));memory.Children().Append(liveMetric(tr(L"利用率"),L"memory",L"%",0,true));
  auto disk=physicalDisks();
  std::vector<StackPanel> panels={cpu,gpu,memory,disk};std::vector<Border> cards;for(int i=0;i<4;i++){auto b=card(panels[i],16);b.MinHeight(208);b.Margin(Thickness{0,0,i%2==1?0.0:12.0,i<2?12.0:0.0});Grid::SetColumn(b,i%2);Grid::SetRow(b,i/2);grid.Children().Append(b);cards.push_back(b);std::vector<std::vector<std::wstring>> keys={{L"cpu",L"cpuClock",L"cpuTemp",L"cpuPower"},{L"gpu",L"gpuClock",L"gpuTemp",L"gpuPower"},{L"memory",L"memoryUsedGB",L"memoryTotalGB"},{L"diskReadMB",L"diskWriteMB",L"diskUsedGB",L"diskTotalGB"}};auto update=[this,b,required=keys[i]](JsonObject m){bool valid=true;if(required.size()==4&&required.front()==L"diskReadMB")return;for(auto const& key:required)if(m.GetNamedValue(key,JsonValue::CreateNullValue()).ValueType()!=JsonValueType::Number){auto status=str(m.GetNamedObject(L"fieldAvailability",JsonObject{}),key.c_str());if(status!=L"initializing"&&status!=L"unsupported")valid=false;}markFeedback(b,valid);};update(monitoring());monitorBindings.push_back(update);}onSizeChanged(grid,[cards](auto const& sender,SizeChangedEventArgs const& event){auto layout=sender.template as<Grid>();double width=event.NewSize().Width;int columns=width>=840?4:width>=560?2:1;if((int)layout.ColumnDefinitions().Size()==columns)return;layout.ColumnDefinitions().Clear();layout.RowDefinitions().Clear();for(int i=0;i<columns;i++){ColumnDefinition c;c.Width({1,GridUnitType::Star});layout.ColumnDefinitions().Append(c);}int rows=(4+columns-1)/columns;for(int i=0;i<rows;i++){RowDefinition r;r.Height({0,GridUnitType::Auto});layout.RowDefinitions().Append(r);}for(int i=0;i<4;i++){Grid::SetColumn(cards[i],i%columns);Grid::SetRow(cards[i],i/columns);cards[i].Margin(Thickness{0,0,i%columns==columns-1?0.0:12.0,i/columns==rows-1?0.0:12.0});}});page.Children().Append(grid);
 }
 JsonObject matchedDevicePlugin(JsonObject device){for(auto const& value:state.GetNamedArray(L"plugins")){auto p=value.GetObject();if(str(p,L"id")==L"hhub.developer-tools"||!flag(p,L"installed")||str(p,L"mode")==L"disabled"||!hasOfflineUi(p))continue;for(auto const& target:p.GetNamedArray(L"hidTargets",JsonArray{})){auto t=target.GetObject();if(num(device,L"vid",-1)==num(t,L"vid",-2)&&num(device,L"pid",-1)==num(t,L"pid",-2))return p;}if(str(device,L"kind")==str(p,L"kind",L"mouse"))for(auto const& value:p.GetNamedArray(L"hidV2Targets",JsonArray{})){auto t=value.GetObject();auto pids=t.GetNamedArray(L"productIds",JsonArray{});if(!pids.Size())continue;bool vendor=false,product=false;for(auto const& v:t.GetNamedArray(L"vendorIds",JsonArray{}))if(v.GetNumber()==num(device,L"vid",-1))vendor=true;for(auto const& v:pids)if(v.GetNumber()==num(device,L"pid",-1))product=true;if(vendor&&product)return p;}}return nullptr;}
 std::wstring deviceDisplayName(JsonObject d){auto p=matchedDevicePlugin(d);return p?str(p,L"deviceName",str(p,L"name").c_str()):str(d,L"name");}
 void openDevicePage(JsonObject d){auto p=matchedDevicePlugin(d);if(p){devicePluginId=str(p,L"id");go(L"device-plugin");}else {invoke(L"selectDetectedDevice",idBody(str(d,L"id")));go(L"device-detail");} }
 winrt::fire_and_forget loadDeviceArt(Image image,JsonObject device){try{auto weakImage=make_weak(image);int decodeHeight=std::isfinite(image.Height())?std::clamp((int)(image.Height()*GetDpiForWindow(hwnd)/96.0),32,700):700;image=nullptr;auto plugin=matchedDevicePlugin(device);if(plugin){auto html=str(plugin.GetNamedObject(L"ui"),L"content");auto start=html.find(L"data:image/png;base64,");if(start!=std::wstring::npos){start+=22;auto end=start;while(end<html.size()&&(iswalnum(html[end])||html[end]==L'+'||html[end]==L'/'||html[end]==L'='))++end;auto buffer=Windows::Security::Cryptography::CryptographicBuffer::DecodeFromBase64String(html.substr(start,end-start));Windows::Storage::Streams::InMemoryRandomAccessStream stream;Windows::Storage::Streams::DataWriter writer(stream);writer.WriteBuffer(buffer);co_await writer.StoreAsync();writer.DetachStream();stream.Seek(0);Microsoft::UI::Xaml::Media::Imaging::BitmapImage source;source.DecodePixelHeight(decodeHeight);co_await source.SetSourceAsync(stream);if(auto target=weakImage.get())target.Source(source);co_return;}}auto id=str(device,L"id"),kind=str(device,L"kind");std::filesystem::path path;path=binaryDir()/L"assets"/(kind==L"mouse"?L"mouse.svg":kind==L"keyboard"?L"keyboard.svg":kind==L"gamepad"?L"gamepad.svg":L"headset.svg");auto file=co_await Windows::Storage::StorageFile::GetFileFromPathAsync(path.wstring());auto stream=co_await file.OpenAsync(Windows::Storage::FileAccessMode::Read);if(path.extension()==L".svg"){Microsoft::UI::Xaml::Media::Imaging::SvgImageSource source;co_await source.SetSourceAsync(stream);if(auto target=weakImage.get())target.Source(source);}else{Microsoft::UI::Xaml::Media::Imaging::BitmapImage source;source.DecodePixelHeight(decodeHeight);co_await source.SetSourceAsync(stream);if(auto target=weakImage.get())target.Source(source);}}catch(...) {}}
 JsonArray visibleDevices(){auto detected=state.GetNamedObject(L"detected",JsonObject{});return detected.GetNamedArray(showAllDevices?L"devices":L"peripherals",JsonArray{});}
 std::wstring deviceSignature(){std::wstring out;for(auto const& v:visibleDevices()){auto d=v.GetObject();out+=str(d,L"id")+str(d,L"name")+str(d,L"kind")+str(d,L"usbIds")+std::wstring(d.GetNamedArray(L"interfaces",JsonArray{}).Stringify());}return out;}
 TextBlock realText(JsonObject initial,std::function<std::wstring(JsonObject)> format,double size=14,bool muted=false){auto t=text(format(initial),size,muted);auto id=str(initial,L"id");deviceBindings.push_back([this,t,id,format]{try{t.Text(format(find(visibleDevices(),id)));}catch(...){t.Text(tr(L"设备已移除"));}});return t;}
 std::wstring selectedDetected(){auto id=str(state,L"selectedDetectedDevice");auto list=visibleDevices();for(auto const& v:list)if(str(v.GetObject(),L"id")==id)return id;return list.Size()?str(list.GetObjectAt(0),L"id"):L"";}
 winrt::fire_and_forget refreshDeviceScan(){if(monitorBusy)co_return;monitorBusy=true;auto lifetime=get_strong();winrt::apartment_context ui;JsonObject next{nullptr};try{co_await winrt::resume_background();next=hub::call(L"deviceScan");}catch(...){}co_await ui;finishBusy();if(next){acceptState(next);render();notice(tr(L"已刷新 Windows 真实设备列表"));}else notice(tr(L"设备扫描失败，请重试"),true);}
 void actualDeviceDetails(JsonObject d){auto p=stack(false,12);p.Children().Append(title(tr(L"真实设备读取"),20));p.Children().Append(realText(d,[](JsonObject v){return tr(L"状态：")+translated(str(v,L"status"))+tr(L"\n连接：")+str(v,L"connection")+tr(L"\n硬件标识：")+str(v,L"usbIds",tr(L"见下方接口标识"))+tr(L"\n制造商：")+str(v,L"manufacturer",tr(L"Windows 未提供"))+tr(L"\n电量：")+(v.HasKey(L"battery")?digits(num(v,L"battery"))+L"% · "+str(v,L"batterySource"):v.HasKey(L"batteryLevel")?str(v,L"batteryLevel"):tr(L"不可用"))+tr(L"\n容器标识：")+str(v,L"containerId",tr(L"Windows 未提供"));},13));
 p.Children().Append(button(tr(L"帮助"),[this]{helpDialog(tr(L"使用说明"),tr(L"接收器存在不代表无线设备本体在线。普通有线耳机显示系统音频端点，无法仅凭声卡识别耳机型号。DPI、回报率、固件和专用无线电量需对应型号协议支持。Windows 提供的蓝牙电量可能是系统缓存值。"));}));p.Children().Append(button(tr(L"重新读取"),[this]{refreshDeviceScan();}));p.Children().Append(title(tr(L"Windows 接口"),18));for(auto const& v:d.GetNamedArray(L"interfaces",JsonArray{})){auto i=v.GetObject();p.Children().Append(text(str(i,L"name")+L" · "+str(i,L"class")+L"\n"+str(i,L"instanceId")+tr(L"\n来源：")+str(i,L"source"),12,true));}
 if(d.HasKey(L"buttons")){p.Children().Append(title(tr(L"XInput 实时状态"),18));p.Children().Append(realText(d,[](JsonObject v){return tr(L"按键掩码：")+digits(num(v,L"buttons"))+tr(L"\n扳机 L / R：")+digits(num(v,L"leftTrigger"))+L" / "+digits(num(v,L"rightTrigger"))+tr(L"\n左摇杆 X / Y：")+digits(num(v,L"leftX"))+L" / "+digits(num(v,L"leftY"))+tr(L"\n右摇杆 X / Y：")+digits(num(v,L"rightX"))+L" / "+digits(num(v,L"rightY"));},13));p.Children().Append(button(tr(L"帮助"),[this]{helpDialog(tr(L"使用说明"),tr(L"逻辑槽位未与 PnP 物理设备建立对应关系，可能与上方 HID 接口属于同一手柄。"));}));}appendDevicePluginActions(p,d);for(auto const& error:state.GetNamedObject(L"detected",JsonObject{}).GetNamedArray(L"errors",JsonArray{}))p.Children().Append(text(std::wstring(error.GetString()),12,true));page.Children().Append(card(p));}
 UIElement deviceArt(JsonObject device,double height){if(str(device,L"kind")==L"peripheral"||str(device,L"kind")==L"tablet"||str(device,L"kind")==L"touchpad"||str(device,L"kind")==L"touchscreen"){FontIcon icon;icon.Glyph(L"\uE772");icon.FontSize(std::min(90.0,height*0.5));icon.Foreground(accent());return icon;}Image image;image.Height(height);image.Stretch(Stretch::Uniform);loadDeviceArt(image,device);return image;}
 static std::wstring deviceKindLabel(std::wstring kind){return kind==L"mouse"?tr(L"鼠标"):kind==L"keyboard"?tr(L"键盘"):kind==L"gamepad"?tr(L"手柄"):kind==L"audio"?tr(L"音频"):kind==L"tablet"?tr(L"手写板"):kind==L"touchpad"?tr(L"触控板"):kind==L"touchscreen"?tr(L"触摸屏"):tr(L"其他设备");}
 winrt::fire_and_forget deviceOverviewDialog(){auto lifetime=get_strong();auto all=state.GetNamedObject(L"detected",JsonObject{}).GetNamedArray(L"devices",JsonArray{});ContentDialog dialog;dialog.XamlRoot(page.XamlRoot());dialog.Title(box_value(tr(L"设备概览")));dialog.CloseButtonText(tr(L"关闭"));auto body=stack(false,10);body.Children().Append(text(tr(L"按物理设备合并接口；接收器存在不代表无线本体在线。"),12,true));std::shared_ptr<std::wstring> chosen=std::make_shared<std::wstring>();
  for(bool others:{false,true}){body.Children().Append(title(others?tr(L"其他设备与系统接口"):tr(L"已连接外设"),17));for(auto const& v:all){auto d=v.GetObject();if(DeviceIsOther(d)!=others)continue;auto row=stack(false,3);row.Children().Append(text(str(d,L"name"),14));row.Children().Append(text(deviceKindLabel(str(d,L"kind"))+L" · "+str(d,L"connection")+L" · "+translated(str(d,L"status")),11,true));Button b;b.HorizontalContentAlignment(HorizontalAlignment::Center);b.VerticalContentAlignment(VerticalAlignment::Center);b.Content(row);b.HorizontalAlignment(HorizontalAlignment::Stretch);b.HorizontalContentAlignment(HorizontalAlignment::Left);auto id=str(d,L"id");auto weakDialog=make_weak(dialog);onClick(b,[chosen,id,weakDialog](auto const&,auto const&){*chosen=id;if(auto dialog=weakDialog.get())dialog.Hide();});body.Children().Append(b);}}
  ScrollViewer view;view.MaxHeight(440);view.VerticalScrollBarVisibility(ScrollBarVisibility::Auto);view.Content(body);dialog.Content(view);co_await showDialog(dialog);if(!chosen->empty()){bool visible=false;for(auto const& v:state.GetNamedObject(L"detected").GetNamedArray(L"peripherals",JsonArray{}))if(str(v.GetObject(),L"id")==*chosen)visible=true;showAllDevices=!visible;invoke(L"selectDetectedDevice",idBody(*chosen));go(L"devices");}
 }
 bool DeviceIsOther(JsonObject device){for(auto const& v:state.GetNamedObject(L"detected",JsonObject{}).GetNamedArray(L"peripherals",JsonArray{}))if(str(v.GetObject(),L"id")==str(device,L"id"))return false;return true;}
 void cycleDevice(int direction){auto list=visibleDevices();if(list.Size()==0)return;int selected=0;for(uint32_t i=0;i<list.Size();i++)if(str(list.GetObjectAt(i),L"id")==selectedDetected())selected=i;selected=(selected+direction+(int)list.Size())%(int)list.Size();invoke(L"selectDetectedDevice",idBody(str(list.GetObjectAt(selected),L"id")),true);}
 void homeSearch(std::wstring query){if(query.empty())return;auto lower=[](std::wstring s){std::transform(s.begin(),s.end(),s.begin(),towlower);return s;};for(auto const& v:visibleDevices()){auto d=v.GetObject();if(lower(str(d,L"name")).find(lower(query))!=std::wstring::npos){invoke(L"selectDetectedDevice",idBody(str(d,L"id")));go(L"device-detail");return;}}for(auto const& v:state.GetNamedArray(L"plugins")){auto p=v.GetObject();if(flag(p,L"installed")&&lower(str(p,L"name")+str(p.GetNamedObject(L"navigation",JsonObject{}),L"title")).find(lower(query))!=std::wstring::npos){openInstalledPlugin(str(p,L"id"));return;}}if(query.find(L"插件")!=std::wstring::npos)go(L"market");else if(query.find(L"场景")!=std::wstring::npos)go(L"scenes");else if(query.find(L"系统")!=std::wstring::npos)go(L"system");else if(query.find(L"设置")!=std::wstring::npos)go(L"settings");else notice(tr(L"没有找到匹配内容"));}
 ComboBox devicePicker(){auto a=visibleDevices();std::vector<std::wstring> names;int index=0;for(uint32_t i=0;i<a.Size();i++){auto d=a.GetObjectAt(i);names.push_back(str(d,L"name"));if(str(d,L"id")==selectedDetected())index=i;}auto c=combo(names,index,300);onSelectionChanged(c,[this,c,a](auto const&,auto const&){if(c.SelectedIndex()<0)return;invoke(L"selectDetectedDevice",idBody(str(a.GetObjectAt(c.SelectedIndex()),L"id")),true);});return c;}
 void layoutDeviceArea(){if(isNativePluginRoute()){positionNativeSurface();return;}
  if((current==L"device-plugin"||current==L"developer")&&pluginView&&pluginView.IsLoaded()&&pageScroll&&pageScroll.ViewportHeight()>0){auto origin=pluginView.TransformToVisual(page).TransformPoint({0,0});double height=std::max(200.0,pageScroll.ViewportHeight()-origin.Y-page.Padding().Bottom-2);if(std::abs(pluginView.Height()-height)>1)pluginView.Height(height);return;}
  if(current!=L"home"||!deviceArea||!pageScroll||pageScroll.ViewportHeight()<=0||!deviceArea.IsLoaded())return;
  auto position=deviceArea.TransformToVisual(page).TransformPoint(Windows::Foundation::Point{0,0});
  double height=std::max(80.0,pageScroll.ViewportHeight()-position.Y-page.Padding().Bottom);
  if(!std::isfinite(deviceArea.Height())||std::abs(deviceArea.Height()-height)>1)deviceArea.Height(height);
 }
 void home(){nativeHomePage();}
 bool homeSelectionBusy=false;
 winrt::fire_and_forget selectHomeDevice(std::wstring id){
  if(homeSelectionBusy||id==selectedDetected())co_return;
  homeSelectionBusy=true;for(auto const& patch:deviceBindings)patch();auto lifetime=get_strong();auto generation=renderGeneration;winrt::apartment_context ui;JsonObject next{nullptr};
  try{co_await resume_background();next=hub::call(L"selectDetectedDevice",idBody(id));}catch(...){}
  co_await ui;homeSelectionBusy=false;
  if(!next){if(current==L"home"&&generation==renderGeneration){notice(tr(L"设备选择失败，请重试"),true);for(auto const& patch:deviceBindings)patch();}co_return;}
  acceptState(next);if(current==L"home"&&generation==renderGeneration)for(auto const& patch:deviceBindings)patch();
 }
 void deviceListPage(){page.Children().Append(actionHeader(text(tr(L"连接的设备与系统接口"),14,true),button(tr(L"刷新设备"),[this]{refreshDeviceScan();})));auto detected=state.GetNamedObject(L"detected",JsonObject{});auto peripherals=detected.GetNamedArray(L"peripherals",JsonArray{});auto all=detected.GetNamedArray(L"devices",JsonArray{});auto append=[this](JsonObject d){auto labels=stack(false,4);labels.Children().Append(title(deviceDisplayName(d),18));labels.Children().Append(realText(d,[](JsonObject v){return translated(str(v,L"status"))+(v.HasKey(L"battery")?L" · "+digits(num(v,L"battery"))+L"%":L"");},13,true));page.Children().Append(card(actionHeader(labels,button(tr(L"查看设备"),[this,d]{openDevicePage(d);})),14));};for(auto const& v:peripherals)append(v.GetObject());if(!peripherals.Size())page.Children().Append(text(tr(L"暂无连接的外设"),14,true));Expander interfaces;interfaces.Header(box_value(tr(L"系统与虚拟接口")));auto rows=stack(false,8);for(auto const& v:all){auto d=v.GetObject();bool external=false;for(auto const& p:peripherals)if(str(p.GetObject(),L"id")==str(d,L"id"))external=true;if(external)continue;rows.Children().Append(actionHeader(text(deviceDisplayName(d),14),button(tr(L"查看接口"),[this,d]{openDevicePage(d);})));}interfaces.Content(rows);page.Children().Append(interfaces);}
 void devices(){if(visibleDevices().Size()==0){page.Children().Append(text(tr(L"未发现可读取的外设。")));page.Children().Append(button(tr(L"刷新设备"),[this]{refreshDeviceScan();}));return;}page.Children().Append(devicePicker());actualDeviceDetails(find(visibleDevices(),selectedDetected()));}
 winrt::fire_and_forget openWindows(std::wstring uri){try{if(uri.rfind(L"ms-settings:",0)!=0)throw hresult_invalid_argument();bool opened=co_await Windows::System::Launcher::LaunchUriAsync(Windows::Foundation::Uri(uri));if(!opened)notice(tr(L"Windows 未提供该设置页面"),true);}catch(hresult_error const& e){notice(std::wstring(e.message()),true);}}
 void finishBusy(){monitorBusy=false;}
 void markFeedback(Border frame,bool confirmed){if(!frame)return;frame.BorderBrush(confirmed?(dark?brush(48,59,73):brush(225,230,237)):brush(196,43,28));frame.BorderThickness(margin(confirmed?1:2));}
 TextBlock pageTitle(std::wstring const& heading){return title(heading,24);}
 void pageHeading(std::wstring heading){if(isNativePluginRoute())return;if(current==L"device-plugin"){devicePluginHeading(heading);return;}Grid header;header.MinHeight(40);ColumnDefinition a;a.Width({1,GridUnitType::Star});ColumnDefinition b;b.Width({0,GridUnitType::Auto});header.ColumnDefinitions().Append(a);header.ColumnDefinitions().Append(b);auto name=pageTitle(heading);name.VerticalAlignment(VerticalAlignment::Center);auto headingRow=stack(true,12);headingRow.VerticalAlignment(VerticalAlignment::Center);if(current==L"devices"||current==L"device-detail"||current==L"system"||current==L"device-plugin"||current==L"market-sources"){Button back;FontIcon arrow;arrow.Glyph(L"\uE72B");back.Content(arrow);back.Width(40);back.Height(40);back.Padding(margin(0));back.VerticalAlignment(VerticalAlignment::Center);ToolTipService::SetToolTip(back,box_value(current==L"market-sources"?tr(L"返回插件市场"):tr(L"返回首页")));Microsoft::UI::Xaml::Automation::AutomationProperties::SetName(back,current==L"market-sources"?tr(L"返回插件市场"):tr(L"返回首页"));onClick(back,[this](auto const&,auto const&){if(current==L"market-sources")go(L"market");else if(current==L"device-detail")go(L"devices");else go(L"home");});headingRow.Children().Append(back);}headingRow.Children().Append(name);if(current==L"market"){auto settingsButton=button(L"",[this]{go(L"market-sources");});FontIcon gear;gear.Glyph(L"\uE713");settingsButton.Content(gear);settingsButton.MinWidth(0);settingsButton.MaxWidth(36);settingsButton.Width(36);settingsButton.Height(36);settingsButton.Padding(margin(0));ToolTipService::SetToolTip(settingsButton,box_value(tr(L"插件库管理")));Microsoft::UI::Xaml::Automation::AutomationProperties::SetName(settingsButton,tr(L"插件库管理"));headingRow.Children().Append(settingsButton);}header.Children().Append(headingRow);page.Children().Append(header);}
#include "GenericPluginPageHost.inc"
 winrt::fire_and_forget openExternal(std::wstring uri){try{co_await Windows::System::Launcher::LaunchUriAsync(Windows::Foundation::Uri(uri));}catch(hresult_error const& e){notice(std::wstring(e.message()),true);}}

 void system(bool showSummary=true){page.Children().Append(sensorAuthorizationControls());summary();monitorDetails();}








#include "SceneConfigurationPage.inc"
 void scenes(){sceneConfigurationPage();}

 winrt::fire_and_forget sceneDialog(JsonObject s){auto name=field(tr(L"场景名称"),str(s,L"name"));auto app=field(tr(L"首个前台应用路径（更多应用在场景详情添加）"),str(s,L"app"));NumberBox priority;priority.Header(box_value(tr(L"场景优先级 · 数值越大越优先")));priority.Minimum(0);priority.Maximum(1000);priority.Value(num(s,L"priority"));NumberBox dpi;dpi.Header(box_value(tr(L"鼠标 DPI")));dpi.Minimum(100);dpi.Maximum(32000);dpi.Value(num(s,L"dpi",1600));auto savedPower=str(s,L"power",tr(L"平衡"));auto power=combo({tr(L"节能"),tr(L"平衡"),tr(L"性能")},savedPower==L"节能"?0:savedPower==L"性能"?2:1);auto p=stack();p.Children().Append(name);p.Children().Append(app);p.Children().Append(priority);ContentDialog dialog;dialog.XamlRoot(page.XamlRoot());dialog.Title(box_value(tr(L"场景配置")));dialog.Content(p);dialog.PrimaryButtonText(tr(L"保存"));dialog.CloseButtonText(tr(L"取消"));auto result=co_await showDialog(dialog);if(result==ContentDialogResult::Primary){try{JsonObject scene=object(std::wstring(s.Stringify()));put(scene,L"id",str(s,L"id").empty()?L"scene-"+std::to_wstring(GetTickCount64()):str(s,L"id"));put(scene,L"name",std::wstring(name.Text()));put(scene,L"app",std::wstring(app.Text()));auto apps=scene.GetNamedArray(L"apps",JsonArray{});if(!app.Text().empty()){if(apps.Size())apps.SetAt(0,JsonValue::CreateStringValue(app.Text()));else apps.Append(JsonValue::CreateStringValue(app.Text()));}else if(apps.Size())apps.RemoveAt(0);scene.SetNamedValue(L"apps",apps);put(scene,L"priority",priority.Value());put(scene,L"dpi",dpi.Value());put(scene,L"power",std::vector<std::wstring>{L"节能",L"平衡",L"性能"}.at(power.SelectedIndex()));JsonObject b;b.SetNamedValue(L"scene",scene);sceneTargetId=str(scene,L"id");sceneSection=L"detail";invoke(L"saveScene",b,true);notice(tr(L"场景配置已保存"));}catch(hresult_error const& e){notice(std::wstring(e.message()),true);}}}

#include "NativePluginConfirm.inc"
#include "PluginCatalogPage.inc"
 void plugins(){installedPluginTiles.clear();auto tools=stack(true,12);tools.Children().Append(button(tr(L"安装本地插件"),[this]{importDialog();},true));tools.Children().Append(button(tr(L"检查更新"),[this]{invoke(L"checkUpdates",JsonObject{},true);}));auto name=pageTitle(tr(L"插件管理"));name.VerticalAlignment(VerticalAlignment::Center);tools.VerticalAlignment(VerticalAlignment::Center);tools.HorizontalAlignment(HorizontalAlignment::Right);Grid header;header.MinHeight(40);ColumnDefinition headingColumn;headingColumn.Width({1,GridUnitType::Star});ColumnDefinition toolsColumn;toolsColumn.Width({0,GridUnitType::Auto});header.ColumnDefinitions().Append(headingColumn);header.ColumnDefinitions().Append(toolsColumn);for(int i=0;i<2;++i){RowDefinition r;r.Height({0,GridUnitType::Auto});header.RowDefinitions().Append(r);}header.Children().Append(name);Grid::SetColumn(tools,1);header.Children().Append(tools);onSizeChanged(header,[name,tools](auto const&,SizeChangedEventArgs const& e){bool narrow=e.NewSize().Width<520;Grid::SetColumnSpan(name,narrow?2:1);Grid::SetRow(tools,narrow?1:0);Grid::SetColumn(tools,narrow?0:1);Grid::SetColumnSpan(tools,narrow?2:1);tools.Margin(narrow?Thickness{0,12,0,0}:Thickness{20,0,0,0});});page.Children().Append(header);pluginDropHint();for(auto const& v:state.GetNamedArray(L"plugins")){auto plugin=v.GetObject();auto id=str(plugin,L"id");if(!flag(plugin,L"installed")||flag(plugin,L"builtin"))continue;auto labels=stack(false,4);auto name=stack(true,10);name.Children().Append(title(str(plugin,L"name"),19));name.Children().Append(text(L"v"+str(plugin,L"version"),12,true));labels.Children().Append(name);auto runtime=state.GetNamedObject(L"pluginRuntime",JsonObject{}).GetNamedObject(id,JsonObject{});labels.Children().Append(text(flag(plugin,L"forceStopped")?tr(L"已强制停止"):!str(runtime,L"error").empty()?tr(L"运行异常"):num(runtime,L"processes")>0?tr(L"运行中"):tr(L"未运行"),12,true));bool offline=hasOfflineUi(plugin);Button pluginOpen{nullptr};if(offline||!nativePluginPage(plugin).empty()){pluginOpen=button(tr(L"打开"),[this,id]{openInstalledPlugin(id);});pluginOpen.Tag(box_value(L"plugin-open:"+id));pluginOpen.IsEnabled(!nativePluginPage(plugin).empty()||str(plugin,L"mode")!=L"disabled");}auto mode=combo({tr(L"按需启动"),tr(L"有设备时运行"),tr(L"手动启动"),tr(L"禁用")},str(plugin,L"mode")==L"resident"?1:str(plugin,L"mode")==L"manual"?2:str(plugin,L"mode")==L"disabled"?3:0,150);onSelectionChanged(mode,[this,mode,id,pluginOpen](auto const&,auto const&){if(mode.SelectedIndex()<0)return;try{std::vector<std::wstring> modes={L"auto",L"resident",L"manual",L"disabled"};auto chosen=modes.at(mode.SelectedIndex());if(chosen==str(find(state.GetNamedArray(L"plugins"),id),L"mode"))return;auto body=idBody(id);put(body,L"mode",chosen);invoke(L"pluginMode",body);if(pluginOpen)pluginOpen.IsEnabled(!nativePluginPage(pluginRecord(state,id)).empty()||chosen!=L"disabled");}catch(hresult_error const& e){notice(std::wstring(e.message()),true);}});auto controls=stack(true,8);controls.Children().Append(mode);if(pluginOpen)controls.Children().Append(pluginOpen);MenuFlyout menu;auto add=[&](std::wstring label,std::function<void()> fn){MenuFlyoutItem item;item.Text(label);onClick(item,[fn](auto const&,auto const&){fn();});menu.Items().Append(item);};if(nativePluginPage(plugin).empty()&&!offline)add(tr(L"启动"),[this,id]{invoke(L"pluginStart",idBody(id),true);});add(tr(L"强制停止"),[this,id]{invoke(L"pluginForceStop",idBody(id),true);});add(tr(L"权限与来源"),[this,plugin]{permissionDialog(plugin);});add(flag(plugin,L"isolated")?tr(L"选择兼容模式"):tr(L"恢复隔离模式"),[this,id,plugin]{if(flag(plugin,L"isolated"))confirmPlugin(id,L"compat");else{auto body=idBody(id);put(body,L"isolated",true);invoke(L"pluginIsolation",body,true);}});for(auto const& u:state.GetNamedObject(L"updates",JsonObject{}).GetNamedArray(L"plugins",JsonArray{})){auto update=u.GetObject();if(str(update,L"id")==id&&flag(update,L"ready"))add(tr(L"审阅并安装更新"),[this,id]{pluginUpdateDialog(id);});}add(tr(L"卸载"),[this,id]{confirmPlugin(id,L"remove");});auto more=button(tr(L"更多"),[]{});more.Flyout(menu);controls.Children().Append(more);auto tile=card(actionHeader(labels,controls),14);installedPluginTiles.push_back({tile,plugin});page.Children().Append(tile);}if(installedPluginTiles.empty())page.Children().Append(actionHeader(text(tr(L"尚未安装插件"),14,true),button(tr(L"打开插件市场"),[this]{go(L"market");})));}

 winrt::fire_and_forget permissionDialog(JsonObject plugin){auto body=stack(false,8);body.Children().Append(text(tr(L"作者：")+str(plugin,L"author")+L" · v"+str(plugin,L"version"),14));body.Children().Append(text(tr(L"更新来源：")+str(plugin,L"updateUrl",tr(L"未声明")),12,true));for(auto const& value:plugin.GetNamedArray(L"permissions",JsonArray{}))body.Children().Append(text(std::wstring(value.GetString()),14));ContentDialog dialog;dialog.XamlRoot(page.XamlRoot());dialog.Title(box_value(tr(L"权限与来源")));dialog.Content(body);dialog.CloseButtonText(tr(L"关闭"));co_await showDialog(dialog);}
 winrt::fire_and_forget confirmPlugin(std::wstring id,std::wstring op){auto lifetime=get_strong();auto plugin=pluginRecord(state,id);if(!plugin)co_return;ContentDialog d;d.XamlRoot(page.XamlRoot());d.Title(box_value(op==L"compat"?tr(L"兼容模式偏好"):tr(L"卸载插件")));d.Content(box_value(op==L"compat"?tr(L"此操作保存插件运行偏好。"):str(plugin,L"uninstallNotice",tr(L"将停止并卸载插件，清理由插件声明的配置与资源。"))));d.PrimaryButtonText(tr(L"确定"));d.CloseButtonText(tr(L"取消"));if(co_await showDialog(d)!=ContentDialogResult::Primary)co_return;try{auto b=idBody(id);if(op==L"compat"){put(b,L"isolated",false);invoke(L"pluginIsolation",b,true);}else{if(surfaceOwner==id)releaseNativeSurface();invoke(L"pluginRemove",b,true);}}catch(hresult_error const& e){notice(std::wstring(e.message()),true);}}
 winrt::fire_and_forget importDialog(){wchar_t path[32768]{};OPENFILENAMEW picker{sizeof(picker)};picker.hwndOwner=hwnd;picker.lpstrFilter=tr(L"H hub 插件包 (*.hhub;*.json)\0*.hhub;*.json\0\0");picker.lpstrFile=path;picker.nMaxFile=32768;picker.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;if(GetOpenFileNameW(&picker))importPackage(path);co_return;}
 winrt::fire_and_forget importPackage(std::filesystem::path path){auto lifetime=get_strong();if(pluginImportBusy){notice(tr(L"请先完成当前插件安装"),true);co_return;}pluginImportBusy=true;try{auto manifest=readPluginPackage(path);auto p=stack();p.Children().Append(text(tr(L"名称：")+str(manifest,L"name")+tr(L"\n作者：")+str(manifest,L"author")+tr(L"\n版本：")+str(manifest,L"version")+tr(L"\n签名：未验证\n更新地址：")+str(manifest,L"updateUrl")));p.Children().Append(text(tr(L"申请权限：")));for(auto const& v:manifest.GetNamedArray(L"permissions",JsonArray{}))p.Children().Append(text(L"• "+std::wstring(v.GetString())));p.Children().Append(button(tr(L"帮助"),[this]{helpDialog(tr(L"使用说明"),tr(L"离线页面插件会加载包中的页面脚本，仅在你确认安装后导入，设备访问受声明权限和型号规则约束。普通包只登记元信息；原生插件包会校验并安装所声明的宿主组件。EQ 驱动需要另行确认和管理员授权。"));}));ContentDialog d;d.XamlRoot(page.XamlRoot());d.Title(box_value(tr(L"确认安装插件")));d.Content(p);d.PrimaryButtonText(tr(L"安装插件"));d.CloseButtonText(tr(L"取消"));if(co_await showDialog(d)==ContentDialogResult::Primary){JsonObject b;b.SetNamedValue(L"manifest",manifest);put(b,L"replaceInstalled",pluginInstalled(state,str(manifest,L"id")));invoke(L"importPlugin",b,true);notice(!nativePluginPage(manifest).empty()?tr(L"原生插件及组件已安装"):hasOfflineUi(manifest)?tr(L"插件已安装，可以从插件卡片打开"):tr(L"本地插件元信息已登记，没有执行硬件代码"));}}catch(hresult_error const& e){notice(std::wstring(e.message()),true);}catch(std::exception const& e){notice(std::wstring(to_hstring(e.what())),true);}pluginImportBusy=false;}
#include "PluginDrop.inc"



#include "AppSettingsPage.inc"


 winrt::fire_and_forget helpDialog(std::wstring heading,std::wstring value){ContentDialog d;d.XamlRoot(page.XamlRoot());d.Title(box_value(heading));d.Content(text(value));d.CloseButtonText(tr(L"关闭"));co_await showDialog(d);}
 winrt::fire_and_forget refreshApplications(){if(applicationsBusy)co_return;applicationsBusy=true;auto requestedPage=current;auto generation=renderGeneration;auto lifetime=get_strong();winrt::apartment_context ui;JsonObject next{nullptr};try{co_await resume_background();next=hub::call(L"applicationScan");}catch(...){}co_await ui;applicationsBusy=false;if(current!=requestedPage||generation!=renderGeneration)co_return;if(next){auto prior=str(state,L"frontendApp");state=next;if(current==L"scenes")for(auto const& update:applicationBindings)update(state);}}
 winrt::fire_and_forget refreshUpdates(){if(updatesBusy)co_return;updatesBusy=true;auto requestedPage=current;auto generation=renderGeneration,revision=settingsRevision;auto lifetime=get_strong();winrt::apartment_context ui;JsonObject next{nullptr};try{co_await resume_background();next=hub::call(L"updateState");}catch(...){}co_await ui;updatesBusy=false;if(current!=requestedPage||generation!=renderGeneration||revision!=settingsRevision)co_return;if(next){auto old=state.GetNamedObject(L"updates",JsonObject{}).Stringify();auto runtime=state.GetNamedObject(L"pluginRuntime",JsonObject{}).Stringify();state=next;if(old!=state.GetNamedObject(L"updates",JsonObject{}).Stringify()||runtime!=state.GetNamedObject(L"pluginRuntime",JsonObject{}).Stringify()){if(current==L"settings")updateSettingsStatus();else if(current==L"plugins")render();}}}
 winrt::fire_and_forget loadApplicationIcon(Image image,std::wstring path){try{auto weakImage=make_weak(image);image=nullptr;auto file=co_await Windows::Storage::StorageFile::GetFileFromPathAsync(path);if(!weakImage.get())co_return;auto thumbnail=co_await file.GetThumbnailAsync(Windows::Storage::FileProperties::ThumbnailMode::SingleItem,32);if(thumbnail&&weakImage.get()){Microsoft::UI::Xaml::Media::Imaging::BitmapImage bitmap;bitmap.DecodePixelHeight(32);co_await bitmap.SetSourceAsync(thumbnail);if(auto target=weakImage.get())target.Source(bitmap);}}catch(...) {}}
 static ScrollViewer listScroll(DependencyObject node){auto scroll=node.try_as<ScrollViewer>();if(scroll)return scroll;for(int i=0;i<VisualTreeHelper::GetChildrenCount(node);i++){auto found=listScroll(VisualTreeHelper::GetChild(node,i));if(found)return found;}return nullptr;}
 UIElement applicationPicker(bool audio){
  auto memory=&sceneChoices;auto body=stack(false,8);TextBox search;search.PlaceholderText(tr(L"搜索应用名称或程序名"));search.Text(memory->query);body.Children().Append(search);CheckBox advanced;advanced.Content(box_value(tr(L"显示后台和辅助进程")));advanced.IsChecked(box_value(memory->advanced).as<Windows::Foundation::IReference<bool>>());body.Children().Append(advanced);ListView list;list.Height(52);list.SelectionMode(ListViewSelectionMode::Single);list.HorizontalAlignment(HorizontalAlignment::Stretch);body.Children().Append(list);TextBlock status=text(L"",11,true);body.Children().Append(status);
  struct ViewState{std::wstring signature;bool rebuilding=false,initial=true,resetScroll=false;};auto view=std::make_shared<ViewState>();auto update=[this,audio,memory,view,list,status](JsonObject next){auto lower=[](std::wstring value){std::transform(value.begin(),value.end(),value.begin(),towlower);return value;};auto query=lower(memory->query);auto all=next.GetNamedObject(L"applications",JsonObject{}).GetNamedArray(L"running",JsonArray{});std::vector<JsonObject> visible;std::wstring signature;
   for(auto const& v:all){auto a=v.GetObject();auto path=str(a,L"path"),id=str(a,L"id");bool session=false;auto executable=lower(str(a,L"executable"));if(executable.rfind(L"hhub",0)==0)continue;bool helper=executable==L"applicationframehost.exe"||executable==L"textinputhost.exe"||executable==L"runtimebroker.exe"||executable==L"shellexperiencehost.exe"||executable==L"startmenuexperiencehost.exe"||executable==L"searchhost.exe";if(helper&&!memory->advanced&&!(audio&&session))continue;if(!memory->advanced&&!flag(a,L"hasWindow")&&!flag(a,L"foreground")&&!(audio&&session))continue;if(!query.empty()&&lower(str(a,L"name")+L" "+str(a,L"executable")+L" "+path).find(query)==std::wstring::npos)continue;put(a,L"audioSession",session);visible.push_back(a);signature+=id+str(a,L"name")+(flag(a,L"foreground")?L"F":L"-")+(session?L"A":L"-");}
   signature+=query+(memory->advanced?L"all":L"main")+memory->selected;if(signature==view->signature)return;view->signature=signature;auto scroll=listScroll(list);double offset=memory->offset;if(scroll&&!view->initial&&!view->resetScroll)offset=scroll.VerticalOffset();view->initial=false;view->resetScroll=false;view->rebuilding=true;list.Items().Clear();int selected=-1;
   for(uint32_t i=0;i<visible.size();i++){auto a=visible[i];auto path=str(a,L"path");Grid row;ColumnDefinition iconColumn;iconColumn.Width({38,GridUnitType::Pixel});ColumnDefinition textColumn;textColumn.Width({1,GridUnitType::Star});row.ColumnDefinitions().Append(iconColumn);row.ColumnDefinitions().Append(textColumn);Image icon;icon.Width(28);icon.Height(28);loadApplicationIcon(icon,path);row.Children().Append(icon);auto labels=stack(false,2);auto name=text(str(a,L"name")+(flag(a,L"foreground")?tr(L" · 最近前台"):L""),14);name.TextWrapping(TextWrapping::NoWrap);name.TextTrimming(TextTrimming::CharacterEllipsis);labels.Children().Append(name);auto info=text(str(a,L"executable")+(audio?(flag(a,L"audioSession")?tr(L" · 正在使用音频"):tr(L" · 尚未播放")):L""),11,true);info.TextWrapping(TextWrapping::NoWrap);info.TextTrimming(TextTrimming::CharacterEllipsis);labels.Children().Append(info);Grid::SetColumn(labels,1);row.Children().Append(labels);ListViewItem item;item.HorizontalContentAlignment(HorizontalAlignment::Stretch);item.Content(row);Microsoft::UI::Xaml::Automation::AutomationProperties::SetName(item,str(a,L"name"));item.Tag(box_value(path));ToolTipService::SetToolTip(item,box_value(path+L"\n"+digits(num(a,L"instances"))+tr(L" 个同路径进程")));list.Items().Append(item);if(lower(path)==lower(memory->selected))selected=(int)i;}
   list.Height(std::clamp<double>(visible.size()*44.0*Windows::UI::ViewManagement::UISettings{}.TextScaleFactor()+8,52,224));list.SelectedIndex(selected);view->rebuilding=false;status.Text(visible.empty()?tr(L"没有匹配的应用，可尝试显示后台进程"):std::to_wstring(visible.size())+tr(L" 个应用 · 同程序进程已合并"));list.UpdateLayout();scroll=listScroll(list);if(scroll)scroll.ChangeView(nullptr,box_value(offset).as<Windows::Foundation::IReference<double>>(),nullptr,true);memory->offset=offset;
  };
  onSelectionChanged(list,[this,audio,memory,view](auto const& sender,auto const&){if(view->rebuilding)return;auto list=sender.template as<ListView>();auto selected=list.SelectedItem().try_as<ListViewItem>();if(!selected)return;auto path=std::wstring(unbox_value<hstring>(selected.Tag()));if(path==memory->selected)return;memory->selected=path;if(auto scroll=listScroll(list))memory->offset=scroll.VerticalOffset();});
  onTextChanged(search,[this,memory,view,update](auto const& sender,auto const&){auto search=sender.template as<TextBox>();memory->query=std::wstring(search.Text());memory->offset=0;view->resetScroll=true;view->signature.clear();update(state);});auto advancedChanged=[this,memory,view,update](auto const& sender,auto const&){auto advanced=sender.template as<CheckBox>();memory->advanced=advanced.IsChecked()&&advanced.IsChecked().Value();memory->offset=0;view->resetScroll=true;view->signature.clear();update(state);};onChecked(advanced,advancedChanged);onUnchecked(advanced,advancedChanged);onLoaded(list,[this,memory](auto const& sender,auto const&){auto list=sender.template as<ListView>();if(auto scroll=listScroll(list)){scroll.ChangeView(nullptr,box_value(memory->offset).as<Windows::Foundation::IReference<double>>(),nullptr,true);onViewChanged(scroll,[memory](auto const& sender,auto const&){memory->offset=sender.template as<ScrollViewer>().VerticalOffset();});}});applicationBindings.push_back(update);update(state);return body;
 }

 winrt::fire_and_forget pluginUpdateDialog(std::wstring id){try{invoke(L"updateState");auto item=find(state.GetNamedObject(L"updates").GetNamedArray(L"plugins"),id);auto manifest=item.GetNamedObject(L"manifest");std::wstring info=tr(L"作者：")+str(manifest,L"author")+tr(L"\n版本：")+str(manifest,L"version")+tr(L"\n申请权限：");for(auto const& v:manifest.GetNamedArray(L"permissions",JsonArray{}))info+=L"\n• "+std::wstring(v.GetString());info+=manifest.HasKey(L"nativeAssets")?tr(L"\n将更新插件组件并保留现有配置。"):tr(L"\n将更新插件元信息与离线页面。");info+=str(manifest,L"installNotice");ContentDialog d;d.XamlRoot(page.XamlRoot());d.Title(box_value(tr(L"审阅插件更新")));d.Content(text(info));d.PrimaryButtonText(tr(L"批准更新"));d.CloseButtonText(tr(L"取消"));if(co_await showDialog(d)==ContentDialogResult::Primary){invoke(L"pluginUpdate",idBody(id),true);}}catch(hresult_error const& e){notice(std::wstring(e.message()),true);}}
};

int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int){winrt::init_apartment(winrt::apartment_type::single_threaded);if(uiMemoryTestProfile())std::set_terminate([]{try{if(auto error=std::current_exception())std::rethrow_exception(error);stage("Terminate without active exception");}catch(hresult_error const& e){stage(("Terminate HRESULT="+std::to_string((uint32_t)e.code().value)+" "+to_string(e.message())).c_str());}catch(std::exception const& e){stage(e.what());}catch(...){stage("Terminate unknown exception");}std::abort();});try{struct OleSession{OleSession(){check_hresult(OleInitialize(nullptr));}~OleSession(){OleUninitialize();}} oleSession;UiInstance instance;if(!instance.primary){if(std::wstring(GetCommandLineW()).find(L"--background-plugins")==std::wstring::npos)instance.activateExisting();return 0;}uiActivationEvent=instance.activation;Application::Start([](auto&&){winrt::make<App>();});return 0;}catch(hresult_error const& e){MessageBoxW(nullptr,e.message().c_str(),L"H hub",MB_ICONERROR);return 1;}catch(...){MessageBoxW(nullptr,tr(L"界面启动失败"),L"H hub",MB_ICONERROR);return 2;}}























