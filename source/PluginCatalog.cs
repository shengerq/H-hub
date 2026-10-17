using System;
using System.IO;
using System.Net;
using System.Text;
using System.Linq;
using System.Collections.Generic;
using System.Security.Cryptography;
using System.Web.Script.Serialization;

internal static class PluginCatalog {
 static readonly JavaScriptSerializer Json = new JavaScriptSerializer { MaxJsonLength = 32*1024*1024, RecursionLimit = 24 };
 const int MaxIndexBytes=4*1024*1024, MaxEntries=4096, MaxCombinedEntries=8192, MaxPackageBytes=8*1024*1024;
 static Dictionary<string,object> Obj(params object[] p) { var o=new Dictionary<string,object>(); for(int i=0;i<p.Length;i+=2)o[(string)p[i]]=p[i+1]; return o; }
 static string S(Dictionary<string,object> o,string k) { object v; return o.TryGetValue(k,out v)&&v is string?(string)v:""; }
 static bool Enabled(Dictionary<string,object> o) { object v; return !o.TryGetValue("enabled",out v)||!(v is bool)||(bool)v; }
 static void CheckUri(Uri u) { if(u.IsFile) { if(u.IsUnc)throw new Exception("本地清单不支持网络共享路径"); } else if(u.Scheme!="https"||!String.IsNullOrEmpty(u.UserInfo))throw new Exception("远程插件库须使用 HTTPS 地址"); }
 static Dictionary<string,object> Normalize(Dictionary<string,object> input) {
  string address=S(input,"address").Trim(),kind=S(input,"kind"),branch=S(input,"branch").Trim(),path=S(input,"indexPath").Trim().TrimStart('/');
  if(address.Length==0)address=S(input,"indexUrl").Trim(); if(branch.Length==0)branch="main"; if(path.Length==0)path="index.json";
  Uri uri; if(Path.IsPathRooted(address)&&!address.StartsWith("/"))uri=new Uri(Path.GetFullPath(address));
  else if(!Uri.TryCreate(address,UriKind.Absolute,out uri)) {
   var parts=address.Trim('/').Split('/'); if(parts.Length!=2||parts.Any(x=>x.Length==0||x==".."))throw new Exception("请输入仓库地址、HTTPS 清单链接或本地 JSON 完整路径");
   uri=new Uri("https://"+(kind=="gitee"?"gitee.com":kind=="gitlab"?"gitlab.com":"github.com")+"/"+address.Trim('/')); if(kind.Length==0||kind=="auto")kind="github";
  }
  CheckUri(uri);
  if(uri.IsFile) { kind=Directory.Exists(uri.LocalPath)?"directory":"file";if(kind=="directory")uri=new Uri(Path.GetFullPath(uri.LocalPath).TrimEnd(Path.DirectorySeparatorChar)+Path.DirectorySeparatorChar); }
  else if(kind!="json") {
   var seg=uri.AbsolutePath.Trim('/').Split('/'); string host=uri.Host.ToLowerInvariant();
   bool github=host=="github.com",gitee=host=="gitee.com",gitlab=host=="gitlab.com"||kind=="gitlab";
   if(github||gitee||gitlab) {
    if(seg.Length<2)throw new Exception("仓库地址须包含所有者与仓库名");
    string root="https://"+uri.Authority+"/"; int marker=Array.IndexOf(seg,"-");
    int repoParts=gitlab?(marker>=0?marker:seg.Length):2;
    string repo=String.Join("/",seg.Take(repoParts)).TrimEnd('/'); if(repo.EndsWith(".git"))repo=repo.Substring(0,repo.Length-4);
    int op=gitlab?marker+1:2;
    if(op>=0&&seg.Length>op+1&&(seg[op]=="blob"||seg[op]=="tree"||seg[op]=="raw")) {
     branch=Uri.UnescapeDataString(seg[op+1]); if(seg.Length>op+2)path=String.Join("/",seg.Skip(op+2));
    } else if(!gitlab&&seg.Length>2) { kind="json"; }
    if(kind!="json") {
     if(branch.Contains("..")||path.Split('/').Any(x=>x==".."))throw new Exception("分支或清单路径无效");
     string suffix=Uri.EscapeDataString(branch)+"/"+path;
     uri=new Uri(github?"https://raw.githubusercontent.com/"+repo+"/"+suffix:root+repo+(gitlab?"/-/raw/":"/raw/")+suffix);
     kind=github?"github":gitee?"gitee":"gitlab";
    }
   } else kind="json";
  }
  var result=Obj("ok",true,"id",S(input,"id"),"name",S(input,"name"),"address",address,"indexUrl",uri.AbsoluteUri,"kind",kind,"branch",branch,"indexPath",path,"enabled",Enabled(input));
  if(S(result,"name").Length==0)result["name"]=uri.IsFile?Path.GetFileNameWithoutExtension(uri.LocalPath):uri.Host+uri.AbsolutePath;
  return result;
 }
 static byte[] Fetch(Uri uri,int limit) {
  CheckUri(uri); if(uri.IsFile) { var info=new FileInfo(uri.LocalPath); if(info.Length>limit)throw new Exception("插件源内容超过限制（"+(limit/1024)+" KB）"); return File.ReadAllBytes(uri.LocalPath); }
  for(int hop=0;hop<6;hop++) {
   CheckUri(uri); if(uri.IsFile)throw new Exception("远程地址不能重定向到本地文件");
   var r=(HttpWebRequest)WebRequest.Create(uri); r.UserAgent="Hhub-Plugin-Catalog/2.0";r.Timeout=12000;r.ReadWriteTimeout=12000;r.AllowAutoRedirect=false;
   using(var response=(HttpWebResponse)r.GetResponse()) {
    if((int)response.StatusCode>=300&&(int)response.StatusCode<400) { uri=new Uri(uri,response.Headers["Location"]);continue; }
    if(response.ContentLength>limit)throw new Exception("插件源内容超过限制（"+(limit/1024)+" KB）");
    using(var stream=response.GetResponseStream())using(var data=new MemoryStream()) { byte[] b=new byte[8192];int n;while((n=stream.Read(b,0,b.Length))>0){if(data.Length+n>limit)throw new Exception("插件源内容超过限制（"+(limit/1024)+" KB）");data.Write(b,0,n);}return data.ToArray(); }
   }
  } throw new Exception("插件源重定向过多");
 }
 static Dictionary<string,object> Parse(byte[] bytes) { var o=Json.DeserializeObject(Encoding.UTF8.GetString(bytes).TrimStart('\ufeff')) as Dictionary<string,object>;if(o==null)throw new Exception("插件清单格式无效");return o; }
 static string Digest(byte[] b) { using(var sha=SHA256.Create())return BitConverter.ToString(sha.ComputeHash(b)).Replace("-","").ToLowerInvariant(); }
 static Uri PackageUri(Uri source,string address) {
  Uri package; if(String.IsNullOrWhiteSpace(address)||!Uri.TryCreate(source,address,out package))throw new Exception("插件包地址无效"); CheckUri(package);
  if(package.IsFile) {
   if(!source.IsFile)throw new Exception("远程插件库不能引用本地文件");
   string folder=Path.GetFullPath(Directory.Exists(source.LocalPath)?source.LocalPath:Path.GetDirectoryName(source.LocalPath)).TrimEnd(Path.DirectorySeparatorChar)+Path.DirectorySeparatorChar;
   if(!Path.GetFullPath(package.LocalPath).StartsWith(folder,StringComparison.OrdinalIgnoreCase))throw new Exception("本地插件包必须位于插件库目录内");
  } return package;
 }
 static Dictionary<string,object> Validate(Dictionary<string,object> input,Dictionary<string,object> feed) {
  var source=new Uri(S(input,"indexUrl"));CheckUri(source);object schema,items;
  if(!feed.TryGetValue("schemaVersion",out schema)||Convert.ToInt32(schema)!=1||!feed.TryGetValue("plugins",out items)||!(items is object[]))throw new Exception("插件清单格式或版本不支持");
  var list=new List<object>();var ids=new HashSet<string>();
  foreach(object value in (object[])items) {
   var e=value as Dictionary<string,object>;if(e==null||list.Count>=MaxEntries)throw new Exception("插件清单条目无效");string id=S(e,"id"),hash=S(e,"sha256");Version version;
   if(id.Length==0||id.Length>80||!ids.Add(id)||S(e,"name").Length==0||S(e,"name").Length>200||S(e,"author").Length==0||S(e,"author").Length>200||S(e,"packageUrl").Length>4096||!Version.TryParse(S(e,"version"),out version)||hash.Length!=64||hash.Any(c=>!Uri.IsHexDigit(c)))throw new Exception("插件条目身份、版本或摘要无效");
   var package=PackageUri(source,S(e,"packageUrl"));
   var entry=Obj("id",id,"name",S(e,"name"),"author",S(e,"author"),"version",S(e,"version"),"description",Short(S(e,"description"),1024),"kind",S(e,"kind"),"packageUrl",package.AbsoluteUri,"sha256",hash.ToLowerInvariant(),"sourceId",S(input,"id"),"sourceName",S(input,"name"),"sourceIndexUrl",source.AbsoluteUri);
   var kind=CatalogKind(e,S(e,"packageUrl"));entry["kind"]=kind;
   entry["group"]=kind=="application"?"applications":"peripherals";
   entry["category"]=kind=="application"?(S(e,"category")=="test"?"test":"application"):kind;
   entry["brand"]=Short(S(e,"brand"),100);
   object tags;if(e.TryGetValue("tags",out tags)&&tags is object[])entry["tags"]=((object[])tags).OfType<string>().Take(16).Select(t=>Short(t,100)).ToArray();list.Add(entry);
  }return Obj("ok",true,"name",S(feed,"name"),"plugins",list.ToArray(),"indexUrl",source.AbsoluteUri,"checkedAt",DateTime.UtcNow.ToString("o"));
 }
 static string Short(string text,int length) { return text.Length>length?text.Substring(0,length):text; }
 static string CatalogKind(Dictionary<string,object> entry,string path) {
  string kind=S(entry,"kind");if(kind=="mouse"||kind=="keyboard"||kind=="headset"||kind=="controller"||kind=="application")return kind;
  if(kind=="headphone"||kind=="headphones")return "headset";
  string category=S(entry,"category");if(category=="mouse"||category=="keyboard"||category=="headset"||category=="controller")return category;
  if(path.Contains("应用插件")||S(entry,"nativePage").Length>0||S(entry,"group")=="applications"||S(entry,"group")=="application")return "application";
  if(path.Contains("鼠标"))return "mouse";if(path.Contains("键盘"))return "keyboard";if(path.Contains("耳机"))return "headset";if(path.Contains("手柄"))return "controller";
  return "other";
 }
 static IEnumerable<string> PackageFiles(string directory,int depth) {
  if(depth>8)throw new Exception("插件库目录层级不能超过 8 层");
  foreach(var file in Directory.GetFiles(directory,"*.hhub").OrderBy(x=>x,StringComparer.Ordinal))if((File.GetAttributes(file)&FileAttributes.ReparsePoint)==0)yield return file;
  foreach(var child in Directory.GetDirectories(directory).OrderBy(x=>x,StringComparer.Ordinal))if(Path.GetFileName(child)!=".git"&&(File.GetAttributes(child)&FileAttributes.ReparsePoint)==0)foreach(var file in PackageFiles(child,depth+1))yield return file;
 }
 static Dictionary<string,object> DirectoryIndex(string directory) {
  string root=Path.GetFullPath(directory).TrimEnd(Path.DirectorySeparatorChar)+Path.DirectorySeparatorChar;var entries=new List<object>();var ids=new HashSet<string>();
  foreach(var file in PackageFiles(root,0)) {
   if(entries.Count>=MaxEntries)throw new Exception("插件库不能超过 "+MaxEntries+" 个插件包");
   var bytes=Fetch(new Uri(file),MaxPackageBytes);Dictionary<string,object> manifest;
   try{manifest=Parse(bytes);}catch(Exception e){throw new Exception(Path.GetFileName(file)+"："+e.Message);}
   if(!ids.Add(S(manifest,"id")))throw new Exception("插件库中存在重复插件 ID："+S(manifest,"id"));
   string relative=file.Substring(root.Length).Replace('\\','/');var parts=relative.Split('/');string kind=CatalogKind(manifest,relative),brand=kind!="application"&&parts.Length>=4?parts[2]:"";
   var entry=Obj("id",S(manifest,"id"),"name",S(manifest,"name"),"author",S(manifest,"author"),"version",S(manifest,"version"),"kind",kind,"group",kind=="application"?"applications":"peripherals","category",kind=="application"&&(relative.Contains("测试型")||S(manifest,"category")=="test")?"test":kind,"brand",brand,"description",Short(S(manifest,"description"),1024),"sha256",Digest(bytes),"packageUrl",String.Join("/",parts.Select(Uri.EscapeDataString)),"tags",new string[]{brand,kind});entries.Add(entry);
  }
  var sorted=entries.Cast<Dictionary<string,object>>().OrderBy(e=>S(e,"kind")=="application"?0:1).ThenBy(e=>S(e,"kind"),StringComparer.Ordinal).ThenBy(e=>S(e,"brand"),StringComparer.Ordinal).ThenBy(e=>S(e,"name"),StringComparer.Ordinal).Cast<object>().ToArray();
  return Obj("schemaVersion",1,"name",new DirectoryInfo(root).Name,"plugins",sorted);
 }
 static Dictionary<string,object> Sync(Dictionary<string,object> input) { var uri=new Uri(S(input,"indexUrl"));CheckUri(uri);return Validate(input,uri.IsFile&&Directory.Exists(uri.LocalPath)?DirectoryIndex(uri.LocalPath):Parse(Fetch(uri,MaxIndexBytes))); }
 static Dictionary<string,object> SyncMany(Dictionary<string,object> input) {
  var sources=(object[])input["sources"];if(sources.Length>64)throw new Exception("最多支持 64 个插件库");string folder=S(input,"cacheDirectory");Directory.CreateDirectory(folder);
  var plugins=new List<object>();var statuses=new List<object>();int succeeded=0,failed=0;
  foreach(var v in sources) {
   var s=(Dictionary<string,object>)v;if(!Enabled(s))continue;string id=S(s,"id");if(id.Length==0||id.Any(c=>!Char.IsLetterOrDigit(c)&&c!='-'))throw new Exception("插件库标识无效");
   string cache=Path.Combine(folder,id+".json");Dictionary<string,object> feed=null;string error="";bool cached=false;
   try { feed=Sync(s);File.WriteAllText(cache,Json.Serialize(feed),new UTF8Encoding(false));succeeded++; }
   catch(Exception e) { failed++;error=e.Message;try { var old=Parse(Fetch(new Uri(Path.GetFullPath(cache)),8*1024*1024));if(S(old,"indexUrl")==S(s,"indexUrl")){feed=Validate(s,Obj("schemaVersion",1,"name",S(old,"name"),"plugins",old["plugins"]));cached=true;} }catch{} }
   var entries=feed==null?new object[0]:(object[])feed["plugins"];
   if(plugins.Count+entries.Length>MaxCombinedEntries)throw new Exception("已启用插件库合计不能超过 "+MaxCombinedEntries+" 个条目，请停用部分插件库");
   foreach(var p in entries) {var entry=new Dictionary<string,object>((Dictionary<string,object>)p);entry["sourceId"]=id;entry["sourceName"]=S(s,"name");entry["sourceIndexUrl"]=S(s,"indexUrl");plugins.Add(entry);}
   statuses.Add(Obj("id",id,"error",error,"cached",cached,"count",entries.Length));
  }return Obj("ok",true,"plugins",plugins.ToArray(),"sources",statuses.ToArray(),"succeeded",succeeded,"failed",failed);
 }
 static Dictionary<string,object> Download(Dictionary<string,object> input) {
  var e=(Dictionary<string,object>)input["entry"];var uri=new Uri(S(e,"packageUrl"));CheckUri(uri);
  if(uri.IsFile) {var source=new Uri(S(e,"sourceIndexUrl"));uri=PackageUri(source,uri.AbsoluteUri);}return Verify(e,Fetch(uri,MaxPackageBytes));
 }
 static Dictionary<string,object> Verify(Dictionary<string,object> e,byte[] bytes) {
  if(bytes.Length>8*1024*1024)throw new Exception("插件包不能超过 8 MB");if(Digest(bytes)!=S(e,"sha256").ToLowerInvariant())throw new Exception("插件包 SHA-256 校验失败");
  var manifest=Parse(bytes);if(S(manifest,"id")!=S(e,"id")||S(manifest,"author")!=S(e,"author")||S(manifest,"version")!=S(e,"version"))throw new Exception("插件包身份与清单不匹配");return Obj("ok",true,"manifest",manifest,"sha256",Digest(bytes));
 }
 public static int Main(string[] args) {
  if(args.Length!=3)return 2;ServicePointManager.SecurityProtocol=SecurityProtocolType.Tls12;Dictionary<string,object> result;
  try { var input=Parse(File.ReadAllBytes(args[1]));switch(args[0]) {
   case "--normalize":result=Normalize(input);break;case "--index-directory":result=Obj("ok",true,"catalog",DirectoryIndex(S(input,"directory")));break;case "--sync-many":result=SyncMany(input);break;case "--sync":result=Sync(input);break;
   case "--download":result=Download(input);break;case "--validate":result=Validate(input,(Dictionary<string,object>)input["catalog"]);break;
   case "--verify-local":result=Verify((Dictionary<string,object>)input["entry"],File.ReadAllBytes(S(input,"packageFile")));break;default:throw new Exception("未知插件库操作");
  }}catch(Exception e){result=Obj("ok",false,"error",e.Message);}
  File.WriteAllText(args[2],Json.Serialize(result),new UTF8Encoding(false));return (bool)result["ok"]?0:1;
 }
}
