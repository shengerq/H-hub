#include "Localization.h"
#include "EqAudio.h"
#ifndef HHUB_SERVICE_ONLY
#include "PluginPage.h"
#include "PluginXamlPage.h"
#endif
using namespace hub;
#include "HostIntegration.h"
#ifndef HHUB_SERVICE_ONLY
extern "C" __declspec(dllexport) BOOL WINAPI HhubPluginCanUnloadNow(){return eqUI::livePages.load()==0;}
#endif
static std::unique_ptr<EqAudio> engine;
static EqAudio& audio(){if(!engine)engine=std::make_unique<EqAudio>();return *engine;}
extern "C" __declspec(dllexport) HRESULT WINAPI HhubPluginRequest(wchar_t const* input,wchar_t** output){
 if(!output)return E_POINTER;*output=nullptr;JsonObject reply;
 try{auto request=object(input);auto op=str(request,L"op");auto body=request.GetNamedObject(L"body",JsonObject{});auto state=request.GetNamedObject(L"state",JsonObject{});JsonObject data;
  if(op==L"host"||op==L"hostStartup"||op==L"hostStop"||op==L"hostUninstall"||op==L"hostShutdown"){if(op!=L"host")put(request,L"event",std::wstring(op==L"hostStartup"?L"startup":op==L"hostStop"?L"stop":op==L"hostUninstall"?L"uninstall":L"shutdown"));reply=eqHostIntegration(request,audio());}
  else if(op==L"model"){Model model({},false);model.state=state;auto operation=str(request,L"operation");if(operation==L"initialize"){auto state=model.state;  if(str(state,L"eqSchema")!=L"12-band"){for(auto key:{L"presets",L"eqRealPrefs",L"eq"})for(auto const& v:state.GetNamedArray(key,JsonArray{})){auto p=v.GetObject();auto gains=p.GetNamedArray(L"bands",JsonArray{});if(gains.Size()==10){gains.Append(JsonValue::CreateNumberValue(0));gains.Append(JsonValue::CreateNumberValue(0));}JsonArray centers;for(double f:{31.25,62.5,125.0,250.0,500.0,1000.0,2000.0,4000.0,8000.0,16000.0,40.0,12000.0})centers.Append(JsonValue::CreateNumberValue(f));if(str(p,L"id")==L"flat"){centers.Clear();for(double f:{31.25,62.5,125.0,250.0,500.0,1000.0,2000.0,4000.0,8000.0,12000.0,16000.0,20000.0})centers.Append(JsonValue::CreateNumberValue(f));}if(!p.HasKey(L"frequencies"))p.SetNamedValue(L"frequencies",centers);}put(state,L"eqSchema",std::wstring(L"12-band"));}model.migrateEqPresets();}else{JsonObject req;put(req,L"op",operation);req.SetNamedValue(L"body",body);model.request(req);}reply.SetNamedValue(L"state",model.state);}
  else if(op==L"shutdown"){engine.reset();}
  else if(op==L"conflict")put(data,L"busy",fxSoundIsRunning());
  else if(op==L"snapshot")data=audio().snapshot(state);
  else if(op==L"node")audio().nodeEnabled(flag(body,L"enabled"),str(body,L"output"));
  else if(op==L"uninstall")audio().uninstall(str(body,L"output"));
  else if(op==L"clear")audio().streams.clear();
  else if(op==L"stop")audio().stop(str(body,L"app"));
  else if(op==L"start")audio().start(body);
  else if(op==L"update")audio().update(body);
  else if(op==L"restore"){put(data,L"sharedDefaultsPreserved",true);}
  else if(op==L"validate"){EqProcessor processor;processor.configure(body);}
  else if(op==L"offlineTest"){
   JsonObject config;JsonArray bands,frequencies;for(int i=0;i<12;i++){bands.Append(JsonValue::CreateNumberValue(0));frequencies.Append(JsonValue::CreateNumberValue(EqDsp::frequencies[i]));}config.SetNamedValue(L"bands",bands);config.SetNamedValue(L"frequencies",frequencies);put(config,L"enabled",false);put(config,L"autoPreamp",false);EqProcessor processor;processor.configure(config);std::vector<float> samples(4096*2);for(int i=0;i<4096;i++)samples[i*2]=samples[i*2+1]=(float)(.1*sin(i*.17));auto before=samples;processor.process(samples.data(),4096);double difference=0;for(size_t i=0;i<samples.size();i++){if(!std::isfinite(samples[i]))throw hresult_error(E_FAIL);difference=std::max(difference,(double)std::abs(samples[i]-before[i]));}if(difference>1e-6)throw hresult_error(E_FAIL,L"旁路采样不一致");put(data,L"bypassError",difference);put(config,L"enabled",true);config.GetNamedArray(L"bands").SetAt(5,JsonValue::CreateNumberValue(6));processor.configure(config);processor.process(samples.data(),4096);for(float sample:samples)if(!std::isfinite(sample))throw hresult_error(E_FAIL);put(data,L"finiteProcessedSamples",true);bool denied=false;config.GetNamedArray(L"bands").SetAt(5,JsonValue::CreateNumberValue(99));try{processor.configure(config);}catch(...){denied=true;}if(!denied)throw hresult_error(E_FAIL);put(data,L"invalidConfigRejected",true);
  }
  else if(op==L"syncScene"||op==L"syncGeneric"){
   if(op==L"syncScene")audio().syncSceneAudio(state);else audio().syncGenericAudio(state);
   data.SetNamedValue(L"state",state);JsonObject runtime;JsonArray streams;for(auto const& [id,stream]:audio().streams)streams.Append(stream->snapshot());runtime.SetNamedValue(L"streams",streams);data.SetNamedValue(L"runtime",runtime);
  }else throw hresult_invalid_argument(L"未知插件调用");
  reply.SetNamedValue(L"data",data);put(reply,L"ok",true);
 }catch(hresult_error const& e){put(reply,L"ok",false);put(reply,L"error",std::wstring(e.message()));}catch(std::exception const& e){put(reply,L"ok",false);put(reply,L"error",std::wstring(to_hstring(e.what())));}catch(...){put(reply,L"ok",false);put(reply,L"error",std::wstring(L"EQ 插件执行失败"));}
 auto text=std::wstring(reply.Stringify());auto memory=static_cast<wchar_t*>(CoTaskMemAlloc((text.size()+1)*sizeof(wchar_t)));if(!memory)return E_OUTOFMEMORY;memcpy(memory,text.c_str(),(text.size()+1)*sizeof(wchar_t));*output=memory;return S_OK;
}
#ifndef HHUB_SERVICE_ONLY
extern "C" __declspec(dllexport) HRESULT WINAPI HhubPluginCreatePage(HWND parent,wchar_t const* input,void** context,HWND* window){if(!context||!window)return E_POINTER;*context=nullptr;*window=nullptr;try{auto page=std::make_unique<PluginPage>();page->create(parent,object(input));*window=page->window;*context=page.release();return S_OK;}catch(hresult_error const& e){JsonObject error;put(error,L"error",std::wstring(e.message()));put(error,L"hresult",(double)e.code().value);put(error,L"stage",PluginPage::creationStage);write(dataDir()/L"eq-module-page-error.json",std::wstring(error.Stringify()));return FAILED(e.code())?e.code().value:E_FAIL;}catch(...){return E_FAIL;}}
extern "C" __declspec(dllexport) HRESULT WINAPI HhubPluginPageCommand(void* context,wchar_t const* input){try{static_cast<PluginPage*>(context)->command(object(input));return S_OK;}catch(...){return E_FAIL;}}
extern "C" __declspec(dllexport) void WINAPI HhubPluginDestroyPage(void* context){delete static_cast<PluginPage*>(context);UnregisterClassW(L"Hhub.NativeEqPage",GetModuleHandleW(nullptr));UnregisterClassW(L"Hhub.EQ.PluginPage",GetModuleHandleW(nullptr));}



extern "C" __declspec(dllexport) HRESULT WINAPI HhubPluginCreateXamlPage(HWND parent,wchar_t const* input,IUnknown* mount,void** context,HWND* window){if(!context||!window||!mount)return E_POINTER;*context=nullptr;*window=nullptr;try{mount->AddRef();auto page=std::make_unique<eqUI::Page>();page->createXaml(parent,object(input),mount);*window=page->window;*context=page.release();return S_OK;}catch(hresult_error const& e){JsonObject error;put(error,L"error",std::wstring(e.message()));put(error,L"stage",PluginPage::creationStage);write(dataDir()/L"eq-module-page-error.json",std::wstring(error.Stringify()));return FAILED(e.code())?e.code().value:E_FAIL;}catch(...){return E_FAIL;}}

#endif // HHUB_SERVICE_ONLY
