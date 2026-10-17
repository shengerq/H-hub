winrt::fire_and_forget setEqEnabled(ToggleSwitch toggle){
 if(!toggle.IsEnabled())co_return;
 auto lifetime=get_strong();winrt::apartment_context ui;auto generation=renderGeneration;bool requested=toggle.IsOn();toggle.IsEnabled(false);
 JsonObject next{nullptr};std::wstring error;
 try{co_await winrt::resume_background();JsonObject body;put(body,L"eqEnabled",requested);next=hub::call(L"settings",body);if(flag(next.GetNamedObject(L"settings"),L"eqEnabled")!=requested)throw hresult_error(E_FAIL,tr(L"EQ 开关未通过回读"));next=hub::call(L"eqRuntime");}catch(hresult_error const& e){error=std::wstring(e.message());}
 co_await ui;if(current!=L"eq"||generation!=renderGeneration)co_return;
 if(next)acceptState(next);toggle.IsOn(flag(state.GetNamedObject(L"settings"),L"eqEnabled"));toggle.IsEnabled(true);updateEqStatus();positionNativeEq();
 if(!error.empty()){notice(error,true);co_return;}
 if(!requested){notice(tr(L"已回传：EQ 已停用，配置保留"));co_return;}
 auto runtime=eqRuntime();if(!flag(runtime,L"driverReady")){notice(tr(L"启用设置已保存；音频驱动未就绪，处理尚未启动"),true);co_return;}
 notice(tr(L"启用设置已回传，音频链路状态见下方"));
}
