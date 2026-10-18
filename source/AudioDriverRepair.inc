bool audioDriverRepairBusy=false;
winrt::fire_and_forget repairAudioDriver(){
 if(audioDriverRepairBusy)co_return;audioDriverRepairBusy=true;auto lifetime=get_strong();winrt::apartment_context ui;std::wstring message;bool failed=false;
 auto inf=binaryDir()/L"drivers"/L"audio"/L"fxvad.inf";auto core=binaryDir()/L"Hhub.Core.exe";auto report=dataDir()/(L"audio-driver-repair-"+std::to_wstring(GetTickCount64())+L".json");
 try{co_await resume_background();if(!std::filesystem::is_regular_file(inf))throw hresult_invalid_argument(L"安装目录没有内置驱动包，请使用新版安装器修复主程序");
  std::wstring args=L"--audio-install-integrated \""+inf.wstring()+L"\" \""+report.wstring()+L"\"";SHELLEXECUTEINFOW launch{sizeof(launch)};launch.fMask=SEE_MASK_NOCLOSEPROCESS;launch.lpVerb=L"runas";launch.lpFile=core.c_str();launch.lpParameters=args.c_str();launch.nShow=SW_HIDE;
  if(!ShellExecuteExW(&launch)){auto error=GetLastError();if(error==ERROR_CANCELLED)message=L"已取消驱动安装，可稍后点击提示重试";else throw hresult_error(HRESULT_FROM_WIN32(error));}
  else{winrt::handle process{launch.hProcess};if(!process)throw hresult_error(E_FAIL,L"无法取得驱动安装进程");WaitForSingleObject(process.get(),INFINITE);DWORD code{};GetExitCodeProcess(process.get(),&code);
   auto result=std::filesystem::exists(report)?object(read(report)):JsonObject{};if((code==0||code==3010)&&flag(result,L"ok"))message=flag(result,L"rebootRequired")?L"音频驱动已安装，请重启 Windows 后使用音效":L"音频驱动安装完成，正在重新检查音频设备";else{failed=true;message=str(result,L"error");if(message.empty())message=L"驱动安装未完成，请重试或查看安装日志";}
  }
 }catch(hresult_error const& e){failed=true;message=std::wstring(e.message());}catch(...){failed=true;message=L"驱动安装未完成，请重试";}
 co_await ui;audioDriverRepairBusy=false;notice(message,failed);try{state=hub::call(L"snapshot");updateEqStatus();}catch(...){}
}
