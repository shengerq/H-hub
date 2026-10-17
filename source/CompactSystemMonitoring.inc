// Original content-sized card proportions, retaining the new text and trend presentation.
TextBlock homeCompactLine(std::function<std::wstring(JsonObject)> format){
 auto label=text(L"",11,true);label.TextWrapping(TextWrapping::NoWrap);label.TextTrimming(TextTrimming::CharacterEllipsis);
 auto update=[this,label,format](JsonObject){auto value=format(homeSnapshot());if(label.Text()!=value){label.Text(value);ToolTipService::SetToolTip(label,box_value(value));}};update(monitoring());monitorBindings.push_back(update);return label;
}
Canvas homeTrend(std::wstring key){
 Canvas chart;chart.Height(20);chart.IsHitTestVisible(false);
 auto lines=std::make_shared<std::vector<Microsoft::UI::Xaml::Shapes::Polyline>>();
 auto update=[this,chart,key,lines](JsonObject){
  double now=clockSeconds(),width=std::max(1.0,chart.ActualWidth()),height=std::max(8.0,chart.ActualHeight());
  for(auto const& line:*lines){line.Points().Clear();line.Visibility(Visibility::Collapsed);}
  Microsoft::UI::Xaml::Shapes::Polyline line{nullptr};double previous=-1;size_t used=0;
  for(auto const& sample:monitorHistory){double time=num(sample,L"sampleTime");auto field=sample.GetNamedObject(L"homeFields",JsonObject{}).GetNamedObject(key,JsonObject{});auto value=field.GetNamedValue(L"value",JsonValue::CreateNullValue());
   if(now-time>60)continue;if(value.ValueType()!=JsonValueType::Number){line=nullptr;previous=-1;continue;}
   if(!line||time-previous>5){if(used==lines->size()){Microsoft::UI::Xaml::Shapes::Polyline next;next.Stroke(accent());next.StrokeThickness(1.5);chart.Children().Append(next);lines->push_back(next);}line=lines->at(used++);line.Visibility(Visibility::Visible);}
   line.Points().Append(Windows::Foundation::Point{(float)(std::clamp(1-(now-time)/60,0.0,1.0)*width),(float)(height-2-std::clamp(value.GetNumber(),0.0,100.0)*(height-4)/100)});previous=time;
  }
 };onSizeChanged(chart,[update](auto const&,auto const&){update(JsonObject{});});monitorBindings.push_back(update);update(monitoring());return chart;
}
void nativeSystemMonitoring(){
 if(shortUi){
  Grid summary;summary.ColumnSpacing(12);
  for(int i=0;i<5;i++){ColumnDefinition column;column.Width(i==4?GridLength{0,GridUnitType::Auto}:GridLength{1,GridUnitType::Star});summary.ColumnDefinitions().Append(column);}
  std::vector<std::wstring> labels={L"CPU",L"GPU",tr(L"内存"),tr(L"磁盘读取")},keys={L"cpu",L"gpu",L"memory",L"diskReadMB"};
  for(int i=0;i<4;i++){auto metric=stack(true,8);metric.VerticalAlignment(VerticalAlignment::Center);metric.Children().Append(text(labels[i],12,true));metric.Children().Append(homeValue(keys[i],i==3?L" MB/s":L"%",i==3?1:0,16));Grid::SetColumn(metric,i);summary.Children().Append(metric);}
  auto details=button(tr(L"系统监测"),[this]{go(L"system");});Grid::SetColumn(details,4);summary.Children().Append(details);page.Children().Append(card(summary,8));return;
 }

 Grid section;section.RowSpacing(compactUi?12:20);RowDefinition headerRow;headerRow.Height({0,GridUnitType::Auto});section.RowDefinitions().Append(headerRow);RowDefinition cardsRow;cardsRow.Height({0,GridUnitType::Auto});section.RowDefinitions().Append(cardsRow);
 auto entry=button(tr(L"系统监测"),[this]{go(L"system");});entry.Content(pageTitle(tr(L"系统监测")));entry.Padding(margin(0));entry.MinHeight(32);entry.HorizontalAlignment(HorizontalAlignment::Left);entry.Background(brush(0,0,0,0));entry.BorderThickness(margin(0));ToolTipService::SetToolTip(entry,box_value(tr(L"打开系统监测")));section.Children().Append(entry);
 std::vector<Border> cards;
 auto body=[&](std::wstring label,std::wstring glyph,std::wstring key,std::wstring modelKey){
  auto panel=stack(false,4);auto labelRow=metricTitle(label,glyph);labelRow.Height(24);panel.Children().Append(labelRow);
  if(!modelKey.empty()){
   auto model=text(L"",11,true);model.TextWrapping(TextWrapping::NoWrap);model.TextTrimming(TextTrimming::CharacterEllipsis);
   auto patch=[this,model,modelKey](JsonObject sample){auto raw=str(sample,modelKey.c_str(),tr(L"等待硬件信息"));model.Text(compactModel(raw,modelKey==L"gpuName"));ToolTipService::SetToolTip(model,box_value(raw));};patch(monitoring());monitorBindings.push_back(patch);panel.Children().Append(model);
  }else panel.Children().Append(text(label==L"内存"?tr(L"物理内存"):tr(L"所有磁盘合计"),11,true));
  panel.Children().Append(homeValue(key,label==L"磁盘"?L" MB/s":L"%",label==L"磁盘"?1:0,24));
  if(label!=L"磁盘")panel.Children().Append(homeTrend(key));else panel.Children().Append(text(tr(L"读取速度"),11,true));return panel;
 };
 auto cpu=body(L"CPU",L"\uE950",L"cpu",L"cpuName");
 cpu.Children().Append(homeCompactLine([this](JsonObject sample){return tr(L"频率 ")+homeFieldText(sample,L"cpuClock",L" MHz")+tr(L" · 温度 ")+homeFieldText(sample,L"cpuTemp",L" °C");}));
 cpu.Children().Append(homeCompactLine([this](JsonObject sample){return tr(L"功耗 ")+homeFieldText(sample,L"cpuPower",L" W",1);}));cards.push_back(card(cpu,16));
 auto gpu=body(L"GPU",L"\uE7F4",L"gpu",L"gpuName");
 gpu.Children().Append(homeCompactLine([this](JsonObject sample){return tr(L"显存 ")+homeFieldText(sample,L"gpuUsedGB",L"",1)+L" / "+homeFieldText(sample,L"gpuTotalGB",L" GB",1);}));
 gpu.Children().Append(homeCompactLine([this](JsonObject sample){return homeFieldText(sample,L"gpuTemp",L" °C")+L" · "+homeFieldText(sample,L"gpuClock",L" MHz")+L" · "+homeFieldText(sample,L"gpuPower",L" W",1);}));cards.push_back(card(gpu,16));
 auto memory=body(tr(L"内存"),L"\uE964",L"memory",L"");
 memory.Children().Append(homeCompactLine([this](JsonObject sample){return tr(L"已用 ")+homeFieldText(sample,L"memoryUsedGB",L" GB",1);}));
 memory.Children().Append(homeCompactLine([this](JsonObject sample){return tr(L"总量 ")+homeFieldText(sample,L"memoryTotalGB",L" GB",1);}));cards.push_back(card(memory,16));
 auto disk=body(tr(L"磁盘"),L"\uEDA2",L"diskReadMB",L"");
 disk.Children().Append(homeCompactLine([this](JsonObject sample){return tr(L"写入 ")+homeFieldText(sample,L"diskWriteMB",L" MB/s",1);}));
 disk.Children().Append(homeCompactLine([this](JsonObject sample){return tr(L"容量 ")+homeFieldText(sample,L"diskUsedGB",L"",1)+L" / "+homeFieldText(sample,L"diskTotalGB",L" GB",1);}));cards.push_back(card(disk,16));
 Grid grid;Grid::SetRow(grid,1);for(auto const& frame:cards){frame.MinHeight(compactUi?164:208);grid.Children().Append(frame);}section.Children().Append(grid);
 auto layout=[this,section,grid,cards](double width){
  int columns=width>=840?4:width>=560?2:1;
  if((int)grid.ColumnDefinitions().Size()!=columns){grid.ColumnDefinitions().Clear();grid.RowDefinitions().Clear();for(int i=0;i<columns;i++){ColumnDefinition c;c.Width({1,GridUnitType::Star});grid.ColumnDefinitions().Append(c);}for(int i=0;i<(4+columns-1)/columns;i++){RowDefinition r;r.Height({0,GridUnitType::Auto});grid.RowDefinitions().Append(r);}for(int i=0;i<4;i++){Grid::SetColumn(cards[i],i%columns);Grid::SetRow(cards[i],i/columns);cards[i].Margin(Thickness{0,0,i%columns==columns-1?0.0:12.0,columns==4?0.0:12.0});}}
 };
 layout(0);onSizeChanged(section,[layout](auto const&,SizeChangedEventArgs const& event){layout(event.NewSize().Width);});page.Children().Append(section);
}
