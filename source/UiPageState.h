#pragma once
#include "Common.h"
#include "MonitoringDemand.h"
namespace hub {
inline void releaseInactiveUiState(JsonObject state,std::wstring const& page){
 if(!monitoringPage(page))for(auto key:{L"monitor",L"homeMonitoring"})if(state.HasKey(key))state.Remove(key);
 if(page!=L"scenes"&&state.HasKey(L"applications"))state.Remove(L"applications");
 if(page!=L"plugins"&&page!=L"market"&&page!=L"settings"&&state.HasKey(L"updates"))state.Remove(L"updates");
}
}
