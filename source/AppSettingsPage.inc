 TextBlock appSettingRow(StackPanel const& host,std::wstring const& name,UIElement const& control,std::wstring const& note=L""){
  Grid row;row.MinHeight(compactUi?34:40);row.ColumnSpacing(20);
  ColumnDefinition labelColumn;labelColumn.Width({1,GridUnitType::Star});row.ColumnDefinitions().Append(labelColumn);
  ColumnDefinition controlColumn;controlColumn.Width({0,GridUnitType::Auto});row.ColumnDefinitions().Append(controlColumn);
  auto labels=stack(false,4);labels.VerticalAlignment(VerticalAlignment::Center);
  auto label=title(name,16);label.TextWrapping(TextWrapping::NoWrap);label.TextTrimming(TextTrimming::CharacterEllipsis);labels.Children().Append(label);
  TextBlock noteLabel{nullptr};if(!note.empty()){noteLabel=text(note,12,true);labels.Children().Append(noteLabel);}row.Children().Append(labels);
  auto action=control.as<FrameworkElement>();action.HorizontalAlignment(HorizontalAlignment::Right);action.VerticalAlignment(VerticalAlignment::Center);
  Grid::SetColumn(action,1);row.Children().Append(action);auto frame=card(row,compactUi?2:6);frame.Tag(box_value(L"hhub-setting-card"));host.Children().Append(frame);return noteLabel;
 }
 void settingsPage(){
  auto settings=state.GetNamedObject(L"settings");auto rows=stack(false,compactUi?2:6);page.Children().Append(rows);
  auto locale=str(settings,L"language",L"system");auto languages=combo({tr(L"跟随系统"),tr(L"简体中文"),L"English"},locale==L"zh-CN"?1:locale==L"en-US"?2:0,180);
  Microsoft::UI::Xaml::Automation::AutomationProperties::SetName(languages,tr(L"界面语言"));
  onSelectionChanged(languages,[this,languages](auto const&,auto const&){if(languages.SelectedIndex()<0)return;std::vector<std::wstring> values={L"system",L"zh-CN",L"en-US"};auto chosen=values.at(languages.SelectedIndex());if(chosen==str(state.GetNamedObject(L"settings"),L"language",L"system"))return;try{JsonObject body;put(body,L"language",chosen);invoke(L"settings",body);notice(tr(L"语言已保存，重新打开界面后生效"));}catch(hresult_error const& e){notice(std::wstring(e.message()),true);}});
  appSettingRow(rows,tr(L"界面语言"),languages,tr(L"重新打开界面后生效；托盘菜单立即更新"));
  auto theme=str(settings,L"theme");auto themes=combo({tr(L"跟随系统"),tr(L"深色"),tr(L"浅色")},theme==L"dark"?1:theme==L"light"?2:0,180);
  Microsoft::UI::Xaml::Automation::AutomationProperties::SetName(themes,tr(L"外观主题"));
  onSelectionChanged(themes,[this,themes](auto const&,auto const&){
   if(themes.SelectedIndex()<0)return;JsonObject body;std::vector<std::wstring> values={L"system",L"dark",L"light"};auto chosen=values.at(themes.SelectedIndex());
   auto previous=str(state.GetNamedObject(L"settings"),L"theme");if(chosen==previous)return;put(body,L"theme",chosen);
   try{invoke(L"settings",body);refreshSystemTheme();}catch(hresult_error const& e){themes.SelectedIndex(previous==L"dark"?1:previous==L"light"?2:0);notice(std::wstring(e.message()),true);}
  });appSettingRow(rows,tr(L"外观主题"),themes);
  auto toggle=[&](wchar_t const* key,wchar_t const* label,bool fallback){
   ToggleSwitch control;control.MinWidth(0);control.Width(64);control.Height(36);control.Padding(margin(0));control.OnContent(box_value(L""));control.OffContent(box_value(L""));control.IsOn(flag(settings,key,fallback));
   Microsoft::UI::Xaml::Automation::AutomationProperties::SetName(control,label);std::wstring setting=key;
   onToggled(control,[this,control,setting](auto const&,auto const&){try{
    JsonObject body;put(body,setting.c_str(),control.IsOn());invoke(L"settings",body);
    if(setting==L"compact")nav.IsPaneOpen(!control.IsOn());
    if(setting==L"reduceMotion"){if(control.IsOn())page.ChildrenTransitions(Microsoft::UI::Xaml::Media::Animation::TransitionCollection{});syncPluginTheme();}
   }catch(hresult_error const& e){notice(std::wstring(e.message()),true);}});
   appSettingRow(rows,label,control);
  };
  toggle(L"reduceMotion",tr(L"减少动画"),false);toggle(L"compact",tr(L"默认折叠侧栏"),false);toggle(L"startup",tr(L"开机自启动到托盘"),false);
  toggle(L"appAutoDetect",tr(L"自动识别前台应用"),true);toggle(L"autoUpdate",tr(L"打开 H hub 时检查插件更新"),true);toggle(L"appAutoUpdate",tr(L"打开 H hub 时检查并自动更新应用"),true);
  settingsUpdateStatus=appSettingRow(rows,tr(L"应用与插件更新"),button(tr(L"检查更新"),[this]{invoke(L"checkUpdates");updateSettingsStatus();notice(tr(L"正在检查更新"));}),settingsStatusText());
  auto backgroundActions=stack(true,8);backgroundActions.Children().Append(button(tr(L"回到托盘"),[this]{window.Close();}));
  backgroundActions.Children().Append(button(tr(L"查看帮助"),[this]{helpDialog(tr(L"运行与更新"),tr(L"关闭窗口后核心留在托盘；右键托盘图标可完全退出。更新只在打开界面或点击检查更新时进行。插件更新需审阅权限并批准；应用自动更新会关闭界面并启动经过 SHA-256 校验的安装包。Windows 设置使用独立按需进程，读取或修改完成后退出。关闭 EQ 或手机遥控将立即停止对应服务，保留配置。"));}));
  appSettingRow(rows,tr(L"后台运行"),backgroundActions);
 }

 std::wstring settingsStatusText(){auto update=state.GetNamedObject(L"updates",JsonObject{});auto app=update.GetNamedObject(L"application",JsonObject{});auto status=flag(update,L"checking")?std::wstring(tr(L"正在检查更新")):L"H hub 1.1.0 · "+translated(str(app,L"status",tr(L"未检查")));if(!str(app,L"error").empty())status+=L"\n"+str(app,L"error");return status;}
 void updateSettingsStatus(){if(settingsUpdateStatus)settingsUpdateStatus.Text(settingsStatusText());}
 // Visit only the app-owned logical elements, leaving control templates intact.
 void visitSettingsContent(UIElement element,std::function<void(UIElement)> const& visit){
  visit(element);if(auto panel=element.try_as<Panel>()){for(auto const& child:panel.Children())visitSettingsContent(child,visit);}else if(auto border=element.try_as<Border>()){if(border.Child())visitSettingsContent(border.Child(),visit);}else if(auto control=element.try_as<ContentControl>()){if(auto child=control.Content().try_as<UIElement>())visitSettingsContent(child,visit);}
 }
 void applySettingsTheme(){
  auto theme=dark?ElementTheme::Dark:ElementTheme::Light;
  visitSettingsContent(page,[this,theme](UIElement const& element){
   if(auto frame=element.try_as<Border>();frame&&unbox_value_or<hstring>(frame.Tag(),{})==L"hhub-setting-card"){frame.Background(dark?brush(30,37,47):brush(255,255,255));frame.BorderBrush(dark?brush(48,59,73):brush(225,230,237));}
   if(auto button=element.try_as<Button>()){auto marker=unbox_value_or<hstring>(button.Tag(),{});if(marker==L"hhub-button"){button.RequestedTheme(theme);button.Background(dark?brush(41,49,61):brush(242,244,248));button.Foreground(dark?brush(245,245,245):brush(27,27,27));}else if(marker==L"hhub-accent"){button.RequestedTheme(theme);styleAccentButton(button);}}
   if(auto label=element.try_as<TextBlock>();label&&unbox_value_or<hstring>(label.Tag(),{})==L"hhub-muted")label.Foreground(dark?brush(164,178,195):brush(95,105,120));
  });
 }
