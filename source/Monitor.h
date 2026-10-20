#pragma once
#include "SensorTransport.h"
namespace hub {
struct Monitor : SensorTransport {
 static bool needsAutoElevation(JsonObject o,bool attempted){
  // Default to the authorized sensor helper when the signed driver is ready.
  // A cancelled prompt stays cancelled for this core session.
  return !attempted&&flag(o,L"sensorReady")&&flag(o,L"pawnInstalled")&&!flag(o,L"sensorElevated")&&!flag(o,L"sensorHasAdminSession")&&!flag(o,L"sensorElevationPending");
 }
 JsonObject sample(bool allowAutoElevation=false){
  if(mousePortTestProfile())allowAutoElevation=false;
  auto now=GetTickCount64();if(cache&&now-sampled<1000&&flag(cache,L"sensorReady")&&!flag(cache,L"sensorElevationPending")&&clockSeconds()-num(cache,L"sensorSampleTime")<5&&!(allowAutoElevation&&needsAutoElevation(cache,autoElevationAttempted)))return cache;sampled=now;JsonObject o;put(o,L"real",true);put(o,L"sampleTime",clockSeconds());put(o,L"cpuName",cpuName);
  number(o,L"cpu",-1);JsonArray gpus;
  for(auto const& adapter:adapters){JsonObject g;put(g,L"name",adapter.name);number(g,L"usage",-1);number(g,L"usedGB",-1);number(g,L"totalGB",-1);gpus.Append(g);}
  o.SetNamedValue(L"gpus",gpus);put(o,L"gpuName",adapters.empty()?std::wstring(L"未识别显卡"):adapters[0].name);number(o,L"gpu",-1);number(o,L"gpuUsedGB",-1);number(o,L"gpuTotalGB",-1);
  sensors(o);
  aggregateSensors(o,o.GetNamedArray(L"sensors",JsonArray{}));
  if(allowAutoElevation&&needsAutoElevation(o,autoElevationAttempted)){
   try{startElevated();}catch(hresult_error const& e){autoElevationAttempted=true;elevationError=std::wstring(e.message());}catch(...){autoElevationAttempted=true;elevationError=L"自动授权采集启动失败，可手动重试";}
   put(o,L"sensorElevationPending",elevation!=nullptr);put(o,L"sensorElevationError",elevationError);
  }
  put(o,L"sensorAutoElevationAttempted",autoElevationAttempted);cache=o;return o;
 }
};
}
