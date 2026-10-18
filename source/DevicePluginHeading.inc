TextBlock devicePluginStatus{nullptr};std::wstring devicePluginConnectionHint;ULONGLONG devicePluginSnapshotTick{};
void clearDevicePluginHeading(){devicePluginStatus=nullptr;devicePluginConnectionHint.clear();}
std::wstring devicePluginStatusText(){
 JsonObject selected{nullptr};unsigned matches=0;
 for(auto const& value:state.GetNamedObject(L"detected",JsonObject{}).GetNamedArray(L"peripherals",JsonArray{})){
  auto device=value.GetObject();auto plugin=matchedDevicePlugin(device);if(!plugin||str(plugin,L"id")!=devicePluginId)continue;
  if(device.HasKey(L"connected")&&!flag(device,L"connected"))continue;
  ++matches;if(str(device,L"id")==selectedDetected()){selected=device;matches=1;break;}selected=device;
 }
 if(!selected)return tr(L"未连接");if(matches>1)return tr(L"多个设备");
 auto connection=str(selected,L"connection");
 if((connection==L"USB / HID"||connection==L"USB")&&!devicePluginConnectionHint.empty())connection=devicePluginConnectionHint;
 if(connection.empty())connection=tr(L"连接方式未提供");else connection=translated(connection);
 auto battery=selected.GetNamedValue(L"battery",JsonValue::CreateNullValue());
 if(battery.ValueType()==JsonValueType::Number){auto value=battery.GetNumber();auto age=num(selected,L"batteryAgeSeconds",0)+(GetTickCount64()-devicePluginSnapshotTick)/1000.0;if(std::isfinite(value)&&value>=0&&value<=100&&age>=0&&age<=60)return digits(value)+L"% · "+connection;}
 return connection;
}
void updateDevicePluginHeading(){if(!devicePluginStatus)return;auto label=devicePluginStatusText();if(devicePluginStatus.Text()!=label)devicePluginStatus.Text(label);}
void devicePluginHeading(std::wstring const& heading){
 devicePluginSnapshotTick=GetTickCount64();
 Grid header;header.Name(L"DevicePluginHeading");header.MinHeight(40);header.ColumnSpacing(20);
 ColumnDefinition left;left.Width({1,GridUnitType::Star});ColumnDefinition right;right.Width({0,GridUnitType::Auto});header.ColumnDefinitions().Append(left);header.ColumnDefinitions().Append(right);
 Grid identity;identity.HorizontalAlignment(HorizontalAlignment::Left);identity.ColumnSpacing(12);identity.VerticalAlignment(VerticalAlignment::Center);
 for(int i=0;i<3;i++){ColumnDefinition column;column.Width(i==1?GridLength{1,GridUnitType::Star}:GridLength{0,GridUnitType::Auto});identity.ColumnDefinitions().Append(column);}
 Button back;FontIcon arrow;arrow.Glyph(L"\uE72B");back.Content(arrow);back.Width(40);back.Height(40);back.Padding(margin(0));back.VerticalAlignment(VerticalAlignment::Center);ToolTipService::SetToolTip(back,box_value(tr(L"返回首页")));Microsoft::UI::Xaml::Automation::AutomationProperties::SetName(back,tr(L"返回首页"));onClick(back,[this](auto const&,auto const&){go(L"home");});identity.Children().Append(back);
 auto name=pageTitle(heading);name.TextWrapping(TextWrapping::NoWrap);name.TextTrimming(TextTrimming::CharacterEllipsis);name.VerticalAlignment(VerticalAlignment::Center);Grid::SetColumn(name,1);identity.Children().Append(name);
 devicePluginStatus=text(L"",13,true);devicePluginStatus.Name(L"DevicePluginStatus");devicePluginStatus.VerticalAlignment(VerticalAlignment::Center);Grid::SetColumn(devicePluginStatus,2);identity.Children().Append(devicePluginStatus);updateDevicePluginHeading();header.Children().Append(identity);
 auto actions=stack(true,8);actions.Name(L"DevicePluginProfiles");actions.HorizontalAlignment(HorizontalAlignment::Right);actions.VerticalAlignment(VerticalAlignment::Center);appendCompactPluginProfiles(actions);Grid::SetColumn(actions,1);header.Children().Append(actions);page.Children().Append(header);
}
std::wstring devicePluginHeaderBootstrap(){return LR"HEADER(
<style>header>.hint,#device-battery{display:none!important}</style>
<script>window.addEventListener('DOMContentLoaded',()=>{
 const header=document.querySelector('main>header');if(!header)return;
 const declared=header.querySelector('h1 span')?.textContent?.trim()||'';
 const labels={'2.4 GHz':'2.4 GHz','2.4GHz':'2.4 GHz','2.4G':'2.4 GHz','Bluetooth':'Bluetooth','蓝牙':'Bluetooth','USB':'USB','有线':'USB'};
 if(labels[declared])chrome.webview.postMessage({type:'hhub.device.connection',connection:labels[declared]});
 header.querySelectorAll(':scope>.hint').forEach(node=>node.remove());
 if(!header.querySelector('button,input,select,a'))header.style.setProperty('display','none','important');
});</script>
)HEADER";}
