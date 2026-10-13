// Public native-phone controls for the reviewed Rapoo adapter; no raw HID on the network.
inline JsonObject phoneControlPlugin(JsonObject state,JsonObject device){
 if(str(device,L"kind")!=L"mouse"||!flag(device,L"connected",true))throw hresult_invalid_argument(L"此设备未连接或没有可用的鼠标控制适配器");
 for(auto const& v:state.GetNamedArray(L"plugins")){auto p=v.GetObject();if(!flag(p,L"installed")||str(p,L"mode")==L"disabled"||!hasOfflineUi(p))continue;
  auto html=str(p.GetNamedObject(L"ui"),L"content");if(html.find(L"function liveKeys(){if(!liveBaseline)")==std::wstring::npos||html.find(L"RapooProtocol.Device")==std::wstring::npos)continue;
  for(auto const& target:p.GetNamedArray(L"hidTargets",JsonArray{})){auto t=target.GetObject();if(num(t,L"vid",-1)==num(device,L"vid",-2)&&num(t,L"pid",-1)==num(device,L"pid",-2)){
   unsigned count=0;for(auto const& d:state.GetNamedObject(L"detected",JsonObject{}).GetNamedArray(L"peripherals",JsonArray{})){auto other=d.GetObject();if(str(other,L"kind")==L"mouse"&&flag(other,L"connected",true)&&num(other,L"vid",-1)==num(device,L"vid",-2)&&num(other,L"pid",-1)==num(device,L"pid",-2))++count;}
   if(count!=1)throw hresult_invalid_argument(L"此旧版协议不能区分多只同型号鼠标，请仅连接目标鼠标");return p;
  }}
 }
 throw hresult_invalid_argument(L"匹配的插件未安装、已禁用或尚未提供手机参数适配器");
}
inline void phoneValidateControlPatch(JsonObject patch){
 if(patch.Size()!=1)throw hresult_invalid_argument(L"每次只调整一个参数");
 for(auto const& pair:patch){auto key=std::wstring(pair.Key());auto v=pair.Value();bool boolean=key==L"motion"||key==L"straight"||key==L"ripple"||key==L"sleep"||key==L"lowBattery";
  if(boolean){if(v.ValueType()!=JsonValueType::Boolean)throw hresult_invalid_argument(L"开关参数无效");continue;}
  if(v.ValueType()!=JsonValueType::Number)throw hresult_invalid_argument(L"参数类型无效");double n=v.GetNumber();if(!std::isfinite(n))throw hresult_invalid_argument();
  if(key==L"dpi"){if(n<50||n>30000||n!=std::floor(n)||std::fmod(n,50)!=0)throw hresult_invalid_argument(L"DPI 为 50–30000，步进 50");}
  else if(key==L"polling"){bool valid=false;for(double rate:{125,250,500,1000,2000,4000,8000})if(n==rate)valid=true;if(!valid)throw hresult_invalid_argument(L"回报率无效");}
  else if(key==L"selected"){if(n<0||n>6||n!=std::floor(n))throw hresult_invalid_argument(L"DPI 档位无效");}
  else if(key==L"lod"){if(n<.7||n>1.7||std::abs(n*10-std::round(n*10))>1e-6)throw hresult_invalid_argument(L"抬升高度范围 0.7–1.7 mm");}
  else if(key==L"press"||key==L"release"){if(n<0||n>32||n!=std::floor(n))throw hresult_invalid_argument(L"按键延迟范围 0–32 ms");}
  else if(key==L"minutes"){if(n<2||n>120||n!=std::floor(n))throw hresult_invalid_argument(L"休眠时间范围 2–120 分钟");}
  else throw hresult_invalid_argument(L"手机不允许调整此参数");
 }
}
inline JsonObject phoneControlValues(JsonObject payload){
 auto cfg=payload.GetNamedObject(L"config");auto stages=cfg.GetNamedArray(L"stages");auto index=(uint32_t)num(cfg,L"selected");if(stages.Size()<1||stages.Size()>7||index>=stages.Size())throw hresult_invalid_argument(L"设备未提供有效 DPI 档位");
 JsonObject result;put(result,L"dpi",stages.GetNumberAt(index));for(auto key:{L"polling",L"selected",L"lod",L"press",L"release",L"minutes",L"motion",L"straight",L"ripple",L"sleep",L"lowBattery"})if(cfg.HasKey(key))result.SetNamedValue(key,cfg.GetNamedValue(key));put(result,L"stageCount",(double)stages.Size());return result;
}
