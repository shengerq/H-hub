bool localizationTestEnabled(){wchar_t value[8]{};GetEnvironmentVariableW(L"HHUB_LOCALIZATION_TEST",value,8);return value[0]==L'1';}
winrt::fire_and_forget runLocalizationCheck(){
 auto lifetime=get_strong();winrt::apartment_context ui;if(!uiMemoryTestProfile())co_return;timer.Stop();profileWorkerTimer.Stop();JsonObject report;JsonArray checks,pages;bool passed=true;
 auto check=[&](bool value,std::wstring name){JsonObject entry;put(entry,L"passed",value);put(entry,L"name",name);checks.Append(entry);passed=passed&&value;};
 try{
  bool english=languageLocale()==L"en-US";check(languageStrings().size()>=594,L"External language pack is loaded");check(std::wstring(tr(L"设置"))==(english?L"Settings":L"设置"),L"Source labels use the selected language");check(translated(L"用户的自定义配置 123")==L"用户的自定义配置 123",L"Custom user content is retained");
  auto settings=state.GetNamedObject(L"settings");auto originalLanguage=str(settings,L"language",L"system");
  for(auto const& route:{L"home",L"system",L"devices",L"scenes",L"market",L"market-sources",L"plugins",L"settings"}){
   go(route);for(int variant=0;variant<2;variant++){
    auto dpi=GetDpiForWindow(hwnd)/96.0;SetWindowPos(hwnd,HWND_TOP,12,12,(int)((variant?900:1240)*dpi),(int)((variant?640:900)*dpi),SWP_SHOWWINDOW);window.Activate();co_await winrt::resume_after(std::chrono::milliseconds(220));co_await ui;page.UpdateLayout();
    JsonObject item;put(item,L"route",std::wstring(route));put(item,L"minimumSize",variant==1);put(item,L"scrollableHeight",pageScroll.ScrollableHeight());put(item,L"viewportHeight",pageScroll.ViewportHeight());JsonArray labels;std::function<void(DependencyObject)> visit=[&](DependencyObject node){if(auto t=node.try_as<TextBlock>();t&&!t.Text().empty())labels.Append(JsonValue::CreateStringValue(t.Text()));for(int i=0;i<VisualTreeHelper::GetChildrenCount(node);i++)visit(VisualTreeHelper::GetChild(node,i));};visit(page);item.SetNamedValue(L"labels",labels);pages.Append(item);
    check(page.ActualWidth()<=pageScroll.ViewportWidth()+1,L"Page fits horizontal viewport: "+std::wstring(route));
    if(std::wstring(route)==L"settings"){
     ComboBox picker{nullptr};visitSettingsContent(page,[&](UIElement const& node){if(auto c=node.try_as<ComboBox>();c&&Microsoft::UI::Xaml::Automation::AutomationProperties::GetName(c)==tr(L"界面语言"))picker=c;});
     check(picker&&picker.Items().Size()==3,L"Language picker exposes system, Chinese and English");check(str(state.GetNamedObject(L"settings"),L"language",L"system")==originalLanguage,L"Rendering does not change the language preference");
     check(pageScroll.ScrollableHeight()<1,L"Settings fits vertically with the language picker");
     if(variant==1&&GetForegroundWindow()==hwnd)saveReviewWindow(dataDir()/(languageLocale()+L"-settings.png"));
    }
   }
  }
 }catch(hresult_error const& e){put(report,L"error",std::wstring(e.message()));passed=false;}catch(...){passed=false;}
 put(report,L"ok",passed);put(report,L"locale",languageLocale());report.SetNamedValue(L"checks",checks);report.SetNamedValue(L"pages",pages);write(dataDir()/L"localization-check.json",std::wstring(report.Stringify()));Exit();
}
