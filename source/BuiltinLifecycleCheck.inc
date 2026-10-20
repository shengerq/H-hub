DispatcherTimer builtinCheckTimer{nullptr};bool builtinCheckBusy=false;size_t builtinCheckStep=0;JsonArray builtinCheckResults;
void runBuiltinLifecycleCheck(){
 if(instanceSuffix().empty())throw hresult_invalid_argument(L"Lifecycle verification requires HHUB_INSTANCE");
 builtinCheckTimer=DispatcherTimer{};builtinCheckTimer.Interval(std::chrono::seconds(3));
 builtinCheckTimer.Tick([this](auto const&,auto const&){
  if(builtinCheckBusy)return;builtinCheckBusy=true;struct BusyReset{bool& flag;~BusyReset(){flag=false;}}busyReset{builtinCheckBusy};
  static std::wstring const routes[]{L"market",L"market-filled",L"market-next",L"market-short",L"windows",L"eq",L"phone",L"home",L"windows",L"eq",L"phone",L"home"};
  if(builtinCheckStep){
   JsonObject sample;auto route=routes[builtinCheckStep-1];put(sample,L"route",route);put(sample,L"memoryMB",workingSet());
   bool market=route.rfind(L"market",0)==0;bool expected=route!=L"home"&&!market;put(sample,L"ok",expected?(nativeSurface&&nativeSurface->alive()&&nativeSurface->local):!nativeSurface);
   if(market&&catalogItems&&catalogItems.Children().Size()){
    auto footer=catalogItems.Children().GetAt(catalogItems.Children().Size()-1).as<Grid>();auto point=footer.TransformToVisual(pageScroll).TransformPoint({0,0});
    auto gap=pageScroll.ViewportHeight()-point.Y-footer.ActualHeight()-page.Padding().Bottom;put(sample,L"footerBottomGap",gap);
    bool centered=true,disabled=true;for(auto const& child:footer.Children())if(auto button=child.try_as<Button>()){centered=centered&&button.HorizontalContentAlignment()==HorizontalAlignment::Center&&button.VerticalContentAlignment()==VerticalAlignment::Center;disabled=disabled&&!button.IsEnabled();}
    bool cardsFit=true;for(uint32_t i=0;i+2<catalogItems.Children().Size();++i)if(auto tile=catalogItems.Children().GetAt(i).try_as<Border>()){auto position=tile.TransformToVisual(pageScroll).TransformPoint({0,0});cardsFit=cardsFit&&position.Y+tile.ActualHeight()<=point.Y+1&&tile.ActualHeight()+1>=tile.DesiredSize().Height;}
    bool noScroll=pageScroll.VerticalScrollMode()==ScrollMode::Disabled&&pageScroll.ScrollableHeight()<1;
    put(sample,L"centered",centered);put(sample,L"paginationDisabled",disabled);put(sample,L"noScroll",noScroll);put(sample,L"cardsFit",cardsFit);put(sample,L"pageSize",(double)catalogPageSize);put(sample,L"pageIndex",(double)catalogPageIndex);put(sample,L"ok",centered&&(route!=L"market"||disabled)&&std::abs(gap)<6&&cardsFit&&noScroll);
   }
   put(sample,L"integratedPage",nativeSurface&&nativeSurface->local);put(sample,L"pagePid",nativeSurface?(double)nativeSurface->remote.pid:0.0);
   unsigned alivePages=0;for(auto const& previous:retiredPages)if(previous.get())++alivePages;put(sample,L"retiredPages",(double)alivePages);
   builtinCheckResults.Append(sample);
  }
  if(builtinCheckStep==std::size(routes)){
   builtinCheckTimer.Stop();JsonObject result;bool passed=true;for(auto const& v:builtinCheckResults)passed=passed&&flag(v.GetObject(),L"ok");put(result,L"ok",passed);result.SetNamedValue(L"samples",builtinCheckResults);write(dataDir()/L"builtin-lifecycle.json",std::wstring(result.Stringify()));window.Close();return;
  }
  auto target=routes[builtinCheckStep++];
  if(target==L"market"){catalogFeed=object(L"{\"plugins\":[]}");catalogTried=true;go(L"market");}
  else if(target==L"market-filled"){
   JsonArray entries;for(int i=0;i<12;++i){JsonObject entry;put(entry,L"id",L"fixture-"+std::to_wstring(i));put(entry,L"name",std::wstring(L"AJAZZ NJ07 MC PRO [A8A4-2177] long plugin title fixture"));put(entry,L"author",std::wstring(L"H hub"));put(entry,L"description",std::wstring(L"Long description for viewport fitting, metadata visibility and pagination without vertical scrolling."));put(entry,L"version",std::wstring(L"0.1.1"));put(entry,L"kind",std::wstring(L"mouse"));entries.Append(entry);}catalogFeed=JsonObject{};catalogFeed.SetNamedValue(L"plugins",entries);catalogPageIndex=0;catalogKindFilter=L"mouse";render();
  }else if(target==L"market-next"){++catalogPageIndex;updateCatalogList();}
  else if(target==L"market-short"){window.AppWindow().Resize({900,640});}
  else go(target);
 });builtinCheckTimer.Start();
}
