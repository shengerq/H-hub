#pragma once
#include "DeviceReader.h"
#include "EqAudio.h"
#include "AudioEndpointBranding.h"
#include <newdev.h>
#include <cfgmgr32.h>
#pragma comment(lib,"newdev.lib")
#pragma comment(lib,"cfgmgr32.lib")
namespace hub {
inline int installIntegratedAudioDriver(std::filesystem::path inf,std::filesystem::path report,bool verifyOnly=false){
 JsonObject result;HDEVINFO set=INVALID_HANDLE_VALUE;SP_DEVINFO_DATA device{sizeof(device)};bool created=false;
 try{
  inf=std::filesystem::absolute(inf);put(result,L"driver",std::wstring(L"FXVAD"));put(result,L"phase",std::wstring(L"verify"));
  if(inf.filename()!=L"fxvad.inf"||!std::filesystem::is_regular_file(inf))throw hresult_invalid_argument(L"缺少 FxSound 原版签名驱动包");
  auto contents=read(inf);if(contents.find(L"DEV_NAME=\"FXVAD\"")==std::wstring::npos||contents.find(L"FxSound Audio Enhancer")==std::wstring::npos)throw hresult_invalid_argument(L"仅支持未经修改的 FxSound 原版驱动包");
  std::wstring driverId=L"FXVAD",hardwareId=L"ROOT\\FXVAD";
  SP_INF_SIGNER_INFO_W signer{};signer.cbSize=sizeof(signer);
  if(!SetupVerifyInfFileW(inf.c_str(),nullptr,&signer))throw hresult_error(HRESULT_FROM_WIN32(GetLastError()),L"驱动包签名校验未通过；不能安装未签名开发包");
  GUID cls{},media{};wchar_t className[256]{};CLSIDFromString(L"{4d36e96c-e325-11ce-bfc1-08002be10318}",&media);
  if(!SetupDiGetINFClassW(inf.c_str(),&cls,className,256,nullptr)||cls!=media)throw hresult_invalid_argument(L"驱动不是音频设备类");
  if(verifyOnly){put(result,L"ok",true);put(result,L"phase",std::wstring(L"verified"));write(report,std::wstring(result.Stringify()));return 0;}
  put(result,L"phase",std::wstring(L"inspect"));set=SetupDiGetClassDevsW(&media,nullptr,nullptr,DIGCF_PRESENT);
  bool found=false,healthy=false;
  if(set!=INVALID_HANDLE_VALUE){for(DWORD i=0;;i++){SP_DEVINFO_DATA entry{sizeof(entry)};if(!SetupDiEnumDeviceInfo(set,i,&entry))break;if(DeviceReader::upper(DeviceReader::prop(set,entry,SPDRP_HARDWAREID))==hardwareId){found=true;ULONG status{},problem{};healthy=CM_Get_DevNode_Status(&status,&problem,entry.DevInst,0)==CR_SUCCESS&&reuseExistingSharedDriver(true,(status&DN_STARTED)!=0,problem);break;}}SetupDiDestroyDeviceInfoList(set);set=INVALID_HANDLE_VALUE;}
  BOOL reboot{};
  if(!healthy){
   if(fxSoundIsRunning())throw hresult_error(E_ABORT,L"修复或安装共享驱动前请先退出 FxSound；已有正常驱动时无需更新");
   put(result,L"phase",std::wstring(L"install"));
   if(!found){set=SetupDiCreateDeviceInfoList(&media,nullptr);if(set==INVALID_HANDLE_VALUE)throw hresult_error(HRESULT_FROM_WIN32(GetLastError()));
    if(!SetupDiCreateDeviceInfoW(set,driverId.c_str(),&media,sharedAudioOriginalName,nullptr,DICD_GENERATE_ID,&device))throw hresult_error(HRESULT_FROM_WIN32(GetLastError()));
    std::vector<wchar_t> hardware(hardwareId.begin(),hardwareId.end());hardware.push_back(0);hardware.push_back(0);
    if(!SetupDiSetDeviceRegistryPropertyW(set,&device,SPDRP_HARDWAREID,(BYTE*)hardware.data(),(DWORD)(hardware.size()*sizeof(wchar_t)))||!SetupDiCallClassInstaller(DIF_REGISTERDEVICE,set,&device))throw hresult_error(HRESULT_FROM_WIN32(GetLastError()));created=true;
   }
   if(!UpdateDriverForPlugAndPlayDevicesW(nullptr,hardwareId.c_str(),inf.c_str(),0,&reboot))throw hresult_error(HRESULT_FROM_WIN32(GetLastError()));created=false;
  }
  put(result,L"driverInstalled",true);put(result,L"reusedExisting",healthy);put(result,L"sharedDriver",true);put(result,L"phase",std::wstring(L"rename"));
  int devicesNamed=0;auto namingSet=SetupDiGetClassDevsW(&media,nullptr,nullptr,DIGCF_PRESENT);
  if(namingSet==INVALID_HANDLE_VALUE)throw hresult_error(HRESULT_FROM_WIN32(GetLastError()));
  try{for(DWORD i=0;;++i){SP_DEVINFO_DATA entry{sizeof(entry)};if(!SetupDiEnumDeviceInfo(namingSet,i,&entry))break;if(DeviceReader::upper(DeviceReader::prop(namingSet,entry,SPDRP_HARDWAREID))!=hardwareId)continue;
   if(!SetupDiSetDeviceRegistryPropertyW(namingSet,&entry,SPDRP_FRIENDLYNAME,reinterpret_cast<BYTE const*>(hhubAudioDisplayName),sizeof(hhubAudioDisplayName)))throw hresult_error(HRESULT_FROM_WIN32(GetLastError()));
   if(DeviceReader::prop(namingSet,entry,SPDRP_FRIENDLYNAME)!=hhubAudioDisplayName)throw hresult_error(E_FAIL,L"EQ 驱动设备名称回读失败");++devicesNamed;
  }}catch(...){SetupDiDestroyDeviceInfoList(namingSet);throw;}SetupDiDestroyDeviceInfoList(namingSet);
  int visible=0,persisted=0;for(unsigned attempt=0;attempt<20;++attempt){visible=brandVisibleAudioEndpoints();persisted=visible?0:brandStoredAudioEndpoints();if(visible+persisted>0)break;Sleep(250);}
  if(!devicesNamed)throw hresult_error(E_FAIL,L"未找到已安装的 EQ 驱动设备");
  put(result,L"renamedDevices",(double)devicesNamed);put(result,L"renamedEndpoints",(double)(visible+persisted));put(result,L"nameUpdated",true);put(result,L"renameVerified",true);put(result,L"endpointNamePending",visible+persisted==0);
  if(!visible&&!persisted)reboot=TRUE;
  put(result,L"displayName",std::wstring(hhubAudioDisplayName));put(result,L"driverSource",std::wstring(L"FxSound official signed driver"));
  put(result,L"ok",true);put(result,L"phase",std::wstring(L"complete"));put(result,L"repair",found&&!healthy);put(result,L"rebootRequired",reboot!=FALSE);write(report,std::wstring(result.Stringify()));if(set!=INVALID_HANDLE_VALUE)SetupDiDestroyDeviceInfoList(set);return reboot?3010:0;
 }catch(hresult_error const& e){put(result,L"ok",false);put(result,L"error",std::wstring(e.message()));put(result,L"hresult",(double)e.code().value);if(created)SetupDiCallClassInstaller(DIF_REMOVE,set,&device);if(set!=INVALID_HANDLE_VALUE)SetupDiDestroyDeviceInfoList(set);write(report,std::wstring(result.Stringify()));return 1;}
}
}
