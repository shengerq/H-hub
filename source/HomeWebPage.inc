// Trusted local homepage. Only the projected display state crosses this bridge.
WebView2 homeView{nullptr};
bool homeReady=false;
std::wstring lastHomeMessage;
std::map<std::wstring,std::pair<size_t,std::wstring>> homeArtCache;

void closeHomeView(){homeReady=false;lastHomeMessage.clear();homeArtCache.clear();if(homeView){releasePluginView(homeView);homeView=nullptr;}}
void layoutHomeWeb(){if(current!=L"home"||!homeView||!pageScroll||pageScroll.ViewportHeight()<=0)return;double height=std::max(180.0,pageScroll.ViewportHeight()-page.Padding().Top-page.Padding().Bottom);if(!std::isfinite(homeView.Height())||std::abs(homeView.Height()-height)>0.5)homeView.Height(height);}

std::wstring homeArt(JsonObject device){
 auto id=str(device,L"id"),kind=str(device,L"kind");auto plugin=matchedDevicePlugin(device);
 auto html=plugin?str(plugin.GetNamedObject(L"ui"),L"content"):std::wstring{};
 auto signature=std::hash<std::wstring>{}(html+kind);auto cached=homeArtCache.find(id);if(cached!=homeArtCache.end()&&cached->second.first==signature)return cached->second.second;
 std::wstring art;auto start=html.find(L"data:image/png;base64,");
 if(start!=std::wstring::npos){auto end=start+22;while(end<html.size()&&(iswalnum(html[end])||html[end]==L'+'||html[end]==L'/'||html[end]==L'='))++end;art=html.substr(start,end-start);}
 else art=L"https://assets.hhub.local/"+std::wstring(kind==L"mouse"?L"mouse.svg":kind==L"keyboard"?L"keyboard.svg":kind==L"gamepad"?L"gamepad.svg":L"headset.svg");
 homeArtCache[id]={signature,art};return art;
}

JsonObject homeWebState(bool available=true){
 JsonObject message,values;JsonArray metrics,devices;auto sample=available?monitoring():JsonObject{};
 put(message,L"type",std::wstring(L"hhub.home.state"));message.SetNamedValue(L"theme",pluginThemeMessage());put(message,L"busy",monitorBusy||homeSelectionBusy);put(message,L"selected",selectedDetected());
 auto value=[&](std::wstring key,std::wstring text,std::wstring hint=L"",bool error=false,bool neutral=false){JsonObject v;put(v,L"text",text);put(v,L"hint",hint);put(v,L"error",error);put(v,L"neutral",neutral);values.SetNamedValue(key,v);};
 auto row=[&](JsonArray rows,std::wstring key,std::wstring label,std::wstring text){JsonObject r;put(r,L"key",key);put(r,L"label",label);rows.Append(r);value(key,text);};
 auto live=[&](JsonArray rows,std::wstring key,std::wstring label,std::wstring unit,int precision=0){auto v=sample.GetNamedValue(key,JsonValue::CreateNullValue());bool valid=v.ValueType()==JsonValueType::Number;auto availability=str(sample.GetNamedObject(L"fieldAvailability",JsonObject{}),key.c_str());auto text=valid?digits(v.GetNumber(),precision)+unit:availability==L"initializing"?L"读取中…":availability==L"needsDriver"?L"需驱动":availability==L"needsElevation"?L"需授权":availability==L"unsupported"?L"未提供":flag(sample,L"virtualMachine")?L"VM 未回传":L"未回传";row(rows,key,label,text);bool neutral=availability==L"initializing"||availability==L"unsupported";value(key,text,str(sample.GetNamedObject(L"unavailableReasons",JsonObject{}),key.c_str()),!valid&&!neutral,!valid&&neutral);};
 auto metric=[&](std::wstring label,std::wstring icon,std::wstring modelKey,JsonArray rows,bool gpu=false){JsonObject m;put(m,L"label",label);put(m,L"icon",icon);put(m,L"modelKey",modelKey);m.SetNamedValue(L"rows",rows);if(!modelKey.empty()){auto raw=str(sample,modelKey.c_str());value(modelKey,raw.empty()?L"读取中…":compactModel(raw,gpu),raw);}metrics.Append(m);};
 JsonArray cpu;live(cpu,L"cpu",L"利用率",L"%");live(cpu,L"cpuClock",L"频率",L" MHz");live(cpu,L"cpuTemp",L"温度",L" °C");live(cpu,L"cpuPower",L"功耗",L" W",1);metric(L"CPU",L"\uE950",L"cpuName",cpu);
 JsonArray gpu;live(gpu,L"gpu",L"利用率",L"%");row(gpu,L"gpuMemory",L"独立显存",amount(sample,L"gpuUsedGB",1)+L" / "+amount(sample,L"gpuTotalGB",1)+L" GB");live(gpu,L"gpuClock",L"频率",L" MHz");live(gpu,L"gpuTemp",L"温度",L" °C");live(gpu,L"gpuPower",L"功耗",L" W",1);metric(L"GPU",L"\uE7F4",L"gpuName",gpu,true);
 JsonArray memory;row(memory,L"memoryCapacity",L"已使用",amount(sample,L"memoryUsedGB",1)+L" / "+amount(sample,L"memoryTotalGB",1)+L" GB");live(memory,L"memory",L"利用率",L"%");metric(L"内存",L"\uE964",L"",memory);
 JsonArray diskRows;auto disks=sample.GetNamedArray(L"disks",JsonArray{});for(uint32_t i=0;i<disks.Size();i++){auto d=disks.GetObjectAt(i);auto prefix=L"disk"+std::to_wstring(i);JsonObject heading;put(heading,L"heading",L"磁盘 "+std::to_wstring(i+1)+L" · "+str(d,L"name"));diskRows.Append(heading);row(diskRows,prefix+L"Capacity",L"已用/总量",amount(d,L"usedGB",1)+L" / "+amount(d,L"totalGB",1)+L" GB");row(diskRows,prefix+L"Speed",L"读 / 写",amount(d,L"readMB",1)+L" / "+amount(d,L"writeMB",1)+L" MB/s");}
 if(!disks.Size()){row(diskRows,L"diskCapacity",L"已用/总量",amount(sample,L"diskUsedGB",1)+L" / "+amount(sample,L"diskTotalGB",1)+L" GB");row(diskRows,L"diskSpeed",L"读 / 写",amount(sample,L"diskReadMB",1)+L" / "+amount(sample,L"diskWriteMB",1)+L" MB/s");}metric(L"磁盘",L"\uEDA2",L"",diskRows);
 std::vector<std::wstring> liveIds;
 for(auto const& v:visibleDevices()){
  auto d=v.GetObject();JsonObject projected;auto id=str(d,L"id"),kind=str(d,L"kind");liveIds.push_back(id);put(projected,L"id",id);put(projected,L"name",deviceDisplayName(d));put(projected,L"status",str(d,L"status"));put(projected,L"connected",flag(d,L"connected"));
  auto battery=d.HasKey(L"battery")?L"电量 "+digits(num(d,L"battery"))+L"%"+(flag(d,L"lowBattery")?L" · 低电量":L""):d.HasKey(L"batteryLevel")?L"电量等级 "+str(d,L"batteryLevel"):L"电量不可用";put(projected,L"battery",battery);put(projected,L"tileStatus",d.HasKey(L"battery")?battery:str(d,L"status",L"状态不可用"));
  put(projected,L"kindLabel",std::wstring(kind==L"mouse"?L"鼠标 / 鼠标接收器":kind==L"keyboard"?L"键盘 / 复合输入设备":kind==L"gamepad"?L"手柄接口":kind==L"audio"?L"音频端点":L"Windows 外设"));put(projected,L"profile",str(d,L"connection")+L"  ·  "+str(d,L"profile"));put(projected,L"art",homeArt(d));put(projected,L"artNote",std::wstring(matchedDevicePlugin(d)?L"插件提供的产品外观":L"外观示意 · 非型号照片"));devices.Append(projected);
 }
 for(auto it=homeArtCache.begin();it!=homeArtCache.end();)if(std::find(liveIds.begin(),liveIds.end(),it->first)==liveIds.end())it=homeArtCache.erase(it);else ++it;
 std::wstring scene=L"当前场景：暂无场景";try{auto s=find(state.GetNamedArray(L"scenes"),str(state,L"currentScene"));scene=L"当前场景："+str(s,L"name")+(flag(state,L"sceneLocked")?L" · 手动锁定":L" · 自动规则");}catch(...){}
 put(message,L"scene",scene);message.SetNamedValue(L"metrics",metrics);message.SetNamedValue(L"values",values);message.SetNamedValue(L"devices",devices);return message;
}
void syncHomeWeb(bool available=true){if(current!=L"home"||!homeReady||!homeView||!homeView.CoreWebView2())return;try{auto json=std::wstring(homeWebState(available).Stringify());if(json!=lastHomeMessage){homeView.CoreWebView2().PostWebMessageAsJson(json);lastHomeMessage=std::move(json);}}catch(hresult_error const& e){notice(L"主页数据更新失败："+std::wstring(e.message()),true);}}
void homeWebAction(std::wstring const& json){
 try{if(json.size()>4096)return;auto request=object(json);if(str(request,L"type")!=L"hhub.home.action")return;auto action=str(request,L"action");
  if(action==L"ready"){homeReady=true;lastHomeMessage.clear();syncHomeWeb();}
  else if(action==L"system")go(L"system");
  else if(action==L"overview")deviceOverviewDialog();
  else if(action==L"interfaces")go(L"devices");
  else if(action==L"refresh"){refreshDeviceScan();syncHomeWeb();}
  else if(action==L"select"||action==L"details"){auto id=str(request,L"id");auto device=find(visibleDevices(),id);if(action==L"select")selectHomeDevice(id);else if(id==selectedDetected())openDevicePage(device);}
 }catch(hresult_error const& e){notice(std::wstring(e.message()),true);syncHomeWeb();}
}
winrt::fire_and_forget loadHomeWeb(WebView2 view,uint64_t generation){
 auto lifetime=get_strong();try{
  using namespace Microsoft::Web::WebView2::Core;
  auto valid=[this,view,generation]{return current==L"home"&&generation==renderGeneration&&homeView==view;};
  auto environment=pluginEnvironment;if(!environment){auto profile=dataDir()/L"plugin-ui"/L"developer-profile";std::filesystem::create_directories(profile);environment=co_await CoreWebView2Environment::CreateWithOptionsAsync(L"",profile.wstring(),nullptr);pluginEnvironment=environment;}if(!valid())co_return;
  co_await view.EnsureCoreWebView2Async(environment);if(!valid())co_return;auto core=view.CoreWebView2();core.Settings().AreHostObjectsAllowed(false);core.Settings().IsWebMessageEnabled(true);core.Settings().AreDefaultContextMenusEnabled(false);
  core.SetVirtualHostNameToFolderMapping(L"home.hhub.local",(binaryDir()/L"webui").wstring(),CoreWebView2HostResourceAccessKind::DenyCors);core.SetVirtualHostNameToFolderMapping(L"assets.hhub.local",(binaryDir()/L"assets").wstring(),CoreWebView2HostResourceAccessKind::Allow);
  std::wstring allowed=L"https://home.hhub.local/index.html";auto key=::winrt::get_unknown(view);
  auto navigation=core.NavigationStarting([allowed](auto const&,CoreWebView2NavigationStartingEventArgs const& e){if(std::wstring(e.Uri())!=allowed)e.Cancel(true);});pluginEventCleanup[key].push_back([weak=make_weak(view),navigation]{if(auto v=weak.get())if(auto c=v.CoreWebView2())c.NavigationStarting(navigation);});
  auto popup=core.NewWindowRequested([](auto const&,CoreWebView2NewWindowRequestedEventArgs const& e){e.Handled(true);});pluginEventCleanup[key].push_back([weak=make_weak(view),popup]{if(auto v=weak.get())if(auto c=v.CoreWebView2())c.NewWindowRequested(popup);});
  auto received=core.WebMessageReceived([this,valid,allowed](auto const&,CoreWebView2WebMessageReceivedEventArgs const& e){if(valid()&&std::wstring(e.Source())==allowed)homeWebAction(std::wstring(e.WebMessageAsJson()));});pluginEventCleanup[key].push_back([weak=make_weak(view),received]{if(auto v=weak.get())if(auto c=v.CoreWebView2())c.WebMessageReceived(received);});
  auto completed=core.NavigationCompleted([this,valid](auto const&,CoreWebView2NavigationCompletedEventArgs const& e){if(!valid())return;if(e.IsSuccess()){homeReady=true;lastHomeMessage.clear();syncHomeWeb();stage("Home WebUI ready");}else notice(L"主页加载失败，请检查本地 webui 文件。",true);});pluginEventCleanup[key].push_back([weak=make_weak(view),completed]{if(auto v=weak.get())if(auto c=v.CoreWebView2())c.NavigationCompleted(completed);});
  core.Navigate(allowed);
 }catch(hresult_error const& e){if(current==L"home"&&generation==renderGeneration)notice(L"主页无法加载："+std::wstring(e.message()),true);}
}
void homeWebPage(){homeView=WebView2{};homeView.Name(L"HomeWebView");homeView.DefaultBackgroundColor(dark?Windows::UI::Color{255,19,24,32}:Windows::UI::Color{255,247,248,250});homeView.Height(std::max(180.0,pageScroll.ViewportHeight()-page.Padding().Top-page.Padding().Bottom));page.Children().Append(homeView);onLoaded(homeView,[this](auto const&,auto const&){layoutHomeWeb();});loadHomeWeb(homeView,renderGeneration);}
