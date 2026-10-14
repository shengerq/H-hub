// Run with Node and Playwright available in NODE_PATH. No device writes are made.
const { chromium } = require('playwright');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const http = require('node:http');
async function run() {
  const root=path.join(__dirname,'webui');
  const server=http.createServer((req,res)=>{
    const file=path.join(root,req.url==='/'?'index.html':req.url.slice(1));
    if(!file.startsWith(root+path.sep)){res.writeHead(403).end();return;}
    res.setHeader('Content-Type',file.endsWith('.js')?'text/javascript':file.endsWith('.css')?'text/css':'text/html');
    try{res.end(fs.readFileSync(file));}catch{res.writeHead(404).end();}
  });
  await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
  let browser;
  try {
    browser=await chromium.launch({channel:'msedge',headless:true});
    const page=await browser.newPage({viewport:{width:1128,height:790}});
    const errors=[];page.on('pageerror',e=>errors.push(e.message));
    await page.addInitScript(()=>{window.homeActions=[];window.chrome=window.chrome||{};window.chrome.webview={postMessage:m=>window.homeActions.push(m),addEventListener:(_,fn)=>{window.deliverHome=m=>fn({data:m});}};});
    await page.goto(`http://127.0.0.1:${server.address().port}/`);
    const image='data:image/svg+xml;base64,'+Buffer.from('<svg xmlns="http://www.w3.org/2000/svg" width="40" height="70"><rect width="40" height="70" rx="18" fill="gray"/></svg>').toString('base64');
    const state={type:'hhub.home.state',theme:{theme:'dark',reduceMotion:false,colors:{accent:'#0078d4'}},busy:false,selected:'d5',scene:'当前场景：日常桌面 · 自动规则',metrics:['CPU','GPU','内存','磁盘'].map((label,i)=>({label,icon:'',modelKey:'',rows:[{key:'metric'+i,label:'利用率'}]})),values:Object.fromEntries([0,1,2,3].map(i=>['metric'+i,{text:'20%'}])),devices:Array.from({length:8},(_,i)=>({id:'d'+i,name:'设备 '+i,tileStatus:'电量 56%',status:'已连接',connected:true,battery:'电量 56%',profile:'USB / HID · 真实只读设备',kindLabel:'鼠标 / 鼠标接收器',art:image,artNote:'外观示意 · 非型号照片'}))};
    const deliver=async()=>page.evaluate(s=>window.deliverHome(s),state);
    await deliver();assert.equal(await page.locator('.device-tile').count(),8);assert.equal(await page.locator('#device-name').textContent(),'设备 5');
    await page.getByRole('button',{name:'选择设备：设备 6',exact:true}).click();assert.equal((await page.evaluate(()=>window.homeActions.at(-1))).id,'d6');
    // A telemetry tick must preserve controls, keyboard focus, and list scrolling.
    await page.evaluate(()=>{window.originalTile=document.querySelector('[data-id="d6"]');window.originalValue=document.querySelector('.metric-value');document.getElementById('device-list').scrollTop=25;window.originalTile.focus();window.oldScroll=document.getElementById('device-list').scrollTop;});
    state.values.metric0.text='42%';await deliver();
    assert.equal(await page.evaluate(()=>window.originalTile===document.querySelector('[data-id="d6"]')&&window.originalValue===document.querySelector('.metric-value')&&document.activeElement===window.originalTile&&window.oldScroll===document.getElementById('device-list').scrollTop),true);
    assert.equal(await page.locator('.metric-value').first().textContent(),'42%');
    for(const action of ['system','overview','interfaces','refresh','details']){await page.locator(`[data-action="${action}"]`).click();assert.equal((await page.evaluate(()=>window.homeActions.at(-1))).action,action);}
    state.busy=true;await deliver();assert.equal(await page.locator('#refresh').isDisabled(),true);
    state.busy=false;state.selected='d6';state.theme.theme='light';state.theme.reduceMotion=true;await deliver();assert.equal(await page.locator('html').getAttribute('data-reduce-motion'),'true');
    for(const width of [1128,700,400]){await page.setViewportSize({width,height:790});await page.waitForTimeout(50);assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),true,`horizontal overflow at ${width}`);}
    state.devices=state.devices.filter(d=>d.id!=='d6');state.selected='d0';await deliver();assert.equal(await page.locator('.device-tile').count(),7);
    state.devices=[];state.selected='';await deliver();assert.equal(await page.locator('#empty').isVisible(),true);assert.equal(await page.locator('#device-info').isVisible(),false);
    assert.deepEqual(errors,[]);console.log('PASS bridge actions, live patching, focus/scroll preservation, busy state, themes, responsive layout, removal and empty state');
  } finally {if(browser)await browser.close();await new Promise(resolve=>server.close(resolve));}
}
run().catch(error=>{console.error(error);process.exitCode=1;});
