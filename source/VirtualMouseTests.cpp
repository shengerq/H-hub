#include "PluginHid.h"
#include "PluginHidV2.h"
#include <iostream>
using namespace hub;
int wmain(int argc,wchar_t** argv){winrt::init_apartment();if(argc!=2||!VirtualMouse::enabled())return 2;try{
 Model model({},false);auto manifest=object(read(std::filesystem::path(argv[1])));JsonObject body;body.SetNamedValue(L"manifest",manifest);JsonObject req;put(req,L"op",std::wstring(L"importPlugin"));req.SetNamedValue(L"body",body);model.request(req);PluginHidV2 transport;auto id=str(manifest,L"id");
 auto call=[&](std::wstring op,std::wstring key=L"",double rid=-1,std::wstring data=L""){JsonObject r;put(r,L"operation",op);put(r,L"key",key);put(r,L"reportId",rid);put(r,L"data",data);JsonObject b;put(b,L"id",id);b.SetNamedValue(L"request",r);return transport.transact(model.state,b);};
 auto found=call(L"enumerate").GetNamedArray(L"devices");if(found.Size()!=1)return 3;auto key=str(found.GetObjectAt(0),L"key");call(L"open",key);call(L"sendFeature",key,14,L"A5 04 02 01 03 40 06");call(L"sendFeature",key,14,L"A5 04 01 01 00 00 00");if(str(call(L"receiveFeature",key,14),L"data").find(L"01 40 06")==std::wstring::npos)return 4;
 bool denied=false;try{call(L"receiveFeature",L"forged",14);}catch(...){denied=true;}if(!denied)return 5;
 denied=false;try{call(L"receiveFeature",key,15);}catch(...){denied=true;}if(!denied)return 6;
 auto plugin=find(model.state.GetNamedArray(L"plugins"),id);put(plugin,L"mode",std::wstring(L"disabled"));denied=false;try{call(L"receiveFeature",key,14);}catch(...){denied=true;}if(!denied)return 7;
 std::cout<<"PASS current native backend: physical manifest import, filtered enumeration, write/readback, forged-token rejection, report policy, disabled-plugin revocation\n";return 0;
 }catch(...){return 9;}}
