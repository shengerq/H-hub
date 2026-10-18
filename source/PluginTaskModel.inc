// Generic task broker between an installed provider and an existing device UI worker.
// The provider owns authorization, parameter validation, and readback projection.
uint64_t pluginTaskSequence=0;
JsonObject taskProvider(JsonObject job,std::wstring operation,JsonObject body={}){
 auto id=str(job,L"provider");auto p=pluginRecord(state,id);if(!p||!flag(p,L"installed")||flag(p,L"forceStopped")||str(p,L"mode")==L"disabled")throw hresult_error(E_ACCESSDENIED,L"请求提供方已停止");
 auto declared=p.GetNamedArray(L"capabilities",JsonArray{});if(std::none_of(declared.begin(),declared.end(),[&](auto const& value){return value.GetString()==operation;}))throw hresult_error(E_ACCESSDENIED,L"提供方未声明任务能力");
 body.SetNamedValue(L"job",job);JsonObject request;put(request,L"op",operation);request.SetNamedValue(L"body",body);request.SetNamedValue(L"state",state);auto& module=nativeModule(id);module.load(id);return module.invoke(request).GetNamedObject(L"data",JsonObject{});
}
void expirePluginTasks(){for(auto const& value:state.GetNamedArray(L"pluginTasks",JsonArray{})){auto job=value.GetObject();if(str(job,L"state")!=L"waiting"&&str(job,L"state")!=L"running")continue;try{if(clockSeconds()>num(job,L"deadline"))throw hresult_invalid_argument(L"设备插件未及时响应");taskProvider(job,L"authorize");}catch(...){put(job,L"state",std::wstring(L"failed"));put(job,L"error",std::wstring(L"请求已过期或授权已撤销"));}}}
void submitPluginTask(JsonObject body){
 expirePluginTasks();auto jobs=state.GetNamedArray(L"pluginTasks",JsonArray{});if(jobs.Size()>64){for(uint32_t i=jobs.Size();i>0;--i){auto status=str(jobs.GetObjectAt(i-1),L"state");if(status!=L"waiting"&&status!=L"running")jobs.RemoveAt(i-1);}}
 if(jobs.Size()>=64)throw hresult_invalid_argument(L"待处理请求过多");for(auto const& value:jobs){auto j=value.GetObject();if(str(j,L"id")==str(body,L"id")&&(str(j,L"state")==L"waiting"||str(j,L"state")==L"running"))throw hresult_invalid_argument(L"此设备正在处理请求");}
 auto target=pluginRecord(state,str(body,L"plugin"));if(!target||!flag(target,L"installed")||flag(target,L"forceStopped")||str(target,L"mode")==L"disabled")throw hresult_invalid_argument(L"设备插件未启用");
 auto job=object(std::wstring(body.Stringify()));put(job,L"requestId",std::to_wstring(GetTickCount64())+L"-"+std::to_wstring(++pluginTaskSequence));put(job,L"state",std::wstring(L"waiting"));put(job,L"deadline",clockSeconds()+60);taskProvider(job,L"authorize");jobs.Append(job);state.SetNamedValue(L"pluginTasks",jobs);
}
JsonArray pluginTasks(std::wstring plugin){expirePluginTasks();JsonArray tasks;for(auto const& value:state.GetNamedArray(L"pluginTasks",JsonArray{})){auto job=value.GetObject();if(str(job,L"plugin")==plugin&&str(job,L"state")==L"waiting")tasks.Append(job);}return tasks;}
JsonObject pluginTaskWorkerRequest(std::wstring operation,JsonObject body){
 expirePluginTasks();JsonObject job{nullptr};for(auto const& value:state.GetNamedArray(L"pluginTasks",JsonArray{}))if(str(value.GetObject(),L"requestId")==str(body,L"requestId"))job=value.GetObject();
 if(!job||str(job,L"plugin")!=str(body,L"plugin")||(str(job,L"state")!=L"waiting"&&str(job,L"state")!=L"running"))throw hresult_invalid_argument(L"请求不存在、已过期或不属于此插件");
 auto result=taskProvider(job,operation==L"pluginTaskValidate"?L"jobValidate":L"jobAck",body);
 if(operation==L"pluginTaskValidate")put(job,L"state",std::wstring(L"running"));else{if(flag(body,L"confirmed")){job.SetNamedValue(L"values",result.GetNamedObject(L"values",JsonObject{}));put(job,L"state",std::wstring(L"confirmed"));put(job,L"sampleTime",clockSeconds());}else{put(job,L"state",std::wstring(L"failed"));put(job,L"error",str(body,L"error",L"设备回读未通过"));}}
 JsonObject answer,data;put(answer,L"ok",true);put(data,L"valid",true);answer.SetNamedValue(L"state",data);return answer;
}
