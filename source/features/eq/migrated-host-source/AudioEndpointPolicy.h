#pragma once
#include "Common.h"
namespace hub {
inline std::wstring preferredEqOutput(JsonArray outputs,std::wstring const& saved,std::wstring const& systemDefault){
 for(auto const& wanted:{saved,systemDefault})if(!wanted.empty())for(auto const& value:outputs){auto item=value.GetObject();if(str(item,L"id")==wanted)return wanted;}
 return outputs.Size()?str(outputs.GetObjectAt(0),L"id"):L"";
}
inline bool upstreamFxSoundProcess(std::wstring path){auto filename=std::filesystem::path(path).filename().wstring();std::transform(filename.begin(),filename.end(),filename.begin(),towupper);return filename==L"FXSOUND.EXE";}
inline bool eqApplicationUsesOutput(JsonObject app,std::wstring const& output,std::wstring const& ownedSource=L""){
 auto endpoints=app.GetNamedArray(L"activeOutputs",JsonArray{});if(!endpoints.Size())return false;
 for(auto const& endpoint:endpoints){auto id=std::wstring(endpoint.GetString());if(id!=output&&(ownedSource.empty()||id!=ownedSource))return false;}return true;
}
}
