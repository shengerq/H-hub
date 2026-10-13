// Included in Model. Mobile requests run through the existing desktop plugin worker.
void expirePhoneDeviceJobs(){for(auto const& v:state.GetNamedArray(L"phoneDeviceControls",JsonArray{})){auto job=v.GetObject();if(str(job,L"state")!=L"waiting"&&str(job,L"state")!=L"running")continue;if(!phoneDeviceAuthorized(job)){put(job,L"state",std::wstring(L"failed"));put(job,L"error",std::wstring(L"手机操作权限已撤销"));}else if(clockSeconds()>num(job,L"deadline")){put(job,L"state",std::wstring(L"failed"));put(job,L"error",std::wstring(L"桌面插件未及时响应，请确认新版 H hub 正在运行并重试读取"));}}}
bool phoneDeviceAuthorized(JsonObject job){if(!flag(state.GetNamedObject(L"settings"),L"phoneEnabled",true))return false;auto runtime=state.GetNamedObject(L"phoneRuntime",JsonObject{});if(!flag(runtime,L"running"))return false;for(auto const& v:runtime.GetNamedArray(L"phones",JsonArray{}))if(str(v.GetObject(),L"id")==str(job,L"phone")&&flag(v.GetObject(),L"control"))return true;return false;}
void requestPhoneDevice(JsonObject body){
 expirePhoneDeviceJobs();auto device=find(state.GetNamedObject(L"detected").GetNamedArray(L"peripherals"),str(body,L"id"));auto plugin=phoneControlPlugin(state,device);auto jobs=state.GetNamedArray(L"phoneDeviceControls",JsonArray{});JsonObject job{nullptr};for(auto const& v:jobs)if(str(v.GetObject(),L"id")==str(body,L"id"))job=v.GetObject();
 if(job&&(str(job,L"state")==L"waiting"||str(job,L"state")==L"running"))throw hresult_invalid_argument(L"此设备正在处理请求，请等待回读");
 if(str(body,L"kind")==L"deviceWrite")phoneValidateControlPatch(body.GetNamedObject(L"patch"));
 if(!job){if(jobs.Size()>=16)throw hresult_invalid_argument(L"待处理设备过多");job=JsonObject{};jobs.Append(job);state.SetNamedValue(L"phoneDeviceControls",jobs);}
 put(job,L"id",str(device,L"id"));put(job,L"plugin",str(plugin,L"id"));put(job,L"phone",str(body,L"deviceId"));put(job,L"requestId",std::to_wstring(GetTickCount64())+L"-"+std::to_wstring(++phoneDeviceSequence));put(job,L"operation",str(body,L"kind")==L"deviceWrite"?std::wstring(L"write"):std::wstring(L"read"));put(job,L"state",std::wstring(L"waiting"));put(job,L"error",std::wstring());put(job,L"deadline",clockSeconds()+60);job.SetNamedValue(L"deviceIdentity",object(std::wstring(device.Stringify())));job.SetNamedValue(L"patch",body.GetNamedObject(L"patch",JsonObject{}));
 if(!phoneDeviceAuthorized(job)){put(job,L"state",std::wstring(L"failed"));throw hresult_error(E_ACCESSDENIED,L"手机操作权限已失效");}
}
uint64_t phoneDeviceSequence=0;
JsonArray phoneDeviceTasks(std::wstring plugin){expirePhoneDeviceJobs();JsonArray tasks;for(auto const& v:state.GetNamedArray(L"phoneDeviceControls",JsonArray{})){auto job=v.GetObject();if(str(job,L"plugin")==plugin&&str(job,L"state")==L"waiting"){if(!phoneDeviceAuthorized(job)){put(job,L"state",std::wstring(L"failed"));put(job,L"error",std::wstring(L"手机操作权限已撤销"));continue;}tasks.Append(job);}}return tasks;}
JsonObject phoneDeviceWorkerRequest(std::wstring op,JsonObject b){
 expirePhoneDeviceJobs();JsonObject job{nullptr};for(auto const& v:state.GetNamedArray(L"phoneDeviceControls",JsonArray{}))if(str(v.GetObject(),L"requestId")==str(b,L"requestId"))job=v.GetObject();if(!job||str(job,L"plugin")!=str(b,L"plugin"))throw hresult_invalid_argument(L"设备请求已过期或不属于此插件");
 if(str(job,L"state")!=L"waiting"&&str(job,L"state")!=L"running")throw hresult_invalid_argument(L"设备请求已完成或过期");
 if(!phoneDeviceAuthorized(job))throw hresult_error(E_ACCESSDENIED,L"手机操作权限已撤销");
 auto device=find(state.GetNamedObject(L"detected").GetNamedArray(L"peripherals"),str(job,L"id"));auto plugin=phoneControlPlugin(state,device);if(str(plugin,L"id")!=str(b,L"plugin"))throw hresult_invalid_argument(L"设备或插件已改变");
 if(op==L"phoneDeviceValidate")put(job,L"state",std::wstring(L"running"));
 else{if(flag(b,L"confirmed")){auto payload=b.GetNamedObject(L"payload");if(payload.Stringify().size()>65536)throw hresult_invalid_argument(L"回读数据过大");auto values=phoneControlValues(payload);if(str(job,L"operation")==L"write")for(auto const& pair:job.GetNamedObject(L"patch")){if(!values.HasKey(pair.Key())||values.GetNamedValue(pair.Key()).Stringify()!=pair.Value().Stringify())throw hresult_invalid_argument(L"设备回读与请求不一致");}job.SetNamedValue(L"values",values);put(job,L"state",std::wstring(L"confirmed"));put(job,L"sampleTime",clockSeconds());put(job,L"error",std::wstring());}
  else{put(job,L"state",std::wstring(L"failed"));put(job,L"error",str(b,L"error",L"插件未通过设备回读"));}}
 JsonObject result,data;put(result,L"ok",true);put(data,L"valid",true);result.SetNamedValue(L"state",data);return result;
}
