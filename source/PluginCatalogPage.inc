std::wstring catalogDescription(JsonObject entry){return str(entry,L"description");}
std::wstring catalogValidationLabel(JsonObject entry){if(str(entry,L"verificationStatus")==L"verified")return tr(L"已验证");if(str(entry,L"verificationStatus")==L"unverified")return tr(L"未实机验证");auto validation=entry.GetNamedObject(L"validation",JsonObject{});if(validation.HasKey(L"hardwareVerified")&&!flag(validation,L"hardwareVerified"))return tr(L"未实机验证");return str(entry,L"description").find(L"未实机验证")!=std::wstring::npos?tr(L"未实机验证"):L"";}
Button catalogDetailsButton(JsonObject entry){
 auto action=button(tr(L"详情"),[]{});Automation::AutomationProperties::SetName(action,tr(L"查看详情 ")+str(entry,L"name"));
 onClick(action,[this,entry,weakAction=make_weak(action)](auto const&,auto const&){auto action=weakAction.get();if(!action)return;Flyout details;auto body=stack(false,8);body.MaxWidth(420);body.Children().Append(title(str(entry,L"name"),17));body.Children().Append(text(catalogDescription(entry),13,true));
 auto field=[this,body](std::wstring label,std::wstring value){if(!value.empty())body.Children().Append(text(label+L"："+value,12,true));};
 field(tr(L"唯一编码"),str(entry,L"identityCode"));field(tr(L"作者"),str(entry,L"author"));field(tr(L"插件库"),str(entry,L"sourceName"));
 auto validation=catalogValidationLabel(entry);field(tr(L"验证状态"),validation);
 field(tr(L"协议"),str(entry,L"protocol"));field(tr(L"硬件标识"),str(entry.GetNamedObject(L"compatibility",JsonObject{}),L"hardwareId"));
 details.Content(body);details.ShowAt(action);});return action;
}
std::wstring catalogEntryKind(JsonObject entry){return catalogKind(entry);}
bool pluginMatches(JsonObject entry){return catalogQueryMatches(entry,pluginQuery);}
void queueCatalogSearch(){if(!catalogSearchDelay){catalogSearchDelay=DispatcherTimer{};catalogSearchDelay.Interval(std::chrono::milliseconds(180));catalogSearchDelay.Tick([this](auto const&,auto const&){catalogSearchDelay.Stop();if(current==L"market")updateCatalogList();});}catalogSearchDelay.Stop();catalogSearchDelay.Start();}
void catalogFilters(StackPanel host){
 catalogFilterHost=Grid{};auto grid=catalogFilterHost;grid.ColumnSpacing(6);grid.RowSpacing(6);
 std::vector<std::pair<std::wstring,std::wstring>> options={{L"mouse",tr(L"鼠标")},{L"keyboard",tr(L"键盘")},{L"headset",tr(L"耳机")},{L"controller",tr(L"手柄")},{L"application",tr(L"应用分类")},{L"other",tr(L"其他")}};
 for(auto const& [key,name]:options){auto action=button(name,[this,key]{if(catalogKindFilter==key&&!catalogQueryActive(pluginQuery))return;catalogKindFilter=key;pluginQuery.clear();catalogPageIndex=0;render();},catalogKindFilter==key&&!catalogQueryActive(pluginQuery));action.Tag(box_value(key));action.HorizontalAlignment(HorizontalAlignment::Stretch);action.MinWidth(0);grid.Children().Append(action);}
 auto total=(int)options.size();auto layout=[grid,total](double width){int count=std::clamp((int)((width+6)/105),1,total);if((int)grid.ColumnDefinitions().Size()==count)return;grid.ColumnDefinitions().Clear();grid.RowDefinitions().Clear();for(int i=0;i<count;i++){ColumnDefinition c;c.Width({1,GridUnitType::Star});grid.ColumnDefinitions().Append(c);}for(int i=0;i<(total+count-1)/count;i++){RowDefinition row;row.Height({0,GridUnitType::Auto});grid.RowDefinitions().Append(row);}for(int i=0;i<total;i++){Grid::SetColumn(grid.Children().GetAt(i).as<FrameworkElement>(),i%count);Grid::SetRow(grid.Children().GetAt(i).as<FrameworkElement>(),i/count);}};
 onSizeChanged(grid,[layout](auto const&,SizeChangedEventArgs const& e){layout(e.NewSize().Width);});layout(900);host.Children().Append(grid);
}
double catalogMeasuredRowHeight=0,catalogMeasuredWidth=0;bool catalogMeasuredCompact=false;
void fitCatalogPage(){
 if(current!=L"market"||!catalogItems||!catalogFooterSpacer||!catalogItems.IsLoaded()||catalogItems.Children().Size()<2||pageScroll.ViewportHeight()<=0)return;
 auto children=catalogItems.Children();auto footer=children.GetAt(children.Size()-1).as<FrameworkElement>();
 auto top=catalogItems.TransformToVisual(page).TransformPoint({0,0}).Y;auto gap=catalogItems.Spacing();
 if(footer.ActualHeight()<=0)return;
 if(std::abs(catalogMeasuredWidth-catalogItems.ActualWidth())>1||catalogMeasuredCompact!=compactUi){catalogMeasuredRowHeight=0;catalogMeasuredWidth=catalogItems.ActualWidth();catalogMeasuredCompact=compactUi;}
 bool hasCards=false;
 for(uint32_t i=0;i+2<children.Size();++i)if(auto tile=children.GetAt(i).try_as<Border>()){
  tile.Measure(Windows::Foundation::Size{(float)catalogItems.ActualWidth(),std::numeric_limits<float>::infinity()});
  catalogMeasuredRowHeight=std::max(catalogMeasuredRowHeight,(double)tile.DesiredSize().Height);hasCards=true;
 }
 if(hasCards){
  auto available=pageScroll.ViewportHeight()-top-page.Padding().Bottom-footer.ActualHeight()-gap;
  auto capacity=(uint32_t)std::clamp((int)std::floor(available/(std::max(64.0,catalogMeasuredRowHeight)+gap)),1,5);
  if(capacity!=catalogPageSize){catalogPageIndex=catalogPageIndex*catalogPageSize/capacity;catalogPageSize=capacity;updateCatalogList();return;}
 }
 double used=gap*(children.Size()-1);
 for(uint32_t i=0;i<children.Size();++i){auto element=children.GetAt(i).as<FrameworkElement>();if(element==catalogFooterSpacer)continue;used+=element.ActualHeight();}
 auto space=std::max(0.0,pageScroll.ViewportHeight()-top-page.Padding().Bottom-used);
 if(std::abs(catalogFooterSpacer.Height()-space)>.5)catalogFooterSpacer.Height(space);
}
void queueCatalogFit(){if(catalogFitQueued||!catalogItems)return;catalogFitQueued=true;page.DispatcherQueue().TryEnqueue(Microsoft::UI::Dispatching::DispatcherQueuePriority::Low,[this]{catalogFitQueued=false;fitCatalogPage();});}
void updateCatalogList(){if(!catalogItems)return;bool searching=catalogQueryActive(pluginQuery);if(catalogFilterHost)for(auto const& child:catalogFilterHost.Children()){auto action=child.as<Button>();auto key=unbox_value_or<hstring>(action.Tag(),hstring{});if(!searching&&key==catalogKindFilter)styleAccentButton(action);else{action.Resources(ResourceDictionary{});action.Background(dark?brush(41,49,61):brush(242,244,248));action.Foreground(dark?brush(245,245,245):brush(27,27,27));}action.Tag(box_value(key));}
 JsonArray matches;std::map<std::wstring,uint32_t> identities;
 for(auto const& value:catalogFeed.GetNamedArray(L"plugins",JsonArray{})){
  auto entry=value.GetObject();if(!pluginMatches(entry)||(!searching&&catalogEntryKind(entry)!=catalogKindFilter))continue;
  auto key=str(entry,L"id")+L"\n"+str(entry,L"author")+L"\n"+str(entry,L"version")+L"\n"+str(entry,L"sha256");
  if(auto found=identities.find(key);found!=identities.end()){auto existing=object(std::wstring(matches.GetObjectAt(found->second).Stringify()));put(existing,L"sourceName",str(existing,L"sourceName")+L" / "+str(entry,L"sourceName"));matches.SetAt(found->second,existing);}else{identities[key]=matches.Size();matches.Append(entry);}
 }
 const uint32_t pages=std::max<uint32_t>(1,(matches.Size()+catalogPageSize-1)/catalogPageSize);catalogPageIndex=std::min(catalogPageIndex,pages-1);const uint32_t begin=catalogPageIndex*catalogPageSize,end=std::min(begin+catalogPageSize,matches.Size());
 JsonArray visible;for(uint32_t i=begin;i<end;i++)visible.Append(matches.GetAt(i));
 JsonArray installedSummary;for(auto const& value:state.GetNamedArray(L"plugins")){auto p=value.GetObject();JsonObject summary;for(auto key:{L"id",L"author",L"version",L"installed"})if(p.HasKey(key))summary.SetNamedValue(key,p.GetNamedValue(key));installedSummary.Append(summary);}
 auto signature=std::wstring(visible.Stringify())+std::wstring(installedSummary.Stringify())+pluginQuery+catalogKindFilter+std::to_wstring(catalogPageIndex)+L"/"+std::to_wstring(catalogPageSize)+L"/"+std::to_wstring(matches.Size())+(catalogBusy?L"busy":L"ready");
 for(auto const& [tile,p]:installedPluginTiles)tile.Visibility(pluginMatches(p)?Visibility::Visible:Visibility::Collapsed);
 if(signature==catalogRenderSignature)return;catalogRenderSignature=std::move(signature);
 releaseCatalogEvents();catalogItems.Children().Clear();auto cleanupStart=pageEventCleanup.size();
 for(uint32_t i=begin;i<end;i++){
  auto p=matches.GetObjectAt(i);auto row=stack(false,4);Grid heading;heading.ColumnSpacing(12);ColumnDefinition nameColumn;nameColumn.Width({1,GridUnitType::Star});ColumnDefinition versionColumn;versionColumn.Width({0,GridUnitType::Auto});heading.ColumnDefinitions().Append(nameColumn);heading.ColumnDefinitions().Append(versionColumn);
  auto name=title(str(p,L"name"),17);name.TextWrapping(TextWrapping::NoWrap);name.TextTrimming(TextTrimming::CharacterEllipsis);ToolTipService::SetToolTip(name,box_value(str(p,L"name")));heading.Children().Append(name);auto version=text(L"v"+str(p,L"version"),12,true);version.VerticalAlignment(VerticalAlignment::Center);Grid::SetColumn(version,1);heading.Children().Append(version);row.Children().Append(heading);
  auto description=text(catalogDescription(p),compactUi?12:13,true);description.MaxLines(1);description.TextWrapping(TextWrapping::NoWrap);description.TextTrimming(TextTrimming::CharacterEllipsis);row.Children().Append(description);
  auto validation=catalogValidationLabel(p);auto metadata=text((validation.empty()?L"":validation+L" · ")+tr(L"作者：")+str(p,L"author"),12,true);metadata.MaxLines(1);metadata.TextWrapping(TextWrapping::NoWrap);metadata.TextTrimming(TextTrimming::CharacterEllipsis);if(!compactUi)row.Children().Append(metadata);ToolTipService::SetToolTip(description,box_value(catalogDescription(p)));
  bool installed=false,conflict=false;std::wstring installedVersion;for(auto const& v:state.GetNamedArray(L"plugins")){auto item=v.GetObject();if(str(item,L"id")==str(p,L"id")&&flag(item,L"installed")){installed=true;installedVersion=str(item,L"version");conflict=str(item,L"author")!=str(p,L"author");}}
  auto action=button(conflict?tr(L"作者冲突"):installed&&installedVersion==str(p,L"version")?tr(L"已安装"):installed?tr(L"下载更新"):tr(L"下载并安装"),[]{},!installed);Automation::AutomationProperties::SetName(action,(installed?tr(L"更新 "):tr(L"安装 "))+str(p,L"name"));auto weakAction=make_weak(action);onClick(action,[this,p,weakAction](auto const&,auto const&){if(auto action=weakAction.get())downloadCatalogPlugin(p,action);});action.IsEnabled(!catalogBusy&&!conflict&&!(installed&&installedVersion==str(p,L"version"))&&!str(p,L"packageUrl").empty());
  auto controls=stack(true,8);controls.Children().Append(catalogDetailsButton(p));if(installed&&!conflict){auto id=str(p,L"id");auto plugin=pluginRecord(state,id);if(plugin&&(!nativePluginPage(plugin).empty()||hasOfflineUi(plugin))){auto open=button(tr(L"打开"),[this,id]{openInstalledPlugin(id);});open.Tag(box_value(L"catalog-open:"+id));Automation::AutomationProperties::SetName(open,tr(L"打开 ")+str(p,L"name"));open.IsEnabled(!nativePluginPage(plugin).empty()||str(plugin,L"mode")!=L"disabled");controls.Children().Append(open);}}controls.Children().Append(action);auto frame=card(actionHeader(row,controls),12);frame.Padding(compactUi?Thickness{10,6,10,6}:Thickness{12,8,12,8});frame.Tag(box_value(str(p,L"id")));catalogItems.Children().Append(frame);
 }
 if(!matches.Size())catalogItems.Children().Append(text(pluginQuery.empty()?tr(L"此分类暂无插件"):tr(L"没有匹配的插件"),13,true));
 catalogFooterSpacer=Border{};catalogFooterSpacer.Height(0);catalogFooterSpacer.Tag(box_value(L"catalog-footer-space"));catalogItems.Children().Append(catalogFooterSpacer);
 Grid navigation;navigation.ColumnSpacing(12);navigation.HorizontalAlignment(HorizontalAlignment::Center);navigation.Tag(box_value(L"catalog-pagination"));for(int i=0;i<3;++i){ColumnDefinition column;column.Width({0,GridUnitType::Auto});navigation.ColumnDefinitions().Append(column);}
 auto previous=button(tr(L"上一页"),[this]{if(catalogPageIndex){--catalogPageIndex;updateCatalogList();}});previous.Tag(box_value(L"catalog-previous"));previous.MinWidth(90);previous.IsEnabled(catalogPageIndex>0);navigation.Children().Append(previous);
 auto counter=text(tr(L"第 ")+std::to_wstring(catalogPageIndex+1)+L" / "+std::to_wstring(pages)+tr(L" 页 · 共 ")+std::to_wstring(matches.Size())+tr(L" 个插件"),13,true);counter.VerticalAlignment(VerticalAlignment::Center);counter.TextAlignment(TextAlignment::Center);counter.Tag(box_value(L"catalog-page-number"));Grid::SetColumn(counter,1);navigation.Children().Append(counter);
 auto next=button(tr(L"下一页"),[this,pages]{if(catalogPageIndex+1<pages){++catalogPageIndex;updateCatalogList();}});next.Tag(box_value(L"catalog-next"));next.MinWidth(90);next.IsEnabled(catalogPageIndex+1<pages);Grid::SetColumn(next,2);navigation.Children().Append(next);catalogItems.Children().Append(navigation);queueCatalogFit();

 for(size_t i=cleanupStart;i<pageEventCleanup.size();++i)catalogEventCleanup.push_back(std::move(pageEventCleanup[i]));pageEventCleanup.resize(cleanupStart);
}
#include "CatalogSourcesPage.inc"
winrt::fire_and_forget downloadCatalogPlugin(JsonObject entry,Button action){if(catalogBusy)co_return;catalogBusy=true;catalogMessage=tr(L"正在下载并校验插件…");auto oldContent=action.Content();action.Content(box_value(tr(L"下载校验中…")));action.IsEnabled(false);auto lifetime=get_strong();winrt::apartment_context ui;JsonObject result{nullptr};std::wstring error;try{JsonObject input;input.SetNamedValue(L"entry",entry);co_await resume_background();result=catalogTask(L"--download",input);}catch(hresult_error const& e){error=std::wstring(e.message());}co_await ui;catalogBusy=false;action.Content(oldContent);action.IsEnabled(true);if(!result||!flag(result,L"ok")){notice(result?str(result,L"error"):error,true);co_return;}auto manifest=result.GetNamedObject(L"manifest");try{validatePluginUi(manifest);validatePluginModule(manifest);}catch(hresult_error const& e){notice(std::wstring(e.message()),true);co_return;}try{if(!confirmCatalogInstall(manifest,entry))co_return;}catch(hresult_error const& e){notice(std::wstring(e.message()),true);co_return;}try{for(auto const& value:state.GetNamedArray(L"plugins")){auto p=value.GetObject();if(str(p,L"id")==str(manifest,L"id")&&flag(p,L"installed")){if(str(p,L"author")!=str(manifest,L"author"))throw hresult_invalid_argument(tr(L"更新作者与原插件不匹配"));break;}}JsonObject input;input.SetNamedValue(L"manifest",manifest);put(input,L"replaceInstalled",true);invoke(L"importPlugin",input);syncNativeNav();if(current==L"market")updateCatalogList();else if(current==L"plugins")render();notice(tr(L"插件已安装"));}catch(hresult_error const& e){notice(std::wstring(e.message()),true);}}

