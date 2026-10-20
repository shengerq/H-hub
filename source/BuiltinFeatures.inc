// Shipped features use the existing isolated module ABI, without a user install step.
#include "BuiltinFeatureManifests.h"
inline bool isBuiltinFeature(std::wstring const& id){return id==L"eq"||id==L"windows"||id==L"phone";}
inline void provisionBuiltinFeatures(JsonObject state,std::filesystem::path const& profile){
 wchar_t executable[32768]{};check_bool(GetModuleFileNameW(nullptr,executable,32768)!=0);
 auto application=std::filesystem::path(executable).parent_path();if(application.filename()==L"core"&&std::filesystem::path(executable).filename()==L"Hhub.exe")application=application.parent_path();auto assets=application/L"features";
 auto entries=state.GetNamedArray(L"plugins");
 for(auto const& value:JsonArray::Parse(builtinFeatureManifests)){
  auto manifest=value.GetObject();auto id=str(manifest,L"id");
  auto installed=pluginRecord(state,id);
  if(installed&&!flag(installed,L"builtin")&&str(installed,L"author")!=str(manifest,L"author"))throw hresult_error(E_ACCESSDENIED,L"内置功能 ID 与现有第三方插件冲突");
  bool valid=true;
  for(auto const& item:manifest.GetNamedArray(L"nativeAssets")){
   auto definition=item.GetObject();auto name=str(definition,L"path");
   auto path=profile/L"native-components"/id/name;
   if(!validAssetName(name)||!nativeNoLinks(path)){valid=false;break;}
   if(!std::filesystem::is_regular_file(path)||std::filesystem::file_size(path)!=num(definition,L"bytes")){valid=false;break;}
   std::ifstream input(path,std::ios::binary);std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(input)),{});
   if(nativeAssetDigest(bytes)!=str(definition,L"sha256")){valid=false;break;}
  }
  std::unique_ptr<NativeAssetTransaction> transaction;
  if(!valid){
   transaction=std::make_unique<NativeAssetTransaction>(profile,id);
   for(auto const& item:manifest.GetNamedArray(L"nativeAssets")){
    auto definition=item.GetObject();auto name=str(definition,L"path");auto path=assets/id/name;
    if(!validAssetName(name)||!nativeNoLinks(path)||!std::filesystem::is_regular_file(path)||std::filesystem::file_size(path)!=num(definition,L"bytes"))throw hresult_error(E_ACCESSDENIED,L"内置功能文件缺失，请重新解压完整应用");
    std::ifstream input(path,std::ios::binary);std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(input)),{});
    if(nativeAssetDigest(bytes)!=str(definition,L"sha256"))throw hresult_error(E_ACCESSDENIED,L"内置功能文件校验失败");
    auto output=transaction->staging/name;std::filesystem::create_directories(output.parent_path());
    std::ofstream stream(output,std::ios::binary);stream.write((char const*)bytes.data(),bytes.size());stream.close();if(!stream)throw hresult_error(E_FAIL,L"内置功能文件写入失败");
   }
   transaction->activate();
  }
  put(manifest,L"builtin",true);put(manifest,L"installed",true);put(manifest,L"connected",true);put(manifest,L"mode",std::wstring(L"auto"));put(manifest,L"forceStopped",false);put(manifest,L"runtime",std::wstring(L"stopped"));put(manifest,L"leaseUntil",0.0);
  bool replaced=false;for(uint32_t i=0;i<entries.Size();++i)if(str(entries.GetObjectAt(i),L"id")==id){entries.SetAt(i,manifest);replaced=true;break;}if(!replaced)entries.Append(manifest);
  if(transaction)transaction->committed=true;
 }
}
