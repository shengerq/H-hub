// Device plugins declare remote controls and implement all parameter mapping; no raw HID on the network.
inline JsonObject phoneControlPlugin(JsonObject state,JsonObject device){
 if(str(device,L"kind")!=L"mouse"||!flag(device,L"connected",true))throw hresult_invalid_argument(L"此设备未连接或没有可用的鼠标控制适配器");
 for(auto const& v:state.GetNamedArray(L"plugins")){auto p=v.GetObject();if(!flag(p,L"installed")||flag(p,L"forceStopped")||str(p,L"mode")==L"disabled"||!hasOfflineUi(p))continue;
  auto ui=p.GetNamedObject(L"ui");if(num(ui,L"remoteAdapterVersion")!=1||!ui.HasKey(L"remoteControls"))continue;
  for(auto const& target:p.GetNamedArray(L"hidTargets",JsonArray{})){auto t=target.GetObject();if(num(t,L"vid",-1)==num(device,L"vid",-2)&&num(t,L"pid",-1)==num(device,L"pid",-2)){
   unsigned count=0;for(auto const& d:state.GetNamedObject(L"detected",JsonObject{}).GetNamedArray(L"peripherals",JsonArray{})){auto other=d.GetObject();if(str(other,L"kind")==L"mouse"&&flag(other,L"connected",true)&&num(other,L"vid",-1)==num(device,L"vid",-2)&&num(other,L"pid",-1)==num(device,L"pid",-2))++count;}
   if(count!=1)throw hresult_invalid_argument(L"此旧版协议不能区分多只同型号鼠标，请仅连接目标鼠标");return p;
  }}
 }
 throw hresult_invalid_argument(L"匹配的插件未安装、已禁用或尚未提供手机参数适配器");
}
inline void phoneValidateControlPatch(JsonObject patch,JsonObject plugin){
 if(patch.Size()!=1)throw hresult_invalid_argument(L"每次只调整一个参数");
 auto controls=plugin.GetNamedObject(L"ui").GetNamedObject(L"remoteControls");
 for(auto const& pair:patch){if(!controls.HasKey(pair.Key()))throw hresult_invalid_argument(L"设备插件未声明此远程参数");auto rule=controls.GetNamedObject(pair.Key());auto value=pair.Value();
  if(str(rule,L"type")==L"boolean"){if(value.ValueType()!=JsonValueType::Boolean)throw hresult_invalid_argument(L"开关参数无效");continue;}
  if(str(rule,L"type")!=L"number"||value.ValueType()!=JsonValueType::Number)throw hresult_invalid_argument(L"参数类型无效");auto n=value.GetNumber();if(!std::isfinite(n))throw hresult_invalid_argument();
  if(rule.HasKey(L"options")){bool allowed=false;for(auto const& item:rule.GetNamedArray(L"options"))if(item.GetNumber()==n)allowed=true;if(!allowed)throw hresult_invalid_argument(L"设备不支持该参数值");}
  else{auto low=num(rule,L"min"),high=num(rule,L"max"),step=num(rule,L"step");if(!std::isfinite(low)||!std::isfinite(high)||!std::isfinite(step)||step<=0||n<low||n>high||std::abs((n-low)/step-std::round((n-low)/step))>1e-6)throw hresult_invalid_argument(L"参数超出设备插件声明的范围");}
 }
}
inline JsonObject phoneControlValues(JsonObject payload){return payload.GetNamedObject(L"remoteValues");}
