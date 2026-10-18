// Exercises the actual model import and navigation/card creation, without devices or services.
DispatcherTimer nativeInstallTimer{nullptr};unsigned nativeInstallTick=0;Model nativeInstallModel{{},false};
void startNativeInstallMemoryCheck(){
 timer.Stop();profileWorkerTimer.Stop();state=nativeInstallModel.state;current=L"plugins";render();ShowWindow(hwnd,SW_HIDE);
 write(dataDir()/L"native-install-memory.csv",L"round,stage,privateMB,workingMB,retainedPages,callbacks\n");
 nativeInstallTimer=DispatcherTimer{};nativeInstallTimer.Interval(std::chrono::milliseconds(250));
 nativeInstallTimer.Tick([this](auto const&,auto const&){
  auto tick=++nativeInstallTick;auto stageIndex=(tick/20)%4;auto round=tick/80;
  if(tick%20==1){
   if(stageIndex==0){for(auto id:{L"eq",L"phone",L"windows"})if(pluginInstalled(nativeInstallModel.state,id))nativeInstallModel.request(object(std::wstring(L"{\"op\":\"pluginRemove\",\"body\":{\"id\":\"")+id+L"\"}}"));}
   else{auto name=stageIndex==1?L"Hhub-EQ-1.0.3.hhub":stageIndex==2?L"Hhub-Phone-Remote-1.0.9.hhub":L"Hhub-Windows-Settings-1.0.9.hhub";JsonObject req,body;body.SetNamedValue(L"manifest",object(read(nativeTestPackage(name))));req.SetNamedValue(L"body",body);put(req,L"op",std::wstring(L"importPlugin"));nativeInstallModel.request(req);}
   state=nativeInstallModel.state;syncNativeNav();render();
  }
  if(tick%20==19){PROCESS_MEMORY_COUNTERS_EX c{};GetProcessMemoryInfo(GetCurrentProcess(),(PROCESS_MEMORY_COUNTERS*)&c,sizeof(c));size_t alive=0;for(auto const& weak:retiredPages)if(weak.get())++alive;std::ofstream out(dataDir()/L"native-install-memory.csv",std::ios::app);out<<round<<","<<stageIndex<<","<<c.PrivateUsage/1048576.0<<","<<c.WorkingSetSize/1048576.0<<","<<alive<<","<<pageEventCleanup.size()+catalogEventCleanup.size()<<"\n";}
  if(tick>=320){nativeInstallTimer.Stop();nativeInstallTimer=nullptr;Exit();}
 });nativeInstallTimer.Start();
}
