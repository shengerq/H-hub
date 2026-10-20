"use strict";
const $=id=>document.getElementById(id);let pairing=null,busy=false,control=false;
const params=new URLSearchParams(location.hash.slice(1));if(params.has("code"))$("code").value=params.get("code");history.replaceState(null,"",location.pathname);
async function request(path,body){const r=await fetch(path,{method:body?"POST":"GET",headers:body?{"Content-Type":"application/json"}:{},body:body?JSON.stringify(body):undefined,credentials:"same-origin",cache:"no-store"});const data=await r.json();if(!r.ok||data.ok===false)throw new Error(data.error||"电脑未响应");return data;}
function message(text){$("message").textContent=text;}
$("connect").onclick=async()=>{try{$("connect").disabled=true;pairing=await request("/api/pair",{name:$("name").value,code:$("code").value});message("请求已发送，请在电脑端批准。配对请求三分钟后过期。");}catch(e){message(e.message);$("connect").disabled=false;}};
async function action(body){try{await request("/api/action",body);message("操作已发送至电脑");await status();}catch(e){message(e.message);}}
async function status(){const reply=await request("/api/status");const data=reply.data||{};control=!!reply.control;$("pair").hidden=true;$("dashboard").hidden=false;$("connection").textContent="已连接";$("device").textContent=reply.deviceName||"我的手机";$("metrics").replaceChildren();for(const [label,key] of [["CPU","cpu"],["内存","memory"],["GPU","gpu"],["CPU 温度","cpuTemp"],["GPU 温度","gpuTemp"]]){const box=document.createElement("div");box.className="metric";box.textContent=label;const value=document.createElement("b");value.textContent=Number.isFinite(data.monitor?.[key])&&data.monitor[key]>=0?Math.round(data.monitor[key])+(key.endsWith("Temp")?" °C":"%"):"不可用";box.append(value);$("metrics").append(box);}renderDevices(data.devices||[]);$("scenes").replaceChildren();for(const scene of data.scenes||[]){const b=document.createElement("button");b.textContent=(scene.id===data.scene?"✓ ":"")+scene.name;b.disabled=!control;b.onclick=()=>action({kind:"scene",id:scene.id});$("scenes").append(b);}renderEq(data.eq||[]);if(!control)message("当前只有查看权限；需要操作时在电脑端开启遥控权限。");}
async function tick(){if(busy)return;busy=true;try{if(pairing){const p=await request("/api/pair-status",{id:pairing.id,pollSecret:pairing.pollSecret});if(p.status==="approved"){pairing=null;message("配对成功");await status();}else if(p.status==="rejected"||p.status==="expired"){pairing=null;$("connect").disabled=false;message("电脑端拒绝了请求");}}else await status();}catch(e){$("connection").textContent="未连接";message(e.message);if(!$("dashboard").hidden){$("pair").hidden=false;$("dashboard").hidden=true;$("connect").disabled=false;}}finally{busy=false;}}

function element(tag,text){const el=document.createElement(tag);if(text)el.textContent=text;return el;}
function renderDevices(devices){
 $("peripherals").replaceChildren();
 for(const d of devices){const row=element("div");row.className="row";row.append(element("strong",d.name),element("p",(d.status||d.connection||"状态未知")+" · 电量 "+(typeof d.battery==="number"?d.battery+"%":d.batteryLevel||"未知")));
 if(d.canVibrate){const b=element("button","测试振动 · 0.3 秒");b.disabled=!control;b.onclick=()=>action({kind:"deviceVibrate",id:d.id});row.append(b);}else row.append(element("p","该外设的远程参数控制尚未接入插件协议。"));
 $("peripherals").append(row);}
 if(!devices.length)$("peripherals").textContent="尚未检测到外设";
}
let eqSignature="";
function renderEq(configs){
 const signature=JSON.stringify([control,configs]);
 if(signature===eqSignature||$("eq").contains(document.activeElement)||$("eq").dataset.dirty==="true")return;
 eqSignature=signature;$("eq").replaceChildren();
 for(const eq of configs){const row=element("div");row.className="row";
 row.append(element("strong",eq.name),element("p",eq.active?"电脑音频链路运行中":"音频链路未启动"));
 const bypass=element("button",eq.enabled?"切为旁路":"启用均衡器");bypass.disabled=!control;bypass.onclick=()=>action({kind:"eqBypass",id:eq.id,enabled:!eq.enabled});
 const run=element("button",eq.active?"停止链路":"启动链路");run.disabled=!control;run.onclick=()=>action({kind:eq.active?"eqStop":"eqStart",id:eq.id});
 row.append(bypass,run);
 const gains=[];for(const [i,gain] of (eq.bands||[]).entries()){const label=element("label");const value=element("span",(eq.frequencies?.[i]||i+1)+" Hz · "+gain+" dB");const input=element("input");input.type="range";input.min=-12;input.max=12;input.step=.5;input.value=gain;input.disabled=!control;input.oninput=()=>{value.textContent=(eq.frequencies?.[i]||i+1)+" Hz · "+input.value+" dB";$("eq").dataset.dirty="true";};label.append(value,input);row.append(label);gains.push(input);}
 const label=element("label","前级增益（-24 至 0 dB）");const preamp=element("input");preamp.type="number";preamp.min=-24;preamp.max=0;preamp.step=.5;preamp.value=eq.preamp||0;preamp.disabled=!control;preamp.oninput=()=>{$("eq").dataset.dirty="true";};label.append(preamp);row.append(label);
 const auto=element("input");auto.type="checkbox";auto.checked=eq.autoPreamp!==false;auto.disabled=!control;auto.onchange=()=>{$("eq").dataset.dirty="true";};const autoLabel=element("label");autoLabel.append(auto,document.createTextNode(" 自动前级衰减"));row.append(autoLabel);
 const save=element("button","应用 EQ 调节");save.disabled=!control;save.onclick=async()=>{const p=Number(preamp.value);if(!Number.isFinite(p)||p < -24||p>0){message("前级范围为 -24 至 0 dB");return;}try{await request("/api/action",{kind:"eqSave",id:eq.id,enabled:eq.enabled,bands:gains.map(x=>Number(x.value)),preamp:p,autoPreamp:auto.checked});$("eq").dataset.dirty="false";eqSignature="";message("EQ 配置已应用");await status();}catch(e){message(e.message);}};
 const cancel=element("button","取消调节");cancel.onclick=()=>{$("eq").dataset.dirty="false";eqSignature="";status().catch(e=>message(e.message));};row.append(save,cancel);$("eq").append(row);
 }
 if(!configs.length)$("eq").textContent="请先在电脑端保存应用与输出设备的 EQ 配置。";
}
tick();setInterval(()=>{if(!document.hidden)tick();},2000);
document.addEventListener("visibilitychange",()=>{if(!document.hidden)tick();});
