#pragma once
#include "Model.h"
#include <algorithm>
#include <cwctype>

namespace hub {
inline constexpr uintmax_t maxPluginPackageBytes=8*1024*1024;
inline JsonObject readPluginPackage(std::filesystem::path const& path){
 auto extension=path.extension().wstring();std::transform(extension.begin(),extension.end(),extension.begin(),towlower);
 if(extension!=L".hhub"&&extension!=L".json")throw hresult_invalid_argument(L"请拖入 .hhub 插件包；软件 .exe 安装包请在资源管理器中运行");
 if(std::filesystem::file_size(path)>maxPluginPackageBytes)throw hresult_invalid_argument(L"插件包不能超过 8 MB");
 auto manifest=object(read(path));validatePluginUi(manifest);validatePluginModule(manifest);return manifest;
}
}
