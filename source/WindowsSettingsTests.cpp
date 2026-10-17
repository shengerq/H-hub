#include "WindowsSettings.h"
using namespace hub;
static void require(bool value,wchar_t const* reason){if(!value)throw hresult_error(E_FAIL,reason);}
int wmain(int argc,wchar_t** argv){
 winrt::init_apartment(winrt::apartment_type::multi_threaded);auto root=binaryDir()/L"test-results";
 auto prefix=L"Software\\HhubWindowsRewriteTests\\"+std::to_wstring(GetCurrentProcessId());JsonObject report;JsonArray passed;
 try{
  WindowsSettings settings(root/L"undo.json",prefix);
  auto pass=[&](wchar_t const* name){passed.Append(JsonValue::CreateStringValue(name));};
  require(windowsOptionCatalog().Size()==12,L"Fixed catalog");pass(L"All 12 rows exist before any Windows reads");
  auto initial=settings.readOption(L"gameMode");require(!flag(initial,L"confirmed")&&initial.GetNamedValue(L"enabled").ValueType()==JsonValueType::Null,L"Unknown preference");pass(L"Missing preferences remain unknown");
  settings.set(L"gameMode",true);require(flag(settings.readOption(L"gameMode"),L"confirmed")&&flag(settings.readOption(L"gameMode"),L"enabled"),L"Write readback");
  settings.set(L"gameMode",false);require(!flag(settings.readOption(L"gameMode"),L"enabled"),L"Off readback");settings.restore(L"gameMode");require(flag(settings.readOption(L"gameMode"),L"enabled"),L"Undo readback");pass(L"On, off and undo return verified actual values");
  DWORD flags=FKF_FILTERKEYSON|FKF_HOTKEYACTIVE|FKF_CLICKON;
  settings.spi=[&](UINT action,UINT,PVOID ptr,UINT){auto s=(FILTERKEYS*)ptr;if(action==SPI_GETFILTERKEYS){s->dwFlags=flags;s->iWaitMSec=123;return TRUE;}if(action==SPI_SETFILTERKEYS){require(s->iWaitMSec==123,L"Filter timing");flags=s->dwFlags;return TRUE;}return FALSE;};
  settings.set(L"filterShortcut",false);require((flags&FKF_FILTERKEYSON)&&(flags&FKF_CLICKON)&&!(flags&FKF_HOTKEYACTIVE),L"Unrelated flags");settings.restore(L"filterShortcut");pass(L"Input writes preserve unrelated settings");
  settings.spi=[](UINT action,UINT,PVOID ptr,UINT){if(action==SPI_GETMOUSE){auto values=(int*)ptr;values[0]=6;values[1]=10;values[2]=0;return TRUE;}if(action==SPI_SETMOUSE)return TRUE;return FALSE;};
  bool rejected=false;try{settings.set(L"mousePrecision",true);}catch(...){rejected=true;}require(rejected,L"Mismatched readback");pass(L"A successful write API with wrong readback is rejected");
  auto v=WindowsSettings::preferenceValues(L"notifications")[0];std::wstring invalid=L"bad";settings.savePreference(v,WindowsSettings::storedValue(REG_SZ,invalid.c_str(),(invalid.size()+1)*sizeof(wchar_t)));
  auto snapshot=settings.snapshot();auto options=snapshot.GetNamedArray(L"options");require(options.Size()==12,L"Per option isolation");bool bad=false,good=false;
  for(auto const& item:options){auto option=item.GetObject();if(str(option,L"id")==L"notifications")bad=!flag(option,L"confirmed")&&!flag(option,L"loading");if(str(option,L"id")==L"gameMode")good=flag(option,L"confirmed");}
  require(bad&&good,L"Read failure isolation");pass(L"One invalid setting does not prevent other rows from loading");
  put(report,L"ok",true);
 }catch(hresult_error const& e){put(report,L"ok",false);put(report,L"error",std::wstring(e.message()));}catch(...){put(report,L"ok",false);}
 RegDeleteTreeW(HKEY_CURRENT_USER,prefix.c_str());report.SetNamedValue(L"passed",passed);write(root/L"windows-tests.json",std::wstring(report.Stringify()));return flag(report,L"ok")?0:1;
}
