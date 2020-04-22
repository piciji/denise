
#define IDC_FRAME           1119

HWND pBrowserWindow::dummyParent = nullptr;

STDMETHODIMP FileDialogEventHandler::OnSelectionChange ( IFileDialog* pfd ) {
    std::string path = getFilePath(pfd);
    
  //  if (!path.empty() && path != filePath) {
        if (state && state->onSelectionChange)
            state->onSelectionChange(path);

        filePath = path;
   // }  

    return S_OK;
}

auto FileDialogEventHandler::getFilePath( IFileDialog* pfd ) -> std::string {
    
    if (!pfd)
        return "";
    
    LPOLESTR pwsz = NULL;
    IShellItem* pItem;
    std::string path = "";
            
    HRESULT hr = pfd->GetFolder(&pItem);
        
    if ( SUCCEEDED(hr)) {
        
        hr = pItem->GetDisplayName ( SIGDN_FILESYSPATH, &pwsz );

        if ( SUCCEEDED(hr) ) {
            
            path = utf8_t(pwsz);
            std::replace( path.begin(), path.end(), '\\', '/');
            CoTaskMemFree ( pwsz );
        }
    }
    
    hr = pfd->GetFileName( &pwsz );    
        
    if ( SUCCEEDED(hr)) {
        
        std::string name = utf8_t(pwsz);       

        path += "/" + name;
        
        CoTaskMemFree ( pwsz );
    }    
    
    return path;
}

STDMETHODIMP FileDialogEventHandler::QueryInterface(REFIID riid, void** ppvObject) { 
    *ppvObject = NULL;
    
    if (riid == IID_IFileDialogEvents) {
        *ppvObject = (IFileDialogEvents*)this;
        return S_OK;
    }
    
    if (riid == IID_IFileDialogControlEvents) {
        *ppvObject = (IFileDialogControlEvents*)this;
        return S_OK;
    }    
    
    return E_NOINTERFACE;
}

IFACEMETHODIMP FileDialogEventHandler::OnButtonClicked ( IFileDialogCustomize* pfdc, DWORD dwIDCtl ) {    
    auto id = dwIDCtl - 1000;
    
    if (id < state->buttons.size()) {
        
        auto button = state->buttons[id];
        
        if (button.onClick)
            if ( button.onClick( filePath, 0 ) )
                pDlg->Close( S_OK );
    }
    
    return S_OK;
}

auto pBrowserWindow::getIFileParent() -> HWND {
    if (pDlg) {   
        IOleWindow* pWindow;
        HRESULT hr = pDlg->QueryInterface(IID_PPV_ARGS(&pWindow));
        if (SUCCEEDED(hr)) {
            HWND hwndDialog;
            hr = pWindow->GetWindow(&hwndDialog);
        
            if (SUCCEEDED(hr)) {
                return hwndDialog;
            }
        }
    }
    return nullptr;
}

auto pBrowserWindow::fileVista(bool save) -> std::string {
    auto& state = browserWindow.state;
    std::string name = "";
    HRESULT hr;      
    
    COMDLG_FILTERSPEC aFileTypes[ state.filters.size() ];
    
    utf16_t* utfConvert[state.filters.size() << 1];
    
    unsigned i = 0;
    for(auto& filter : state.filters) {
        std::vector<std::string> tokens = String::split(filter, '(');

        if(tokens.size() != 2)
            continue;
        
        std::string part1 = tokens[0];
        String::delSpaces(part1);
        
        std::string part2 = tokens[1];
        part2.pop_back();        
        String::delSpaces(part2);
        
        std::replace( part2.begin(), part2.end(), ',', ';');
        
        utfConvert[(i << 1) + 0] = new utf16_t( part1 );
        utfConvert[(i << 1) + 1] = new utf16_t( part2 );

        aFileTypes[i] = { *utfConvert[(i << 1) + 0], *utfConvert[(i << 1) + 1] };
        
        i++;
    }   
    
    if (save)
        hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, 
            IID_IFileSaveDialog, reinterpret_cast<void**>(&pDlg));
    else
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, 
            IID_IFileOpenDialog, reinterpret_cast<void**>(&pDlg));
    
    if ( FAILED(hr) )
        return "";        
    
    if (!state.textOk.empty())
        pDlg->SetOkButtonLabel( (LPCWSTR)utf16_t(state.textOk) );    
    
    pDialogEventHandler = new FileDialogEventHandler;    
    pDialogEventHandler->state = &state;
    pDialogEventHandler->pDlg = pDlg;
    
    pDlg->SetFileTypes ( state.filters.size(), aFileTypes );
    
    for(i = 0; i < (state.filters.size() << 1); i++ )
        delete utfConvert[i];
    
    utf16_t wtitle(state.title.c_str());
    
    std::string path = state.path;
    std::replace( path.begin(), path.end(), '/', '\\');
    
    utf16_t wpath(path.c_str());
    
    IShellItem* location;
    SHCreateItemFromParsingName(wpath, nullptr, IID_IShellItem, reinterpret_cast<void **>(&location));
    
    pDlg->SetTitle ( wtitle );
    pDlg->SetFolder( location );
        
    if (state.buttons.size() > 0) {        
        IFileDialogCustomize* pDlgc = nullptr;
        hr = pDlg->QueryInterface(IID_IFileDialogCustomize, reinterpret_cast<void**>(&pDlgc) );

        if ( SUCCEEDED(hr) ) {
            
            unsigned i = 0;
            for(auto& button : state.buttons) {                                
                pDlgc->AddPushButton(1000 + i++, utf16_t(button.text) );
            }
        }    
    }    
    
    pDlg->Advise(pDialogEventHandler, &cookie);  

    if (dummyParent)
        SetWindowLongPtr(dummyParent, GWLP_USERDATA, (LONG_PTR)this);
        
    hr = pDlg->Show ( (state.window && state.modal) ? state.window->p.hwnd : dummyParent );
    
    if(!pDlg || !SUCCEEDED(hr))
        return "";     
    
    pDlg->Unadvise(cookie);  
    
    IShellItem* pItem;

    hr = pDlg->GetResult ( &pItem );

    if ( SUCCEEDED(hr) ) {
        LPOLESTR pwsz = NULL;

        hr = pItem->GetDisplayName ( SIGDN_FILESYSPATH, &pwsz );

        if ( SUCCEEDED(hr) ) {
            name = utf8_t(pwsz);
            std::replace( name.begin(), name.end(), '\\', '/');
            CoTaskMemFree ( pwsz );
        }
    }
                        
    if (pDialogEventHandler)
        delete pDialogEventHandler;
    pDialogEventHandler = nullptr;
    
    pDlg->Release();
    pDlg = nullptr;
    dialogHwnd = nullptr;
    
    return name;
}

// legacy mode
auto pBrowserWindow::file(bool save) -> std::string {
    
    auto& state = browserWindow.state;
    
    if (!state.contentView.id && (pApplication::version >= WindowsVista))
        return fileVista(save);
    
    pApplication::currentWorkingDirectory(); //unfortunately file dialog overwrites cwd, so get it before, if not already done
    std::string path = state.path;
    utf16_t wtitle(state.title.c_str());
    std::replace( path.begin(), path.end(), '/', '\\');
    utf16_t wpath(path.c_str());
    std::string filters = "";

    for(auto& filter : state.filters) {
        std::vector<std::string> tokens = String::split(filter, '(');

        if(tokens.size() != 2) continue;
        std::string part = tokens.at(1);
        part.pop_back();
        String::delSpaces(part);
        std::replace( part.begin(), part.end(), ',', ';');
        filters += filter + "\t" + part + "\t";
    }

    utf16_t wfilters(filters.c_str());
    wchar_t wname[PATH_MAX + 1] = L"";

    wchar_t* p = wfilters;
    while(*p != L'\0') {
        if(*p == L'\t') *p = L'\0';
        p++;
    }

    if(!path.empty()) {
        //clear COMDLG32 MRU (most recently used) file list
        //this is required in order for lpstrInitialDir to be honored in Windows 7 and above
        SHDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ComDlg32\\LastVisitedPidlMRU");
        SHDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ComDlg32\\OpenSavePidlMRU");
    }
    
    if (dummyParent)
        SetWindowLongPtr(dummyParent, GWLP_USERDATA, (LONG_PTR)this);

    OPENFILENAME ofn;
    memset(&ofn, 0, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = (state.window && state.modal) ? state.window->p.hwnd : dummyParent;
    ofn.lpstrFilter = wfilters;
    ofn.lpstrInitialDir = wpath;
    ofn.lpstrFile = wname;
    ofn.lpstrTitle = wtitle;
    ofn.nMaxFile = PATH_MAX;
    ofn.hInstance = GetModuleHandle(0);
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ENABLESIZING;
    ofn.lpstrDefExt = L"";
    
    if (state.templateId != -1) {
        ofn.Flags |= OFN_ENABLEHOOK | OFN_ENABLETEMPLATE;
        ofn.lpfnHook = pBrowserWindow::OfnHookProc;
        ofn.lpTemplateName = MAKEINTRESOURCE(state.templateId);
        ofn.lCustData = (LPARAM)this;
    }

    bool result = !save ? GetOpenFileName(&ofn) : GetSaveFileName(&ofn);
    if(!result) return "";
    std::string name = utf8_t(wname);
    std::replace( name.begin(), name.end(), '\\', '/');
    dialogHwnd = nullptr;
    return name;
}

auto pBrowserWindow::createTooltip(HWND hwnd) -> void {
    
    auto colorBg = browserWindow.state.contentView.backgroundColor;    
    auto overrideBg = browserWindow.state.contentView.overrideBackgroundColor;    
    auto colorFg = browserWindow.state.contentView.foregroundColor;  
    auto overrideFg = browserWindow.state.contentView.overrideForegroundColor;    
    auto colorTooltips = browserWindow.state.contentView.colorTooltips;
            
    hwndTip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL,
            WS_POPUP | TTS_ALWAYSTIP | TTS_USEVISUALSTYLE,
            CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT,
            hwnd, NULL, GetModuleHandle(0), 0);

    if (colorTooltips && (overrideBg || overrideFg))
        SetWindowTheme(hwndTip, L" ", L" ");

    if (colorTooltips && overrideFg)
        SendMessage(hwndTip, TTM_SETTIPTEXTCOLOR, RGB((colorFg >> 16) & 0xff, (colorFg >> 8) & 0xff, colorFg & 0xff), 0);
    if (colorTooltips && overrideBg)
        SendMessage(hwndTip, TTM_SETTIPBKCOLOR, RGB((colorBg >> 16) & 0xff, (colorBg >> 8) & 0xff, colorBg & 0xff), 0);

    if (listFont)
        SendMessage(hwndTip, WM_SETFONT, (WPARAM)listFont, 0);                                                  

    RECT rectSetMargin = {5, 5, 5, 3};
    SendMessage(hwndTip, TTM_SETMARGIN, 0, (LPARAM)&rectSetMargin);  
}

auto CALLBACK pBrowserWindow::OfnHookProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) -> UINT_PTR {
    
    TCHAR wFilePath[256];
    OPENFILENAME* ofn = nullptr;
    pBrowserWindow* context = nullptr;
    BrowserWindow::State* state = nullptr;
    
    if (uMsg != WM_INITDIALOG)
        ofn = (OPENFILENAME*) (LONG_PTR)GetWindowLongPtr(hDlg, DWLP_USER);                  
    else
        ofn = (OPENFILENAME*)lParam;    

    if (!ofn)
        return FALSE;
    
    context = (pBrowserWindow*) ofn->lCustData;
    if (context)
        state = &context->browserWindow.state;       
          
    HWND listBox = GetDlgItem(hDlg, state->contentView.id);
    
    switch (uMsg) {
        case WM_INITDIALOG: {
            context->inited = false;
            context->dialogHwnd = GetParent(hDlg);
            
            if (!state->textOk.empty())
                SetDlgItemText( context->dialogHwnd, IDOK, (LPCWSTR)utf16_t(state->textOk) );
            
            if (!state->textCancel.empty())
                SetDlgItemText( context->dialogHwnd, IDCANCEL, (LPCWSTR)utf16_t(state->textCancel) );
            
            if (listBox) {
                if (state->contentView.font != "")
                    context->listFont = pFont::create( state->contentView.font );
                else
                    context->listFont = pFont::create( Font::system() );

                SendMessage(listBox, WM_SETFONT, (WPARAM)context->listFont, 0);
                
                auto size = pFont::size(context->listFont, " ");
                
                SendMessage(listBox, LB_SETITEMHEIGHT, 0, size.height);
            }
            auto colorBg = state->contentView.backgroundColor;                
            if (state->contentView.overrideBackgroundColor)
                context->listBgBrush = CreateSolidBrush( RGB((colorBg >> 16) & 0xff, (colorBg >> 8) & 0xff, colorBg & 0xff) );
            
            for(auto& button : state->buttons) {
                SetDlgItemText(hDlg, button.id, (LPCWSTR)utf16_t(button.text) );
            }                            
                                  
            SetWindowLongPtr(listBox, GWLP_USERDATA, (LONG_PTR)context);
            WNDPROC wndprocOrig = (WNDPROC)SetWindowLongPtr(listBox, GWLP_WNDPROC, (LONG_PTR)subclassListbox);
            SetProp( listBox, L"OLDWNDPROC", (HANDLE)wndprocOrig );
            context->lastItem = -1;
            
            if (listBox)
                context->adjustDialogByScreenResolution(hDlg, listBox);
            
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);
            
            return TRUE;
        } 
        
        case WM_CTLCOLORLISTBOX: {
            auto colorBg = state->contentView.backgroundColor;
            auto colorFg = state->contentView.foregroundColor;                  
            
            if (state->contentView.overrideBackgroundColor)
                SetBkColor((HDC)wParam, RGB((colorBg >> 16) & 0xff, (colorBg >> 8) & 0xff, colorBg & 0xff)); 
            if (state->contentView.overrideForegroundColor)
                SetTextColor((HDC)wParam, RGB((colorFg >> 16) & 0xff, (colorFg >> 8) & 0xff, colorFg & 0xff) );
            
            if (state->contentView.overrideBackgroundColor)
                return (LRESULT)context->listBgBrush;
            break;
        }       
        
        case WM_NOTIFY: {
            
            if (((OFNOTIFY*)lParam)->hdr.code == CDN_INITDONE) {
                context->inited = true;
                if (context)                    
                    context->resize(hDlg, true);                    
                
            } else if ( ((OFNOTIFY*)lParam)->hdr.code == CDN_SELCHANGE ) {
                
                if (listBox) {
                    SendMessage(listBox, LB_RESETCONTENT, 0, 0);
                    context->toolTips.clear();
                }
            
                if (SendMessage(((OFNOTIFY*)lParam)->hdr.hwndFrom, CDM_GETFILEPATH, 256, (LPARAM)wFilePath) >= 0) {
                    
                    if (!(GetFileAttributes(wFilePath) & FILE_ATTRIBUTE_DIRECTORY)) {       
                           
                        context->selectedPath = utf8_t(wFilePath);
                        
                        if (!context->selectedPath.empty() && state && state->onSelectionChange) {
                            std::replace( context->selectedPath.begin(), context->selectedPath.end(), '\\', '/');
                            auto rows = state->onSelectionChange( context->selectedPath );            

                            if (listBox) {
                                unsigned maximumWidth = 0;                                                            
                                for( auto& row : rows ) {                                
                                    SendMessage( listBox, LB_ADDSTRING, 0, (LPARAM)(wchar_t*)utf16_t(row.entry) );

                                    maximumWidth = std::max(maximumWidth, pFont::size(context->listFont, row.entry).width);
                                    
                                    context->toolTips.push_back( row.tooltip );
                                }

                                SendMessage( listBox, LB_SETHORIZONTALEXTENT, maximumWidth + 8, 0);
                            }
                        }
                    }                                        
                } else if (((OFNOTIFY*)lParam)->hdr.code == CDN_FOLDERCHANGE) {
                    
                }                
            }
        } break;
        
        case WM_COMMAND: {
            auto widgetId = LOWORD(wParam);
            
            for (auto& button : context->browserWindow.state.buttons) {
                
                if (widgetId == button.id) {
                    
                    if (button.onClick) {
                        if (button.onClick( context->selectedPath, context->contentViewSelection() ))
                            PostMessage(context->dialogHwnd, WM_COMMAND, IDCANCEL, 0); //end dialog
                    }
                    break;
                }                                
            }  
            
            if (listBox && widgetId == state->contentView.id) {
                if (HIWORD(wParam) == LBN_DBLCLK) {
                    if (state->contentView.onDblClick) {
                        context->contentSelection = SendMessage(listBox, LB_GETCURSEL, 0, 0);
                        
                        if (state->contentView.onDblClick(context->selectedPath, context->contentSelection ))
                            PostMessage(context->dialogHwnd, WM_COMMAND, IDCANCEL, 0); //end dialog
                    }                    
                }   
                
                else if (HIWORD(wParam) == LBN_SELCHANGE) { 
                    context->contentSelection = SendMessage(listBox, LB_GETCURSEL, 0, 0);
                }
            }            
            break;
        }
        
        case WM_SIZE:
            if (context)
                context->resize(hDlg);
            return TRUE;
            
        case WM_CLOSE:
            EndDialog(context->dialogHwnd,IDCANCEL);
            return TRUE;
            
        case WM_DESTROY:
            return TRUE;
    }
    
    return FALSE;
}

auto CALLBACK pBrowserWindow::subclassListbox(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    
    pBrowserWindow* instance = (pBrowserWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    WNDPROC lpOldWndProc = (WNDPROC)GetProp( hwnd, L"OLDWNDPROC" );

    switch (msg) {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONDOWN: 
        case WM_MBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
            instance->relayMesssageToToolTip(hwnd, msg, wparam, lparam);
            break;
        
        case WM_MOUSEMOVE: {
            if ((wparam & MK_LBUTTON) == 0) {
                POINT point;
                point.x = LOWORD(lparam);
                point.y = HIWORD(lparam);
                MapWindowPoints( hwnd, nullptr, (LPPOINT)&point, 1 );
                int curItem = LBItemFromPt( hwnd, point, false );

                if (curItem >= 0) {
                    if ( instance->lastItem != curItem ) {
                        RECT rect;
                        SendMessage(hwnd, LB_GETITEMRECT, curItem, (LPARAM)&rect);
                        instance->setToolTip( hwnd, curItem, rect );
                    }
                }

                instance->relayMesssageToToolTip(hwnd, msg, wparam, lparam);
            }
        } break;
    }
    
    return CallWindowProc(lpOldWndProc, hwnd, msg, wparam, lparam);
}

auto pBrowserWindow::relayMesssageToToolTip(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) -> void {
    if (!hwndTip)
        return;
    
    MSG msg;
    msg.hwnd    = hwnd;
    msg.message = umsg;
    msg.wParam  = wparam;
    msg.lParam  = lparam;
    msg.pt.x    = LOWORD(lparam);
    msg.pt.y    = HIWORD(lparam);
    
    SendMessage( hwndTip, TTM_RELAYEVENT, 0, (LPARAM)&msg);
}

auto pBrowserWindow::setToolTip(HWND hwnd, int curItem, RECT rect) -> void {
    
    if (!hwndTip)
        createTooltip(hwnd);
    
    if (toolTips.size() <= curItem)
        return;    
   
    lastItem = curItem;
               
    TOOLINFO toolInfo = { 0 };
    toolInfo.cbSize = sizeof(toolInfo);
    toolInfo.hwnd = hwnd;
    
    while ( SendMessage(hwndTip, TTM_ENUMTOOLS, 0, (LPARAM)&toolInfo) )
        SendMessage(hwndTip, TTM_DELTOOL, 0, (LPARAM)&toolInfo);
    
    if (toolTips[curItem].empty())
        return;
    
    utf16_t wtooltip( toolTips[curItem] );
    
    toolInfo.uFlags = 0;
    toolInfo.uId = curItem;
    toolInfo.rect.left    = rect.left;
    toolInfo.rect.top     = rect.top;
    toolInfo.rect.right   = rect.right;
    toolInfo.rect.bottom  = rect.bottom;         
    toolInfo.lpszText = wtooltip;
    
    SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
}

auto pBrowserWindow::adjustDialogByScreenResolution(HWND fileDialogView, HWND listBox) -> void {
   
    RECT rDialogView;
    RECT rListBox;
    
    GetClientRect(fileDialogView, &rDialogView);
    GetWindowRect(listBox, &rListBox);   
    
    // 12 -> 472
    // 11 -> 445    
    // 10 -> 390
    // 9  -> 362
    // 8  -> 332
       
    auto newWidth = browserWindow.state.contentView.width;
    
    auto _listWidth = std::abs(rListBox.right - rListBox.left);
    
    SetWindowPos(listBox, 0, 0, 0, newWidth, std::abs(rListBox.bottom - rListBox.top), SWP_NOMOVE);   
    
    SetWindowPos(fileDialogView, 0, 0, 0, std::abs(rDialogView.right - rDialogView.left) - _listWidth + newWidth,  std::abs(rDialogView.bottom - rDialogView.top), SWP_NOMOVE);
}

auto pBrowserWindow::resize( HWND fileDialogView, bool init ) -> void {
    if (!inited || !browserWindow.state.resizeTemplate)
        return;
    
    static signed dpiX = pFont::dpi().x;
    RECT rDialogView;
    RECT rCustomView;
    RECT rListBox;
    
    HWND customView = GetDlgItem(fileDialogView, IDC_FRAME);
    
    if (!customView)
        return;
    
    GetClientRect(fileDialogView, &rDialogView);
    GetWindowRect(customView, &rCustomView);    
    MapWindowPoints(NULL, fileDialogView, (POINT*)&rCustomView, 2);
         
    HWND listBox = GetDlgItem(fileDialogView, browserWindow.state.contentView.id);
    
    if (listBox) {
        GetWindowRect(listBox, &rListBox);       
        MapWindowPoints(NULL, fileDialogView, (POINT*)&rListBox, 2);
    }
            
    if (init) {
        widthCustomView = std::abs(rDialogView.right - rDialogView.left) - rCustomView.right;
        customGapTop = std::abs(rCustomView.top - rDialogView.top);
        customGapBottom = std::abs(rCustomView.bottom - rDialogView.bottom);
        
        if (listBox) {
            listRelativeX = std::abs(rListBox.left - rCustomView.right);
            listWidth = std::abs(rListBox.left - rListBox.right);
        }

        buttons.clear();
        for (auto& button : browserWindow.state.buttons) {
            HWND hwnd = GetDlgItem(fileDialogView, button.id);
            if (!hwnd)
                continue;
            
            RECT rect;

            GetWindowRect(hwnd, &rect);
            MapWindowPoints(NULL, fileDialogView, (POINT*)&rect, 2);

            int width = std::abs(rect.right - rect.left);
            int height = std::abs(rect.bottom - rect.top);
            int relativeX = std::abs(rect.left - rCustomView.right);
            int relativeY = std::abs(rect.top - (listBox ? rListBox.bottom : rCustomView.top) );

            buttons.push_back({hwnd, width, height, relativeX, relativeY});
        }   
    }   
    
    int dialogWidth = std::abs(rDialogView.right - rDialogView.left);
    int dialogHeight = std::abs(rDialogView.bottom - rDialogView.top);    
    int customX = dialogWidth - widthCustomView;

    int buttonTotalHeight = 0;
    
    for (auto& button : buttons) {
        
        buttonTotalHeight = std::max(buttonTotalHeight, button.relativeY + button.height);
    }
    
    int contentHeight = dialogHeight - buttonTotalHeight - customGapTop - customGapBottom + (browserWindow.state.resizeAdjust * dpiX) / 72;
    
    if (listBox) {       
        
        MoveWindow(listBox, customX + listRelativeX, rDialogView.top + customGapTop, listWidth, contentHeight, TRUE);    
    }
       
    for (auto& button : buttons) {
        
        MoveWindow(button.hwnd, customX + button.relativeX, rDialogView.top + customGapTop + contentHeight + button.relativeY, button.width, button.height, TRUE);
        
        InvalidateRect(button.hwnd, 0, false);
    }
                    
    UpdateWindow(fileDialogView);
}

auto pBrowserWindow::setForeground() -> void {
    
    if (!dialogHwnd && pDlg)
        dialogHwnd = getIFileParent();
        
    if (dialogHwnd)
        SetForegroundWindow( dialogHwnd );
}

auto pBrowserWindow::close() -> void {    
    
    if (!dialogHwnd && pDlg)
        dialogHwnd = getIFileParent();

    if (dialogHwnd)
        PostMessage(dialogHwnd, WM_COMMAND, IDCANCEL, 0); //end dialog    
}

auto pBrowserWindow::visible() -> bool {
    if (!dialogHwnd && pDlg)
        dialogHwnd = getIFileParent();
    
    if (dialogHwnd)
        return IsWindowVisible(dialogHwnd);
    
    return false;
}

pBrowserWindow::pBrowserWindow(BrowserWindow& browserWindow) : browserWindow(browserWindow) {
    
    static bool initialized = false;
    
    if (!initialized) {
        WNDCLASS wc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hbrBackground = 0;
        wc.hCursor = nullptr;
        wc.hIcon = nullptr;
        wc.hInstance = GetModuleHandle(0);
        wc.lpfnWndProc = pBrowserWindow::CustomWndProc;
        wc.lpszClassName = L"fileDialogDummy";
        wc.lpszMenuName = 0;
        wc.style = CS_VREDRAW | CS_HREDRAW;
        RegisterClass(&wc);
        
        dummyParent = CreateWindow(L"fileDialogDummy", L"fileDialogDummy", WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, nullptr, nullptr, nullptr, nullptr);
        
        initialized = true;
    }
}

auto CALLBACK pBrowserWindow::CustomWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    pBrowserWindow* worker = (pBrowserWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    return worker->wndProc(hwnd, msg, wparam, lparam);
}

auto pBrowserWindow::wndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {

    switch(msg) {
        case WM_ENTERIDLE: {
            MSG _msg;
            while(!PeekMessage(&_msg, 0, 0, 0, PM_NOREMOVE)) {
                if (Application::loop)
                    Application::loop();
            }            
            break;
        }
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

auto pBrowserWindow::contentViewSelection() -> unsigned {
        
    return contentSelection;
}

pBrowserWindow::~pBrowserWindow() {
    
    if (listBgBrush)
        DeleteObject(listBgBrush);
    
    if (listFont)
        pFont::free(listFont);
    
    if (pDlg)
        pDlg->Release();    
    
    if (pDialogEventHandler)
        delete pDialogEventHandler;    
    
    if (hwndTip)
        DestroyWindow(hwndTip);
    
    pDialogEventHandler = nullptr;
    
    pDlg = nullptr;    
    
    listBgBrush = nullptr;
    
    listFont = nullptr;
    
    dialogHwnd = nullptr;
    
    hwndTip = nullptr;
}

auto pBrowserWindow::directory() -> std::string {
    auto& state = browserWindow.state;
    
    pApplication::currentWorkingDirectory();
    wchar_t wname[PATH_MAX + 1] = L"";
    utf16_t wtitle( state.title );

    BROWSEINFO bi;
    bi.hwndOwner = state.window ? state.window->p.hwnd : 0;
    bi.pidlRoot = NULL;
    bi.pszDisplayName = wname;
    bi.lpszTitle = wtitle;
    bi.ulFlags = BIF_NEWDIALOGSTYLE | BIF_RETURNONLYFSDIRS;
    bi.lpfn = pBrowserWindow::PathCallbackProc;
    bi.lParam = (LPARAM)&state;
    bi.iImage = 0;
    bool result = false;
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

    if(pidl && SHGetPathFromIDList(pidl, wname)) {
        result = true;
        IMalloc *imalloc = 0;
        if(SUCCEEDED(SHGetMalloc(&imalloc))) {
            imalloc->Free(pidl);
            imalloc->Release();
        }
    }

    if(!result) return "";
    std::string name = utf8_t(wname);
    if(name.empty()) return "";
    std::replace( name.begin(), name.end(), '\\', '/');
    if (name.back() != '/') name.push_back('/');
    return name;
}

auto CALLBACK pBrowserWindow::PathCallbackProc(HWND hwnd, UINT msg, LPARAM lparam, LPARAM lpdata) -> int {
    if(msg == BFFM_INITIALIZED) {
        if(lpdata) {
            auto state = (BrowserWindow::State*)lpdata;
            utf16_t wtitle( state->title );
            std::string path = state->path;
            std::replace( path.begin(), path.end(), '/', '\\');
            utf16_t wpath( path );

            if( !state->title.empty() ) SetWindowText(hwnd, wtitle);
            SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)(wchar_t*)wpath);
        }
    }
    return 0;
}
