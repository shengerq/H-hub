struct SensorColumnRows {
 std::wstring signature;
 std::map<uintptr_t,std::pair<uint32_t,TextBlock>> values;
 std::vector<JsonObject> sensors;
 bool stale=false;
 std::wstring emptyMessage;
};
std::wstring rawSensorUnit(std::wstring const& type){
 return type==L"Temperature"?L" °C":type==L"Fan"?L" RPM":type==L"Power"?L" W":type==L"Clock"?L" MHz":type==L"Load"||type==L"Control"||type==L"Level"?L"%":type==L"Voltage"?L" V":type==L"SmallData"?L" MB":type==L"Data"?L" GB":type==L"Throughput"?L" B/s":L"";
}
Border sensorColumn(bool gpu){
 auto body=stack(false,8);body.Children().Append(title(gpu?L"GPU":L"CPU",20));
 body.Children().Append(liveText([gpu](JsonObject sample){return str(sample,gpu?L"gpuName":L"cpuName",tr(L"等待硬件信息"));},12));
 auto trend=homeTrend(gpu?L"gpu":L"cpu");trend.Height(48);body.Children().Append(trend);
 // WinUI's expanded overlay scrollbar must not cover the right-aligned readings.
 ListView rows;rows.Height(240);rows.SelectionMode(ListViewSelectionMode::None);rows.IsItemClickEnabled(false);rows.Padding(Thickness{0,0,24,0});ScrollViewer::SetHorizontalScrollMode(rows,ScrollMode::Disabled);ScrollViewer::SetHorizontalScrollBarVisibility(rows,ScrollBarVisibility::Disabled);body.Children().Append(rows);
 auto cache=std::make_shared<SensorColumnRows>();
 sensorLists.erase(std::remove_if(sensorLists.begin(),sensorLists.end(),[](auto const& weak){return !weak.get();}),sensorLists.end());sensorLists.push_back(make_weak(rows));
 auto containers=rows.ContainerContentChanging([this,cache](ListViewBase const&,ContainerContentChangingEventArgs const& args){
  auto index=unbox_value_or<uint32_t>(args.Item(),UINT32_MAX);auto key=reinterpret_cast<uintptr_t>(winrt::get_unknown(args.ItemContainer()));cache->values.erase(key);
  if(args.InRecycleQueue())return;
  auto container=args.ItemContainer();container.HorizontalContentAlignment(HorizontalAlignment::Stretch);container.Padding(Thickness{0,0,0,0});container.MinHeight(28);
  if(index>=cache->sensors.size()){container.Content(text(cache->emptyMessage,12,true));args.Handled(true);return;}
  auto sensor=cache->sensors[index];Grid row;row.MinHeight(24);row.ColumnSpacing(12);ColumnDefinition labelColumn;labelColumn.Width({1,GridUnitType::Star});row.ColumnDefinitions().Append(labelColumn);ColumnDefinition valueColumn;valueColumn.Width({0,GridUnitType::Auto});row.ColumnDefinitions().Append(valueColumn);
  auto label=text(str(sensor,L"name"),12,true);label.TextWrapping(TextWrapping::NoWrap);label.TextTrimming(TextTrimming::CharacterEllipsis);label.VerticalAlignment(VerticalAlignment::Center);ToolTipService::SetToolTip(label,box_value(str(sensor,L"hardware")+L" · "+str(sensor,L"type")));row.Children().Append(label);
  auto reading=text(L"",13);reading.TextWrapping(TextWrapping::NoWrap);reading.VerticalAlignment(VerticalAlignment::Center);reading.HorizontalAlignment(HorizontalAlignment::Right);Grid::SetColumn(reading,1);row.Children().Append(reading);
  auto value=sensor.GetNamedValue(L"value",JsonValue::CreateNullValue());bool valid=value.ValueType()==JsonValueType::Number&&std::isfinite(value.GetNumber());reading.Text(cache->stale?tr(L"等待更新"):valid?digits(value.GetNumber(),1)+rawSensorUnit(str(sensor,L"type")):tr(L"未提供"));cache->values[key]={index,reading};container.Content(row);args.Handled(true);
 });
 pageEventCleanup.push_back([weak=make_weak(rows),containers,cache]{if(auto list=weak.get())list.ContainerContentChanging(containers);cache->values.clear();cache->sensors.clear();});
 auto update=[this,rows,cache,gpu](JsonObject sample){
  std::vector<JsonObject> sensors;std::wstring signature;
  for(auto const& value:sample.GetNamedArray(L"sensors",JsonArray{})){
   auto sensor=value.GetObject();auto category=str(sensor,L"category");if(gpu?category.rfind(L"Gpu",0)!=0:category!=L"Cpu")continue;
   sensors.push_back(sensor);signature+=str(sensor,L"id")+L"|"+str(sensor,L"hardware")+L"|"+str(sensor,L"name")+L"|"+str(sensor,L"type")+L";";
  }
  bool ready=flag(sample,L"sensorReady"),stale=str(homeSnapshot(),L"status")==L"stale";
  cache->sensors=sensors;cache->stale=stale;cache->emptyMessage=!ready?tr(L"正在读取传感器…"):gpu?tr(L"当前 GPU 未返回传感器读数"):tr(L"当前 CPU 未返回传感器读数");
  if(sensors.empty())signature=ready?(gpu?L"gpu-empty":L"cpu-empty"):L"initializing";
  if(signature!=cache->signature||rows.Items().Size()==0){
   cache->signature=signature;cache->values.clear();std::vector<IInspectable> indices;
   for(uint32_t i=0;i<sensors.size();++i)indices.push_back(box_value(i));if(indices.empty())indices.push_back(box_value(UINT32_MAX));
   rows.ItemsSource(single_threaded_observable_vector<IInspectable>(std::move(indices)));
  }
  for(auto const& [container,entry]:cache->values){auto i=entry.first;if(i>=sensors.size())continue;
   auto sensor=sensors[i];auto value=sensor.GetNamedValue(L"value",JsonValue::CreateNullValue());bool valid=value.ValueType()==JsonValueType::Number&&std::isfinite(value.GetNumber());
   entry.second.Text(stale?tr(L"等待更新"):valid?digits(value.GetNumber(),1)+rawSensorUnit(str(sensor,L"type")):tr(L"未提供"));
  }
 };update(monitoring());monitorBindings.push_back(update);return card(body,16);
}
Grid cpuGpuSensorColumns(){
 Grid grid;grid.ColumnSpacing(16);grid.RowSpacing(12);for(int i=0;i<2;i++){ColumnDefinition c;c.Width({1,GridUnitType::Star});grid.ColumnDefinitions().Append(c);RowDefinition r;r.Height({0,GridUnitType::Auto});grid.RowDefinitions().Append(r);}
 auto cpu=sensorColumn(false),gpu=sensorColumn(true);Grid::SetColumn(gpu,1);grid.Children().Append(cpu);grid.Children().Append(gpu);
 onSizeChanged(grid,[grid,gpu](auto const&,SizeChangedEventArgs const& event){bool narrow=event.NewSize().Width<560;grid.ColumnDefinitions().GetAt(1).Width(narrow?GridLength{0,GridUnitType::Pixel}:GridLength{1,GridUnitType::Star});Grid::SetColumn(gpu,narrow?0:1);Grid::SetRow(gpu,narrow?1:0);});return grid;
}
