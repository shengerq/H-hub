DispatcherTimer sensorCapacityTimer{nullptr};unsigned sensorCapacityStep=0;JsonArray sensorCapacitySamples;std::vector<weak_ref<ListView>> capacityOldLists;
void makeSensorCapacityFixture(uint32_t count){
 JsonObject sample;JsonArray sensors;put(sample,L"sampleTime",clockSeconds());put(sample,L"sensorSampleTime",clockSeconds());put(sample,L"sensorReady",true);put(sample,L"cpuName",std::wstring(L"Synthetic many-core CPU"));put(sample,L"gpuName",std::wstring(L"Synthetic multi-GPU"));
 for(uint32_t i=0;i<count*2;++i){JsonObject sensor;put(sensor,L"id",L"capacity-"+std::to_wstring(i));put(sensor,L"name",L"Synthetic sensor "+std::to_wstring(i));put(sensor,L"hardware",std::wstring(i<count?L"CPU":L"GPU"));put(sensor,L"category",std::wstring(i<count?L"Cpu":L"GpuNvidia"));put(sensor,L"type",std::wstring(L"Load"));put(sensor,L"value",(double)(i%100));sensors.Append(sensor);}
 sample.SetNamedValue(L"sensors",sensors);state.SetNamedValue(L"monitor",sample);state.SetNamedValue(L"homeMonitoring",HomeMonitoring::build(state,clockSeconds()));go(L"system");render();
}
void runSensorCapacityCheck(){
 if(instanceSuffix().empty())throw hresult_invalid_argument(L"Capacity verification requires an isolated profile");
 monitoringFixture=true;monitoringActive=false;++monitorEpoch;stopMonitoringInBackground(monitorEpoch.load());
 sensorCapacityTimer=DispatcherTimer{};sensorCapacityTimer.Interval(std::chrono::seconds(3));
 sensorCapacityTimer.Tick([this](auto const&,auto const&){
  if(sensorCapacityStep){
   JsonObject sample;put(sample,L"step",(double)sensorCapacityStep);put(sample,L"workingSetMB",workingSet());PROCESS_MEMORY_COUNTERS_EX counters{};GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),sizeof(counters));put(sample,L"privateCommitMB",counters.PrivateUsage/1048576.0);
   JsonArray lists;bool passed=true;unsigned live=0;
   for(auto const& weak:sensorLists)if(auto list=weak.get()){++live;unsigned realized=0;for(uint32_t i=0;i<list.Items().Size();++i)if(list.ContainerFromIndex(i))++realized;JsonObject info;put(info,L"items",(double)list.Items().Size());put(info,L"realized",(double)realized);lists.Append(info);passed=passed&&realized>0&&realized<128;}
   if(sensorCapacityStep<4)passed=passed&&live==2;
   else{for(auto const& weak:capacityOldLists)if(weak.get())passed=false;put(sample,L"oldListsReleased",passed);}
   sample.SetNamedValue(L"lists",lists);put(sample,L"ok",passed);sensorCapacitySamples.Append(sample);
  }
  if(sensorCapacityStep==4){sensorCapacityTimer.Stop();JsonObject report;bool passed=true;for(auto const& value:sensorCapacitySamples)passed=passed&&flag(value.GetObject(),L"ok");put(report,L"ok",passed);put(report,L"syntheticData",true);report.SetNamedValue(L"samples",sensorCapacitySamples);write(dataDir()/L"sensor-capacity.json",std::wstring(report.Stringify()));monitoringFixture=false;window.Close();return;}
  if(sensorCapacityStep==0)makeSensorCapacityFixture(256);
  else if(sensorCapacityStep==1)makeSensorCapacityFixture(1024);
  else if(sensorCapacityStep==2){for(auto const& weak:sensorLists)if(auto list=weak.get())list.ScrollIntoView(box_value((uint32_t)1023));}
  else{capacityOldLists=sensorLists;go(L"market");}
  ++sensorCapacityStep;
 });sensorCapacityTimer.Start();
}
