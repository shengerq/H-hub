#pragma once
#include <ole2.h>
#include <shellapi.h>
namespace hub {
// A small OLE target isolates Explorer file drops from the XAML storage-item broker.
// It exists only while the plugin-manager drop area is visible.
struct NativePluginDrop:winrt::implements<NativePluginDrop,::IDropTarget> {
 HWND window{};bool registered=false,hover=false,filesOffered=false,dark=false;HFONT heading{},caption{};
 std::wstring label,description;std::function<bool()> enabled;std::function<void(std::vector<std::filesystem::path>)> dropped;
 ~NativePluginDrop(){close();}
 void close(){if(registered){registered=false;RevokeDragDrop(window);}if(window){DestroyWindow(window);window=nullptr;}if(heading){DeleteObject(heading);heading=nullptr;}if(caption){DeleteObject(caption);caption=nullptr;}}
 void highlight(bool value){if(hover==value)return;hover=value;if(window)InvalidateRect(window,nullptr,FALSE);}
 bool accepts(){return window&&IsWindowVisible(window)&&enabled&&enabled();}
 static FORMATETC fileFormat(){return FORMATETC{CF_HDROP,nullptr,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};}
 HRESULT __stdcall DragEnter(::IDataObject* data,DWORD,POINTL,DWORD* effect) noexcept override {
  auto format=fileFormat();filesOffered=data&&data->QueryGetData(&format)==S_OK;return DragOver(0,{},effect);
 }
 HRESULT __stdcall DragOver(DWORD,POINTL,DWORD* effect) noexcept override {
  if(!effect)return E_POINTER;try{bool allow=filesOffered&&accepts()&&(*effect&DROPEFFECT_COPY);*effect=allow?DROPEFFECT_COPY:DROPEFFECT_NONE;highlight(allow);return S_OK;}catch(...){*effect=DROPEFFECT_NONE;return E_FAIL;}
 }
 HRESULT __stdcall DragLeave() noexcept override {filesOffered=false;highlight(false);return S_OK;}
 HRESULT __stdcall Drop(::IDataObject* data,DWORD,POINTL,DWORD* effect) noexcept override {
  if(!effect)return E_POINTER;bool allow=(*effect&DROPEFFECT_COPY)!=0;*effect=DROPEFFECT_NONE;filesOffered=false;highlight(false);
  try{
   if(!allow||!data||!accepts())return S_OK;
   auto format=fileFormat();STGMEDIUM medium{};auto hr=data->GetData(&format,&medium);if(FAILED(hr))return hr;
   struct Release{STGMEDIUM& value;~Release(){ReleaseStgMedium(&value);}} release{medium};
   std::vector<std::filesystem::path> paths;auto handle=(HDROP)medium.hGlobal;auto count=DragQueryFileW(handle,0xffffffff,nullptr,0);
   // The callback validates the count. Avoid allocating an unbounded list for an accidental multi-file drop.
   if(count!=1)paths.resize(count?2:0);
   else{auto length=DragQueryFileW(handle,0,nullptr,0);std::wstring path(length+1,L'\0');DragQueryFileW(handle,0,path.data(),length+1);path.resize(length);paths.emplace_back(path);}
   if(dropped)dropped(std::move(paths));*effect=DROPEFFECT_COPY;return S_OK;
  }catch(...){return E_FAIL;}
 }
 static LRESULT CALLBACK proc(HWND h,UINT message,WPARAM w,LPARAM l){
  auto self=(NativePluginDrop*)GetWindowLongPtrW(h,GWLP_USERDATA);
  if(message==WM_NCCREATE){self=(NativePluginDrop*)((CREATESTRUCTW*)l)->lpCreateParams;SetWindowLongPtrW(h,GWLP_USERDATA,(LONG_PTR)self);}
  if(!self)return DefWindowProcW(h,message,w,l);
  if(message==WM_ERASEBKGND)return 1;
  if(message==WM_PAINT){PAINTSTRUCT paint{};auto dc=BeginPaint(h,&paint);RECT r{};GetClientRect(h,&r);double dpi=GetDpiForWindow(h)/96.0;
   auto bg=self->hover?(self->dark?RGB(28,48,66):RGB(230,242,255)):(self->dark?RGB(30,37,47):RGB(255,255,255));
   SetDCBrushColor(dc,bg);FillRect(dc,&r,(HBRUSH)GetStockObject(DC_BRUSH));auto pen=CreatePen(PS_SOLID,std::max(1,(int)dpi),self->hover?RGB(48,157,230):(self->dark?RGB(48,59,73):RGB(221,227,235)));auto oldPen=SelectObject(dc,pen);auto oldBrush=SelectObject(dc,GetStockObject(NULL_BRUSH));RoundRect(dc,0,0,r.right,r.bottom,(int)(12*dpi),(int)(12*dpi));SelectObject(dc,oldBrush);SelectObject(dc,oldPen);DeleteObject(pen);
   SetBkMode(dc,TRANSPARENT);SetTextColor(dc,self->dark?RGB(245,245,245):RGB(27,27,27));auto oldFont=SelectObject(dc,self->heading);RECT title{(int)(14*dpi),(int)(12*dpi),r.right-(int)(14*dpi),(int)(36*dpi)};DrawTextW(dc,self->label.c_str(),-1,&title,DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
   SelectObject(dc,self->caption);SetTextColor(dc,self->dark?RGB(164,178,195):RGB(95,105,120));RECT caption{(int)(14*dpi),(int)(38*dpi),r.right-(int)(14*dpi),r.bottom-(int)(6*dpi)};DrawTextW(dc,self->description.c_str(),-1,&caption,DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);SelectObject(dc,oldFont);EndPaint(h,&paint);return 0;
  }
  return DefWindowProcW(h,message,w,l);
 }
 void create(HWND parent,std::wstring title,std::wstring detail){
  label=std::move(title);description=std::move(detail);static ATOM atom=[] {WNDCLASSW c{};c.lpfnWndProc=proc;c.hInstance=GetModuleHandleW(nullptr);c.lpszClassName=L"Hhub.PluginFileDrop";c.hCursor=LoadCursorW(nullptr,IDC_ARROW);return RegisterClassW(&c);}();
  if(!atom&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)throw hresult_error(HRESULT_FROM_WIN32(GetLastError()));
  window=CreateWindowExW(WS_EX_LAYERED,L"Hhub.PluginFileDrop",label.c_str(),WS_CHILD,0,0,1,1,parent,nullptr,GetModuleHandleW(nullptr),this);check_bool(window!=nullptr);check_bool(SetLayeredWindowAttributes(window,0,255,LWA_ALPHA));
  auto dpi=GetDpiForWindow(parent)/96.0;heading=CreateFontW(-(int)(16*dpi),0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");caption=CreateFontW(-(int)(13*dpi),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
  check_hresult(RegisterDragDrop(window,this));registered=true;
 }
};
}
