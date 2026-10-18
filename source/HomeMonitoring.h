#pragma once
#include "Common.h"
#include <cmath>

namespace hub {
// Versioned read-only contract shared by the native homepage and core service.
struct HomeMonitoring {
 static JsonObject trendSample(JsonObject sample,JsonObject snapshot){
  JsonObject result,fields;result.SetNamedValue(L"sampleTime",sample.GetNamedValue(L"sampleTime",JsonValue::CreateNullValue()));
  auto source=snapshot.GetNamedObject(L"fields",JsonObject{});
  for(auto key:{L"cpu",L"gpu",L"memory"}){result.SetNamedValue(key,sample.GetNamedValue(key,JsonValue::CreateNullValue()));JsonObject field;field.SetNamedValue(L"value",source.GetNamedObject(key,JsonObject{}).GetNamedValue(L"value",JsonValue::CreateNullValue()));fields.SetNamedValue(key,field);}
  result.SetNamedValue(L"homeFields",fields);return result;
 }
 static double number(JsonObject object,wchar_t const* key){
  auto value=object.GetNamedValue(key,JsonValue::CreateNullValue());
  return value.ValueType()==JsonValueType::Number&&std::isfinite(value.GetNumber())?value.GetNumber():-1;
 }
 static JsonObject build(JsonObject state,double now){
  auto sample=state.GetNamedObject(L"monitor",JsonObject{});
  JsonObject result,fields;put(result,L"version",1.0);
  double timestamp=number(sample,L"sensorSampleTime");
  bool ready=flag(sample,L"sensorReady"),stale=ready&&(timestamp<=0||now-timestamp>=5||timestamp>now+1);
  put(result,L"sampleTime",number(sample,L"sampleTime"));put(result,L"sensorSampleTime",timestamp);
  put(result,L"status",std::wstring(!ready?L"initializing":stale?L"stale":L"live"));
  put(result,L"virtualMachine",flag(sample,L"virtualMachine"));
  auto availability=sample.GetNamedObject(L"fieldAvailability",JsonObject{});
  auto reasons=sample.GetNamedObject(L"unavailableReasons",JsonObject{});
  auto sources=sample.GetNamedObject(L"metricSources",JsonObject{});
  for(auto key:{L"cpu",L"cpuClock",L"cpuTemp",L"cpuPower",L"gpu",L"gpuClock",L"gpuTemp",L"gpuPower",L"gpuUsedGB",L"gpuTotalGB",L"memory",L"memoryUsedGB",L"memoryTotalGB",L"diskReadMB",L"diskWriteMB",L"diskUsedGB",L"diskTotalGB"}){
   JsonObject field;double value=number(sample,key);std::wstring name=key;
   bool percent=name==L"cpu"||name==L"gpu"||name==L"memory";
   bool valid=value>=0&&(!percent||value<=100);
   auto status=!ready?std::wstring(L"initializing"):stale?std::wstring(L"stale"):valid?std::wstring(L"available"):str(availability,key,L"unavailable");
   field.SetNamedValue(L"value",valid&&ready&&!stale?JsonValue::CreateNumberValue(value):JsonValue::CreateNullValue());
   put(field,L"status",status);put(field,L"reason",stale?std::wstring(L"传感器数据已过期，等待新的采样"):str(reasons,key,L"当前硬件未返回此读数"));
   put(field,L"source",str(sources,key,str(sample,L"monitorBackend",L"LibreHardwareMonitor").c_str()));fields.SetNamedValue(key,field);
  }
  result.SetNamedValue(L"fields",fields);
  JsonArray devices;auto detected=state.GetNamedObject(L"detected",JsonObject{});
  for(auto const& value:detected.GetNamedArray(L"peripherals",JsonArray{})){
   auto device=value.GetObject();auto item=object(std::wstring(device.Stringify()));
   for(auto key:{L"id",L"name",L"kind",L"status",L"connection",L"profile",L"batterySource"})put(item,key,str(device,key));
   put(item,L"connected",flag(device,L"connected"));double battery=number(device,L"battery");
   bool valid=battery>=0&&battery<=100&&(!device.HasKey(L"connected")||flag(device,L"connected"));
   item.SetNamedValue(L"battery",valid?JsonValue::CreateNumberValue(battery):JsonValue::CreateNullValue());
   put(item,L"batteryStatus",std::wstring(valid?L"available":L"unavailable"));
   put(item,L"lowBattery",valid&&flag(device,L"lowBattery"));
   put(item,L"charging",valid&&flag(device,L"batteryIsCharging"));devices.Append(item);
  }
  result.SetNamedValue(L"devices",devices);put(result,L"deviceSampleTime",number(detected,L"sampleTime"));return result;
 }
};
}
