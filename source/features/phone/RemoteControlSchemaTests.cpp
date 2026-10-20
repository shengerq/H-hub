#include "Common.h"
#include <iostream>
namespace hub {
#include "PhoneDeviceSupport.inc"
}
int wmain(int argc,wchar_t** argv){
 winrt::init_apartment();if(argc!=2)return 2;using namespace hub;unsigned checks=0;
 try{auto plugin=object(read(argv[1]));
  auto check=[&](std::wstring json,bool accepted){bool passed=true;try{phoneValidateControlPatch(object(json),plugin);}catch(...){passed=false;}if(passed!=accepted)throw std::runtime_error("schema boundary mismatch");++checks;};
  check(LR"({"dpi":50})",true);check(LR"({"dpi":30000})",true);check(LR"({"dpi":49})",false);check(LR"({"dpi":30050})",false);check(LR"({"dpi":51})",false);
  check(LR"({"polling":8000})",true);check(LR"({"polling":3000})",false);check(LR"({"polling":"1000"})",false);
  check(LR"({"lod":0.7})",true);check(LR"({"lod":0.8})",true);check(LR"({"lod":1.7})",true);check(LR"({"lod":0.75})",false);
  check(LR"({"press":32})",true);check(LR"({"press":33})",false);check(LR"({"sleep":true})",true);check(LR"({"sleep":1})",false);
  check(LR"({"selected":6})",true);check(LR"({"selected":7})",false);check(LR"({"minutes":120})",true);check(LR"({"minutes":121})",false);
  check(LR"({"firmware":1})",false);check(LR"({"dpi":800,"polling":1000})",false);check(LR"({})",false);
  // A different synthetic device defines its own key and range, no host/model branch.
  plugin=object(LR"({"ui":{"remoteControls":{"custom":{"type":"number","min":2,"max":8,"step":2}}}})");
  check(LR"({"custom":4})",true);check(LR"({"custom":5})",false);check(LR"({"custom":10})",false);check(LR"({"dpi":800})",false);
  std::cout<<"PASS "<<checks<<" remote control schema boundary tests; no device I/O\n";return 0;
 }catch(std::exception const& e){std::cerr<<e.what()<<"\n";return 1;}catch(...){return 1;}
}
