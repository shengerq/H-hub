#pragma once
// Runs without launching hardware readers or Windows authorization prompts.
namespace hub {
inline int verifySensorAuthorization(){
 JsonArray checks;bool passed=true;
 auto check=[&](std::wstring name,bool ok){JsonObject item;put(item,L"name",name);put(item,L"ok",ok);checks.Append(item);passed=passed&&ok;};
 auto sample=[](bool ready,bool driver,bool elevated,bool session,bool pending){JsonObject o;put(o,L"sensorReady",ready);put(o,L"pawnInstalled",driver);put(o,L"sensorElevated",elevated);put(o,L"sensorHasAdminSession",session);put(o,L"sensorElevationPending",pending);o.SetNamedValue(L"gpus",JsonArray{});MonitorProjection::aggregateSensors(o,JsonArray{});return o;};
 auto state=[](JsonObject o){return str(o.GetNamedObject(L"fieldAvailability"),L"cpuTemp");};
 check(L"ordinary reader needs authorization",state(sample(true,true,false,false,false))==L"needsElevation");
 check(L"pending authorization is distinct",state(sample(true,true,false,false,true))==L"authorizing");
 check(L"connected administrator waits for first sample",state(sample(false,true,false,true,false))==L"initializing");
 check(L"missing driver is distinct",state(sample(true,false,false,false,false))==L"needsDriver");
 check(L"authorized reader without temperature is not authorization failure",state(sample(true,true,true,true,false))==L"unavailable");
 auto ordinary=sample(true,true,false,false,false);
 check(L"first automatic authorization allowed",Monitor::needsAutoElevation(ordinary,false));
 check(L"cancelled authorization does not prompt repeatedly",!Monitor::needsAutoElevation(ordinary,true));
 check(L"pending authorization cannot start twice",!Monitor::needsAutoElevation(sample(true,true,false,false,true),false));
 check(L"administrator session cannot start twice",!Monitor::needsAutoElevation(sample(true,true,false,true,false),false));
 JsonObject report;put(report,L"ok",passed);put(report,L"syntheticData",true);report.SetNamedValue(L"checks",checks);write(dataDir()/L"sensor-authorization-check.json",std::wstring(report.Stringify()));return passed?0:1;
}
}
