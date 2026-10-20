inline std::vector<NativeAssetDefinition> nativeAssetDefinitions(std::wstring const& id){
 if(id==L"windows")return {
  {L"Hhub.Windows.exe",L"81d38042bb8da34ae919e3297ec74bc134ac72db0b191e9a23f643da07535783"},
 };
 if(id==L"phone")return {
  {L"Hhub.Phone.exe",L"0ff78849a39f8fcd38966fc3a6eb77f31f093d82c19bd8984174a42050984a55"},
  {L"phone-web/app.js",L"f44634cd31c75146a8ac301e5a59f62f9d05a4030e8610adce3be905654110c8"},
  {L"phone-web/index.html",L"62e3811ba22b6dfac277b7a91e601c1c9e13d0f5b260ab2dbda4b946d8c1b405"},
  {L"phone-web/style.css",L"cf7e6a8617de0cfca772f898aa13b7d2d129d4e7d9561a567079e8d8b50e78d4"},
 };
 if(id==L"eq")return {
  {L"HhubAudioDriverSetup.exe",L"60eacd329ede8d2c95b89c35c03418e09e0850bf855f2417f1bd8993ffb5ca96"},
 };
 return {};
}

inline bool nativeAssetHashApproved(std::wstring const& id,NativeAssetDefinition const& definition,std::wstring const& hash){return definition.hash==hash||(id==L"eq"&&definition.name==L"HhubAudioDriverSetup.exe"&&hash==L"de35164f4abe5dd20fb5efedbddff92ea04d4d06738d34bd445cacadc3f98811" );}

