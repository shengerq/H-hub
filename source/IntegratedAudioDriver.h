#pragma once
#include "DeviceReader.h"
#include "EqAudio.h"
#include "AudioNodeName.h"
#include <newdev.h>
#pragma comment(lib,"newdev.lib")
namespace hub {
inline int installIntegratedAudioDriver(std::filesystem::path inf,std::filesystem::path report,bool verifyOnly=false){
 JsonObject result;HDEVINFO set=INVALID_HANDLE_VALUE;SP_DEVINFO_DATA device{sizeof(device)};bool created=false;
 try{
  inf=std::filesystem::absolute(inf);put(result,L"driver",std::wstring(L"HHUBVAD"));put(result,L"phase",std::wstring(L"verify"));
  if(inf.filename()!=L"fxvad.inf"||!std::filesystem::is_regular_file(inf))throw hresult_invalid_argument(L"缺少 H hub 专用音频驱动包");
  auto contents=read(inf);bool upstream=contents.find(L"DEV_NAME=\"FXVAD\"")!=std::wstring::npos&&contents.find(L"FxSound Audio Enhancer")!=std::wstring::npos;bool native=contents.find(L"DEV_NAME=\"HHUBVAD\"")!=std::wstring::npos&&contents.find(L"H hub Audio")!=std::wstring::npos;if(!upstream&&!native)throw hresult_invalid_argument(L"驱动设备标识不匹配");std::wstring driverId=upstream?L"FXVAD":L"HHUBVAD",hardwareId=L"ROOT\\"+driverId,deviceName=upstream?L"FxSound Audio Enhancer":L"H hub Audio";put(result,L"driver",driverId);
  SP_INF_SIGNER_INFO_W signer{};signer.cbSize=sizeof(signer);
  if(!SetupVerifyInfFileW(inf.c_str(),nullptr,&signer))throw hresult_error(HRESULT_FROM_WIN32(GetLastError()),L"驱动包签名校验未通过；不能安装未签名开发包");
  GUID cls{},media{};wchar_t className[256]{};CLSIDFromString(L"{4d36e96c-e325-11ce-bfc1-08002be10318}",&media);
  if(!SetupDiGetINFClassW(inf.c_str(),&cls,className,256,nullptr)||cls!=media)throw hresult_invalid_argument(L"驱动不是音频设备类");
  if(verifyOnly){put(result,L"ok",true);put(result,L"phase",std::wstring(L"verified"));write(report,std::wstring(result.Stringify()));return 0;}put(result,L"phase",std::wstring(L"install"));set=SetupDiGetClassDevsW(&media,nullptr,nullptr,DIGCF_PRESENT);
  bool found=false;if(set!=INVALID_HANDLE_VALUE){for(DWORD i=0;;i++){SP_DEVINFO_DATA entry{sizeof(entry)};if(!SetupDiEnumDeviceInfo(set,i,&entry))break;auto ids=DeviceReader::upper(DeviceReader::prop(set,entry,SPDRP_HARDWAREID));if(ids==DeviceReader::upper(hardwareId)){found=true;break;}}SetupDiDestroyDeviceInfoList(set);set=INVALID_HANDLE_VALUE;}
  if(!found){set=SetupDiCreateDeviceInfoList(&media,nullptr);if(set==INVALID_HANDLE_VALUE)throw hresult_error(HRESULT_FROM_WIN32(GetLastError()));
   if(!SetupDiCreateDeviceInfoW(set,driverId.c_str(),&media,deviceName.c_str(),nullptr,DICD_GENERATE_ID,&device))throw hresult_error(HRESULT_FROM_WIN32(GetLastError()));
   std::vector<wchar_t> hardware(hardwareId.begin(),hardwareId.end());hardware.push_back(0);hardware.push_back(0);
   if(!SetupDiSetDeviceRegistryPropertyW(set,&device,SPDRP_HARDWAREID,(BYTE*)hardware.data(),(DWORD)(hardware.size()*sizeof(wchar_t)))||!SetupDiCallClassInstaller(DIF_REGISTERDEVICE,set,&device))throw hresult_error(HRESULT_FROM_WIN32(GetLastError()));created=true;
  }
  EqNode::physicalDefaults();auto physicalRoles=EqNode::physicalRoleDefaults();BOOL reboot{};if(!UpdateDriverForPlugAndPlayDevicesW(nullptr,hardwareId.c_str(),inf.c_str(),0,&reboot))throw hresult_error(HRESULT_FROM_WIN32(GetLastError()));
  EqNode::physicalDefaults(L"",physicalRoles);put(result,L"nameUpdated",renameIntegratedAudioNode({})==0);put(result,L"ok",true);put(result,L"phase",std::wstring(L"complete"));put(result,L"repair",found);put(result,L"rebootRequired",reboot!=FALSE);write(report,std::wstring(result.Stringify()));if(set!=INVALID_HANDLE_VALUE)SetupDiDestroyDeviceInfoList(set);return reboot?3010:0;
 }catch(hresult_error const& e){put(result,L"ok",false);put(result,L"error",std::wstring(e.message()));put(result,L"hresult",(double)e.code().value);if(created)SetupDiCallClassInstaller(DIF_REMOVE,set,&device);if(set!=INVALID_HANDLE_VALUE)SetupDiDestroyDeviceInfoList(set);write(report,std::wstring(result.Stringify()));return 1;}
}
}




