#include "CatalogBrowse.h"
#include <iostream>
using namespace hub;
int wmain(int argc,wchar_t** argv){winrt::init_apartment(winrt::apartment_type::multi_threaded);try{
 if(argc!=2)return 2;auto feed=object(read(argv[1]));int peripheral=0,application=0;JsonObject keyboard;
 auto require=[](bool condition,char const* label){if(!condition)throw std::runtime_error(label);};
 for(auto const& value:feed.GetNamedArray(L"plugins")){auto entry=value.GetObject();peripheral+=catalogBrowseMatches(entry,L"peripheral");application+=catalogBrowseMatches(entry,L"application");if(catalogKind(entry)==L"keyboard")keyboard=entry;}
 require(peripheral==331&&application==4,"scoped counts");require(catalogBrowseMatches(keyboard,L"peripheral",L"keyboard",L"asus rog",catalogModel(keyboard),L"ASUS  HFX"),"combined filters and token search");require(!catalogBrowseMatches(keyboard,L"peripheral",L"mouse"),"kind isolation");require(!catalogBrowseMatches(keyboard,L"peripheral",L"keyboard",L"AJAZZ"),"brand isolation");require(catalogQueryMatches(keyboard,L"VID_0B05 PID_1B7E"),"hardware identifier search");require(catalogQueryMatches(keyboard,L"   "),"blank search");require(!catalogQueryMatches(keyboard,L"ASUS NONEXISTENT_823"),"all tokens required");
 auto brands=catalogBrowseOptions(feed,L"keyboard");require(brands.size()==1&&brands[0]==L"ASUS ROG","scoped brand choices");auto models=catalogBrowseOptions(feed,L"keyboard",L"ASUS ROG",true);require(models.size()==1&&models[0]==catalogModel(keyboard),"scoped model choices");require(catalogBrowseOptions(feed,L"keyboard",L"AJAZZ",true).empty(),"foreign models excluded");
 auto alias=object(LR"({"kind":"gamepad","name":"Gamepad"})");require(catalogKind(alias)==L"controller","kind alias");require(catalogModel(alias)==L"Gamepad","missing model fallback");put(alias,L"group",std::wstring(L"application"));require(catalogBrowseMatches(alias,L"application",L"mouse",L"other",L"other"),"application scope ignores device filters");
 std::cout<<"PASS scoped counts, brand/model choices, combined filters, token/hardware search and legacy aliases";return 0;
 }catch(std::exception const& e){std::cerr<<e.what();return 1;}catch(...){std::cerr<<"browse tests failed";return 1;}}
