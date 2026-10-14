'use strict';
(() => {
  const bridge = window.chrome?.webview;
  const byId = id => document.getElementById(id);
  const setText = (node, value) => { value = String(value ?? ''); if (node.textContent !== value) node.textContent = value; };
  const setImage = (node, value) => { if (node.getAttribute('src') !== value) node.setAttribute('src', value); };
  let selected = '', previousSelected = null, diskKey = '', busy = false;
  const tiles = new Map(), fields = new Map();
  const post = (action, id = '') => bridge?.postMessage({type:'hhub.home.action', action, id});
  function metricRow(parent, row) {
    const node = document.createElement('div'); node.className = 'metric-row';
    const label = document.createElement('span'); label.className = 'muted'; setText(label,row.label);
    const value = document.createElement('span'); value.className = 'metric-value';
    node.append(label,value); parent.append(node); fields.set(row.key,value);
  }
  function createMetrics(message) {
    byId('metrics').replaceChildren(); fields.clear();
    for (const metric of message.metrics) {
      const card = document.createElement('article'); card.className = 'card metric-card';
      const heading = document.createElement('div'); heading.className = 'metric-heading';
      const icon = document.createElement('span'); icon.className = 'metric-icon'; icon.setAttribute('aria-hidden','true'); setText(icon,metric.icon);
      const label = document.createElement('strong'); setText(label,metric.label); heading.append(icon,label);
      if (metric.modelKey) { const model = document.createElement('span'); model.className='muted model'; fields.set(metric.modelKey,model); heading.append(model); }
      card.append(heading);
      for (const row of metric.rows) {
        if (row.heading) { const label=document.createElement('div');label.className='muted disk-name';setText(label,row.heading);label.title=row.heading; if(card.children.length>1){const divider=document.createElement('div');divider.className='disk-divider';card.append(divider);}card.append(label); }
        else metricRow(card,row);
      }
      byId('metrics').append(card);
    }
  }
  function update(message) {
    document.documentElement.dataset.theme = message.theme.theme;
    document.documentElement.dataset.reduceMotion = String(message.theme.reduceMotion);
    document.documentElement.style.setProperty('--accent',message.theme.colors.accent);
    const nextKey = JSON.stringify(message.metrics.map(m=>[m.label,m.rows.map(r=>[r.key,r.heading])]));
    if(nextKey!==diskKey){diskKey=nextKey;createMetrics(message);}
    for(const [key,value] of Object.entries(message.values)) {
      const node=fields.get(key); if(!node)continue;
      setText(node,value.text);node.title=value.hint||'';
      if(node.classList.contains('metric-value')){node.classList.toggle('unavailable',!!value.error);node.classList.toggle('neutral',!!value.neutral);}
    }
    selected=message.selected; busy=message.busy; byId('refresh').disabled=busy;
    setText(byId('device-count'),message.devices.length+' 个外设');
    const ids=new Set(message.devices.map(d=>d.id));
    for(const [id,tile]of tiles)if(!ids.has(id)){tile.remove();tiles.delete(id);}
    message.devices.forEach((device,index)=>{
      let tile=tiles.get(device.id);
      if(!tile){tile=document.createElement('button');tile.className='device-tile';tile.dataset.id=device.id;const image=document.createElement('img');image.alt='';const labels=document.createElement('span');const name=document.createElement('span');name.className='tile-name';const status=document.createElement('span');status.className='tile-status';labels.append(name,status);tile.append(image,labels);tile.addEventListener('click',()=>{if(!busy)post('select',device.id);});tiles.set(device.id,tile);}
      tile.setAttribute('aria-pressed',String(device.id===selected));tile.setAttribute('aria-label','选择设备：'+device.name);tile.title=device.name;
      setImage(tile.querySelector('img'),device.art);setText(tile.querySelector('.tile-name'),device.name);setText(tile.querySelector('.tile-status'),device.tileStatus);
      const at=byId('device-list').children[index];if(at!==tile)byId('device-list').insertBefore(tile,at||null);
    });
    const device=message.devices.find(d=>d.id===selected);
    byId('device-info').hidden=!device;byId('device-art').hidden=!device;byId('empty').hidden=!!device;
    if(device){setText(byId('device-name'),device.name);setText(byId('device-kind'),device.kindLabel);setText(byId('device-status'),device.status);setText(byId('device-battery'),device.battery);setText(byId('device-profile'),device.profile);setText(byId('art-note'),device.artNote);setImage(byId('hero-image'),device.art);byId('connection-dot').classList.toggle('connected',device.connected);}
    setText(byId('scene'),message.scene);byId('scene').title=message.scene;
    if(previousSelected!==selected){previousSelected=selected;const tile=tiles.get(selected);if(tile){const list=byId('device-list');list.scrollTop=Math.max(0,tile.offsetTop-list.offsetTop-(list.clientHeight-tile.clientHeight)/2);}}
  }
  document.querySelectorAll('[data-action]').forEach(button=>button.addEventListener('click',()=>post(button.dataset.action,button.dataset.action==='details'?selected:'')));
  bridge?.addEventListener('message',event=>{if(event.data?.type==='hhub.home.state'){update(event.data);byId('load-status').hidden=true;}});
  post('ready');
  if(!bridge){byId('load-status').hidden=false;setText(byId('load-status'),'请从 H hub 应用打开主页以连接实时数据。');}
})();
