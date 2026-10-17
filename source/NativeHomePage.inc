// Native WinUI 3 monitoring page. Telemetry patches existing controls in place.
JsonObject homeSnapshot(){if(state.HasKey(L"homeMonitoring"))return state.GetNamedObject(L"homeMonitoring");return HomeMonitoring::build(state,clockSeconds());}
std::wstring homeFieldText(JsonObject snapshot,std::wstring const& key,std::wstring const& unit,int precision=0){
 auto field=snapshot.GetNamedObject(L"fields",JsonObject{}).GetNamedObject(key,JsonObject{});
 auto value=field.GetNamedValue(L"value",JsonValue::CreateNullValue());
 if(value.ValueType()==JsonValueType::Number)return digits(value.GetNumber(),precision)+unit;
 auto status=translated(str(field,L"status"));
 return status==L"initializing"?tr(L"读取中…"):status==L"stale"?tr(L"等待更新"):status==L"needsDriver"?tr(L"需驱动"):status==L"needsElevation"?tr(L"需授权"):tr(L"未提供");
}
TextBlock homeValue(std::wstring key,std::wstring unit,int precision=0,double size=14){
 auto value=text(L"",size);auto update=[this,value,key,unit,precision](JsonObject){
  auto snapshot=homeSnapshot();value.Text(homeFieldText(snapshot,key,unit,precision));
  auto field=snapshot.GetNamedObject(L"fields",JsonObject{}).GetNamedObject(key,JsonObject{});
  bool valid=field.GetNamedValue(L"value",JsonValue::CreateNullValue()).ValueType()==JsonValueType::Number;
  value.Foreground(valid?(dark?brush(245,245,245):brush(27,27,27)):(dark?brush(164,178,195):brush(95,105,120)));
  ToolTipService::SetToolTip(value,box_value(valid?str(field,L"source"):str(field,L"reason")));
 };update(monitoring());monitorBindings.push_back(update);return value;
}
#include "CompactSystemMonitoring.inc"
#include "DeviceMonitoringPage.inc"
void nativeHomePage(){nativeSystemMonitoring();nativeDeviceMonitoring();}
