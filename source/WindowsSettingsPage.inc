 bool windowsUninstalling=false;
 bool windowsPluginReady(){auto p=pluginRecord(state,L"windows");return p&&flag(p,L"installed")&&str(p,L"mode")!=L"disabled"&&!windowsUninstalling;}
 void resetWindowsPluginUi(){++windowsMutationGeneration;windowsLoaded=false;windowsRefreshQueued=false;windowsChanges.clear();windowsPending.clear();windowsExpired.clear();windowsIssues.clear();windowsBindings.clear();windowsSignatures.clear();windowsOptionsHost=nullptr;if(state.HasKey(L"windowsSettings"))state.Remove(L"windowsSettings");}
 bool windowsWriteBusy=false,windowsReadBusy=false,windowsLoaded=false,windowsApplying=false;
 uint64_t windowsMutationGeneration=0;std::wstring windowsActiveWrite;
 struct WindowsChange{std::wstring id,operation;bool enabled;uint64_t ticket,started;};
 std::deque<WindowsChange> windowsChanges;
 std::map<std::wstring,uint64_t> windowsPending;
 std::map<std::wstring,bool> windowsExpired;
 static constexpr uint64_t windowsFeedbackGraceMs=1000,windowsMinimumLoadingMs=250;
 double windowsLockedOffset=0;
 StackPanel windowsOptionsHost{nullptr};
 std::map<std::wstring,std::function<void(JsonObject)>> windowsBindings;
 std::map<std::wstring,std::wstring> windowsSignatures;
 void lockWindowsScroll(bool locked){
  if(current!=L"windows"||!pageScroll)return;
  if(locked)windowsLockedOffset=pageScroll.VerticalOffset();
  pageScroll.VerticalScrollMode(locked?ScrollMode::Disabled:ScrollMode::Enabled);
  pageScroll.ChangeView(nullptr,box_value(windowsLockedOffset).as<Windows::Foundation::IReference<double>>(),nullptr,true);
 }
 JsonObject windowsActual(std::wstring const& id){
  if(windowsLoaded&&!windowsIssues.count(L"page"))for(auto const& v:state.GetNamedObject(L"windowsSettings",JsonObject{}).GetNamedArray(L"options",JsonArray{}))
   if(str(v.GetObject(),L"id")==id)return v.GetObject();
  for(auto const& v:windowsOptionCatalog())if(str(v.GetObject(),L"id")==id){auto item=v.GetObject();put(item,L"loading",!windowsLoaded);return item;}
  return JsonObject{};
 }
 void updateWindowsControls(std::wstring const& only=L""){
  if(current!=L"windows")return;
  windowsApplying=true;
  for(auto const& [id,update]:windowsBindings)if(only.empty()||only==id){
   auto actual=windowsActual(id);auto signature=std::wstring(actual.Stringify())+(windowsPending.count(id)?L"pending":L"")+(windowsExpired.count(id)?L"expired":L"")+(windowsIssues.count(id)?windowsIssues.at(id):L"")+(windowsIssues.count(L"page")?windowsIssues.at(L"page"):L"");
   if(windowsSignatures[id]!=signature){update(actual);windowsSignatures[id]=signature;}
  }
  windowsApplying=false;
 }
 winrt::fire_and_forget windowsFeedbackDeadline(std::wstring id,uint64_t ticket){
  auto lifetime=get_strong();winrt::apartment_context ui;
  co_await winrt::resume_after(std::chrono::milliseconds(windowsFeedbackGraceMs));co_await ui;
  auto pending=windowsPending.find(id);if(pending!=windowsPending.end()&&pending->second==ticket){windowsExpired[id]=true;updateWindowsControls(id);}
 }
 winrt::fire_and_forget changeWindowsPreference(std::wstring id,bool enabled,std::wstring operation=L"set"){
  if(!windowsPluginReady()||windowsPending.count(id))co_return;
  auto ticket=++windowsMutationGeneration;windowsPending[id]=ticket;windowsExpired.erase(id);
  windowsChanges.push_back({id,operation,enabled,ticket,GetTickCount64()});updateWindowsControls(id);windowsFeedbackDeadline(id,ticket);
  if(windowsWriteBusy)co_return;
  windowsWriteBusy=true;lockWindowsScroll(true);processWindowsChanges();co_return;
 }
 winrt::fire_and_forget processWindowsChanges(){
  auto lifetime=get_strong();winrt::apartment_context ui;
  while(!windowsChanges.empty()&&windowsPluginReady()){
  auto request=windowsChanges.front();windowsChanges.pop_front();auto id=request.id;auto operation=request.operation;auto enabled=request.enabled;windowsActiveWrite=id;
  JsonObject actual{nullptr};std::wstring error;
  try{
   co_await resume_background();JsonObject body;put(body,L"id",id);put(body,L"enabled",enabled);put(body,L"single",true);
   actual=WindowsWorker::run(operation,body,id==L"gpuScheduling");
   if(str(actual,L"id")!=id)throw hresult_error(E_FAIL,tr(L"回传的设置不匹配"));
   if(operation==L"set"&&(!flag(actual,L"confirmed")||actual.GetNamedValue(L"enabled",JsonValue::CreateNullValue()).ValueType()!=JsonValueType::Boolean||flag(actual,L"enabled")!=enabled))
    throw hresult_error(E_FAIL,tr(L"开关状态未通过回读验证"));
  }catch(hresult_error const& e){error=std::wstring(e.message());try{JsonObject body;put(body,L"id",id);actual=WindowsWorker::run(L"read",body);}catch(...) {actual=nullptr;}}
  catch(...){error=tr(L"未收到有效的设置回传");}
  co_await ui;
  auto elapsed=GetTickCount64()-request.started;
  bool confirmed=error.empty()&&actual&&flag(actual,L"confirmed")&&!flag(actual,L"restartPending");
  auto minimum=confirmed?windowsMinimumLoadingMs:windowsFeedbackGraceMs;
  if(elapsed<minimum){co_await winrt::resume_after(std::chrono::milliseconds(minimum-elapsed));co_await ui;}
  if(actual){auto data=state.GetNamedObject(L"windowsSettings",JsonObject{});auto options=data.GetNamedArray(L"options",JsonArray{});bool found=false;
   for(uint32_t i=0;i<options.Size();++i)if(str(options.GetObjectAt(i),L"id")==id){options.SetAt(i,actual);found=true;break;}
   if(!found)options.Append(actual);data.SetNamedValue(L"options",options);state.SetNamedValue(L"windowsSettings",data);
  }
  if(error.empty())windowsIssues.erase(id);else windowsIssues[id]=error;
  windowsPending.erase(id);windowsExpired.erase(id);updateWindowsControls(id);
  }
  windowsWriteBusy=false;windowsActiveWrite.clear();lockWindowsScroll(false);
  if(windowsRefreshQueued&&current==L"windows")refreshWindows();
 }
 void windowsOption(JsonObject metadata,StackPanel host){
  auto id=str(metadata,L"id");Grid row;row.Height(compactUi?40:44);row.ColumnSpacing(20);
  ColumnDefinition left;left.Width({1,GridUnitType::Star});row.ColumnDefinitions().Append(left);
  ColumnDefinition right;right.Width({112,GridUnitType::Pixel});row.ColumnDefinitions().Append(right);
  auto label=title(translated(str(metadata,L"name")),16);label.VerticalAlignment(VerticalAlignment::Center);label.TextWrapping(TextWrapping::NoWrap);label.TextTrimming(TextTrimming::CharacterEllipsis);row.Children().Append(label);
  Grid control;control.Width(112);control.Height(40);control.HorizontalAlignment(HorizontalAlignment::Right);control.VerticalAlignment(VerticalAlignment::Center);Grid::SetColumn(control,1);row.Children().Append(control);
  ToggleSwitch toggle;toggle.MinWidth(0);toggle.Width(64);toggle.Height(36);toggle.Padding(margin(0));toggle.OnContent(box_value(L""));toggle.OffContent(box_value(L""));toggle.HorizontalAlignment(HorizontalAlignment::Right);toggle.VerticalAlignment(VerticalAlignment::Center);
  Microsoft::UI::Xaml::Automation::AutomationProperties::SetName(toggle,translated(str(metadata,L"name")));control.Children().Append(toggle);
  ProgressRing loading;loading.Width(20);loading.Height(20);loading.Margin(Thickness{0,0,76,0});loading.HorizontalAlignment(HorizontalAlignment::Right);loading.VerticalAlignment(VerticalAlignment::Center);control.Children().Append(loading);
  auto frame=card(row,compactUi?5:10);frame.BorderThickness(margin(2));
  auto update=[this,id,toggle,loading,label,frame](JsonObject actual){
   bool pending=flag(actual,L"loading"),saving=windowsPending.count(id)!=0;
   auto value=actual.GetNamedValue(L"enabled",JsonValue::CreateNullValue());bool known=flag(actual,L"confirmed")&&value.ValueType()==JsonValueType::Boolean;
   if(!saving)toggle.IsOn(known&&value.GetBoolean());
   toggle.IsEnabled(!pending&&!saving&&flag(actual,L"available"));
   bool busy=pending||saving;toggle.Visibility(Visibility::Visible);loading.IsActive(busy);loading.Visibility(busy?Visibility::Visible:Visibility::Collapsed);
   std::wstring message=pending?tr(L"正在读取开关状态"):str(actual,L"status",tr(L"未收到可靠的状态回传"));
   if(saving)message=tr(L"正在等待 Windows 回读确认");
   bool issue=windowsIssues.count(id)||windowsIssues.count(L"page");
   if(windowsIssues.count(id))message=windowsIssues.at(id);else if(windowsIssues.count(L"page"))message=windowsIssues.at(L"page");
   bool expired=windowsExpired.count(id)!=0;
   if(expired)message=tr(L"暂未收到 Windows 回传，仍在等待确认");
   // Keep the existing border during the grace period; only this row can time out.
   if(!saving||expired){bool failed=expired||(!pending&&(!known||issue||flag(actual,L"restartPending")));
    frame.BorderBrush(failed?brush(196,43,28):(dark?brush(48,59,73):brush(225,230,237)));}
   ToolTipService::SetToolTip(frame,box_value(message));ToolTipService::SetToolTip(label,box_value(str(actual,L"note",message.c_str())));
   Microsoft::UI::Xaml::Automation::AutomationProperties::SetHelpText(toggle,message);
  };
  windowsBindings[id]=update;onToggled(toggle,[this,id,toggle](auto const&,auto const&){if(!windowsApplying)changeWindowsPreference(id,toggle.IsOn());});host.Children().Append(frame);
  onLoaded(control,[this,id,row,control,toggle](auto const&,auto const&){auto pos=control.TransformToVisual(row).TransformPoint(Windows::Foundation::Point{0,0});stage(("Windows row="+to_string(id)+" width="+std::to_string(row.ActualWidth())+" controlX="+std::to_string(pos.X)+" toggleWidth="+std::to_string(toggle.ActualWidth())).c_str());});
 }
 void windowsPage(){if(!windowsPluginReady()){page.Children().Append(text(tr(L"Windows 设置插件已禁用，请在插件管理中启用。"),14,true));return;}
  windowsBindings.clear();windowsSignatures.clear();windowsOptionsHost=stack(false,compactUi?5:8);page.Children().Append(windowsOptionsHost);
  auto options=windowsOptionCatalog();for(uint32_t i=0;i<options.Size();i+=2){Grid pair;pair.ColumnSpacing(8);pair.RowSpacing(compactUi?5:8);for(int j=0;j<2;j++){ColumnDefinition column;column.Width({1,GridUnitType::Star});pair.ColumnDefinitions().Append(column);RowDefinition row;row.Height({0,GridUnitType::Auto});pair.RowDefinitions().Append(row);}auto left=stack(false,0),right=stack(false,0);pair.Children().Append(left);pair.Children().Append(right);windowsOption(options.GetObjectAt(i),left);if(i+1<options.Size())windowsOption(options.GetObjectAt(i+1),right);onSizeChanged(pair,[this,right](auto const&,SizeChangedEventArgs const& e){bool columns=e.NewSize().Width>=(compactUi?760:960);Grid::SetColumn(right,columns?1:0);Grid::SetRow(right,columns?0:1);Grid::SetColumnSpan(right,columns?1:2);});Grid::SetColumnSpan(left,2);onSizeChanged(pair,[this,left](auto const&,SizeChangedEventArgs const& e){Grid::SetColumnSpan(left,e.NewSize().Width>=(compactUi?760:960)?1:2);});windowsOptionsHost.Children().Append(pair);}
  if(windowsWriteBusy)pageScroll.VerticalScrollMode(ScrollMode::Disabled);
  auto bringToken=pageScroll.BringIntoViewRequested([this](auto const&,Microsoft::UI::Xaml::BringIntoViewRequestedEventArgs const& e){if(current==L"windows"&&windowsWriteBusy)e.Handled(true);});
  pageEventCleanup.push_back([weak=make_weak(pageScroll),bringToken]{if(auto scroll=weak.get())scroll.BringIntoViewRequested(bringToken);});
  // Fixed row geometry and in-place state updates prevent focus and layout jumps.
  onViewChanged(pageScroll,[this](auto const&,auto const&){if(current==L"windows"&&windowsWriteBusy&&std::abs(pageScroll.VerticalOffset()-windowsLockedOffset)>0.5)
   pageScroll.ChangeView(nullptr,box_value(windowsLockedOffset).as<Windows::Foundation::IReference<double>>(),nullptr,true);});
  updateWindowsControls();
 }

