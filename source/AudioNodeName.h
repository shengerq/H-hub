#pragma once
#include "EqNode.h"
namespace hub {
inline bool integratedAudioName(std::wstring name){return EqNode::integrated(name);}
inline int renameIntegratedAudioNode(std::filesystem::path report){JsonObject result;try{if(fxSoundIsRunning())throw hresult_error(E_ABORT,L"FxSound 正在使用共享音频节点");auto renamed=EqNode::rename();put(result,L"ok",renamed>0);put(result,L"renamed",(double)renamed);if(!report.empty())write(report,std::wstring(result.Stringify()));return renamed?0:1;}catch(hresult_error const& e){put(result,L"ok",false);put(result,L"error",std::wstring(e.message()));if(!report.empty())write(report,std::wstring(result.Stringify()));return 1;}}
inline void restoreIntegratedDefaultOutput(std::wstring preferred){if(!fxSoundIsRunning())EqNode::physicalDefaults(preferred);}
}
