#pragma once
#include "EqNode.h"
namespace hub {
inline bool integratedAudioName(std::wstring name){return EqNode::integrated(name);}
inline int renameIntegratedAudioNode(std::filesystem::path report){JsonObject result;try{auto renamed=0; // Shared driver naming is preserved. Migration is explicit installer work.
put(result,L"ok",true);put(result,L"sharedDriver",true);put(result,L"renamed",(double)renamed);if(!report.empty())write(report,std::wstring(result.Stringify()));return 0;}catch(hresult_error const& e){put(result,L"ok",false);put(result,L"error",std::wstring(e.message()));if(!report.empty())write(report,std::wstring(result.Stringify()));return 1;}}
inline void restoreIntegratedDefaultOutput(std::wstring){/* Preserve the shared system defaults. */}
}
