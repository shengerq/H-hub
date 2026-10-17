
// Opt-in isolated real plugin UI measurement; Common.h blocks all actual device IPC.
DispatcherTimer pluginMemoryTimer{nullptr};unsigned pluginMemoryTick=0;
winrt::fire_and_forget inspectPluginMemory(unsigned tick){
 auto lifetime=get_strong();auto view=pluginView;auto generation=renderGeneration;
 try{if(!view||!view.CoreWebView2())co_return;auto core=view.CoreWebView2();
 auto dom=co_await core.ExecuteScriptAsync(L"JSON.stringify({ready:document.readyState,title:document.title,nodes:document.getElementsByTagName('*').length,text:document.body.innerText.substring(0,220),width:innerWidth,height:innerHeight})");
 co_await core.CallDevToolsProtocolMethodAsync(L"Performance.enable",L"{}");
 auto metrics=co_await core.CallDevToolsProtocolMethodAsync(L"Performance.getMetrics",L"{}");
 auto counters=co_await core.CallDevToolsProtocolMethodAsync(L"Memory.getDOMCounters",L"{}");
 if(generation!=renderGeneration)co_return;
 JsonObject result;put(result,L"tick",(double)tick);put(result,L"dom",std::wstring(dom));put(result,L"metrics",std::wstring(metrics));put(result,L"counters",std::wstring(counters));
 write(dataDir()/(L"plugin-memory-dom-"+std::to_wstring(tick)+L".json"),std::wstring(result.Stringify()));
 }catch(hresult_error const& e){stage(("Plugin memory inspect error: "+to_string(e.message())).c_str());}
}
void startPluginMemoryCheck(){
 go(L"plugins");write(dataDir()/L"plugin-memory-phase.txt",L"baseline");
 pluginMemoryTimer=DispatcherTimer{};pluginMemoryTimer.Interval(std::chrono::seconds(1));
 pluginMemoryTimer.Tick([this](auto const&,auto const&){
  auto tick=++pluginMemoryTick;
  if(tick==15||tick==95){devicePluginId=L"hhub.rapoo-vt3s-max";go(L"device-plugin");write(dataDir()/L"plugin-memory-phase.txt",tick==15?L"loaded-1":L"loaded-2");}
  if(tick==65||tick==145){go(L"plugins");write(dataDir()/L"plugin-memory-phase.txt",tick==65?L"unloaded-1":L"unloaded-2");}
  if(tick==25||tick==55||tick==105||tick==135)inspectPluginMemory(tick);
  if(tick>=175){pluginMemoryTimer.Stop();pluginMemoryTimer=nullptr;write(dataDir()/L"plugin-memory-phase.txt",L"complete");Exit();}
 });pluginMemoryTimer.Start();
}
