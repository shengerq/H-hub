#pragma once
#include "Common.h"
namespace hub {
inline std::wstring phoneEqKey(JsonObject config){auto input=to_string(str(config,L"app")+L"\n"+str(config,L"output"));BCRYPT_ALG_HANDLE algorithm{};if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw hresult_error(E_FAIL);unsigned char digest[32]{};auto status=BCryptHash(algorithm,nullptr,0,(PUCHAR)input.data(),(ULONG)input.size(),digest,32);BCryptCloseAlgorithmProvider(algorithm,0);if(status<0)throw hresult_error(E_FAIL);std::wstring result;for(auto n:digest){wchar_t part[3]{};swprintf_s(part,L"%02x",n);result+=part;}return result;}
}
