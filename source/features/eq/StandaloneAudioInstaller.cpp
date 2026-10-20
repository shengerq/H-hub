#include "IntegratedAudioDriver.h"
using namespace hub;
static std::filesystem::path extractBundle(HINSTANCE instance){
 wchar_t temporary[32768]{};if(!GetTempPathW(32768,temporary))throw hresult_error(HRESULT_FROM_WIN32(GetLastError()));
 auto folder=std::filesystem::path(temporary)/(L"HhubAudioDriver-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));std::filesystem::create_directories(folder);
 struct Item{int id;wchar_t const* name;};
 for(auto item:{Item{201,L"fxvad.inf"},Item{202,L"fxvad.sys"},Item{203,L"fxvadntamd64.cat"},Item{204,L"LICENSE.txt"},Item{205,L"license.rtf"}}){
  auto resource=FindResourceW(instance,MAKEINTRESOURCEW(item.id),RT_RCDATA);if(!resource)throw hresult_error(E_FAIL,L"驱动安装包缺少文件");auto data=LoadResource(instance,resource);auto bytes=SizeofResource(instance,resource);auto source=LockResource(data);if(!source||!bytes)throw hresult_error(E_FAIL,L"驱动文件损坏");std::ofstream output(folder/item.name,std::ios::binary);output.write((char const*)source,bytes);if(!output)throw hresult_error(E_FAIL,L"无法解压驱动文件");
 }return folder;
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR,int){
 winrt::init_apartment(winrt::apartment_type::multi_threaded);std::filesystem::path folder,report;bool verify=false,fromPlugin=false;int count{};auto args=CommandLineToArgvW(GetCommandLineW(),&count);
 if(args){if(count==3&&std::wstring(args[1])==L"--verify-only"){verify=true;report=args[2];}else if(count==3&&(std::wstring(args[1])==L"--from-plugin"||std::wstring(args[1])==L"--from-setup")){fromPlugin=true;report=args[2];}LocalFree(args);}
 if(!verify&&!fromPlugin&&MessageBoxW(nullptr,L"安装或修复 Hhub 虚拟音频驱动（Windows x64）。\n\n使用 FxSound 官方签名驱动，显示名称设为 H hub Audio。已有正常驱动时直接复用，并校验改名结果。",L"Hhub 虚拟音频驱动独立安装",MB_OKCANCEL|MB_ICONINFORMATION)!=IDOK)return 0;
 try{
  folder=extractBundle(instance);if(report.empty())report=dataDir()/L"audio-driver-standalone-install.json";
  auto code=installIntegratedAudioDriver(folder/L"fxvad.inf",report,verify);auto result=object(read(report));
  if(!verify&&!fromPlugin){auto message=flag(result,L"ok")?(code==3010?std::wstring(L"驱动安装完成。请重启 Windows 后使用 Hhub EQ。"):std::wstring(L"驱动安装完成。可以打开 Hhub 使用 EQ。")):str(result,L"error",L"驱动安装未完成。请查看安装日志。");MessageBoxW(nullptr,message.c_str(),L"Hhub 虚拟音频驱动",flag(result,L"ok")?MB_OK|MB_ICONINFORMATION:MB_OK|MB_ICONERROR);}
  std::error_code error;std::filesystem::remove_all(folder,error);return code;
 }catch(hresult_error const& error){if(!verify&&!fromPlugin)MessageBoxW(nullptr,error.message().c_str(),L"Hhub 虚拟音频驱动",MB_OK|MB_ICONERROR);if(!report.empty()){JsonObject result;put(result,L"ok",false);put(result,L"error",std::wstring(error.message()));write(report,std::wstring(result.Stringify()));}return 1;}
}
