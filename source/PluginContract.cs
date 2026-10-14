using System;
using System.IO;
using System.Text;
using System.Linq;
using System.Collections.Generic;
using System.Security.Cryptography;
using System.Web.Script.Serialization;

// Shared byte-for-byte with H-hub-plugins/scripts/PluginContract.cs.
internal static class PluginContract {
 public static string S(Dictionary<string,object> o,string key){object v;return o.TryGetValue(key,out v)&&v is string?(string)v:"";}
 static Dictionary<string,object> O(Dictionary<string,object> o,string key){object v;var d=o.TryGetValue(key,out v)?v as Dictionary<string,object>:null;if(d==null)throw new Exception("缺少对象："+key);return d;}
 static object[] A(Dictionary<string,object> o,string key){object v;var a=o.TryGetValue(key,out v)?v as object[]:null;if(a==null)throw new Exception("缺少数组："+key);return a;}
 static bool B(Dictionary<string,object> o,string key){object v;if(!o.TryGetValue(key,out v)||!(v is bool))throw new Exception("需要布尔字段："+key);return (bool)v;}
 static void Exact(string s,string key){if(String.IsNullOrWhiteSpace(s)||s!=s.Trim()||s.Length>256||s.Contains("*")||s.Contains("?")||s.Any(Char.IsControl))throw new Exception("需要明确的设备字段："+key);}
 public static string Canonical(Dictionary<string,object> c){
  object version;if(!c.TryGetValue("version",out version)||!(version is int)|| (int)version!=1)throw new Exception("设备契约版本不支持");
  string scope=S(c,"scope"),name=S(c,"deviceName"),hardware=S(c,"hardwareId");var firmwares=A(c,"firmwareVersions");bool mobile=B(c,"mobileUi");
  if(scope!="device"&&scope!="tool")throw new Exception("设备契约 scope 无效");
  if(scope=="device"){Exact(name,"deviceName");Exact(hardware,"hardwareId");if(hardware!=hardware.ToUpperInvariant())throw new Exception("硬件识别码必须大写");}
  else if(name!=""||hardware!=""||firmwares.Length!=0||mobile)throw new Exception("工具插件不能声明设备匹配或手机 UI");
  if(firmwares.Length>64)throw new Exception("固件版本过多");var seen=new HashSet<string>(StringComparer.Ordinal);foreach(var f in firmwares){if(!(f is string))throw new Exception("固件版本必须为字符串");Exact((string)f,"firmwareVersions");if(!seen.Add((string)f))throw new Exception("重复固件版本");}
  var fields=new List<string>{"hhub-contract/1",scope,name,hardware,mobile?"true":"false"};fields.AddRange(seen.OrderBy(x=>x,StringComparer.Ordinal));
  return Pack(fields);
 }
 static string Pack(IEnumerable<string> fields){var b=new StringBuilder();foreach(string f in fields)b.Append(Encoding.UTF8.GetByteCount(f)).Append(':').Append(f);return b.ToString();}
 public static string Code(Dictionary<string,object> entry){var c=O(entry,"compatibility");string data=Pack(new[]{S(entry,"id"),S(entry,"author"),S(entry,"version"),S(entry,"sha256").ToLowerInvariant(),Canonical(c)});using(var sha=SHA256.Create())return "HHUB1-"+BitConverter.ToString(sha.ComputeHash(Encoding.UTF8.GetBytes(data))).Replace("-","").ToLowerInvariant();}
 public static void Validate(Dictionary<string,object> entry,Dictionary<string,object> feed){
  if(!entry.ContainsKey("compatibility")){if(entry.ContainsKey("identityCode"))throw new Exception("编码缺少设备契约");entry["verificationStatus"]="unverified";entry["mobileUiVerified"]=false;return;}
  var c=O(entry,"compatibility");string code=Code(entry);if(S(entry,"identityCode")!=code)throw new Exception("插件唯一编码与设备契约或摘要不匹配");
  bool verified=false,mobileVerified=false;object records;
  if(feed.TryGetValue("verificationRecords",out records)){
   if(!(records is object[]))throw new Exception("验证记录格式无效");int count=0;
   foreach(var item in (object[])records){var r=item as Dictionary<string,object>;if(r==null)throw new Exception("验证记录无效");if(S(r,"identityCode")!=code)continue;if(++count>1)throw new Exception("重复验证记录");
    Uri evidence;DateTimeOffset date;if(String.IsNullOrWhiteSpace(S(r,"reviewer"))||!DateTimeOffset.TryParse(S(r,"reviewedAt"),out date)||date> DateTimeOffset.UtcNow||!Uri.TryCreate(S(r,"evidenceUrl"),UriKind.Absolute,out evidence)||evidence.Scheme!="https")throw new Exception("验证记录缺少维护者、时间或证据链接");
    verified=B(r,"hardwareVerified");mobileVerified=B(r,"mobileUiVerified");if(mobileVerified&&(!verified||!B(c,"mobileUi")))throw new Exception("手机验证与插件声明不一致");
   }
  }
  if(verified&&(S(c,"scope")!="device"||A(c,"firmwareVersions").Length==0))throw new Exception("实机验证需要明确的设备及固件版本");
  entry["verificationStatus"]=verified?"verified":"unverified";entry["mobileUiVerified"]=mobileVerified;
 }
 public static bool Matches(Dictionary<string,object> entry,Dictionary<string,object> device){
  if(!entry.ContainsKey("compatibility"))return false;var c=O(entry,"compatibility");Canonical(c);
  return S(c,"scope")=="device"&&S(c,"deviceName")==S(device,"deviceName")&&S(c,"hardwareId")==S(device,"hardwareId").ToUpperInvariant()&&!String.IsNullOrWhiteSpace(S(device,"firmwareVersion"))&&A(c,"firmwareVersions").Contains(S(device,"firmwareVersion"));
 }
 public static void VerifyManifest(Dictionary<string,object> entry,Dictionary<string,object> manifest){if(entry.ContainsKey("compatibility")&&Canonical(O(entry,"compatibility"))!=Canonical(O(manifest,"compatibility")))throw new Exception("插件包设备契约与清单不匹配");}
 public static int Main(string[] args){
  try{if(args.Length!=1)throw new Exception("Usage: PluginContract.exe index.json");var json=new JavaScriptSerializer{MaxJsonLength=32*1024*1024};var feed=(Dictionary<string,object>)json.DeserializeObject(File.ReadAllText(args[0],Encoding.UTF8));var codes=new HashSet<string>();foreach(var item in A(feed,"plugins")){var e=(Dictionary<string,object>)item;Validate(e,feed);if(e.ContainsKey("compatibility")&&!codes.Add(S(e,"identityCode")))throw new Exception("重复插件编码");string file=S(e,"packageUrl");if(Path.GetFileName(file)!=file||!file.EndsWith(".hhub",StringComparison.Ordinal))throw new Exception("包路径必须是根目录 .hhub 文件");byte[] bytes=File.ReadAllBytes(Path.Combine(Path.GetDirectoryName(Path.GetFullPath(args[0])),file));string hash;using(var sha=SHA256.Create())hash=BitConverter.ToString(sha.ComputeHash(bytes)).Replace("-","").ToLowerInvariant();if(hash!=S(e,"sha256"))throw new Exception("包摘要不匹配");var p=(Dictionary<string,object>)json.DeserializeObject(Encoding.UTF8.GetString(bytes));foreach(string key in new[]{"id","name","author","version"})if(S(e,key)!=S(p,key))throw new Exception("包身份不匹配："+key);VerifyManifest(e,p);Console.WriteLine("PASS "+S(e,"id")+" "+S(e,"identityCode"));}return 0;}catch(Exception e){Console.Error.WriteLine(e.Message);return 1;}
 }
}
