#pragma once
#include "Localization.h"
#include "WindowsOptions.h"
#include "WindowsWorkerClient.h"
#undef GetCurrentTime
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Controls.Primitives.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Xaml.Automation.h>
#include <winrt/Windows.UI.Text.h>
namespace hub::windowsUI {
using namespace winrt;using namespace Microsoft::UI::Xaml;using namespace Microsoft::UI::Xaml::Controls;using namespace Microsoft::UI::Xaml::Media;
inline std::atomic<unsigned> livePages{};
struct Page:std::enable_shared_from_this<Page>{
 Border mount{nullptr};Grid root{nullptr};StackPanel page{nullptr};ScrollViewer pageScroll{nullptr};JsonObject state;std::wstring current=L"windows";bool compactUi=false,shortUi=false,dark=false,windowsRefreshQueued=false,attached=true;std::map<std::wstring,std::wstring> windowsIssues;std::vector<std::function<void()>> pageEventCleanup;
 Page(){++livePages;}~Page(){--livePages;}
 auto get_strong(){return shared_from_this();}
 static Thickness margin(double n){return {n,n,n,n};}
 SolidColorBrush brush(unsigned r,unsigned g,unsigned b,unsigned a=255){return SolidColorBrush(Windows::UI::Color{(BYTE)a,(BYTE)r,(BYTE)g,(BYTE)b});}
 TextBlock text(std::wstring const& value,double size=14,bool muted=false){TextBlock t;t.Text(value);t.FontSize(size);t.TextWrapping(TextWrapping::Wrap);if(muted)t.Foreground(dark?brush(164,178,195):brush(95,105,120));return t;}
 TextBlock title(std::wstring const& value,double size=22){auto t=text(value,size);t.FontWeight(Windows::UI::Text::FontWeights::SemiBold());return t;}
 StackPanel stack(bool horizontal=false,double spacing=12){StackPanel p;p.Spacing(spacing);if(horizontal)p.Orientation(Orientation::Horizontal);return p;}
 Border card(UIElement child,double padding=20){Border b;b.Background(dark?brush(30,37,47):brush(255,255,255));b.BorderBrush(dark?brush(48,59,73):brush(225,230,237));b.BorderThickness(margin(1));b.CornerRadius({10,10,10,10});b.Padding(margin(compactUi?std::min(padding,12.0):padding));b.Child(child);return b;}
 void stage(std::string const&){}
 template<class F>void onToggled(ToggleSwitch value,F fn){auto token=value.Toggled(fn);pageEventCleanup.push_back([value,token]{value.Toggled(token);});}
 template<class F>void onLoaded(FrameworkElement value,F fn){auto token=value.Loaded(fn);pageEventCleanup.push_back([value,token]{value.Loaded(token);});}
 template<class F>void onSizeChanged(FrameworkElement value,F fn){auto token=value.SizeChanged(fn);pageEventCleanup.push_back([value,token]{value.SizeChanged(token);});}
 template<class F>void onViewChanged(ScrollViewer value,F fn){auto token=value.ViewChanged(fn);pageEventCleanup.push_back([value,token]{value.ViewChanged(token);});}
 void revoke(){for(auto& remove:pageEventCleanup)remove();pageEventCleanup.clear();windowsBindings.clear();windowsSignatures.clear();}
 void attach(IUnknown* content,JsonObject initial){mount=Windows::Foundation::IInspectable{content,take_ownership_from_abi}.as<Border>();state=initial.GetNamedObject(L"state",JsonObject{});dark=flag(initial,L"dark");initializeLanguage(str(state.GetNamedObject(L"settings",JsonObject{}),L"language",L"system"));render();refreshWindows();}
 void detach(){attached=false;current=L"detached";windowsUninstalling=true;++windowsMutationGeneration;windowsChanges.clear();windowsRefreshQueued=false;revoke();mount.Child(nullptr);page=nullptr;root=nullptr;pageScroll=nullptr;windowsOptionsHost=nullptr;}
 void render(){if(!attached)return;auto oldOffset=pageScroll?pageScroll.VerticalOffset():0;revoke();root=Grid{};root.Background(dark?brush(18,24,32):brush(245,247,250));root.RequestedTheme(dark?ElementTheme::Dark:ElementTheme::Light);pageScroll=ScrollViewer{};pageScroll.VerticalScrollBarVisibility(ScrollBarVisibility::Auto);pageScroll.HorizontalScrollBarVisibility(ScrollBarVisibility::Disabled);pageScroll.HorizontalScrollMode(ScrollMode::Disabled);pageScroll.ZoomMode(ZoomMode::Disabled);page=StackPanel{};page.Spacing(shortUi?8:compactUi?12:20);page.Padding(compactUi?Thickness{12,8,20,12}:Thickness{12,12,28,12});Grid header;header.MinHeight(40);auto name=title(tr(L"Windows 设置"),24);name.VerticalAlignment(VerticalAlignment::Center);header.Children().Append(name);page.Children().Append(header);windowsPage();pageScroll.Content(page);root.Children().Append(pageScroll);mount.Child(root);onSizeChanged(pageScroll,[this](auto const&,SizeChangedEventArgs const& e){bool compact=e.NewSize().Height>0&&e.NewSize().Height<720,shortLayout=e.NewSize().Height>0&&e.NewSize().Height<430;if(compact==compactUi&&shortLayout==shortUi)return;compactUi=compact;shortUi=shortLayout;auto self=shared_from_this();mount.DispatcherQueue().TryEnqueue([self]{if(self->attached)self->render();});});pageScroll.ChangeView(nullptr,box_value(oldOffset).as<Windows::Foundation::IReference<double>>(),nullptr,true);}
 void command(JsonObject data){auto action=str(data,L"action");if(action==L"refresh")refreshWindows();else if(action==L"theme"){dark=flag(data,L"dark",dark);render();}}
#include "WindowsSettingsPage.inc"
#include "WindowsRefresh.inc"
};
}
