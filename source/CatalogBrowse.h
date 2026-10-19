#pragma once
#include "Common.h"
#include <set>
#include <sstream>
namespace hub {
inline std::wstring catalogFold(std::wstring value){for(auto& c:value)c=(wchar_t)towlower(c);return value;}
inline std::wstring catalogKind(JsonObject entry){
 auto kind=str(entry,L"kind"),group=str(entry,L"group");
 if(kind==L"application"||group==L"application"||group==L"applications")return L"application";
 if(kind.empty())kind=str(entry,L"category");
 if(kind==L"gamepad")kind=L"controller";
 if(kind==L"headphone"||kind==L"headphones")kind=L"headset";
 return kind==L"mouse"||kind==L"keyboard"||kind==L"headset"||kind==L"controller"?kind:L"other";
}
inline std::wstring catalogModel(JsonObject entry){auto value=str(entry,L"model");if(value.empty())value=str(entry.GetNamedObject(L"compatibility",JsonObject{}),L"deviceName");return value.empty()?str(entry,L"name"):value;}
inline bool catalogQueryMatches(JsonObject entry,std::wstring query){
 auto compatibility=entry.GetNamedObject(L"compatibility",JsonObject{});
 auto hay=catalogFold(str(entry,L"identityCode")+L" "+str(entry,L"name")+L" "+str(entry,L"id")+L" "+str(entry,L"author")+L" "+str(entry,L"brand")+L" "+catalogModel(entry)+L" "+str(compatibility,L"hardwareId")+L" "+str(entry,L"description")+L" "+str(entry,L"sourceName")+std::wstring(entry.GetNamedArray(L"tags",JsonArray{}).Stringify()));
 std::wistringstream words(query);std::wstring word;while(words>>word)if(hay.find(catalogFold(word))==std::wstring::npos)return false;return true;
}
inline bool catalogBrowseMatches(JsonObject entry,std::wstring scope,std::wstring kind=L"all",std::wstring brand=L"",std::wstring model=L"",std::wstring query=L""){
 if((catalogKind(entry)==L"application")!=(scope==L"application"))return false;
 if(scope!=L"application"&&((kind!=L"all"&&catalogKind(entry)!=kind)||(!brand.empty()&&catalogFold(str(entry,L"brand"))!=catalogFold(brand))||(!model.empty()&&catalogFold(catalogModel(entry))!=catalogFold(model))))return false;
 return catalogQueryMatches(entry,query);
}
inline std::vector<std::wstring> catalogBrowseOptions(JsonObject feed,std::wstring kind,std::wstring brand=L"",bool models=false){
 std::map<std::wstring,std::wstring> choices;
 for(auto const& value:feed.GetNamedArray(L"plugins",JsonArray{})){auto entry=value.GetObject();if(!catalogBrowseMatches(entry,L"peripheral",kind,brand))continue;auto label=models?catalogModel(entry):str(entry,L"brand");if(!label.empty())choices.emplace(catalogFold(label),label);}
 std::vector<std::wstring> result;for(auto const& [key,label]:choices)result.push_back(label);return result;
}
}
