Button catalogCheckButton(DependencyObject root,std::wstring const& tag){
 Button found{nullptr};std::function<void(DependencyObject)> visit=[&](DependencyObject node){if(found)return;if(auto button=node.try_as<Button>();button&&unbox_value_or<hstring>(button.Tag(),hstring{})==tag){found=button;return;}for(int i=0;i<VisualTreeHelper::GetChildrenCount(node);i++)visit(VisualTreeHelper::GetChild(node,i));};visit(root);if(!found)throw hresult_error(E_FAIL,L"Missing catalog control: "+tag);return found;
}
void invokeCatalogCheckButton(DependencyObject root,std::wstring const& tag){auto button=catalogCheckButton(root,tag);auto peer=Automation::Peers::ButtonAutomationPeer(button);peer.GetPattern(Automation::Peers::PatternInterface::Invoke).as<Automation::Provider::IInvokeProvider>().Invoke();}
Windows::Foundation::IAsyncAction checkCatalogPageInteractions(){
 auto lifetime=get_strong();winrt::apartment_context ui;JsonObject report;JsonArray checks;bool ok=true;
 auto check=[&](bool passed,std::wstring name){JsonObject row;put(row,L"passed",passed);put(row,L"check",name);checks.Append(row);ok=ok&&passed;};
 try{
  auto total=catalogFeed.GetNamedArray(L"plugins").Size();put(report,L"entries",(double)total);
  check(total>=300&&catalogItems.Children().Size()==catalogPageSize+1,L"Catalog entries mount only one bounded page plus navigation");
  auto first=unbox_value<hstring>(catalogItems.Children().GetAt(0).as<FrameworkElement>().Tag());invokeCatalogCheckButton(catalogItems,L"catalog-next");check(catalogPageIndex==1&&unbox_value<hstring>(catalogItems.Children().GetAt(0).as<FrameworkElement>().Tag())!=first,L"Actual next-page button changes the displayed package identities");invokeCatalogCheckButton(catalogItems,L"catalog-previous");check(catalogPageIndex==0&&unbox_value<hstring>(catalogItems.Children().GetAt(0).as<FrameworkElement>().Tag())==first,L"Previous page restores the first package");
  size_t bound=catalogEventCleanup.size();bool bounded=true;for(int i=0;i<6;i++){invokeCatalogCheckButton(catalogItems,L"catalog-next");bounded=bounded&&catalogItems.Children().Size()<=catalogPageSize+1&&catalogEventCleanup.size()<=bound+2;}
  check(bounded,L"Repeated pagination keeps card and callback counts bounded");
  while(catalogCheckButton(catalogItems,L"catalog-next").IsEnabled())invokeCatalogCheckButton(catalogItems,L"catalog-next");check(catalogItems.Children().Size()==((total-1)%catalogPageSize)+2&&!catalogCheckButton(catalogItems,L"catalog-next").IsEnabled(),L"Last page has the exact remaining entries and disables next");
  invokeCatalogCheckButton(catalogFilterHost,L"keyboard");check(catalogKindFilter==L"keyboard"&&catalogPageIndex==0&&catalogItems.Children().Size()==2,L"Keyboard category resets paging and displays its one package");
  SetWindowTextW(nativeCatalogSearch->edit,L"A");SetWindowTextW(nativeCatalogSearch->edit,L"ASUS");SetWindowTextW(nativeCatalogSearch->edit,L"ASUS ROG");co_await winrt::resume_after(std::chrono::milliseconds(300));co_await ui;
  check(pluginQuery==L"ASUS ROG"&&catalogItems.Children().Size()==2&&unbox_value<hstring>(catalogItems.Children().GetAt(0).as<FrameworkElement>().Tag())==L"hhub.rog-falchion-ace-hfx",L"Debounced search matches brand metadata from the directory");
  SetWindowTextW(nativeCatalogSearch->edit,L"HHUB_NO_MATCH_746281");co_await winrt::resume_after(std::chrono::milliseconds(300));co_await ui;check(catalogItems.Children().Size()==2&&!catalogItems.Children().GetAt(0).try_as<Border>(),L"Empty search results show a message without package cards");
  SetWindowTextW(nativeCatalogSearch->edit,L"");co_await winrt::resume_after(std::chrono::milliseconds(300));co_await ui;invokeCatalogCheckButton(catalogFilterHost,L"application");check(catalogItems.Children().Size()==5,L"Application category contains exactly four plugins");
  invokeCatalogCheckButton(catalogFilterHost,L"mouse");check(catalogItems.Children().Size()==catalogPageSize+1&&catalogKindFilter==L"mouse",L"Mouse packages remain paginated after category changes");
  auto retired=nativeCatalogSearch->window;SetWindowTextW(nativeCatalogSearch->edit,L"queued");go(L"home");co_await winrt::resume_after(std::chrono::milliseconds(300));co_await ui;check(!nativeCatalogSearch&&!catalogSearchDelay&&!IsWindow(retired)&&catalogEventCleanup.empty(),L"Leaving the market cancels pending search and releases windows and page events");
 }catch(hresult_error const& error){ok=false;put(report,L"error",std::wstring(error.message()));}
 put(report,L"ok",ok);report.SetNamedValue(L"checks",checks);write(dataDir()/L"catalog-page-checks.json",std::wstring(report.Stringify()));
}
