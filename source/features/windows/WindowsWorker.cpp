#include "WindowsSettings.h"
#include "WindowsWorkerClient.h"
#include <memory>
using namespace hub;
// Each request owns its Windows session; no persistent polling or cached state.
static JsonObject executeWindowsRequest(JsonObject request){
 auto op=str(request,L"op");auto body=request.GetNamedObject(L"body",JsonObject{});WindowsSettings settings;
 if(op==L"snapshot")return settings.snapshot(flag(body,L"switchesOnly"));
 auto id=str(body,L"id");bool valid=false;
 for(auto const& item:windowsOptionCatalog())if(str(item.GetObject(),L"id")==id)valid=true;
 if(op==L"power"){settings.setPower(id);JsonObject result;put(result,L"activePower",WindowsSettings::activePower());return result;}
 if(!valid)throw hresult_invalid_argument(L"未知的 Windows 设置");
 if(op==L"read")return settings.readOption(id);
 if(op==L"set"){
  auto desired=body.GetNamedValue(L"enabled",JsonValue::CreateNullValue());
  if(desired.ValueType()!=JsonValueType::Boolean)throw hresult_invalid_argument(L"开关必须是布尔值");
  settings.set(id,desired.GetBoolean());
 }else if(op==L"restore")settings.restore(id);
 else throw hresult_invalid_argument(L"未知的 Windows 设置操作");
 auto actual=settings.readOption(id);put(actual,L"loading",false);
 if(op==L"set"&&(!flag(actual,L"confirmed")||flag(actual,L"enabled")!=body.GetNamedBoolean(L"enabled")))
  throw hresult_error(E_FAIL,L"Windows 未确认目标开关状态");
 if(flag(body,L"single"))return actual;
 return settings.snapshot();
}
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int){
 winrt::init_apartment(winrt::apartment_type::multi_threaded);
 int count{};auto args=CommandLineToArgvW(GetCommandLineW(),&count);if(!args||count!=3){if(args)LocalFree(args);return 2;}
 std::filesystem::path input=args[1],output=args[2];LocalFree(args);JsonObject response;std::unique_ptr<WindowsWorker::Lease> execution;
 try{execution=std::make_unique<WindowsWorker::Lease>(L"Execution");if(!std::filesystem::exists(input))return 2;auto request=object(read(input));response.SetNamedValue(L"data",executeWindowsRequest(request));put(response,L"ok",true);}
 catch(hresult_error const& e){put(response,L"ok",false);put(response,L"error",std::wstring(e.message()));}
 catch(...){put(response,L"ok",false);put(response,L"error",std::wstring(L"Windows 设置未返回有效结果"));}
 if(!std::filesystem::exists(input))return 2;try{write(output,std::wstring(response.Stringify()));}catch(...){return 3;}
 return flag(response,L"ok")?0:1;
}
