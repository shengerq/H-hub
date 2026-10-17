bool settingsThemeTestEnabled(){wchar_t value[8]{};GetEnvironmentVariableW(L"HHUB_SETTINGS_THEME_TEST",value,8);return value[0]==L'1';}
winrt::fire_and_forget runSettingsThemeCheck(){
 auto lifetime=get_strong();winrt::apartment_context ui;if(!uiMemoryTestProfile())co_return;timer.Stop();profileWorkerTimer.Stop();JsonObject report;JsonArray checks;bool passed=true;
 auto check=[&](bool ok,std::wstring const& name){JsonObject value;put(value,L"name",name);put(value,L"passed",ok);checks.Append(value);passed=passed&&ok;};
 try{
  current=L"settings";nav.IsPaneOpen(false);ShowWindow(hwnd,SW_RESTORE);
  for(int scenario=0;scenario<4;scenario++){
   bool compact=scenario%2,reduced=scenario>=2;JsonObject motion;put(motion,L"reduceMotion",reduced);invoke(L"settings",motion);
   auto dpi=GetDpiForWindow(hwnd)/96.0;SetWindowPos(hwnd,nullptr,12,12,(int)((compact?900:1240)*dpi),(int)((compact?640:900)*dpi),SWP_NOZORDER|SWP_NOACTIVATE);render();co_await winrt::resume_after(std::chrono::milliseconds(200));co_await ui;
   auto originalPage=page;auto generation=renderGeneration;ComboBox picker{nullptr};std::vector<FrameworkElement> controls;
   visitSettingsContent(page,[&](UIElement const& node){if(node.try_as<Button>()||node.try_as<ToggleSwitch>()||node.try_as<ComboBox>())controls.push_back(node.as<FrameworkElement>());if(auto choice=node.try_as<ComboBox>())picker=choice;});
   check(picker&&controls.size()>=10,L"Settings exposes the theme picker and existing setting controls");if(!picker)throw hresult_error(E_FAIL,L"Missing theme picker");
   auto unloads=std::make_shared<unsigned>(0);std::vector<winrt::event_token> tokens;for(auto const& control:controls)tokens.push_back(control.Unloaded([unloads](auto const&,auto const&){++*unloads;}));
   for(int index:{2,1,0,1,2,0}){
    auto offset=std::min(20.0,pageScroll.ScrollableHeight());pageScroll.ChangeView(nullptr,box_value(offset).as<Windows::Foundation::IReference<double>>(),nullptr,true);co_await winrt::resume_after(std::chrono::milliseconds(40));co_await ui;auto before=pageScroll.VerticalOffset();
    bool focused=picker.Focus(FocusState::Keyboard);picker.SelectedIndex(index);co_await winrt::resume_after(std::chrono::milliseconds(100));co_await ui;
    check(page==originalPage&&generation==renderGeneration,L"Theme selection keeps the same settings page");check(*unloads==0,L"Buttons and switches remain loaded throughout theme selection");
    std::vector<FrameworkElement> after;bool palettes=true;
    visitSettingsContent(page,[&](UIElement const& node){if(node.try_as<Button>()||node.try_as<ToggleSwitch>()||node.try_as<ComboBox>())after.push_back(node.as<FrameworkElement>());if(auto button=node.try_as<Button>();button&&unbox_value_or<hstring>(button.Tag(),{})==L"hhub-button"){auto color=button.Background().as<SolidColorBrush>().Color();palettes=palettes&&color.R==(dark?41:242)&&button.ActualTheme()==(dark?ElementTheme::Dark:ElementTheme::Light);}if(auto frame=node.try_as<Border>();frame&&unbox_value_or<hstring>(frame.Tag(),{})==L"hhub-setting-card")palettes=palettes&&frame.Background().as<SolidColorBrush>().Color().R==(dark?30:255);});
    bool same=after.size()==controls.size();for(size_t i=0;same&&i<controls.size();i++)same=winrt::get_unknown(after[i])==winrt::get_unknown(controls[i]);
    check(same&&palettes&&dark==preferredDarkTheme(),L"Existing controls receive the correct light/dark palette");check(std::abs(pageScroll.VerticalOffset()-before)<1,L"Theme selection preserves scroll position");check(!focused||Microsoft::UI::Xaml::Input::FocusManager::GetFocusedElement(shell.XamlRoot())==picker,L"Theme picker retains keyboard focus");
    if(index==1||index==2)co_await saveThemeCheckImage(page,dataDir()/(std::wstring(compact?L"settings-compact-":L"settings-wide-")+(index==1?L"dark.png":L"light.png")));
   }
   auto update=state.GetNamedObject(L"updates",JsonObject{});put(update,L"checking",!flag(update,L"checking"));state.SetNamedValue(L"updates",update);refreshUpdates();
   for(int wait=0;updatesBusy&&wait<50;wait++){co_await winrt::resume_after(std::chrono::milliseconds(20));co_await ui;}
   check(!updatesBusy&&page==originalPage&&generation==renderGeneration&&*unloads==0,L"Background update polling changes the status label without rebuilding controls");check(settingsUpdateStatus&&settingsUpdateStatus.Text()==settingsStatusText(),L"Update status text is refreshed in place");
   for(size_t i=0;i<controls.size();i++)controls[i].Unloaded(tokens[i]);
  }
 }catch(hresult_error const& error){passed=false;put(report,L"error",std::wstring(error.message()));}
 report.SetNamedValue(L"checks",checks);put(report,L"ok",passed);write(dataDir()/L"settings-theme-check.json",std::wstring(report.Stringify()));window.DispatcherQueue().TryEnqueue(Microsoft::UI::Dispatching::DispatcherQueuePriority::Low,[this]{window.Close();});
}
