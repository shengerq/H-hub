#pragma once
#include <filesystem>
#include <winrt/Windows.Foundation.h>

namespace hub {
// Uri.AbsoluteUri preserves Unicode on Windows; Chromium serializes it as UTF-8
// percent escapes. Build the allowed file URL in Chromium's serialized form.
inline std::wstring pluginDocumentUrl(std::filesystem::path const& file){
 auto path=std::filesystem::absolute(file).generic_wstring();std::wstring url=L"file:///";size_t start=0;
 while(start<path.size()){auto end=path.find(L'/',start);auto part=path.substr(start,end==std::wstring::npos?end:end-start);if(start==0&&part.size()==2&&part[1]==L':')url+=part;else url+=winrt::Windows::Foundation::Uri::EscapeComponent(part);if(end==std::wstring::npos)break;url+=L'/';start=end+1;}
 return url;
}
}
