#pragma once
#include "Common.h"
#include <cmath>
#include <map>
namespace hub {
struct MonitorProjection {
 static void number(JsonObject o,wchar_t const* key,double n){if(n<0||!std::isfinite(n))o.SetNamedValue(key,JsonValue::CreateNullValue());else put(o,key,n);}
 static double numeric(JsonObject o,wchar_t const* key){auto v=o.GetNamedValue(key,JsonValue::CreateNullValue());return v.ValueType()==JsonValueType::Number?v.GetNumber():-1;}
 static void aggregateSensors(JsonObject o,JsonArray a){
  for(auto key:{L"cpuTemp",L"cpuPower",L"cpuClock",L"gpuTemp",L"gpuPower",L"gpuClock",L"gpuFan",L"fan",L"memoryTemp",L"diskTemp"})number(o,key,-1);
  // Aggregate only sensors with the matching hardware and units. Keep raw names in details.
  double cpuTemp=-1,cpuClockSum=0;int clocks=0;
  for(auto const& v:a){auto s=v.GetObject();auto raw=s.GetNamedValue(L"value");if(raw.ValueType()!=JsonValueType::Number)continue;double n=raw.GetNumber();auto category=str(s,L"category"),type=str(s,L"type"),name=str(s,L"name"),hardware=str(s,L"hardware");
   if(category==L"Cpu"){
    if(type==L"Load"&&name==L"CPU Total")number(o,L"cpu",std::clamp(n,0.0,100.0));
    if(type==L"Temperature"&&name.find(L"Distance") == std::wstring::npos&&(name.find(L"Core")!=std::wstring::npos||name.find(L"Package")!=std::wstring::npos||name.find(L"Tctl")!=std::wstring::npos))cpuTemp=std::max(cpuTemp,n);
    if(type==L"Power"&&(name==L"CPU Package"||name==L"Package"))number(o,L"cpuPower",n);
    if(type==L"Clock"&&name.find(L"Core")!=std::wstring::npos){cpuClockSum+=n;clocks++;}
   }
   auto gpuName=str(o,L"gpuName");if(category.rfind(L"Gpu",0)==0&&(hardware==gpuName||hardware.find(gpuName)!=std::wstring::npos||gpuName.find(hardware)!=std::wstring::npos)){
    if(type==L"Temperature"&&name==L"GPU Core")number(o,L"gpuTemp",n);
    if(type==L"Clock"&&name==L"GPU Core")number(o,L"gpuClock",n);
    if(type==L"Power"&&(name==L"GPU Package"||name==L"GPU Power"))number(o,L"gpuPower",n);
    if(type==L"Fan")number(o,L"gpuFan",n);
   }
   if(category==L"Memory"&&type==L"Temperature")number(o,L"memoryTemp",n);
   if(category==L"Storage"&&type==L"Temperature"&&o.GetNamedValue(L"diskTemp").ValueType()==JsonValueType::Null)number(o,L"diskTemp",n);
  }number(o,L"cpuTemp",cpuTemp);if(clocks)number(o,L"cpuClock",cpuClockSum/clocks);
  aggregateMemoryStorage(o,a);


  bool limited=(!flag(o,L"sensorElevated")||!flag(o,L"pawnInstalled"))&&cpuTemp<0&&numeric(o,L"cpuPower")==0;
  put(o,L"cpuAccessLimited",limited);if(limited)number(o,L"cpuPower",-1);
  applyDeclaredMetrics(o,o.GetNamedObject(L"metrics",JsonObject{}));
  auto gpus=o.GetNamedArray(L"gpus",JsonArray{});
  for(uint32_t i=0;i<gpus.Size();i++){
   auto g=gpus.GetObjectAt(i);put(g,L"usageSource",std::wstring(L"LibreHardwareMonitor · GPU Core"));
   auto normalize=[](std::wstring name){std::transform(name.begin(),name.end(),name.begin(),towupper);name.erase(std::remove(name.begin(),name.end(),L' '),name.end());return name;};
   for(auto const& v:a){auto s=v.GetObject();auto raw=s.GetNamedValue(L"value",JsonValue::CreateNullValue());if(raw.ValueType()!=JsonValueType::Number)continue;auto hw=normalize(str(s,L"hardware")),adapter=normalize(str(g,L"name"));if(str(s,L"category").rfind(L"Gpu",0)!=0||hw.empty()||adapter.empty()||(hw!=adapter&&hw.find(adapter)==std::wstring::npos&&adapter.find(hw)==std::wstring::npos))continue;
    auto type=str(s,L"type"),name=str(s,L"name");double n=raw.GetNumber();
    if(type==L"Load"&&name==L"GPU Core"&&numeric(g,L"usage")<0){number(g,L"usage",std::clamp(n,0.0,100.0));put(g,L"usageSource",std::wstring(L"LibreHardwareMonitor · GPU Core"));}
    if(type==L"SmallData"&&name==L"D3D Dedicated Memory Used"&&numeric(g,L"usedGB")<0){number(g,L"usedGB",n/1024.0);put(g,L"memorySource",std::wstring(L"LibreHardwareMonitor · D3D Dedicated Memory Used"));}
    if(type==L"SmallData"&&name==L"GPU Memory Total")number(g,L"totalGB",n/1024.0);
   }
   if(i==0){if(numeric(g,L"usage")>=0)number(o,L"gpu",numeric(g,L"usage"));if(numeric(g,L"usedGB")>=0)number(o,L"gpuUsedGB",numeric(g,L"usedGB"));if(numeric(g,L"totalGB")>=0)number(o,L"gpuTotalGB",numeric(g,L"totalGB"));put(o,L"gpuUsageSource",str(g,L"usageSource"));}
  }
  JsonObject reasons,availability;bool elevated=flag(o,L"sensorElevated"),ready=flag(o,L"sensorReady");
  auto explain=[&](wchar_t const* key,std::wstring message){if(numeric(o,key)<0)put(reasons,key,message);};
  explain(L"cpuTemp",!flag(o,L"pawnInstalled")?L"Libre CPU 传感器需要 PawnIO；管理员授权不能代替驱动":!elevated?L"驱动已安装，等待 Windows 授权读取 CPU 温度与功耗":L"管理员采集未返回有效 CPU 温度；查看原始传感器和读取提示");
  explain(L"cpuPower",limited?L"CPU 读取受限，返回的 0 W 无法确认，已显示为空":L"采集器未返回有效 CPU 封装功耗");
  explain(L"gpu",L"LibreHardwareMonitor 未返回显卡利用率");
  explain(L"memory",L"LibreHardwareMonitor 未返回内存读数");
  for(auto key:{L"diskReadMB",L"diskWriteMB",L"diskUsedGB"})explain(key,L"硬件采集与 Windows 磁盘统计均未返回有效读数；查看读取提示");
  o.SetNamedValue(L"unavailableReasons",reasons);
  for(auto key:{L"cpuTemp",L"cpuClock",L"cpuPower",L"gpu",L"gpuClock",L"gpuTemp",L"gpuPower",L"memory",L"diskReadMB",L"diskWriteMB",L"diskUsedGB"}){
   if(numeric(o,key)>=0)put(availability,key,std::wstring(L"available"));
   else if(flag(o,L"sensorElevationPending"))put(availability,key,std::wstring(L"authorizing"));
   else if(!ready||(!elevated&&flag(o,L"sensorHasAdminSession")))put(availability,key,std::wstring(L"initializing"));
   else if(!flag(o,L"pawnInstalled")&&str(o,L"monitorBackend",L"librehardwaremonitor")==L"librehardwaremonitor"&&std::wstring(key).rfind(L"cpu",0)==0)put(availability,key,std::wstring(L"needsDriver"));
   else if(!elevated&&(std::wstring(key).rfind(L"cpu",0)==0||std::wstring(key)==L"fan"||std::wstring(key)==L"diskTemp"))put(availability,key,std::wstring(L"needsElevation"));
   else if(std::wstring(key)==L"fan")put(availability,key,std::wstring(L"unsupported"));
   else put(availability,key,std::wstring(L"unavailable"));
  }o.SetNamedValue(L"fieldAvailability",availability);
 }
 static void aggregateMemoryStorage(JsonObject o,JsonArray a){
  for(auto key:{L"memory",L"memoryUsedGB",L"memoryTotalGB",L"diskReadMB",L"diskWriteMB",L"diskUsedGB",L"diskTotalGB"})number(o,key,-1);
  double used=-1,free=-1;std::map<std::wstring,JsonObject> storage;
  for(auto const& v:a){auto s=v.GetObject();auto category=str(s,L"category"),id=str(s,L"hardwareId"),name=str(s,L"name"),type=str(s,L"type");double n=numeric(s,L"value");
   if(category==L"Memory"&&id==L"/ram"&&n>=0){if(type==L"Data"&&name==L"Memory Used")used=n;if(type==L"Data"&&name==L"Memory Available")free=n;if(type==L"Load"&&name==L"Memory")number(o,L"memory",n);}
   if(category!=L"Storage")continue;if(id.empty())id=str(s,L"hardware");auto [it,inserted]=storage.try_emplace(id,JsonObject{});auto d=it->second;if(inserted){put(d,L"id",id);put(d,L"name",str(s,L"hardware"));for(auto key:{L"totalGB",L"freeGB",L"usedGB",L"usage",L"readMB",L"writeMB"})number(d,key,-1);}if(n<0)continue;
   if(type==L"Data"&&name==L"Total Space")number(d,L"totalGB",n);if(type==L"Data"&&name==L"Free Space")number(d,L"freeGB",n);if(type==L"Load"&&name==L"Used Space")number(d,L"usage",n);
   if(type==L"Throughput"&&name==L"Read Rate")number(d,L"readMB",n/1048576.0);if(type==L"Throughput"&&name==L"Write Rate")number(d,L"writeMB",n/1048576.0);
  }
  number(o,L"memoryUsedGB",used);if(used>=0&&free>=0){number(o,L"memoryTotalGB",used+free);if(numeric(o,L"memory")<0&&used+free>0)number(o,L"memory",used/(used+free)*100);}
  JsonArray disks;double read=0,write=0,total=0,occupied=0;bool reads=!storage.empty(),writes=reads,capacity=reads;
  for(auto const& [id,d]:storage){double t=numeric(d,L"totalGB"),f=numeric(d,L"freeGB"),load=numeric(d,L"usage");if(t>=0&&f>=0&&f<=t)number(d,L"usedGB",t-f);else if(t>=0&&load>=0&&load<=100)number(d,L"usedGB",t*load/100);double u=numeric(d,L"usedGB"),r=numeric(d,L"readMB"),w=numeric(d,L"writeMB");if(r<0)reads=false;else read+=r;if(w<0)writes=false;else write+=w;if(t<0||u<0)capacity=false;else{total+=t;occupied+=u;}disks.Append(d);}
  o.SetNamedValue(L"disks",disks);number(o,L"diskReadMB",reads?read:-1);number(o,L"diskWriteMB",writes?write:-1);number(o,L"diskUsedGB",capacity?occupied:-1);number(o,L"diskTotalGB",capacity?total:-1);put(o,L"memorySource",std::wstring(L"LibreHardwareMonitor · /ram"));put(o,L"diskSource",std::wstring(L"LibreHardwareMonitor · Storage"));
 }
 static void applyDeclaredMetrics(JsonObject o,JsonObject metrics){
  JsonObject sources;
  for(auto const& pair:{std::pair{L"cpu",L"%"},std::pair{L"cpuTemp",L"C"},std::pair{L"cpuPower",L"W"},std::pair{L"cpuClock",L"MHz"},std::pair{L"gpu",L"%"},std::pair{L"gpuTemp",L"C"},std::pair{L"gpuPower",L"W"},std::pair{L"gpuClock",L"MHz"},std::pair{L"gpuUsedGB",L"GiB"},std::pair{L"gpuTotalGB",L"GiB"},std::pair{L"memory",L"%"},std::pair{L"memoryUsedGB",L"GiB"},std::pair{L"memoryTotalGB",L"GiB"},std::pair{L"diskReadMB",L"MiB/s"},std::pair{L"diskWriteMB",L"MiB/s"},std::pair{L"diskUsedGB",L"GB"},std::pair{L"diskTotalGB",L"GB"}}){
   auto metric=metrics.GetNamedObject(pair.first,JsonObject{});double value=numeric(metric,L"value");
   if(value<0||str(metric,L"unit")!=pair.second)continue;
   if((std::wstring(pair.second)==L"%"&&value>100)||(std::wstring(pair.first)==L"cpuTemp"&&value>150))continue;
   if(numeric(o,pair.first)<0)number(o,pair.first,value);
   if(numeric(o,pair.first)==value)put(sources,pair.first,str(metric,L"source"));
  }
  o.SetNamedValue(L"metricSources",sources);
  if(str(sources,L"diskReadMB")==L"windows-storage"||str(sources,L"diskUsedGB")==L"windows-storage")put(o,L"diskSource",std::wstring(L"Windows 磁盘统计（仅补充缺失项）"));
 }
};
}
