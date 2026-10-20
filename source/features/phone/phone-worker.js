async function phoneAdapterCall(action){let error;for(let i=0;i<40;i++){try{return await action();}catch(e){error=e;if(!/正在同步|正在读取|稍后重试/.test(e.message))throw e;await new Promise(r=>setTimeout(r,150));}}throw error;}
async function processPhoneTask(state){
 const task=(state.remote||[]).find(t=>t.provider==='phone'&&t.state==='waiting');if(!task)return false;
 try{
  const adapter=window.hhubProfileAdapter;if(!adapter?.capture||!adapter?.apply||!adapter?.connect)throw Error('插件缺少完整读取、写入和回读适配器');
  await call('remoteValidate',{requestId:task.requestId});
  const device=task.deviceIdentity||{},probe={deviceIdentity:device};
  await phoneAdapterCall(()=>adapter.connect({containerId:device.containerId,vendorId:device.vid,productId:device.pid}));
  let payload=await phoneAdapterCall(()=>adapter.capture(probe));
  if(task.operation==='write'){
   if(typeof adapter.patch!=='function')throw Error('设备插件未提供远程参数适配器');const desired=adapter.patch(payload,task.patch);
   await call('remoteValidate',{requestId:task.requestId});
   if(await phoneAdapterCall(()=>adapter.apply(desired))!==true)throw Error('设备未通过参数回读确认');
   payload=await phoneAdapterCall(()=>adapter.capture(probe));
  }
  if(typeof adapter.remoteValues!=='function')throw Error('设备插件未提供远程参数投影');payload.remoteValues=adapter.remoteValues(payload);await call('remoteAck',{requestId:task.requestId,confirmed:true,payload});
 }catch(e){try{await call('remoteAck',{requestId:task.requestId,confirmed:false,error:e.message});}catch{} }
 return true;
}

taskExtensions.push(processPhoneTask);
