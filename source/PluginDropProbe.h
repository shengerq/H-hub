#pragma once
// Used only by the isolated UI regression profile. Supplies the same OLE file data as Explorer.
struct PluginDropProbe {
 struct Source:winrt::implements<Source,::IDropSource>{HRESULT __stdcall QueryContinueDrag(BOOL escape,DWORD keys) noexcept override{return escape?DRAGDROP_S_CANCEL:(keys&MK_LBUTTON)?S_OK:DRAGDROP_S_DROP;}HRESULT __stdcall GiveFeedback(DWORD) noexcept override{return DRAGDROP_S_USEDEFAULTCURSORS;}};
 HWND window{};winrt::com_ptr<::IDataObject> data;HRESULT lastResult=E_PENDING;DWORD effect{};
 ~PluginDropProbe(){if(window)DestroyWindow(window);}
 static LRESULT CALLBACK proc(HWND h,UINT m,WPARAM w,LPARAM l){
  auto self=(PluginDropProbe*)GetWindowLongPtrW(h,GWLP_USERDATA);if(m==WM_NCCREATE){self=(PluginDropProbe*)((CREATESTRUCTW*)l)->lpCreateParams;SetWindowLongPtrW(h,GWLP_USERDATA,(LONG_PTR)self);}if(!self)return DefWindowProcW(h,m,w,l);
  if(m==WM_PAINT){PAINTSTRUCT p{};auto dc=BeginPaint(h,&p);RECT r{};GetClientRect(h,&r);SetDCBrushColor(dc,RGB(41,49,61));FillRect(dc,&r,(HBRUSH)GetStockObject(DC_BRUSH));SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(245,245,245));DrawTextW(dc,L"Drag this test package into the drop area below",-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);EndPaint(h,&p);return 0;}
  if(m==WM_LBUTTONDOWN){auto source=winrt::make_self<Source>();self->lastResult=DoDragDrop(self->data.get(),source.get(),DROPEFFECT_COPY,&self->effect);return 0;}
  return DefWindowProcW(h,m,w,l);
 }
 void create(HWND parent,std::filesystem::path const& file,RECT rect){
  auto pidl=ILCreateFromPathW(file.c_str());if(!pidl)throw winrt::hresult_error(E_FAIL,L"Test file PIDL unavailable");winrt::com_ptr<IShellFolder> folder;PCUITEMID_CHILD child{};auto hr=SHBindToParent(pidl,IID_PPV_ARGS(folder.put()),&child);if(SUCCEEDED(hr))hr=folder->GetUIObjectOf(nullptr,1,&child,IID_IDataObject,nullptr,data.put_void());ILFree(pidl);winrt::check_hresult(hr);
  static ATOM atom=[] {WNDCLASSW c{};c.lpfnWndProc=proc;c.hInstance=GetModuleHandleW(nullptr);c.lpszClassName=L"Hhub.PluginDropProbe";c.hCursor=LoadCursorW(nullptr,IDC_HAND);return RegisterClassW(&c);}();
  window=CreateWindowExW(WS_EX_LAYERED,L"Hhub.PluginDropProbe",L"Plugin drop test source",WS_CHILD,rect.left,rect.top,rect.right,rect.bottom,parent,nullptr,GetModuleHandleW(nullptr),this);winrt::check_bool(window!=nullptr);SetLayeredWindowAttributes(window,0,255,LWA_ALPHA);SetWindowPos(window,HWND_TOP,rect.left,rect.top,rect.right,rect.bottom,SWP_NOACTIVATE|SWP_SHOWWINDOW);UpdateWindow(window);
 }
};
