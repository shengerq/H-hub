DispatcherTimer installedMemoryTimer{nullptr};unsigned installedMemoryStep=0;bool installedMemoryBusy=false;JsonArray installedMemorySamples;
void runInstalledMemoryCheck(){
 if(instanceSuffix().empty())throw hresult_invalid_argument(L"Installed memory verification requires an isolated profile");
 static std::wstring const phases[]{L"home",L"minimized",L"restored",L"windows",L"eq",L"phone",L"system",L"market"};
 auto stage=[this](std::wstring phase){JsonObject value;put(value,L"phase",phase);put(value,L"corePid",num(state,L"corePid"));write(dataDir()/L"memory-stage.json",std::wstring(value.Stringify()));};stage(phases[0]);
 installedMemoryTimer=DispatcherTimer{};installedMemoryTimer.Interval(std::chrono::seconds(7));
 installedMemoryTimer.Tick([this,stage](auto const&,auto const&){
  if(installedMemoryBusy)return;installedMemoryBusy=true;struct Reset{bool& value;~Reset(){value=false;}}reset{installedMemoryBusy};
  JsonObject sample;put(sample,L"phase",phases[installedMemoryStep]);put(sample,L"workingSetMB",workingSet());PROCESS_MEMORY_COUNTERS_EX counters{};GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),sizeof(counters));put(sample,L"privateCommitMB",counters.PrivateUsage/1048576.0);put(sample,L"builtinPageInMainUi",nativeSurface&&nativeSurface->local);installedMemorySamples.Append(sample);
  JsonObject report;report.SetNamedValue(L"samples",installedMemorySamples);put(report,L"corePid",num(state,L"corePid"));write(dataDir()/L"memory-ui.json",std::wstring(report.Stringify()));
  if(++installedMemoryStep==std::size(phases)){installedMemoryTimer.Stop();stage(L"ui-closed");window.Close();return;}
  auto next=phases[installedMemoryStep];
  if(next==L"minimized")ShowWindow(hwnd,SW_MINIMIZE);
  else if(next==L"restored")ShowWindow(hwnd,SW_RESTORE);
  else go(next);
  stage(next);
 });installedMemoryTimer.Start();
}
