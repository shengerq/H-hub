#include "Common.h"
#include "NativePluginPage.h"
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.XamlTypeInfo.h>
#include <winrt/Microsoft.UI.Xaml.Markup.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <microsoft.ui.xaml.window.h>
#include <shellapi.h>
#include <future>
#undef GetCurrentTime

using namespace hub;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
struct PluginApplication:ApplicationT<PluginApplication,Markup::IXamlMetadataProvider>{
 XamlTypeInfo::XamlControlsXamlMetaDataProvider metadata;
 Window window{nullptr};Border mount{nullptr};std::unique_ptr<NativePluginPage> page;
 HANDLE channel=INVALID_HANDLE_VALUE;std::thread server;std::wstring owner,mode,pipeName;HWND hwnd{};
 PluginApplication(std::wstring id,std::wstring role,std::wstring pipe):owner(id),mode(role),pipeName(pipe){}
 Markup::IXamlType GetXamlType(Windows::UI::Xaml::Interop::TypeName const& value){return metadata.GetXamlType(value);}
 Markup::IXamlType GetXamlType(hstring const& value){return metadata.GetXamlType(value);}
 com_array<Markup::XmlnsDefinition> GetXmlnsDefinitions(){return metadata.GetXmlnsDefinitions();}
 void OnLaunched(LaunchActivatedEventArgs const&){
  Resources().MergedDictionaries().Append(XamlControlsResources{});
  window=Window{};mount=Border{};window.Content(mount);window.as<IWindowNative>()->get_WindowHandle(&hwnd);
  auto queue=mount.DispatcherQueue();auto self=get_strong();
  server=std::thread([this,self,queue]{winrt::init_apartment(winrt::apartment_type::multi_threaded);channel=CreateFileW(pipeName.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,nullptr);if(channel==INVALID_HANDLE_VALUE){queue.TryEnqueue([this]{Exit();});return;}
   JsonObject command;bool initialized=false;
   while(pluginPipeRead(channel,command,INFINITE)){
    JsonObject reply;try{auto kind=str(command,L"kind");
     if(kind==L"initialize"){auto manifest=command.GetNamedObject(L"manifest");if(str(manifest,L"id")!=owner)throw hresult_invalid_argument();validatePluginUi(manifest);installedNativeManifests()[owner]=manifest;verifyNativeAssets(owner);nativeModule(owner).load(owner);
      if(mode==L"page"){auto done=std::make_shared<std::promise<void>>();auto future=done->get_future();auto initial=command.GetNamedObject(L"initial");auto parent=(HWND)(uintptr_t)num(command,L"parent");queue.TryEnqueue([this,done,initial,parent]{try{
       window.ExtendsContentIntoTitleBar(true);SetWindowLongPtrW(hwnd,GWL_STYLE,(GetWindowLongPtrW(hwnd,GWL_STYLE)&~(WS_POPUP|WS_CAPTION|WS_THICKFRAME|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX))|WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS);SetWindowLongPtrW(hwnd,GWL_EXSTYLE,GetWindowLongPtrW(hwnd,GWL_EXSTYLE)&~WS_EX_APPWINDOW);SetLastError(0);auto previous=SetParent(hwnd,parent);if(!previous&&GetLastError())throw hresult_error(HRESULT_FROM_WIN32(GetLastError()));
       page=std::make_unique<NativePluginPage>();page->create(hwnd,initial,winrt::get_unknown(mount),owner);mount.SizeChanged([this](auto const&,auto const&){if(page&&page->window){RECT bounds{};GetClientRect(hwnd,&bounds);SetWindowPos(page->window,nullptr,0,0,bounds.right,bounds.bottom,SWP_NOZORDER|SWP_NOACTIVATE|SWP_SHOWWINDOW);}});SetWindowPos(hwnd,nullptr,0,0,1,1,SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_SHOWWINDOW);done->set_value();
      }catch(...){done->set_exception(std::current_exception());}});future.get();}initialized=true;
     }else if(!initialized)throw hresult_invalid_argument();
     else if(kind==L"request")reply.SetNamedValue(L"reply",nativeModule(owner).invoke(command.GetNamedObject(L"request")));
     else if(kind==L"window")put(reply,L"window",(double)(uintptr_t)hwnd);
     else if(kind==L"pageCommand"){auto done=std::make_shared<std::promise<void>>();auto future=done->get_future();auto request=command.GetNamedObject(L"request");queue.TryEnqueue([this,done,request]{try{if(page)page->command(request);done->set_value();}catch(...){done->set_exception(std::current_exception());}});future.get();}
     else throw hresult_invalid_argument();put(reply,L"ok",true);
    }catch(hresult_error const& e){put(reply,L"ok",false);put(reply,L"error",std::wstring(e.message()));}catch(...){put(reply,L"ok",false);put(reply,L"error",std::wstring(L"插件进程处理失败"));}if(!pluginPipeWrite(channel,reply))break;
   }
   CloseHandle(channel);channel=INVALID_HANDLE_VALUE;queue.TryEnqueue([this]{page.reset();nativeModule(owner).unload();window.Close();Exit();});
  });
 }
 ~PluginApplication(){if(channel!=INVALID_HANDLE_VALUE)CancelIoEx(channel,nullptr);if(server.joinable())server.join();}
};
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int){try{winrt::init_apartment(winrt::apartment_type::single_threaded);nativeModuleLocal=true;int count{};auto args=CommandLineToArgvW(GetCommandLineW(),&count);if(!args||count!=5||std::wstring(args[1])!=L"--plugin-host")return 2;std::wstring id=args[2],mode=args[3],pipe=args[4];LocalFree(args);if(!validPluginId(id)||(mode!=L"page"&&mode!=L"service"))return 2;Application::Start([&](auto&&){winrt::make<PluginApplication>(id,mode,pipe);});return 0;}catch(...){return 1;}}
