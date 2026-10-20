#pragma once
#include <cstdint>
#include <string_view>
namespace hub {
inline bool monitoringPage(std::wstring_view page){return page==L"home"||page==L"system";}
// Only explicit monitoring requests renew the lease. Other page polling cannot
// keep the sensor process alive after a lost UI or a missed stop notification.
struct MonitoringDemand {
 static constexpr uint64_t timeoutMs=6000;
 uint64_t renewed{};bool active=false;
 void request(uint64_t now){renewed=now;active=true;}
 void stop(){active=false;}
 bool expired(uint64_t now)const{return active&&now-renewed>=timeoutMs;}
};
}
