DispatcherTimer eqRenderCheckTimer{nullptr};unsigned eqRenderCheckStep=0;bool eqRenderCheckBusy=false;JsonArray eqRenderSamples;
void runEqRenderingCheck(){
 if(instanceSuffix().empty())throw hresult_invalid_argument(L"EQ rendering check requires an isolated instance");
 eqRenderCheckTimer=DispatcherTimer{};eqRenderCheckTimer.Interval(std::chrono::seconds(3));
 eqRenderCheckTimer.Tick([this](auto const&,auto const&){
  if(eqRenderCheckBusy)return;eqRenderCheckBusy=true;struct Reset{bool& value;~Reset(){value=false;}}reset{eqRenderCheckBusy};
  auto sample=[&](std::wstring phase){
   JsonObject result;put(result,L"phase",phase);bool ok=nativeSurface&&nativeSurface->local&&nativeSurface->alive();HWND editor=ok?FindWindowExW(nativeSurface->window,nullptr,L"Hhub.NativeEqPage",nullptr):nullptr;
   ok=ok&&editor&&IsWindowVisible(editor);put(result,L"visible",ok);put(result,L"dark",dark);
   if(editor){
    bool singleLayer=(GetWindowLongPtrW(editor,GWL_EXSTYLE)&WS_EX_LAYERED)==0;put(result,L"singleLayer",singleLayer);ok=ok&&singleLayer;
    bool owned=(HMODULE)GetClassLongPtrW(editor,GCLP_HMODULE)==nativeModule(surfaceOwner).library;put(result,L"classOwnedByFeature",owned);ok=ok&&owned;
    RECT rect{};GetClientRect(editor,&rect);int width=rect.right,height=rect.bottom;
    if(width>0&&height>0){
     HDC source=GetDC(editor),memory=CreateCompatibleDC(source);BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=width;info.bmiHeader.biHeight=-height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;void* pixels{};HBITMAP bitmap=CreateDIBSection(source,&info,DIB_RGB_COLORS,&pixels,nullptr,0);auto old=SelectObject(memory,bitmap);
     bool captured=BitBlt(memory,0,0,width,height,source,0,0,SRCCOPY)!=FALSE;GdiFlush();size_t black=0,white=0;auto bytes=(unsigned char*)pixels;
     if(captured&&bytes)for(size_t i=0;i<(size_t)width*height;i++){auto b=bytes+i*4;if(b[0]<4&&b[1]<4&&b[2]<4)black++;if(b[0]>235&&b[1]>235&&b[2]>235)white++;}
     double blackRatio=(double)black/std::max<size_t>(1,(size_t)width*height),whiteRatio=(double)white/std::max<size_t>(1,(size_t)width*height);put(result,L"blackRatio",blackRatio);put(result,L"whiteRatio",whiteRatio);put(result,L"width",(double)width);put(result,L"height",(double)height);
     ok=ok&&captured&&blackRatio<.05&&(dark?whiteRatio<.15:whiteRatio>.4);
     if(captured&&pixels){BITMAPFILEHEADER header{};header.bfType=0x4d42;header.bfOffBits=sizeof(header)+sizeof(BITMAPINFOHEADER);header.bfSize=header.bfOffBits+width*height*4;std::ofstream file(dataDir()/(L"eq-render-"+phase+L".bmp"),std::ios::binary);file.write((char*)&header,sizeof(header));file.write((char*)&info.bmiHeader,sizeof(BITMAPINFOHEADER));file.write((char*)pixels,width*height*4);}
     SelectObject(memory,old);DeleteObject(bitmap);DeleteDC(memory);ReleaseDC(editor,source);
    }else ok=false;
   }
   auto paneBrush=nav.pane.Background().try_as<SolidColorBrush>();bool themed=paneBrush&&(dark?paneBrush.Color().R<40:paneBrush.Color().R>235);put(result,L"navigationTheme",themed);ok=ok&&themed;put(result,L"ok",ok);eqRenderSamples.Append(result);
  };
  try{
   switch(eqRenderCheckStep++){
    case 0:put(state.GetNamedObject(L"settings"),L"theme",std::wstring(L"dark"));go(L"plugin:eq");break;
    case 1:sample(L"dark");break;
    case 2:sample(L"dark-idle");go(L"home");break;
    case 3:put(state.GetNamedObject(L"settings"),L"theme",std::wstring(L"light"));go(L"plugin:eq");break;
    case 4:sample(L"light-reopen");put(state.GetNamedObject(L"settings"),L"theme",std::wstring(L"dark"));go(L"home");go(L"plugin:eq");nav.IsPaneOpen(!nav.IsPaneOpen());break;
    case 5:sample(L"dark-navigation");SetWindowPos(hwnd,nullptr,0,0,1080,760,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);break;
    default:{sample(L"dark-resize");bool ok=true;for(auto const& value:eqRenderSamples)ok=ok&&flag(value.GetObject(),L"ok");JsonObject report;put(report,L"ok",ok);report.SetNamedValue(L"samples",eqRenderSamples);write(dataDir()/L"eq-rendering.json",std::wstring(report.Stringify()));eqRenderCheckTimer.Stop();window.Close();break;}
   }
  }catch(hresult_error const& error){JsonObject result;put(result,L"ok",false);put(result,L"error",std::wstring(error.message()));write(dataDir()/L"eq-rendering.json",std::wstring(result.Stringify()));eqRenderCheckTimer.Stop();window.Close();}
 });eqRenderCheckTimer.Start();
}
