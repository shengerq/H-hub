#pragma once
#include <winrt/Microsoft.UI.Xaml.Automation.h>
#include <winrt/Microsoft.UI.Xaml.Automation.Peers.h>
namespace hub {
// Small, bounded notification surface. It does not instantiate InfoBar templates.
struct LightFeedback {
 Microsoft::UI::Xaml::Controls::Border root{nullptr};Microsoft::UI::Xaml::Controls::TextBlock heading{nullptr},message{nullptr};Microsoft::UI::Xaml::Controls::Button close{nullptr};
 bool dark=false;Microsoft::UI::Xaml::Controls::InfoBarSeverity severity=Microsoft::UI::Xaml::Controls::InfoBarSeverity::Informational;std::function<void()> changed;
 LightFeedback(std::nullptr_t=nullptr){}
 void initialize(){using namespace Microsoft::UI::Xaml;using namespace Microsoft::UI::Xaml::Controls;using namespace Microsoft::UI::Xaml::Media;root=Border{};root.CornerRadius({10,10,10,10});root.Padding({12,10,10,10});root.BorderThickness({1,1,1,1});Grid body;ColumnDefinition textColumn;textColumn.Width({1,GridUnitType::Star});body.ColumnDefinitions().Append(textColumn);ColumnDefinition closeColumn;closeColumn.Width({0,GridUnitType::Auto});body.ColumnDefinitions().Append(closeColumn);StackPanel labels;labels.Spacing(4);heading=TextBlock{};heading.FontSize(14);heading.FontWeight(Windows::UI::Text::FontWeights::SemiBold());heading.TextWrapping(TextWrapping::Wrap);heading.Visibility(Visibility::Collapsed);message=TextBlock{};message.FontSize(13);message.TextWrapping(TextWrapping::Wrap);labels.Children().Append(heading);labels.Children().Append(message);body.Children().Append(labels);close=Button{};close.Content(winrt::box_value(L"×"));close.FontSize(18);close.Width(30);close.Height(30);close.MinWidth(0);close.MinHeight(0);close.Padding({0,0,0,0});close.Margin({10,0,0,0});close.BorderThickness({0,0,0,0});close.Background(SolidColorBrush(Windows::UI::Color{0,0,0,0}));close.VerticalAlignment(VerticalAlignment::Top);Automation::AutomationProperties::SetName(close,L"关闭提示");close.Click([this](auto const&,auto const&){IsOpen(false);});Grid::SetColumn(close,1);body.Children().Append(close);root.Child(body);Automation::AutomationProperties::SetLiveSetting(root,Automation::Peers::AutomationLiveSetting::Polite);IsOpen(false);paint();}
 void paint(){using namespace Microsoft::UI::Xaml::Media;using Microsoft::UI::Xaml::Controls::InfoBarSeverity;auto color=dark?Windows::UI::Color{255,35,50,68}:Windows::UI::Color{255,235,243,253};if(severity==InfoBarSeverity::Error)color=dark?Windows::UI::Color{255,74,36,40}:Windows::UI::Color{255,253,235,235};else if(severity==InfoBarSeverity::Success)color=dark?Windows::UI::Color{255,31,59,44}:Windows::UI::Color{255,235,249,239};root.Background(SolidColorBrush(color));root.BorderBrush(SolidColorBrush(dark?Windows::UI::Color{255,67,84,103}:Windows::UI::Color{255,207,220,235}));auto text=SolidColorBrush(dark?Windows::UI::Color{255,242,245,249}:Windows::UI::Color{255,27,37,51});heading.Foreground(text);message.Foreground(text);close.Foreground(text);}
 void theme(bool value){dark=value;paint();}
 void IsOpen(bool value){root.Visibility(value?Microsoft::UI::Xaml::Visibility::Visible:Microsoft::UI::Xaml::Visibility::Collapsed);if(changed)changed();}
 bool IsOpen()const{return root&&root.Visibility()==Microsoft::UI::Xaml::Visibility::Visible;}
 void IsClosable(bool value){close.Visibility(value?Microsoft::UI::Xaml::Visibility::Visible:Microsoft::UI::Xaml::Visibility::Collapsed);}
 void Title(std::wstring const& value){heading.Text(value);heading.Visibility(value.empty()?Microsoft::UI::Xaml::Visibility::Collapsed:Microsoft::UI::Xaml::Visibility::Visible);}
 void Message(std::wstring const& value){message.Text(value);Automation::AutomationProperties::SetName(root,value);}
 void Severity(Microsoft::UI::Xaml::Controls::InfoBarSeverity value){severity=value;paint();}
 void Margin(Microsoft::UI::Xaml::Thickness value){root.Margin(value);}void MaxWidth(double value){root.MaxWidth(value);}void VerticalAlignment(Microsoft::UI::Xaml::VerticalAlignment value){root.VerticalAlignment(value);}void HorizontalAlignment(Microsoft::UI::Xaml::HorizontalAlignment value){root.HorizontalAlignment(value);}
 operator Microsoft::UI::Xaml::FrameworkElement()const{return root;}operator Microsoft::UI::Xaml::UIElement()const{return root;}
};
}
