using System;
using System.Collections.Generic;
using System.Web.Script.Serialization;
internal static class PluginContractTests {
 static Dictionary<string,object> Parse(string s){return (Dictionary<string,object>)new JavaScriptSerializer().DeserializeObject(s);}
 static Dictionary<string,object> Entry(){var e=Parse("{\"id\":\"test.mouse\",\"author\":\"Author\",\"version\":\"1.0.0\",\"sha256\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\",\"compatibility\":{\"version\":1,\"scope\":\"device\",\"deviceName\":\"Mouse A\",\"hardwareId\":\"USB\\\\VID_1234&PID_5678\",\"firmwareVersions\":[\"1.2.3\"],\"mobileUi\":true}}");e["identityCode"]=PluginContract.Code(e);return e;}
 static void Require(bool v){if(!v)throw new Exception("Assertion failed");}
 static void Reject(Action action){bool failed=false;try{action();}catch{failed=true;}Require(failed);}
 public static int Main(){try{
  var e=Entry();var c=(Dictionary<string,object>)e["compatibility"];var empty=Parse("{\"verificationRecords\":[]}");PluginContract.Validate(e,empty);Require((string)e["verificationStatus"]=="unverified");
  var device=Parse("{\"deviceName\":\"Mouse A\",\"hardwareId\":\"usb\\\\vid_1234&pid_5678\",\"firmwareVersion\":\"1.2.3\"}");Require(PluginContract.Matches(e,device));device["firmwareVersion"]="1.2.4";Require(!PluginContract.Matches(e,device));device["firmwareVersion"]="";Require(!PluginContract.Matches(e,device));device["firmwareVersion"]="1.2.3";device["deviceName"]="Mouse B";Require(!PluginContract.Matches(e,device));device["deviceName"]="Mouse A";device["hardwareId"]="USB\\VID_1234&PID_5679";Require(!PluginContract.Matches(e,device));
  var record=Parse("{\"reviewer\":\"maintainer\",\"reviewedAt\":\"2026-01-01T00:00:00Z\",\"evidenceUrl\":\"https://example.com/report\",\"hardwareVerified\":true,\"mobileUiVerified\":true}");record["identityCode"]=e["identityCode"];var feed=new Dictionary<string,object>{{"verificationRecords",new object[]{record}}};PluginContract.Validate(e,feed);Require((string)e["verificationStatus"]=="verified"&&(bool)e["mobileUiVerified"]);
  e["sha256"]=new string('b',64);Reject(()=>PluginContract.Validate(e,feed));e["identityCode"]=PluginContract.Code(e);PluginContract.Validate(e,feed);Require((string)e["verificationStatus"]=="unverified");
  c["firmwareVersions"]=new object[]{"*"};Reject(()=>PluginContract.Code(e));c["firmwareVersions"]=new object[]{};e["identityCode"]=PluginContract.Code(e);record["identityCode"]=e["identityCode"];Reject(()=>PluginContract.Validate(e,feed));Require(!PluginContract.Matches(e,device));
  e=Entry();var manifest=Entry();PluginContract.VerifyManifest(e,manifest);((Dictionary<string,object>)manifest["compatibility"])["mobileUi"]=false;Reject(()=>PluginContract.VerifyManifest(e,manifest));
  var legacy=Parse("{\"verificationStatus\":\"verified\",\"mobileUiVerified\":true}");PluginContract.Validate(legacy,empty);Require((string)legacy["verificationStatus"]=="unverified"&&!(bool)legacy["mobileUiVerified"]);Require(!PluginContract.Matches(legacy,device));
  e=Entry();record["identityCode"]=e["identityCode"];record["evidenceUrl"]="http://example.com/report";Reject(()=>PluginContract.Validate(e,feed));record["evidenceUrl"]="https://example.com/report";record["hardwareVerified"]=false;Reject(()=>PluginContract.Validate(e,feed));
  var grouped=Parse("{\"kind\":\"audio\"}");PluginContract.Classify(grouped);Require((string)grouped["group"]=="peripheral"&&(string)grouped["category"]=="headset");
  grouped=Parse("{\"kind\":\"developer-tools\"}");PluginContract.Classify(grouped);Require((string)grouped["group"]=="application"&&(string)grouped["category"]=="testing");
  foreach(var category in new[]{"mouse","keyboard","headset","gamepad","other"}){grouped=Parse("{\"group\":\"peripheral\"}");grouped["category"]=category;PluginContract.Classify(grouped);Require((string)grouped["category"]==category);}
  foreach(var category in new[]{"experimental","testing","other"}){grouped=Parse("{\"group\":\"application\"}");grouped["category"]=category;PluginContract.Classify(grouped);Require((string)grouped["category"]==category);}
  Reject(()=>PluginContract.Classify(Parse("{\"group\":\"peripheral\",\"category\":\"testing\"}")));
  Reject(()=>PluginContract.Classify(Parse("{\"group\":\"application\",\"category\":\"mouse\"}")));
  Reject(()=>PluginContract.Classify(Parse("{\"group\":\"unknown\",\"category\":\"other\"}")));
  Reject(()=>PluginContract.Classify(Parse("{\"group\":\"application\"}")));
  grouped=Entry();grouped["group"]="application";grouped["category"]="testing";Reject(()=>PluginContract.Classify(grouped));
  var classifiedEntry=Entry();string oldCode=(string)classifiedEntry["identityCode"];PluginContract.Classify(classifiedEntry);Require(oldCode==PluginContract.Code(classifiedEntry));
  var root=System.IO.Path.GetTempPath();Require(PluginContract.PackagePath(root,"catalog/peripherals/mouse/VAXEE/XE__Vaxee.hhub").StartsWith(System.IO.Path.GetFullPath(root)));foreach(var bad in new[]{"../bad.hhub","/root.hhub","a/../bad.hhub","a//b.hhub","C:/bad.hhub","a/%2e%2e/b.hhub","a\\b.hhub","a.hhub?q"})Reject(()=>PluginContract.PackagePath(root,bad));
  Console.WriteLine("PASS safe nested package paths and traversal rejection");
  Console.WriteLine("PASS peripheral/application classification, legacy audio/tools, invalid categories and identity preservation");
  Console.WriteLine("PASS exact matching, missing firmware, tampering, stale evidence, UI mismatch, legacy and verification evidence");return 0;
 }catch(Exception ex){Console.Error.WriteLine(ex);return 1;}}
}
