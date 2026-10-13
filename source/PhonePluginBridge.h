#pragma once
namespace hub {
inline std::wstring phonePluginBridge(){return LR"PHONEBRIDGE(
function phonePatchPayload(payload,patch){
 const result=JSON.parse(JSON.stringify(payload)),c=result.config,keys=Object.keys(patch);
 if(!c||keys.length!==1)throw Error('每次只调整一个参数');
 const key=keys[0],value=patch[key];
 if(key==='dpi'){c.stages[c.selected]=value;if(!c.independent)c.axisY[c.selected]=value;}
 else if(key==='selected'){if(!Number.isInteger(value)||value<0||value>=c.stages.length)throw Error('DPI 档位不存在');c.selected=value;}
 else if(['polling','lod','press','release','minutes','motion','straight','ripple','sleep','lowBattery'].includes(key)){if(!Object.hasOwn(c,key))throw Error('设备没有提供此参数');c[key]=value;}
 else throw Error('手机不允许调整此参数');
 c.x=c.stages[c.selected];c.y=c.axisY[c.selected];c.independent=c.x!==c.y;
 if(key==='polling'){const index=RapooProtocol.polls.indexOf(c.polling),allowed=RapooProtocol.allowedModes[index];if(!allowed)throw Error('回报率不支持');if(!allowed.includes(RapooProtocol.modes.indexOf(c.mode)))c.mode=RapooProtocol.modes[Math.max(...allowed)];}
 return result;
}
async function phoneAdapterCall(action){let error;for(let i=0;i<40;i++){try{return await action();}catch(e){error=e;if(!/正在同步|正在读取|稍后重试/.test(e.message))throw e;await new Promise(r=>setTimeout(r,150));}}throw error;}
async function processPhoneTask(state){
 const task=(state.remote||[]).find(t=>t.state==='waiting');if(!task)return false;
 try{
  const adapter=window.hhubProfileAdapter;if(!adapter?.capture||!adapter?.apply||!adapter?.connect)throw Error('插件缺少完整读取、写入和回读适配器');
  await call('remoteValidate',{requestId:task.requestId});
  const device=task.deviceIdentity||{},probe={deviceIdentity:device};
  await phoneAdapterCall(()=>adapter.connect({containerId:device.containerId,vendorId:device.vid,productId:device.pid}));
  let payload=await phoneAdapterCall(()=>adapter.capture(probe));
  if(task.operation==='write'){
   const desired=phonePatchPayload(payload,task.patch);
   await call('remoteValidate',{requestId:task.requestId});
   if(await phoneAdapterCall(()=>adapter.apply(desired))!==true)throw Error('设备未通过参数回读确认');
   payload=await phoneAdapterCall(()=>adapter.capture(probe));
  }
  await call('remoteAck',{requestId:task.requestId,confirmed:true,payload});
 }catch(e){try{await call('remoteAck',{requestId:task.requestId,confirmed:false,error:e.message});}catch{} }
 return true;
}
)PHONEBRIDGE";}
}
