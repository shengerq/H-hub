// Service requests do not allocate a XAML application, window or dispatcher.
static int runPluginService(std::wstring const& owner,std::wstring const& pipeName){
 auto channel=CreateFileW(pipeName.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,nullptr);
 if(channel==INVALID_HANDLE_VALUE)return 3;
 JsonObject command;bool initialized=false;
 try{
  while(pluginPipeRead(channel,command,INFINITE)){
   JsonObject reply;bool stop=false;
   try{
    auto kind=str(command,L"kind");
    if(kind==L"initialize"){
     auto manifest=command.GetNamedObject(L"manifest");if(str(manifest,L"id")!=owner)throw hresult_invalid_argument();
     validatePluginUi(manifest);installedNativeManifests()[owner]=manifest;verifyNativeAssets(owner);nativeModule(owner).load(owner);initialized=true;
    }else if(!initialized)throw hresult_invalid_argument();
    else if(kind==L"request")reply.SetNamedValue(L"reply",nativeModule(owner).invoke(command.GetNamedObject(L"request")));
    else if(kind==L"shutdown"){stop=true;nativeModule(owner).unload();}
    else throw hresult_invalid_argument();
    put(reply,L"ok",true);
   }catch(hresult_error const& e){put(reply,L"ok",false);put(reply,L"error",std::wstring(e.message()));}
   catch(...){put(reply,L"ok",false);put(reply,L"error",std::wstring(L"功能服务处理失败"));}
   if(stop||!pluginPipeWrite(channel,reply))break;
  }
 }catch(...) {}
 nativeModule(owner).unload();CloseHandle(channel);return 0;
}
