package cn.hhub.mobile;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.graphics.Color;
import android.view.View;
import android.widget.*;
import android.content.*;
import android.net.Uri;
import android.provider.MediaStore;
import android.provider.Settings;
import org.json.*;
import java.util.concurrent.*;

public final class MainActivity extends Activity {
    private final Handler handler=new Handler(Looper.getMainLooper());
    private final ExecutorService worker=Executors.newSingleThreadExecutor();
    private LinearLayout root,content,navigation; private TextView notice,connection;private ScrollView scroll;private HubStyle style;private String renderedSignature="";private final java.util.Map<String,TextView> metricValues=new java.util.HashMap<>();private JSONObject queuedAction;private int queuedGeneration;
    private HubClient client; private JSONObject pending,data=new JSONObject();
    private PairingStore store; private boolean started,busy,control,eqDirty; private int generation,page;
    private EditText pairAddress,pairPin,pairName,pairCode;
    private String eqSelectionId="";private String pendingDeviceId="";private JSONObject savedEqDraft;private EqCurveView eqCurve;private SeekBar eqPreamp;private CheckBox eqAutomatic;
    private static final int SCAN=101,CAPTURE=102,PHOTO=103;
    private final Runnable tick=new Runnable(){public void run(){if(started){refresh();handler.postDelayed(this,2000);}}};
    private interface Work { JSONObject run() throws Exception; }
    private interface Result { void accept(JSONObject data) throws Exception; }

    @Override public void onCreate(Bundle state){boolean night=(getResources().getConfiguration().uiMode&android.content.res.Configuration.UI_MODE_NIGHT_MASK)==android.content.res.Configuration.UI_MODE_NIGHT_YES;setTheme(getResources().getIdentifier(night?"HubThemeDark":"HubThemeLight","style",getPackageName()));super.onCreate(state);style=new HubStyle(this);store=new PairingStore(this);buildShell();
        try{client=store.load();}catch(Exception e){store.clear();message("已保存的授权无法读取，请重新配对");}
        if(state!=null&&client!=null){page=state.getInt("page",0);eqSelectionId=state.getString("eqSelectionId","");String draft=state.getString("eqDraft");if(draft!=null)try{savedEqDraft=new JSONObject(draft);eqDirty=true;}catch(JSONException ignored){}}
        if(client==null)showPair();else showDashboard();
        if(getIntent().getData()!=null)importQr(getIntent().getData().toString());}
    @Override protected void onNewIntent(Intent intent){super.onNewIntent(intent);setIntent(intent);if(intent.getData()!=null)importQr(intent.getData().toString());}
    @Override protected void onSaveInstanceState(Bundle out){super.onSaveInstanceState(out);out.putInt("page",page);out.putString("eqSelectionId",eqSelectionId);if(eqDirty&&eqCurve!=null&&eqPreamp!=null&&eqAutomatic!=null){JSONArray values=new JSONArray();for(double gain:eqCurve.gains())values.put(Double.valueOf(gain));out.putString("eqDraft",json("id",eqSelectionId,"bands",values,"preamp",eqPreamp.getProgress()/2.0-24,"autoPreamp",eqAutomatic.isChecked()).toString());}}
    @Override protected void onStart(){super.onStart();started=true;handler.post(tick);}
    @Override protected void onStop(){started=false;handler.removeCallbacks(tick);super.onStop();}
    @Override protected void onDestroy(){generation++;worker.shutdownNow();super.onDestroy();}
    private int dp(int n){return Math.round(n*getResources().getDisplayMetrics().density);}
    private TextView text(String value,int size){return style.text(value,size,false);}
    private TextView muted(String value){return style.text(value,13,true);}
    private TextView heading(String value,int size){TextView t=text(value,size);t.setTypeface(android.graphics.Typeface.create("sans-serif-medium",android.graphics.Typeface.NORMAL));return t;}
    private LinearLayout column(){LinearLayout v=new LinearLayout(this);v.setOrientation(LinearLayout.VERTICAL);return v;}

    private void buildShell(){
        root=column();root.setBackgroundColor(style.background);root.setOnApplyWindowInsetsListener((view,insets)->{root.setPadding(0,insets.getSystemWindowInsetTop(),0,insets.getSystemWindowInsetBottom());return insets;});
        LinearLayout header=new LinearLayout(this);header.setGravity(android.view.Gravity.CENTER_VERTICAL);header.setPadding(dp(20),dp(12),dp(16),dp(10));
        ImageView brand=new ImageView(this);brand.setImageResource(getResources().getIdentifier("hhub","drawable",getPackageName()));header.addView(brand,new LinearLayout.LayoutParams(dp(34),dp(34)));
        LinearLayout labels=column();labels.setPadding(dp(10),0,0,0);labels.addView(heading("H hub",18));connection=muted("连接你的电脑");labels.addView(connection);header.addView(labels,new LinearLayout.LayoutParams(0,-2,1));
        Button settings=new Button(this);settings.setText("连接");style.button(settings,false);settings.setOnClickListener(v->connectionHelp());header.addView(settings,new LinearLayout.LayoutParams(dp(72),dp(42)));root.addView(header);
        notice=muted("");notice.setPadding(dp(16),dp(10),dp(16),dp(10));notice.setBackground(style.surface(style.control,6,true));LinearLayout.LayoutParams info=new LinearLayout.LayoutParams(-1,-2);info.setMargins(dp(20),0,dp(20),dp(10));root.addView(notice,info);notice.setVisibility(View.GONE);
        scroll=new ScrollView(this);scroll.setFillViewport(true);scroll.setClipToPadding(false);content=column();content.setPadding(dp(20),dp(10),dp(20),dp(20));scroll.addView(content);root.addView(scroll,new LinearLayout.LayoutParams(-1,0,1));
        navigation=new LinearLayout(this);navigation.setPadding(dp(12),dp(8),dp(12),dp(8));root.addView(navigation);setContentView(root);
    }
    private void message(String value){notice.setText(value);notice.setVisibility(value==null||value.isEmpty()?View.GONE:View.VISIBLE);}
    private Button button(LinearLayout parent,String title,Runnable click,boolean enabled){Button b=new Button(this);b.setText(title);b.setEnabled(enabled);style.button(b,title.startsWith("扫描")||title.equals("申请配对")||title.startsWith("应用")||title.startsWith("读取"));b.setOnClickListener(v->click.run());LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(-1,dp(46));lp.topMargin=dp(8);parent.addView(b,lp);return b;}
    private EditText input(String title,String value,int type){content.addView(muted(title));EditText input=new EditText(this);input.setSingleLine(true);input.setInputType(type);input.setText(value);style.input(input);content.addView(input);return input;}
    private static JSONObject json(Object... pairs){JSONObject result=new JSONObject();try{for(int i=0;i<pairs.length;i+=2)result.put((String)pairs[i],pairs[i+1]);}catch(JSONException e){throw new IllegalArgumentException(e);}return result;}

    private void showPair(){renderedSignature="";navigation.setVisibility(View.GONE);content.removeAllViews();connection.setText("局域网 · HTTPS 安全配对");
        content.addView(heading("连接电脑",26));content.addView(muted("扫码建立局域网连接"));final LinearLayout outer=content;content=style.card(outer);
        button(content,"扫描电脑二维码",this::scanQr,true);
        button(content,"从相册识别二维码",()->launch(new Intent(Intent.ACTION_GET_CONTENT).setType("image/*").addCategory(Intent.CATEGORY_OPENABLE),PHOTO),true);
        button(content,"粘贴系统扫码结果",()->{android.content.ClipboardManager clipboard=(android.content.ClipboardManager)getSystemService(CLIPBOARD_SERVICE);ClipData clip=clipboard.getPrimaryClip();if(clip!=null&&clip.getItemCount()>0)importQr(clip.getItemAt(0).coerceToText(this).toString());else message("剪贴板没有二维码内容");},true);
        pairAddress=input("电脑地址","https://",android.text.InputType.TYPE_CLASS_TEXT|android.text.InputType.TYPE_TEXT_VARIATION_URI);
        pairPin=input("证书 SHA-256 指纹（扫码自动填入）","",android.text.InputType.TYPE_CLASS_TEXT|android.text.InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
        pairName=input("手机名称",android.os.Build.MODEL,android.text.InputType.TYPE_CLASS_TEXT);
        pairCode=input("六位配对码","",android.text.InputType.TYPE_CLASS_NUMBER);pairCode.setFilters(new android.text.InputFilter[]{new android.text.InputFilter.LengthFilter(6)});
        button(content,"申请配对",()->{if(busy)return;try{
            String value=pairCode.getText().toString();if(!value.matches("[0-9]{6}"))throw new Exception("配对码需要六位数字");
            String deviceName=pairName.getText().toString().trim();if(deviceName.isEmpty()||deviceName.length()>64)throw new Exception("手机名称需为 1–64 个字符");
            final HubClient candidate=new HubClient(pairAddress.getText().toString(),pairPin.getText().toString());client=candidate;generation++;
            submit(()->candidate.request("/api/pair",json("name",deviceName,"code",value)),reply->{pending=reply;message("请求已发送，请在电脑批准；三分钟后过期");content.removeAllViews();content.addView(text("等待电脑批准",24));button(content,"取消配对",this::disconnect,true);},true);
        }catch(Exception e){client=null;message(e.getMessage());}},true);
        Button help=button(content,"连接帮助",this::connectionHelp,true);content=outer;
    }
    private void importQr(String value){if(eqDirty){new android.app.AlertDialog.Builder(this).setTitle("存在未保存的 EQ 调节").setMessage("切换连接前要放弃这些调节吗？").setPositiveButton("放弃并继续",(d,w)->{eqDirty=false;importQr(value);}).setNegativeButton("取消",null).show();return;}try{final PairingQr qr=PairingQr.parse(value,System.currentTimeMillis()/1000);
        if(client!=null&&!client.token.isEmpty()&&client.fingerprint.equals(qr.fingerprint)){
            if(busy){message("正在同步，请稍后再次导入配对信息");return;}
            final HubClient endpoint=new HubClient(qr.address,qr.fingerprint);endpoint.token=client.token;
            submit(()->endpoint.request("/api/status",null),reply->{client=endpoint;generation++;store.save(endpoint);control=reply.optBoolean("control");data=reply.optJSONObject("data");if(data==null)data=new JSONObject();eqDirty=false;showDashboard();connection.setText("已连接 · "+(control?"可操作":"仅查看"));message("已切换电脑网络地址，保留原授权");},true);return;
        }
        Runnable fill=()->{if(client!=null||pending!=null)disconnect();pairAddress.setText(qr.address);pairPin.setText(qr.fingerprint);pairCode.setText(qr.code);message("已读取电脑配对信息，核对地址后点击申请配对");};
        if(client!=null&&!client.token.isEmpty())new android.app.AlertDialog.Builder(this).setTitle("连接二维码中的电脑？").setMessage("当前手机连接将被清除。电脑端的设备授权可在电脑撤销。").setPositiveButton("切换",(d,w)->fill.run()).setNegativeButton("取消",null).show();else fill.run();
    }catch(Exception e){message(e.getMessage());}}
    private void launch(Intent intent,int request){try{if(request<0)startActivity(intent);else startActivityForResult(intent,request);}catch(ActivityNotFoundException|SecurityException e){message("系统未提供此功能，可用相册识别或手动连接");}}
    private void scanQr(){Intent scanner=new Intent("com.google.zxing.client.android.SCAN").putExtra("SCAN_MODE","QR_CODE_MODE");
        if(scanner.resolveActivity(getPackageManager())!=null){launch(scanner,SCAN);return;}
        try{java.io.File file=QrCaptureProvider.file(this);if(file.exists())file.delete();Uri output=QrCaptureProvider.uri();
            Intent camera=new Intent(MediaStore.ACTION_IMAGE_CAPTURE).putExtra(MediaStore.EXTRA_OUTPUT,output).addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION|Intent.FLAG_GRANT_READ_URI_PERMISSION);
            camera.setClipData(ClipData.newRawUri("H hub 配对二维码",output));launch(camera,CAPTURE);message("使用系统相机拍下电脑二维码，照片会在识别后删除");
        }catch(Exception e){message(e.getMessage());}}
    @Override protected void onActivityResult(int request,int result,Intent intent){super.onActivityResult(request,result,intent);
        if(request==CAPTURE)revokeUriPermission(QrCaptureProvider.uri(),Intent.FLAG_GRANT_READ_URI_PERMISSION|Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
        if(result!=RESULT_OK){if(request==CAPTURE)QrCaptureProvider.file(this).delete();return;}
        if(request==SCAN){if(intent!=null)importQr(intent.getStringExtra("SCAN_RESULT"));}
        else if(request==CAPTURE||request==PHOTO){final Uri photo=request==CAPTURE?QrCaptureProvider.uri():intent==null?null:intent.getData();if(photo==null){message("未收到二维码图片");return;}
            submit(()->{try{return json("value",QrPhotoDecoder.decode(this,photo));}finally{if(request==CAPTURE)QrCaptureProvider.file(this).delete();}},reply->importQr(reply.getString("value")),true);}
    }
    private void connectionHelp(){new android.app.AlertDialog.Builder(this).setTitle("连接与网络").setItems(new String[]{"扫描配对二维码","Wi-Fi 设置","蓝牙配对","网络共享设置","蓝牙连接帮助","断开本机连接"},(dialog,which)->{switch(which){case 0:scanQr();break;case 1:launch(new Intent(Settings.ACTION_WIFI_SETTINGS),-1);break;case 2:launch(new Intent(Settings.ACTION_BLUETOOTH_SETTINGS),-1);break;case 3:launch(new Intent(Settings.ACTION_WIRELESS_SETTINGS),-1);break;case 4:new android.app.AlertDialog.Builder(this).setTitle("蓝牙连接辅助").setMessage("在手机开启蓝牙网络共享，电脑连接手机的 PAN，再选择电脑蓝牙网卡地址重新扫码。需要两端支持蓝牙网络共享。").setPositiveButton("知道了",null).show();break;case 5:new android.app.AlertDialog.Builder(this).setTitle("断开连接？").setMessage("清除本机授权；电脑端授权可在电脑撤销。").setPositiveButton("断开",(d,w)->disconnect()).setNegativeButton("取消",null).show();break;}}).show();}
    private void disconnect(){generation++;queuedAction=null;pendingDeviceId="";pending=null;client=null;data=new JSONObject();control=false;eqDirty=false;store.clear();showPair();message("已断开本机连接；可在电脑端撤销设备授权");}
    private void submit(Work work,Result result,boolean action){if(busy)return;busy=true;final int ticket=generation;
        worker.execute(()->{JSONObject reply=null;Exception failure=null;try{reply=work.run();}catch(Exception e){failure=e;}final JSONObject answer=reply;final Exception error=failure;
            handler.post(()->{busy=false;if(ticket!=generation||isDestroyed())return;
                if(error!=null){if(!action)connection.setText("连接未就绪");message(error.getMessage()==null?"无法连接电脑":error.getMessage());
                    if(error instanceof HubClient.ApiException&&((HubClient.ApiException)error).status==401){store.clear();client=null;pending=null;showPair();message("电脑授权已失效，请重新配对");}
                    else if(pending!=null&&error instanceof HubClient.ApiException&&((HubClient.ApiException)error).status==403){pending=null;client=null;showPair();message("配对请求已失效，请重试");}if(client!=null&&!client.token.isEmpty()&&!eqDirty){renderedSignature="";showDashboard();}drainAction();return;}
                try{result.accept(answer);}catch(Exception e){message(e.getMessage());}drainAction();
            });});
    }
    private void drainAction(){if(queuedAction!=null&&!busy&&started){JSONObject next=queuedAction;queuedAction=null;if(queuedGeneration==generation)act(next);}}
    private void refresh(){if(queuedAction!=null){drainAction();if(busy)return;}if(busy||client==null)return;final HubClient active=client;
        if(pending!=null){final JSONObject request=json("id",pending.optString("id"),"pollSecret",pending.optString("pollSecret"),"native",true);
            submit(()->active.request("/api/pair-status",request),reply->{String status=reply.optString("status");
                if(status.equals("approved")){String token=reply.optString("token");if(token.length()<32)throw new Exception("电脑服务版本过旧，请更新支持原生 App 的版本");active.token=token;pending=null;store.save(active);message("配对成功");showDashboard();refresh();}
                else if(status.equals("rejected")){pending=null;client=null;showPair();message("电脑拒绝了配对请求");}},false);
        }else if(!active.token.isEmpty())submit(()->active.request("/api/status",null),reply->{boolean previous=control;control=reply.optBoolean("control");data=reply.optJSONObject("data");if(data==null)data=new JSONObject();
            connection.setText("已连接 · "+(control?"可操作":"仅查看"));if(previous!=control&&!control){eqDirty=false;message("电脑已关闭操作权限");}
            checkDeviceCompletion();if(!(page==3&&eqDirty&&eqCurve!=null))showDashboard();},false);
    }
    private void act(JSONObject body){if(!control){message("请在电脑端允许此手机操作");return;}if(eqDirty&&!body.optString("kind").equals("eqSave")){message("请先应用或取消 EQ 调节");return;}if(busy){if(queuedAction==null){queuedAction=body;queuedGeneration=generation;message("请求已排队，等待当前同步完成");}else message("请等待当前操作完成");return;}final HubClient active=client;if(active==null)return;
        submit(()->active.request("/api/action",body),reply->{eqDirty=false;savedEqDraft=null;boolean device=body.optString("kind").equals("deviceRead")||body.optString("kind").equals("deviceWrite");if(device)pendingDeviceId=body.optString("id");message(device?"请求已提交，等待设备回读":"电脑已接受操作");JSONObject updated=reply.optJSONObject("data");if(updated!=null)data=updated;checkDeviceCompletion();showDashboard();},true);}

    private void checkDeviceCompletion(){if(pendingDeviceId.isEmpty())return;JSONArray devices=data.optJSONArray("devices");if(devices==null)return;for(int i=0;i<devices.length();i++){JSONObject device=devices.optJSONObject(i);if(device==null||!device.optString("id").equals(pendingDeviceId))continue;JSONObject status=device.optJSONObject("configuration");if(status==null)return;if(status.optString("state").equals("confirmed")){pendingDeviceId="";message("设备参数已回读确认");}else if(status.optString("state").equals("failed")){pendingDeviceId="";message(status.optString("error","设备操作未完成"));}return;}}
    private void showDashboard(){
        navigation.setVisibility(View.VISIBLE);
        if(!Integer.valueOf(page).equals(navigation.getTag())){navigation.setTag(Integer.valueOf(page));String[] names={"监测","外设","场景","EQ"};navigation.removeAllViews();
        for(int i=0;i<names.length;i++){final int selected=i;LinearLayout tab=column();tab.setGravity(android.view.Gravity.CENTER);tab.setPadding(0,dp(6),0,dp(6));if(page==i)tab.setBackground(style.surface(style.control,8,false));ImageView icon=new ImageView(this);style.icon(icon,i,page==i);tab.addView(icon,new LinearLayout.LayoutParams(dp(23),dp(23)));TextView label=style.text(names[i],12,page!=i);if(page==i)label.setTextColor(style.accent);tab.addView(label);tab.setContentDescription(names[i]);tab.setOnClickListener(v->{if(eqDirty){message("请先应用或取消 EQ 调节");return;}page=selected;renderedSignature="";showDashboard();scroll.scrollTo(0,0);});navigation.addView(tab,new LinearLayout.LayoutParams(0,dp(64),1));}}
        if(page==0&&renderedSignature.equals("monitor:"+control)){updateMetrics();return;}
        String signature=page==0?"monitor:"+control:page==1?"devices:"+control+data.optJSONArray("devices"):page==2?"scenes:"+control+data.optJSONArray("scenes")+data.optJSONArray("profiles")+data.optString("scene")+data.optBoolean("sceneLocked"):"eq:"+control+eqSelectionId+eqParameterSignature()+data.optJSONArray("presets");
        if(signature.equals(renderedSignature))return;
        renderedSignature=signature;int offset=scroll.getScrollY();content.removeAllViews();metricValues.clear();
        if(!control){LinearLayout permission=style.card(content);permission.addView(muted("当前仅查看 · 操作权限需在电脑开启"));}
        if(page==0)monitor();else if(page==1)devices();else if(page==2)scenes();else equalizer();
        scroll.post(()->scroll.scrollTo(0,offset));
    }

    private void monitor(){
        content.addView(heading("系统监测",26));content.addView(muted("电脑运行状态"));
        String[][] rows={{"CPU 使用率","cpu","GPU 使用率","gpu"},{"内存使用率","memory","CPU 温度","cpuTemp"},{"GPU 温度","gpuTemp","已用内存","memoryUsedGB"}};
        for(String[] row:rows){LinearLayout line=new LinearLayout(this);content.addView(line);for(int i=0;i<4;i+=2){LinearLayout holder=column();LinearLayout.LayoutParams hp=new LinearLayout.LayoutParams(0,-2,1);hp.rightMargin=i==0?dp(10):0;line.addView(holder,hp);LinearLayout tile=style.card(holder);tile.addView(muted(row[i]));TextView value=heading("—",28);tile.addView(value);metricValues.put(row[i+1],value);}}
        LinearLayout details=style.card(content);details.addView(muted("内存总量"));TextView memory=text("—",18);details.addView(memory);metricValues.put("memoryTotalGB",memory);TextView timestamp=muted("等待采样");content.addView(timestamp);metricValues.put("sampleTime",timestamp);updateMetrics();
    }
    private void updateMetrics(){JSONObject m=data.optJSONObject("monitor");if(m==null)m=new JSONObject();for(java.util.Map.Entry<String,TextView> entry:metricValues.entrySet()){String key=entry.getKey();if(key.equals("sampleTime")){entry.getValue().setText(data.has("sampleTime")?"更新于 "+new java.text.SimpleDateFormat("HH:mm:ss",java.util.Locale.ROOT).format(new java.util.Date((long)(data.optDouble("sampleTime")*1000))):"等待采样");continue;}double n=m.optDouble(key,Double.NaN);String unit=key.endsWith("Temp")?"°C":key.endsWith("GB")?"GB":"%";entry.getValue().setText(Double.isFinite(n)&&n>=0?String.format(java.util.Locale.ROOT,key.endsWith("GB")?"%.1f %s":"%.0f %s",n,unit):"不可用");}}
    private void valueRow(LinearLayout parent,String label,String value,Runnable click,boolean enabled){
        LinearLayout row=new LinearLayout(this);row.setGravity(android.view.Gravity.CENTER_VERTICAL);row.setPadding(0,dp(8),0,dp(8));row.setMinimumHeight(dp(52));TextView title=text(label,14);row.addView(title,new LinearLayout.LayoutParams(0,-2,1));TextView current=text(value+(click==null?"":"  ›"),15);current.setTextColor(click!=null&&enabled?style.accent:style.muted);row.addView(current);parent.addView(row);View divider=new View(this);divider.setBackgroundColor(style.border);parent.addView(divider,new LinearLayout.LayoutParams(-1,dp(1)));if(click!=null){row.setEnabled(enabled);row.setOnClickListener(v->click.run());}}
    private void numberControl(String id,String key,String title,double value,double min,double max,double step){
        EditText input=new EditText(this);input.setInputType(android.text.InputType.TYPE_CLASS_NUMBER|android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL);input.setText(String.valueOf(value));style.input(input);LinearLayout box=column();box.setPadding(dp(24),dp(8),dp(24),0);box.addView(input);box.addView(muted("范围 "+min+"–"+max+"，步进 "+step));
        android.app.AlertDialog dialog=new android.app.AlertDialog.Builder(this).setTitle(title).setView(box).setPositiveButton("应用",null).setNegativeButton("取消",null).create();dialog.setOnShowListener(ignored->{dialog.getButton(android.app.AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{try{double number=Double.parseDouble(input.getText().toString());if(!Double.isFinite(number)||number<min||number>max||Math.abs(number/step-Math.rint(number/step))>1e-6){input.setError("数值超出范围或步进不正确");return;}dialog.dismiss();act(json("kind","deviceWrite","id",id,"patch",json(key,number)));}catch(NumberFormatException e){input.setError("请输入有效数值");}});});dialog.show();
    }
    private void chooseControl(String id,String key,String title,String[] labels,double[] values,int selected){new android.app.AlertDialog.Builder(this).setTitle(title).setSingleChoiceItems(labels,selected,(d,index)->{d.dismiss();act(json("kind","deviceWrite","id",id,"patch",json(key,values[index])));}).setNegativeButton("取消",null).show();}
    private void switchControl(LinearLayout parent,String id,String key,String title,boolean on,boolean enabled){Switch toggle=new Switch(this);toggle.setText(title);toggle.setTextColor(style.text);toggle.setTextSize(14);toggle.setChecked(on);toggle.setEnabled(enabled);toggle.setMinimumHeight(dp(52));toggle.setThumbTintList(new android.content.res.ColorStateList(new int[][]{new int[]{android.R.attr.state_checked},new int[]{}},new int[]{style.accent,style.muted}));parent.addView(toggle);toggle.setOnCheckedChangeListener((button,checked)->{toggle.setEnabled(false);act(json("kind","deviceWrite","id",id,"patch",json(key,checked)));});}
    private void devices(){
        content.addView(heading("我的设备",26));content.addView(muted("选择设备，读取并调节真实参数"));JSONArray devices=data.optJSONArray("devices");
        if(devices==null||devices.length()==0){LinearLayout empty=style.card(content);empty.addView(text("尚未检测到外设",16));return;}
        for(int i=0;i<devices.length();i++){JSONObject d=devices.optJSONObject(i);if(d==null)continue;final String id=d.optString("id");LinearLayout tile=style.card(content);tile.addView(heading(d.optString("name","外设"),18));tile.addView(muted(d.optString("status",d.optString("connection","已检测"))));
            if(d.has("battery")||d.has("batteryLevel"))valueRow(tile,"电量",d.has("battery")?d.optInt("battery")+"%":d.optString("batteryLevel"),null,false);
            if(d.optBoolean("canVibrate"))button(tile,"测试振动",()->act(json("kind","deviceVibrate","id",id)),control);
            if(!d.optBoolean("canConfigure")){tile.addView(muted(d.optString("controlReason","此设备暂未提供手机控制入口")));continue;}
            JSONObject status=d.optJSONObject("configuration");String state=status==null?"":status.optString("state");boolean waiting=state.equals("waiting")||state.equals("running");
            TextView feedback=muted(waiting?"正在读取或写入，等待设备回读":state.equals("confirmed")?"参数已回读确认":state.equals("failed")?status.optString("error","操作未完成"):"先读取设备参数");if(state.equals("failed"))feedback.setTextColor(Color.parseColor("#c42b1c"));tile.addView(feedback);
            button(tile,waiting?"等待回读…":"读取当前设置",()->act(json("kind","deviceRead","id",id)),control&&!waiting);
            if(status!=null&&status.has("sampleTime"))tile.addView(muted("上次回读 "+new java.text.SimpleDateFormat("HH:mm:ss",java.util.Locale.ROOT).format(new java.util.Date((long)(status.optDouble("sampleTime")*1000)))));JSONObject values=status==null?null:status.optJSONObject("values");if(values==null)continue;boolean enabled=control&&!waiting;
            final double dpi=values.optDouble("dpi");valueRow(tile,"当前档位 DPI",String.format(java.util.Locale.ROOT,"%.0f",dpi),()->numberControl(id,"dpi","DPI",dpi,50,30000,50),enabled);
            if(values.has("selected")){int count=values.optInt("stageCount",1);String[] labels=new String[count];double[] indexes=new double[count];for(int n=0;n<count;n++){labels[n]="档位 "+(n+1);indexes[n]=n;}valueRow(tile,"DPI 档位",String.valueOf(values.optInt("selected")+1),()->chooseControl(id,"selected","DPI 档位",labels,indexes,values.optInt("selected")),enabled);}
            if(values.has("polling")){final double hz=values.optDouble("polling");String[] labels={"125 Hz","250 Hz","500 Hz","1000 Hz","2000 Hz","4000 Hz","8000 Hz"};double[] rates={125,250,500,1000,2000,4000,8000};int index=0;for(int n=0;n<rates.length;n++)if(rates[n]==hz)index=n;final int chosen=index;valueRow(tile,"回报率",String.format(java.util.Locale.ROOT,"%.0f Hz",hz),()->chooseControl(id,"polling","回报率",labels,rates,chosen),enabled);}
            for(String[] field:new String[][]{{"lod","抬升高度","mm"},{"press","按下延迟","ms"},{"release","释放延迟","ms"},{"minutes","休眠时间","分钟"}}){String key=field[0];if(!values.has(key))continue;final double value=values.optDouble(key);double min=key.equals("lod")?.7:key.equals("minutes")?2:0,max=key.equals("lod")?1.7:key.equals("minutes")?120:32,step=key.equals("lod")?.1:1;valueRow(tile,field[1],String.format(java.util.Locale.ROOT,key.equals("lod")?"%.1f %s":"%.0f %s",value,field[2]),()->numberControl(id,key,field[1],value,min,max,step),enabled);}
            for(String[] field:new String[][]{{"motion","运动同步"},{"straight","直线修正"},{"ripple","纹波修正"},{"sleep","自动休眠"},{"lowBattery","低电量提醒"}})if(values.has(field[0]))switchControl(tile,id,field[0],field[1],values.optBoolean(field[0]),enabled);
        }
    }
    private String profileState(String value){switch(value){case "confirmed":return "设备已回读确认";case "failed":return "配置应用失败";case "waitingAudio":return "等待音频链路确认";case "waitingPlugin":return "等待插件连接与回读";default:return "未在当前场景应用";}}
    private void scenes(){content.addView(heading("场景与配置",26));content.addView(text(data.optBoolean("sceneLocked")?"手动锁定场景":"根据电脑前台应用自动切换",15));
        if(data.optBoolean("sceneLocked"))button(content,"恢复自动场景",()->act(json("kind","sceneAuto")),control);
        JSONArray scenes=data.optJSONArray("scenes");if(scenes!=null)for(int i=0;i<scenes.length();i++){JSONObject scene=scenes.optJSONObject(i);if(scene==null)continue;final String id=scene.optString("id");
            button(content,(id.equals(data.optString("scene"))?"✓ ":"")+scene.optString("name")+" · "+scene.optInt("bindingCount")+" 套配置",()->act(json("kind","scene","id",id)),control);
            content.addView(text("关联应用 "+scene.optInt("appCount")+" · 优先级 "+scene.optInt("priority"),14));}
        content.addView(text("已保存的外设配置",20));JSONArray profiles=data.optJSONArray("profiles");if(profiles==null||profiles.length()==0){content.addView(text("请先在电脑建立具名配置并绑定到场景",15));return;}
        for(int i=0;i<profiles.length();i++){JSONObject p=profiles.optJSONObject(i);if(p==null)continue;final String id=p.optString("id");content.addView(text(p.optString("deviceName","外设")+" · "+p.optString("name"),18));
            content.addView(text(profileState(p.optString("state")),14));if(!p.optString("error").isEmpty())content.addView(text(p.optString("error"),14));
            if(p.optBoolean("inCurrentScene")&&p.optBoolean("saved")){button(content,"在当前场景使用此配置",()->act(json("kind","profileSelect","id",id)),control);
                if(p.optBoolean("selected")&&!p.optString("state").equals("confirmed"))button(content,"重试应用与回读",()->act(json("kind","profileRetry","id",id)),control);}
        }
        content.addView(text("切换配置只提交应用请求，只有插件或音频链路回读确认后才显示成功。配置创建、编辑和场景绑定在电脑完成。",14));
    }

    private String eqParameterSignature(){StringBuilder signature=new StringBuilder();JSONArray configs=data.optJSONArray("eq");if(configs!=null)for(int i=0;i<configs.length();i++){JSONObject c=configs.optJSONObject(i);if(c!=null)for(String key:new String[]{"id","name","outputName","enabled","active","bands","frequencies","preamp","autoPreamp","backgroundAutoStart","preset","state","error","acknowledged"})signature.append(key).append(c.opt(key));}return signature.toString();}
    private JSONObject mergeEqDraft(JSONObject base){if(!eqDirty||savedEqDraft==null||!savedEqDraft.optString("id").equals(base.optString("id")))return base;try{JSONObject result=new JSONObject(base.toString());for(String key:new String[]{"bands","preamp","autoPreamp"})result.put(key,savedEqDraft.get(key));return result;}catch(JSONException e){return base;}}
    private void equalizer(){
        content.addView(heading("均衡器",26));content.addView(muted("选择预设与输出，拖动曲线或旋钮调节"));
        final JSONArray configs=data.optJSONArray("eq");if(configs==null||configs.length()==0){LinearLayout empty=style.card(content);empty.addView(text("电脑上还没有音频配置",18));empty.addView(muted("先在电脑选择应用和输出设备，保存一套 EQ 配置。"));return;}
        int current=0;java.util.List<String> targets=new java.util.ArrayList<>();for(int i=0;i<configs.length();i++){JSONObject item=configs.optJSONObject(i);targets.add(item==null?"不可用配置":item.optString("name")+" · "+item.optString("outputName"));if(item!=null&&item.optString("id").equals(eqSelectionId))current=i;}
        final int index=current;JSONObject base=configs.optJSONObject(index);if(base==null)return;final JSONObject config=mergeEqDraft(base);eqSelectionId=config.optString("id");final String id=eqSelectionId;
        final JSONArray savedBands=config.optJSONArray("bands"),frequencies=config.optJSONArray("frequencies");if(savedBands==null||savedBands.length()!=12||frequencies==null||frequencies.length()!=12){LinearLayout empty=style.card(content);empty.addView(text("此配置的频段参数不完整",16));return;}
        boolean wide=getResources().getConfiguration().orientation==android.content.res.Configuration.ORIENTATION_LANDSCAPE;LinearLayout selectors=style.card(content);LinearLayout selectorRow=new LinearLayout(this);selectorRow.setOrientation(wide?LinearLayout.HORIZONTAL:LinearLayout.VERTICAL);selectors.addView(selectorRow);LinearLayout presetColumn=column(),outputColumn=column();selectorRow.addView(presetColumn,new LinearLayout.LayoutParams(wide?0:-1,-2,wide?1:0));LinearLayout.LayoutParams outputParams=new LinearLayout.LayoutParams(wide?0:-1,-2,wide?1:0);if(wide)outputParams.leftMargin=dp(16);selectorRow.addView(outputColumn,outputParams);
        outputColumn.addView(muted("应用 / 输出设备"));final Spinner target=new Spinner(this);target.setAdapter(new ArrayAdapter<String>(this,android.R.layout.simple_spinner_dropdown_item,targets));target.setSelection(index);outputColumn.addView(target);
        target.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener(){public void onNothingSelected(AdapterView<?> p){}public void onItemSelected(AdapterView<?> p,View v,int position,long row){if(position==index)return;if(eqDirty){message("请先应用或取消 EQ 调节");target.setSelection(index);return;}JSONObject item=configs.optJSONObject(position);if(item!=null){eqSelectionId=item.optString("id");renderedSignature="";showDashboard();}}});
        presetColumn.addView(muted("声音预设"));JSONArray presets=data.optJSONArray("presets");final java.util.List<String> presetIds=new java.util.ArrayList<>(),presetNames=new java.util.ArrayList<>();presetIds.add("");presetNames.add("自定义");int chosen=0;
        if(presets!=null)for(int i=0;i<presets.length();i++){JSONObject preset=presets.optJSONObject(i);if(preset==null)continue;presetIds.add(preset.optString("id"));presetNames.add(preset.optString("name"));if(preset.optString("id").equals(config.optString("preset")))chosen=presetIds.size()-1;}
        final Spinner preset=new Spinner(this);preset.setAdapter(new ArrayAdapter<String>(this,android.R.layout.simple_spinner_dropdown_item,presetNames));preset.setSelection(chosen);preset.setEnabled(control);presetColumn.addView(preset);
        button(presetColumn,"应用预设",()->{String selected=presetIds.get(Math.max(0,preset.getSelectedItemPosition()));if(selected.isEmpty()){message("自定义预设由下方曲线调节");return;}act(json("kind","eqPreset","id",id,"preset",selected));},control);
        LinearLayout status=style.card(content);status.addView(heading(config.optString("outputName","音频输出"),16));status.addView(muted(config.optString("state","等待电脑状态")));if(!config.optString("error").isEmpty())status.addView(text(config.optString("error"),14));
        double[] bands=new double[12],centers=new double[12];for(int i=0;i<12;i++){bands[i]=savedBands.optDouble(i);centers[i]=frequencies.optDouble(i);}
        LinearLayout eqArea=new LinearLayout(this);eqArea.setOrientation(wide?LinearLayout.HORIZONTAL:LinearLayout.VERTICAL);content.addView(eqArea);LinearLayout editor=style.card(eqArea);if(wide){LinearLayout.LayoutParams ep=new LinearLayout.LayoutParams(0,-2,1);ep.leftMargin=dp(12);ep.bottomMargin=dp(12);editor.setLayoutParams(ep);}editor.addView(heading("12 段均衡曲线",18));editor.addView(muted("±12 dB · 拖动后点击应用"));
        final TextView selectedValue=text(EqCurveView.frequency(centers[0])+" · "+bands[0]+" dB",15);selectedValue.setTextColor(style.accent);
        final SeekBar precise=new SeekBar(this);precise.setMax(48);precise.setProgress((int)Math.round((bands[0]+12)*2));precise.setEnabled(control);style.tint(precise);
        final Spinner bandPicker=new Spinner(this);java.util.List<String> bandNames=new java.util.ArrayList<>();for(double hz:centers)bandNames.add(EqCurveView.frequency(hz));bandPicker.setAdapter(new ArrayAdapter<String>(this,android.R.layout.simple_spinner_dropdown_item,bandNames));
        final EqCurveView curve=new EqCurveView(this,bands,centers,style,(band,gain)->{eqDirty=true;preset.setSelection(0);bandPicker.setSelection(band);precise.setProgress((int)Math.round((gain+12)*2));selectedValue.setText(EqCurveView.frequency(centers[band])+" · "+gain+" dB");});
        eqCurve=curve;curve.setEnabled(control);HorizontalScrollView graphScroll=new HorizontalScrollView(this);graphScroll.setHorizontalScrollBarEnabled(true);graphScroll.setFillViewport(true);graphScroll.addView(curve,new android.view.ViewGroup.LayoutParams(Math.max(dp(660),getResources().getDisplayMetrics().widthPixels-dp(wide?290:72)),dp(wide?250:300)));editor.addView(graphScroll);
        editor.addView(selectedValue);LinearLayout precision=column();precision.addView(bandPicker);precision.addView(precise);precision.setVisibility(View.GONE);button(editor,"精确调节频段",()->precision.setVisibility(precision.getVisibility()==View.GONE?View.VISIBLE:View.GONE),true);editor.addView(precision);
        bandPicker.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener(){public void onNothingSelected(AdapterView<?> p){}public void onItemSelected(AdapterView<?> p,View view,int band,long row){double gain=curve.gains()[band];precise.setProgress((int)Math.round((gain+12)*2));selectedValue.setText(EqCurveView.frequency(centers[band])+" · "+gain+" dB");}});
        precise.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){public void onProgressChanged(SeekBar b,int progress,boolean user){if(user){eqDirty=true;preset.setSelection(0);int band=bandPicker.getSelectedItemPosition();double gain=progress/2.0-12;curve.setGain(band,gain);selectedValue.setText(EqCurveView.frequency(centers[band])+" · "+gain+" dB");}}public void onStartTrackingTouch(SeekBar b){}public void onStopTrackingTouch(SeekBar b){}});
        LinearLayout processing=style.card(eqArea);if(wide){eqArea.removeView(processing);eqArea.addView(processing,0);LinearLayout.LayoutParams pp=new LinearLayout.LayoutParams(dp(208),-2);pp.bottomMargin=dp(12);processing.setLayoutParams(pp);}processing.addView(heading("音频处理",18));final TextView attenuation=text("前级增益 · "+config.optDouble("preamp",0)+" dB",14);processing.addView(attenuation);final SeekBar preamp=new SeekBar(this);preamp.setMax(48);preamp.setProgress((int)Math.round((config.optDouble("preamp",0)+24)*2));preamp.setEnabled(control);eqPreamp=preamp;style.tint(preamp);processing.addView(preamp);
        preamp.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){public void onProgressChanged(SeekBar b,int progress,boolean user){if(user){eqDirty=true;preset.setSelection(0);attenuation.setText("前级增益 · "+(progress/2.0-24)+" dB");}}public void onStartTrackingTouch(SeekBar b){}public void onStopTrackingTouch(SeekBar b){}});
        final CheckBox automatic=new CheckBox(this);automatic.setText("自动前级衰减");automatic.setTextColor(style.text);automatic.setChecked(config.optBoolean("autoPreamp",true));automatic.setEnabled(control);eqAutomatic=automatic;processing.addView(automatic);automatic.setOnCheckedChangeListener((b,on)->{eqDirty=true;preset.setSelection(0);});
        button(processing,"应用调节",()->{JSONArray gains=new JSONArray();for(double gain:curve.gains())gains.put(Double.valueOf(gain));act(json("kind","eqSave","id",id,"enabled",config.optBoolean("enabled"),"bands",gains,"preamp",preamp.getProgress()/2.0-24,"autoPreamp",automatic.isChecked()));},control);
        button(processing,"取消调节",()->{eqDirty=false;savedEqDraft=null;renderedSignature="";showDashboard();},true);
        button(processing,config.optBoolean("enabled")?"切为旁路":"启用均衡器",()->act(json("kind","eqBypass","id",id,"enabled",!config.optBoolean("enabled"))),control);
        button(processing,config.optBoolean("active")||config.optBoolean("backgroundAutoStart")?"停止此输出处理":"启动后台音频处理",()->act(json("kind",config.optBoolean("active")||config.optBoolean("backgroundAutoStart")?"eqStop":"eqStart","id",id)),control);
    }
}
