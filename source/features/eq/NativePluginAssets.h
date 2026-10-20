#pragma once
#pragma comment(lib,"bcrypt.lib")
#pragma comment(lib,"crypt32.lib")
// Included inside hub after Model's JSON helpers. Only host-pinned binaries and
// fixed, plugin-owned relative names can be materialized from optional packages.
struct NativeAssetDefinition{std::wstring name,hash;};
#include "NativePluginAssetPins.inc"
inline std::wstring nativeAssetDigest(std::vector<unsigned char> const& bytes){BCRYPT_ALG_HANDLE algorithm{};if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw hresult_error(E_FAIL);unsigned char hash[32]{};auto status=BCryptHash(algorithm,nullptr,0,const_cast<PUCHAR>(bytes.data()),(ULONG)bytes.size(),hash,32);BCryptCloseAlgorithmProvider(algorithm,0);if(status<0)throw hresult_error(E_FAIL);std::wstring text;for(auto byte:hash){wchar_t part[3]{};swprintf_s(part,L"%02x",byte);text+=part;}return text;}
inline void clearNativeAssetFolder(std::filesystem::path const& folder,std::wstring const& id){std::error_code ec;for(auto const& item:nativeAssetDefinitions(id)){std::filesystem::remove(folder/item.name,ec);if(ec)throw hresult_error(E_FAIL,L"插件组件仍在使用，清理未完成");}std::filesystem::remove(folder/L"phone-web",ec);ec.clear();std::filesystem::remove(folder,ec);}
struct NativeAssetTransaction{
 std::filesystem::path destination,staging,backup;std::wstring id;bool activated=false,committed=false;
 NativeAssetTransaction(std::filesystem::path root,std::wstring plugin):id(plugin){auto base=root/L"native-components";std::filesystem::create_directories(base);destination=base/id;auto suffix=std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64());staging=base/(id+L".install-"+suffix);backup=base/(id+L".previous-"+suffix);std::filesystem::create_directory(staging);}
 ~NativeAssetTransaction(){try{if(activated&&!committed){clearNativeAssetFolder(destination,id);if(std::filesystem::exists(backup))std::filesystem::rename(backup,destination);}if(std::filesystem::exists(staging))clearNativeAssetFolder(staging,id);if(committed&&std::filesystem::exists(backup))clearNativeAssetFolder(backup,id);}catch(...) {}}
 void activate(){if(std::filesystem::exists(destination))std::filesystem::rename(destination,backup);try{std::filesystem::rename(staging,destination);activated=true;}catch(...){if(std::filesystem::exists(backup))std::filesystem::rename(backup,destination);throw;}}
};
inline std::unique_ptr<NativeAssetTransaction> installNativePluginAssets(JsonObject plugin,std::filesystem::path root={}){
 if(!plugin.HasKey(L"nativeAssets"))return {};
 auto id=str(plugin,L"id");auto definitions=nativeAssetDefinitions(id);auto assets=plugin.GetNamedArray(L"nativeAssets");if(definitions.empty()||assets.Size()!=definitions.size()||str(plugin.GetNamedObject(L"requiredHost",JsonObject{}),L"apiVersion")!=L"1.1")throw hresult_invalid_argument(L"原生插件组件需要支持 1.1 组件契约的 Hhub，组件数量或宿主契约无效");
 struct Decoded{std::wstring name;std::vector<unsigned char> bytes;};std::vector<Decoded> decoded;JsonArray metadata;
 for(auto const& value:assets){auto item=value.GetObject();auto name=str(item,L"path"),hash=str(item,L"sha256"),data=str(item,L"data");auto definition=std::find_if(definitions.begin(),definitions.end(),[&](auto const& d){return d.name==name&&nativeAssetHashApproved(id,d,hash);});if(definition==definitions.end()||std::any_of(decoded.begin(),decoded.end(),[&](auto const& d){return d.name==name;}))throw hresult_invalid_argument(L"插件包含未获宿主认可的组件，或组件重复");
  if(data.empty()||data.size()>6*1024*1024||str(item,L"encoding")!=L"base64")throw hresult_invalid_argument(L"插件组件编码或大小无效");DWORD size{};if(!CryptStringToBinaryW(data.c_str(),(DWORD)data.size(),CRYPT_STRING_BASE64|CRYPT_STRING_STRICT,nullptr,&size,nullptr,nullptr)||size>4*1024*1024)throw hresult_invalid_argument(L"插件组件无法解码");Decoded file{name,std::vector<unsigned char>(size)};if(!CryptStringToBinaryW(data.c_str(),(DWORD)data.size(),CRYPT_STRING_BASE64|CRYPT_STRING_STRICT,file.bytes.data(),&size,nullptr,nullptr)||nativeAssetDigest(file.bytes)!=hash)throw hresult_invalid_argument(L"插件组件 SHA-256 校验失败");
  JsonObject meta;put(meta,L"path",name);put(meta,L"sha256",hash);put(meta,L"bytes",(double)size);metadata.Append(meta);decoded.push_back(std::move(file));
 }
 std::unique_ptr<NativeAssetTransaction> transaction;if(!root.empty()){transaction=std::make_unique<NativeAssetTransaction>(root,id);for(auto const& file:decoded){auto path=transaction->staging/file.name;std::filesystem::create_directories(path.parent_path());std::ofstream output(path,std::ios::binary);output.write((char const*)file.bytes.data(),file.bytes.size());if(!output)throw hresult_error(E_FAIL,L"无法写入插件组件");}transaction->activate();}
 plugin.SetNamedValue(L"nativeAssets",metadata);return transaction;
}
inline void removeNativePluginAssets(std::filesystem::path root,std::wstring const& id){if(!nativeAssetDefinitions(id).empty())clearNativeAssetFolder(root/L"native-components"/id,id);auto folder=root/L"native-components";if(std::filesystem::exists(folder)&&std::filesystem::is_empty(folder))std::filesystem::remove(folder);}

