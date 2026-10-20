// Device identity, battery and visibility come from the original reader/plugins.
JsonObject homeDevice(std::wstring const& id){for(auto const& item:visibleDevices()){auto device=item.GetObject();if(str(device,L"id")==id)return device;}return JsonObject{};}
std::wstring homeBattery(JsonObject device){
 auto value=device.GetNamedValue(L"battery",JsonValue::CreateNullValue());
 if(value.ValueType()==JsonValueType::Number)return tr(L"电量 ")+digits(value.GetNumber())+L"%"+(flag(device,L"lowBattery")?tr(L" · 低电量"):L"");
 return device.HasKey(L"batteryLevel")?tr(L"电量等级 ")+str(device,L"batteryLevel"):tr(L"电量不可用");
}
std::wstring homeDeviceKind(JsonObject device){auto kind=str(device,L"kind");return kind==L"mouse"?tr(L"鼠标 / 鼠标接收器"):kind==L"keyboard"?tr(L"键盘 / 复合输入设备"):kind==L"gamepad"?tr(L"手柄接口"):kind==L"audio"?tr(L"音频端点"):tr(L"Windows 外设");}
void nativeDeviceMonitoring(){
 auto heading=pageTitle(tr(L"我的设备"));if(shortUi)heading.FontSize(20);
 auto count=text(std::to_wstring(visibleDevices().Size())+tr(L" 个外设"),12,true);count.VerticalAlignment(VerticalAlignment::Center);
 auto refresh=button(tr(L"刷新设备"),[this]{refreshDeviceScan();});
 auto busy=[this,refresh,count]{refresh.IsEnabled(!monitorBusy&&!homeSelectionBusy);count.Text(std::to_wstring(visibleDevices().Size())+tr(L" 个外设"));};busy();deviceBindings.push_back(busy);
 auto actions=stack(true,16);actions.Children().Append(count);actions.Children().Append(buttonBar({button(tr(L"设备概览"),[this]{deviceOverviewDialog();}),button(tr(L"全部接口"),[this]{go(L"devices");}),refresh}));page.Children().Append(actionHeader(heading,actions));
 auto devices=visibleDevices();
 if(shortUi){
  auto selected=selectedDetected();auto device=homeDevice(selected);auto labels=stack(false,3);
  auto label=text(devices.Size()?deviceDisplayName(device)+L" · "+homeBattery(device):tr(L"未发现连接的外设，可在“全部接口”中查看。"),13,true);label.TextWrapping(TextWrapping::NoWrap);label.TextTrimming(TextTrimming::CharacterEllipsis);labels.Children().Append(label);
  auto scene=text(L"",12,true);scene.TextWrapping(TextWrapping::NoWrap);scene.TextTrimming(TextTrimming::CharacterEllipsis);labels.Children().Append(scene);
  auto patch=[this,scene,label]{auto devices=visibleDevices();auto device=homeDevice(selectedDetected());label.Text(devices.Size()?deviceDisplayName(device)+L" · "+homeBattery(device):tr(L"未发现连接的外设，可在“全部接口”中查看。"));auto selected=find(state.GetNamedArray(L"scenes"),str(state,L"currentScene"));scene.Text(tr(L"当前场景：")+str(selected,L"name")+(flag(state,L"sceneLocked")?tr(L" · 手动锁定"):tr(L" · 自动规则")));};patch();deviceBindings.push_back(patch);
  if(devices.Size())page.Children().Append(card(actionHeader(labels,button(tr(L"设备详情"),[this]{auto id=selectedDetected();if(!id.empty())openDevicePage(find(visibleDevices(),id));})),8));else page.Children().Append(card(labels,8));return;
 }
 Grid workspace;ColumnDefinition detail;detail.Width({64,GridUnitType::Star});ColumnDefinition list;list.Width({36,GridUnitType::Star});workspace.ColumnDefinitions().Append(detail);workspace.ColumnDefinitions().Append(list);
 for(int i=0;i<2;i++){RowDefinition row;row.Height({0,GridUnitType::Auto});workspace.RowDefinitions().Append(row);}
 Grid heroBody;heroBody.MinHeight(140);heroBody.RowSpacing(8);
 for(int i=0;i<3;i++){RowDefinition row;row.Height(i==1?GridLength{1,GridUnitType::Star}:GridLength{0,GridUnitType::Auto});heroBody.RowDefinitions().Append(row);}
 auto info=stack(false,8);auto kind=text(L"",12,true);auto typeRow=stack(true,12);typeRow.Children().Append(text(tr(L"当前设备"),12,true));typeRow.Children().Append(kind);info.Children().Append(typeRow);
 auto name=title(L"",24);auto details=button(tr(L"设备详情"),[this]{auto id=selectedDetected();if(!id.empty())openDevicePage(find(visibleDevices(),id));});info.Children().Append(actionHeader(name,details));
 Grid connection;connection.ColumnSpacing(8);for(int i=0;i<3;i++){ColumnDefinition column;column.Width(i==1?GridLength{1,GridUnitType::Star}:GridLength{0,GridUnitType::Auto});connection.ColumnDefinitions().Append(column);}
 Border dot;dot.Width(8);dot.Height(8);dot.CornerRadius(CornerRadius{4,4,4,4});dot.VerticalAlignment(VerticalAlignment::Center);connection.Children().Append(dot);
 auto status=text(L"",13);status.TextWrapping(TextWrapping::NoWrap);status.TextTrimming(TextTrimming::CharacterEllipsis);Grid::SetColumn(status,1);connection.Children().Append(status);
 auto battery=text(L"",12,true);Grid::SetColumn(battery,2);connection.Children().Append(battery);info.Children().Append(connection);
 auto profile=text(L"",12,true);info.Children().Append(profile);if(!devices.Size())info.Visibility(Visibility::Collapsed);heroBody.Children().Append(info);
 Grid artHost;if(!devices.Size()){auto empty=text(tr(L"未发现连接的外设。点击“全部接口”查看所有接口。"),14,true);empty.VerticalAlignment(VerticalAlignment::Center);artHost.Children().Append(empty);}Grid::SetRow(artHost,1);heroBody.Children().Append(artHost);auto artNote=text(L"",10,true);artNote.HorizontalAlignment(HorizontalAlignment::Center);Grid::SetRow(artNote,2);heroBody.Children().Append(artNote);
 auto previousArt=std::make_shared<std::wstring>();
 auto heroPatch=[this,name,kind,status,battery,profile,dot,details,artHost,artNote,previousArt]{
  auto id=selectedDetected();auto device=homeDevice(id);name.Text(deviceDisplayName(device));kind.Text(homeDeviceKind(device));status.Text(translated(str(device,L"status")));battery.Text(homeBattery(device));
  ToolTipService::SetToolTip(battery,box_value(str(device,L"batterySource")));profile.Text(str(device,L"connection")+L" · "+str(device,L"profile"));dot.Background(flag(device,L"connected")?brush(15,123,15):brush(120,120,120));details.IsEnabled(!homeSelectionBusy&&!id.empty());
  if(*previousArt!=id){*previousArt=id;artHost.Children().Clear();Image art;art.Stretch(Stretch::Uniform);art.HorizontalAlignment(HorizontalAlignment::Stretch);art.VerticalAlignment(VerticalAlignment::Stretch);Microsoft::UI::Xaml::Automation::AutomationProperties::SetName(art,tr(L"当前设备外观：")+deviceDisplayName(device));artHost.Children().Append(art);loadDeviceArt(art,device);artNote.Text(matchedDevicePlugin(device)?tr(L"插件提供的产品外观"):tr(L"外观示意 · 非型号照片"));}
 };heroPatch();deviceBindings.push_back(heroPatch);auto hero=card(heroBody,20);hero.MinHeight(180);workspace.Children().Append(hero);
 Grid shelf;shelf.RowSpacing(8);RowDefinition items;items.Height({1,GridUnitType::Star});shelf.RowDefinitions().Append(items);RowDefinition footer;footer.Height({0,GridUnitType::Auto});shelf.RowDefinitions().Append(footer);
 auto tiles=stack(false,6);ScrollViewer scroll;scroll.VerticalScrollBarVisibility(ScrollBarVisibility::Auto);scroll.HorizontalScrollBarVisibility(ScrollBarVisibility::Disabled);scroll.HorizontalScrollMode(ScrollMode::Disabled);scroll.Content(tiles);shelf.Children().Append(scroll);
 for(auto const& item:devices){
  auto device=item.GetObject();auto id=str(device,L"id");Grid tileBody;tileBody.ColumnSpacing(8);ColumnDefinition picture;picture.Width({48,GridUnitType::Pixel});tileBody.ColumnDefinitions().Append(picture);ColumnDefinition labelsColumn;labelsColumn.Width({1,GridUnitType::Star});tileBody.ColumnDefinitions().Append(labelsColumn);
  Image thumbnail;thumbnail.Width(46);thumbnail.Height(46);thumbnail.Stretch(Stretch::Uniform);tileBody.Children().Append(thumbnail);loadDeviceArt(thumbnail,device);
  auto labels=stack(false,4);auto label=text(deviceDisplayName(device),12);label.MaxLines(2);label.TextTrimming(TextTrimming::CharacterEllipsis);auto tileStatus=text(L"",10,true);tileStatus.TextWrapping(TextWrapping::NoWrap);tileStatus.TextTrimming(TextTrimming::CharacterEllipsis);labels.Children().Append(label);labels.Children().Append(tileStatus);Grid::SetColumn(labels,1);tileBody.Children().Append(labels);
  Button tile;tile.Content(tileBody);tile.Padding(margin(8));tile.HorizontalAlignment(HorizontalAlignment::Stretch);tile.HorizontalContentAlignment(HorizontalAlignment::Stretch);tile.MinHeight(64);
  Microsoft::UI::Xaml::Automation::AutomationProperties::SetName(tile,tr(L"选择设备：")+deviceDisplayName(device));onClick(tile,[this,id](auto const&,auto const&){selectHomeDevice(id);});
  auto patch=[this,tile,tileStatus,id]{auto device=homeDevice(id);bool selected=id==selectedDetected();tileStatus.Text(device.HasKey(L"battery")?homeBattery(device):str(device,L"status",tr(L"状态不可用")));tile.IsEnabled(!homeSelectionBusy);tile.Background(selected?(dark?brush(35,57,80):brush(235,245,255)):(dark?brush(30,37,47):brush(255,255,255)));tile.BorderBrush(selected?accent():(dark?brush(48,59,73):brush(225,230,237)));tile.BorderThickness(margin(selected?2:1));};patch();deviceBindings.push_back(patch);tiles.Children().Append(tile);
 }
 auto scene=text(L"",12,true);scene.TextWrapping(TextWrapping::NoWrap);scene.TextTrimming(TextTrimming::CharacterEllipsis);Grid::SetRow(scene,1);shelf.Children().Append(scene);
 auto scenePatch=[this,scene]{std::wstring value=tr(L"当前场景：暂无场景");try{auto selected=find(state.GetNamedArray(L"scenes"),str(state,L"currentScene"));value=tr(L"当前场景：")+str(selected,L"name")+(flag(state,L"sceneLocked")?tr(L" · 手动锁定"):tr(L" · 自动规则"));}catch(...){}scene.Text(value);ToolTipService::SetToolTip(scene,box_value(value));};scenePatch();deviceBindings.push_back(scenePatch);
 Grid::SetColumn(shelf,1);shelf.Margin(Thickness{16,0,0,0});workspace.Children().Append(shelf);
 onSizeChanged(workspace,[this,workspace,shelf,hero](auto const&,SizeChangedEventArgs const& e){bool narrow=e.NewSize().Width<720;workspace.RowDefinitions().GetAt(0).Height(narrow?GridLength{0,GridUnitType::Auto}:GridLength{1,GridUnitType::Star});shelf.Height(narrow?260.0:std::nan(""));hero.MinHeight(narrow?240.0:180.0);if(narrow){deviceArea=nullptr;workspace.Height(std::nan(""));}else{deviceArea=workspace;layoutDeviceArea();}workspace.ColumnDefinitions().GetAt(1).Width(narrow?GridLength{0,GridUnitType::Pixel}:GridLength{36,GridUnitType::Star});Grid::SetRow(shelf,narrow?1:0);Grid::SetColumn(shelf,narrow?0:1);shelf.Margin(narrow?Thickness{0,12,0,0}:Thickness{16,0,0,0});});page.Children().Append(workspace);
}
