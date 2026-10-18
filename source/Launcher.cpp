#include <windows.h>
#include <filesystem>
#include <string>
// Compatibility entry point for old shortcuts. The launcher exits immediately;
// normal shortcuts and the tray open runtime/Hhub.exe directly.
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR arguments,int show){
 wchar_t executable[32768]{};GetModuleFileNameW(nullptr,executable,32768);
 auto application=std::filesystem::path(executable).parent_path();auto ui=application/L"runtime"/L"Hhub.exe";
 std::wstring command=L"\""+ui.wstring()+L"\" "+arguments;
 STARTUPINFOW start{sizeof(start)};start.dwFlags=STARTF_USESHOWWINDOW;start.wShowWindow=(WORD)show;PROCESS_INFORMATION process{};
 if(!CreateProcessW(ui.c_str(),command.data(),nullptr,nullptr,FALSE,0,nullptr,application.c_str(),&start,&process)){
  MessageBoxW(nullptr,L"H hub could not start. Please reinstall H hub.\nH hub 无法启动，请重新安装。",L"H hub",MB_ICONERROR);return 1;
 }
 CloseHandle(process.hThread);CloseHandle(process.hProcess);return 0;
}
