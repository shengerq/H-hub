#pragma once
#include "MonitorProjection.h"
#include <dxgi1_2.h>
#include <map>
#include <cmath>
#include <algorithm>
#include <thread>
#include <mutex>
#include <memory>
#pragma comment(lib,"dxgi.lib")

namespace hub {
struct SensorTransport : MonitorProjection {
 JsonObject cache{nullptr},sensorCache{nullptr}; ULONGLONG sampled{},sensorTime{},sentTime{},startAttempt{};
 HANDLE worker{},job{},input{},output{}; bool pending{},autoElevationAttempted{}; std::string buffer;std::wstring sensorError;
 struct Elevation {std::mutex mutex;HANDLE process{};DWORD error{};ULONGLONG launched{};bool finished=false;~Elevation(){if(process)CloseHandle(process);}};
 std::shared_ptr<Elevation> elevation;HANDLE elevationPipe{},elevationEvent{};OVERLAPPED elevationConnect{};bool connecting=false,elevatedTransport=false;ULONGLONG elevationStarted{};std::wstring elevationError;
 struct Adapter {std::wstring name,luid;double dedicated;};std::vector<Adapter> adapters;
 std::wstring cpuName=L"CPU";
 SensorTransport(){
  wchar_t name[512]{};DWORD size=sizeof(name);
  if(RegGetValueW(HKEY_LOCAL_MACHINE,L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",L"ProcessorNameString",RRF_RT_REG_SZ,nullptr,name,&size)==ERROR_SUCCESS)cpuName=name;
  IDXGIFactory1* factory{};
  if(SUCCEEDED(CreateDXGIFactory1(__uuidof(IDXGIFactory1),(void**)&factory))){
   for(UINT i=0;;i++){IDXGIAdapter1* a{};if(factory->EnumAdapters1(i,&a)==DXGI_ERROR_NOT_FOUND)break;if(!a)break;DXGI_ADAPTER_DESC1 d{};a->GetDesc1(&d);a->Release();if(d.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)continue;
    wchar_t luid[80]{};swprintf_s(luid,L"luid_0x%08x_0x%08x",(unsigned)d.AdapterLuid.HighPart,(unsigned)d.AdapterLuid.LowPart);adapters.push_back({d.Description,luid,(double)d.DedicatedVideoMemory/1073741824.0});
   }factory->Release();
  }
  std::stable_sort(adapters.begin(),adapters.end(),[](auto const&a,auto const&b){return a.dedicated>b.dedicated;});
 }
 ~SensorTransport(){stopSensors();}
 bool workerIo(bool writing,void* data,DWORD size,DWORD& transferred){
  auto pipe=writing?input:output;if(!elevatedTransport)return writing?WriteFile(pipe,data,size,&transferred,nullptr):ReadFile(pipe,data,size,&transferred,nullptr);
  winrt::handle event(CreateEventW(nullptr,TRUE,FALSE,nullptr));if(!event)return false;OVERLAPPED operation{};operation.hEvent=event.get();
  BOOL ok=writing?WriteFile(pipe,data,size,&transferred,&operation):ReadFile(pipe,data,size,&transferred,&operation);
  if(ok)return GetOverlappedResult(pipe,&operation,&transferred,FALSE)!=FALSE;if(GetLastError()!=ERROR_IO_PENDING)return false;
  if(WaitForSingleObject(event.get(),500)!=WAIT_OBJECT_0){CancelIoEx(pipe,&operation);GetOverlappedResult(pipe,&operation,&transferred,TRUE);return false;}
  return GetOverlappedResult(pipe,&operation,&transferred,FALSE)!=FALSE;
 }
 bool workerWrite(char const* data,DWORD size,DWORD& transferred){return workerIo(true,const_cast<char*>(data),size,transferred);}
 void cancelElevation(){if(elevationPipe){CancelIoEx(elevationPipe,nullptr);if(connecting){DWORD ignored{};GetOverlappedResult(elevationPipe,&elevationConnect,&ignored,TRUE);}CloseHandle(elevationPipe);elevationPipe=nullptr;}if(elevationEvent){CloseHandle(elevationEvent);elevationEvent=nullptr;}connecting=false;elevationConnect={};elevation.reset();}
 void stopSensors(bool resetBackoff=false){cancelElevation();if(input){DWORD n;workerWrite("quit\n",5,n);CloseHandle(input);input=nullptr;}if(worker)WaitForSingleObject(worker,250);if(job){CloseHandle(job);job=nullptr;}if(worker){CloseHandle(worker);worker=nullptr;}if(output){CloseHandle(output);output=nullptr;}elevatedTransport=false;pending=false;std::string{}.swap(buffer);sensorCache=nullptr;cache=nullptr;sensorTime=sentTime=0;if(resetBackoff)startAttempt=0;}
 // Pause releases the helper and sensor cache, rather than retaining an idle reader.
 void pauseSensors(){stopSensors(true);}
 void startElevated(){
  if(elevation||(elevatedTransport&&worker&&WaitForSingleObject(worker,0)==WAIT_TIMEOUT))return;
  autoElevationAttempted=true;
  GUID nonce{};check_hresult(CoCreateGuid(&nonce));wchar_t token[64]{};StringFromGUID2(nonce,token,64);auto name=L"Hhub.Sensors."+userSid()+L"."+token,full=L"\\\\.\\pipe\\"+name;
  auto descriptor=L"D:P(A;;GA;;;"+userSid()+L")";PSECURITY_DESCRIPTOR security{};if(!ConvertStringSecurityDescriptorToSecurityDescriptorW(descriptor.c_str(),SDDL_REVISION_1,&security,nullptr))throw hresult_error(HRESULT_FROM_WIN32(GetLastError()));
  // Client sensor frames arrive in the server's input buffer, not its output buffer.
  SECURITY_ATTRIBUTES attributes{sizeof(attributes),security,FALSE};elevationPipe=CreateNamedPipeW(full.c_str(),PIPE_ACCESS_DUPLEX|FILE_FLAG_FIRST_PIPE_INSTANCE|FILE_FLAG_OVERLAPPED,PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_REJECT_REMOTE_CLIENTS,1,4096,1048576,0,&attributes);LocalFree(security);
  if(elevationPipe==INVALID_HANDLE_VALUE){elevationPipe=nullptr;throw hresult_error(HRESULT_FROM_WIN32(GetLastError()));}
  elevationEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);if(!elevationEvent){auto error=GetLastError();cancelElevation();throw hresult_error(HRESULT_FROM_WIN32(error));}elevationConnect={};elevationConnect.hEvent=elevationEvent;
  if(ConnectNamedPipe(elevationPipe,&elevationConnect))SetEvent(elevationEvent);else{auto error=GetLastError();if(error==ERROR_IO_PENDING)connecting=true;else if(error==ERROR_PIPE_CONNECTED)SetEvent(elevationEvent);else{cancelElevation();throw hresult_error(HRESULT_FROM_WIN32(error));}}
  auto context=std::make_shared<Elevation>();elevation=context;elevationStarted=GetTickCount64();elevationError.clear();cache=nullptr;
  auto exe=(binaryDir()/L"sensors"/L"Hhub.exe").wstring();auto arguments=L"--pipe "+name+L" --parent "+std::to_wstring(GetCurrentProcessId());
  try{std::thread([context,exe,arguments]{auto initialized=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);SHELLEXECUTEINFOW launch{sizeof(launch)};launch.fMask=SEE_MASK_NOCLOSEPROCESS|SEE_MASK_FLAG_NO_UI;launch.lpVerb=L"runas";launch.lpFile=exe.c_str();launch.lpParameters=arguments.c_str();launch.nShow=SW_HIDE;BOOL ok=ShellExecuteExW(&launch);DWORD error=ok?0:GetLastError();{std::lock_guard lock(context->mutex);context->process=ok?launch.hProcess:nullptr;context->error=error;context->launched=GetTickCount64();context->finished=true;}if(SUCCEEDED(initialized))CoUninitialize();}).detach();}catch(...){cancelElevation();throw;}
 }
 void progressElevation(){
  if(!elevation)return;auto context=elevation;std::unique_lock lock(context->mutex);if(!context->finished)return;
  if(context->error){elevationError=context->error==ERROR_CANCELLED?L"已取消授权，继续普通权限采集":L"管理员采集启动失败："+std::to_wstring(context->error);lock.unlock();cancelElevation();return;}
  if(WaitForSingleObject(elevationEvent,0)!=WAIT_OBJECT_0){if(WaitForSingleObject(context->process,0)==WAIT_OBJECT_0||GetTickCount64()-context->launched>20000){elevationError=L"管理员采集未能连接，请重试";lock.unlock();cancelElevation();}return;}
  DWORD transferred{},client{};if((connecting&&!GetOverlappedResult(elevationPipe,&elevationConnect,&transferred,FALSE))||!GetNamedPipeClientProcessId(elevationPipe,&client)||client!=GetProcessId(context->process)){elevationError=L"无法验证管理员采集进程，已断开连接";lock.unlock();cancelElevation();return;}
  HANDLE duplicate{};if(!DuplicateHandle(GetCurrentProcess(),elevationPipe,GetCurrentProcess(),&duplicate,0,FALSE,DUPLICATE_SAME_ACCESS)){elevationError=L"管理员采集连接初始化失败";lock.unlock();cancelElevation();return;}
  auto process=context->process;context->process=nullptr;auto pipe=elevationPipe;elevationPipe=nullptr;auto event=elevationEvent;elevationEvent=nullptr;connecting=false;lock.unlock();elevation.reset();stopSensors();CloseHandle(event);worker=process;input=duplicate;output=pipe;elevatedTransport=true;elevationError.clear();startAttempt=0;
 }
 SensorTransport(SensorTransport const&)=delete;
 void startWorker(){
  if(worker)return;auto now=GetTickCount64();if(startAttempt&&now-startAttempt<10000)return;startAttempt=now;SECURITY_ATTRIBUTES sa{sizeof(sa),nullptr,TRUE};HANDLE childIn{},childOut{};
  if(!CreatePipe(&childIn,&input,&sa,0))return;
  // A complete sensor frame may exceed the default 4 KB anonymous-pipe buffer.
  if(!CreatePipe(&output,&childOut,&sa,1048576)){CloseHandle(childIn);CloseHandle(input);input=nullptr;return;}
  SetHandleInformation(input,HANDLE_FLAG_INHERIT,0);SetHandleInformation(output,HANDLE_FLAG_INHERIT,0);
  auto exe=binaryDir()/L"sensors"/L"Hhub.exe";std::wstring line=L"\""+exe.wstring()+L"\"";
  STARTUPINFOW si{sizeof(si)};si.dwFlags=STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;si.wShowWindow=SW_HIDE;si.hStdInput=childIn;si.hStdOutput=childOut;
  si.hStdError=CreateFileW(L"NUL",GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,OPEN_EXISTING,0,nullptr);
  PROCESS_INFORMATION pi{};
  if(CreateProcessW(exe.c_str(),line.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW|CREATE_SUSPENDED,nullptr,exe.parent_path().c_str(),&si,&pi)){
   job=CreateJobObjectW(nullptr,nullptr);JOBOBJECT_EXTENDED_LIMIT_INFORMATION limit{};limit.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
   if(job&&SetInformationJobObject(job,JobObjectExtendedLimitInformation,&limit,sizeof(limit))&&AssignProcessToJobObject(job,pi.hProcess)){worker=pi.hProcess;ResumeThread(pi.hThread);}else{TerminateProcess(pi.hProcess,1);CloseHandle(pi.hProcess);if(job)CloseHandle(job);job=nullptr;}
   CloseHandle(pi.hThread);
  }
  if(si.hStdError!=INVALID_HANDLE_VALUE)CloseHandle(si.hStdError);CloseHandle(childIn);CloseHandle(childOut);
  if(!worker){CloseHandle(input);CloseHandle(output);input=output=nullptr;}
 }
 void sensors(JsonObject o){
  progressElevation();
  DWORD waitingBytes{};
  bool responseWaiting=output&&PeekNamedPipe(output,nullptr,0,nullptr,&waitingBytes,nullptr)&&waitingBytes>0;
  // Time away from the monitoring page is not a worker timeout when its reply is already queued.
  // Hardware discovery can take longer than subsequent updates. Only time out
  // a missing response; a frame queued while the UI was closed is not a hang.
  auto responseTimeout=sensorTime?20000ULL:60000ULL;
  if(worker&&(WaitForSingleObject(worker,0)!=WAIT_TIMEOUT||(pending&&GetTickCount64()-sentTime>responseTimeout&&!responseWaiting)))stopSensors();
  startWorker();DWORD available{};
  if(output&&PeekNamedPipe(output,nullptr,0,nullptr,&available,nullptr)&&available){char data[8192];while(available){DWORD got{};if(!workerIo(false,data,std::min<DWORD>(available,sizeof(data)),got)||!got){sensorError=L"传感器管道读取失败："+std::to_wstring(GetLastError());break;}buffer.append(data,got);if(buffer.size()>1048576){buffer.clear();sensorError=L"传感器响应超过大小限制";break;}if(!PeekNamedPipe(output,nullptr,0,nullptr,&available,nullptr))break;}
  }
  // Drain complete frames already buffered too, keeping the newest sample instead of a stale queue head.
  for(auto newline=buffer.find('\n');newline!=std::string::npos;newline=buffer.find('\n')){try{auto line=buffer.substr(0,newline);if(line.rfind("\xEF\xBB\xBF",0)==0)line.erase(0,3);sensorCache=object(std::wstring(to_hstring(line)));sensorTime=GetTickCount64();sensorError.clear();}catch(hresult_error const& e){sensorError=std::wstring(e.message());}catch(...){sensorError=L"传感器响应格式错误";}buffer.erase(0,newline+1);pending=false;}
  auto now=GetTickCount64();if(worker&&!pending&&WaitForSingleObject(worker,0)==WAIT_TIMEOUT){DWORD sent{};pending=workerWrite("sample\n",7,sent)&&sent==7;sentTime=now;}
  put(o,L"sensorElevationPending",elevation!=nullptr);put(o,L"sensorElevationError",elevationError);
  put(o,L"sensorHasAdminSession",elevatedTransport);
  put(o,L"sensorBufferedBytes",(double)buffer.size());put(o,L"sensorRequestPending",pending);
  bool fresh=sensorCache&&now-sensorTime<10000&&clockSeconds()-num(sensorCache,L"sampleTime")<10;
  put(o,L"sensorStatus",std::wstring(fresh?str(sensorCache,L"source")+L" · 实时":!worker?L"传感器组件无法启动":WaitForSingleObject(worker,0)!=WAIT_TIMEOUT?L"传感器组件已退出":pending&&now-sentTime>15000?(sensorTime?L"传感器读取较慢，等待回传":L"正在初始化硬件传感器"):L"传感器初始化中 / 等待采样"));
  SC_HANDLE manager=OpenSCManagerW(nullptr,nullptr,SC_MANAGER_CONNECT);SC_HANDLE driver=manager?OpenServiceW(manager,L"PawnIO",SERVICE_QUERY_STATUS):nullptr;SERVICE_STATUS status{};bool installed=driver!=nullptr;bool running=driver&&QueryServiceStatus(driver,&status)&&status.dwCurrentState==SERVICE_RUNNING;if(driver)CloseServiceHandle(driver);if(manager)CloseServiceHandle(manager);put(o,L"pawnInstalled",installed||(fresh&&flag(sensorCache,L"driverInstalled")));put(o,L"pawnRunning",running);put(o,L"sensorElevated",fresh&&flag(sensorCache,L"elevated"));wchar_t machine[256]{};DWORD machineSize=sizeof(machine);RegGetValueW(HKEY_LOCAL_MACHINE,L"HARDWARE\\DESCRIPTION\\System\\BIOS",L"SystemProductName",RRF_RT_REG_SZ,nullptr,machine,&machineSize);auto model=std::wstring(machine);put(o,L"virtualMachine",model.find(L"Virtual")!=std::wstring::npos||model.find(L"VMware")!=std::wstring::npos);
  JsonArray a=fresh?sensorCache.GetNamedArray(L"sensors",JsonArray{}):JsonArray{};o.SetNamedValue(L"sensors",a);
  put(o,L"sensorReady",fresh);o.SetNamedValue(L"sensorHardware",fresh?sensorCache.GetNamedArray(L"hardware",JsonArray{}):JsonArray{});
  number(o,L"sensorSampleTime",fresh?num(sensorCache,L"sampleTime"):-1);
  o.SetNamedValue(L"sensorErrors",fresh?sensorCache.GetNamedArray(L"errors",JsonArray{}):JsonArray{});
  if(!sensorError.empty())o.GetNamedArray(L"sensorErrors").Append(JsonValue::CreateStringValue(sensorError));
  if(fresh){o.SetNamedValue(L"metrics",sensorCache.GetNamedObject(L"metrics",JsonObject{}));o.SetNamedValue(L"volumes",sensorCache.GetNamedArray(L"volumes",JsonArray{}));put(o,L"monitorBackend",str(sensorCache,L"backend"));put(o,L"sensorSource",str(sensorCache,L"source"));}

 }
};
}
