
#define IDC_FRAME           1119
#define IDC_BUTTON          1113
#define IDC_BUTTON1         1114
#define IDC_BUTTON2         1115

HWND pBrowserWindow::dummyParent = nullptr;

STDMETHODIMP FileDialogEventHandler::OnSelectionChange ( IFileDialog* pfd ) {
    std::vector<std::string> curSelectedFiles;
    std::vector<std::string> resultFiles;
    std::string path = getFilePath(pfd, curSelectedFiles);

  //  if (!path.empty() && path != filePath) {
        if (state && state->onSelectionChange)
            state->onSelectionChange(path);

        filePath = path;
        pBrowserWindow& p = browserWindow->p;

        if (state->orderBySelected) {
            for(auto& selectedFile : p.selectedFiles) { // sorted selection before
                unsigned pos = 0;
                for(auto& curSelectedFile : curSelectedFiles) { // unsorted current selection
                    if (selectedFile == curSelectedFile) {
                        resultFiles.push_back( selectedFile );
                        GUIKIT::Vector::eraseVectorPos(curSelectedFiles, pos);
                        break;
                    }
                    pos++;
                }
            }

            // if more than one file was added to selection (shift key)
            std::sort(curSelectedFiles.begin(), curSelectedFiles.end(), [](std::string& a, std::string& b) -> bool
            { return a < b; });

            for(auto& curSelectedFile : curSelectedFiles)
                resultFiles.push_back( curSelectedFile );

            p.selectedFiles = resultFiles;
        }

   // }  

    return S_OK;
}

auto FileDialogEventHandler::getFilePath( IFileDialog* pfd, std::vector<std::string>& multiples ) -> std::string {
    
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

        auto temp = GUIKIT::String::split(name, '\"');

        for(auto& test : temp) {
            if (test != " ")
                multiples.push_back(test);
        }

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
        
        if (button.onClick) {
            std::vector<std::string> curSelectedFiles;
            filePath = getFilePath(pDlg, curSelectedFiles);

            if (button.onClick(filePath, 0))
                pDlg->Close(S_OK);
        }
    }
    
    return S_OK;
}

IFACEMETHODIMP FileDialogEventHandler::OnCheckButtonToggled(IFileDialogCustomize* pfdc, DWORD dwIDCtl, BOOL bChecked) {
    if (dwIDCtl == 2000) {
        if (state->orderBySelected) {
            state->orderBySelected->checked = bChecked;
            state->orderBySelected->onToggle(bChecked);
        }
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

auto pBrowserWindow::fileVista(bool save, bool multi) -> std::vector<std::string> {
    auto& state = browserWindow.state;
    std::string name = "";
    std::vector<std::string> out;
    HRESULT hr;

    if (save)
        multi = false;

    COMDLG_FILTERSPEC* aFileTypes = new COMDLG_FILTERSPEC[state.filters.size()];

    unsigned i = 0;
    for(auto& filter : state.filters) {
        std::vector<std::string> tokens = String::split(filter, '(');

        if(tokens.size() != 2)
            continue;
        
        std::string part1 = tokens[0];
        
        std::string part2 = tokens[1];
        part2.pop_back();        
        String::delSpaces(part2);
        
        std::replace( part2.begin(), part2.end(), ',', ';');
        
        utf16_t* u1 = new utf16_t(part1);
        utf16_t* u2 = new utf16_t(part2);

        aFileTypes[i] = {*u1, *u2};
        i++;
    }   
    
    if (save)
        hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, 
            IID_IFileSaveDialog, reinterpret_cast<void**>(&pDlg));
    else
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, 
            IID_IFileOpenDialog, reinterpret_cast<void**>(&pDlg));
    
    if ( FAILED(hr) )
        return {""};
    
    if (!state.textOk.empty())
        pDlg->SetOkButtonLabel( (LPCWSTR)utf16_t(state.textOk) );    
    
    pDialogEventHandler = new FileDialogEventHandler;    
    pDialogEventHandler->state = &state;
    pDialogEventHandler->browserWindow = &browserWindow;
    pDialogEventHandler->pDlg = pDlg;
    
    pDlg->SetFileTypes ( state.filters.size(), aFileTypes );
    
    for (i = 0; i < state.filters.size(); i++) {
        delete aFileTypes[i].pszName;
        delete aFileTypes[i].pszSpec;        
    }

    delete[] aFileTypes;

    utf16_t wtitle(state.title.c_str());
    
    std::string path = state.path;
    std::replace( path.begin(), path.end(), '/', '\\');
    
    utf16_t wpath(path.c_str());
    
    IShellItem* location;
    SHCreateItemFromParsingName(wpath, nullptr, IID_IShellItem, reinterpret_cast<void **>(&location));
    
    pDlg->SetTitle ( wtitle );
    pDlg->SetFolder( location );

    if (multi) {
        DWORD dwFlags;
        pDlg->GetOptions(&dwFlags);
        pDlg->SetOptions(dwFlags | FOS_ALLOWMULTISELECT);
    }

    IFileDialogCustomize* pDlgc = nullptr;
    if (state.buttons.size() > 0) {
        hr = pDlg->QueryInterface(IID_IFileDialogCustomize, reinterpret_cast<void**>(&pDlgc) );

        if ( SUCCEEDED(hr) ) {
            unsigned i = 0;
            for(auto& button : state.buttons) {                                
                pDlgc->AddPushButton(1000 + i++, utf16_t(button.text) );
            }
        }    
    }

    if (state.orderBySelected) {
        if (!pDlgc)
            hr = pDlg->QueryInterface(IID_IFileDialogCustomize, reinterpret_cast<void**>(&pDlgc) );

        if ( SUCCEEDED(hr) ) {
            pDlgc->AddCheckButton(2000, utf16_t(state.orderBySelected->text), state.orderBySelected->checked );
        }
    }

    selectedFiles.clear();
    pDlg->Advise(pDialogEventHandler, &cookie);

    if (dummyParent)
        SetWindowLongPtr(dummyParent, GWLP_USERDATA, (LONG_PTR)this);
        
    hr = pDlg->Show ( (state.window && state.modal) ? state.window->p.hwnd : dummyParent );
    
    if(!pDlg || !SUCCEEDED(hr))
        return {""};
    
    pDlg->Unadvise(cookie);  

    if (!multi) {
        IShellItem* pItem = nullptr;

        hr = pDlg->GetResult( &pItem );

        if ( SUCCEEDED(hr) ) {
            LPOLESTR pwsz = NULL;

            hr = pItem->GetDisplayName ( SIGDN_FILESYSPATH, &pwsz );

            if ( SUCCEEDED(hr) ) {
                name = utf8_t(pwsz);
                std::replace( name.begin(), name.end(), '\\', '/');
                CoTaskMemFree ( pwsz );
            }
        }
        if(pItem) pItem->Release();
        out.push_back(name);

    } else {
        IShellItemArray* pItems = nullptr;
        hr = reinterpret_cast<IFileOpenDialog*>(pDlg)->GetResults( &pItems );

        if ( SUCCEEDED(hr) ) {
            DWORD elements;
            pItems->GetCount(&elements);

            for (unsigned i = 0; i < elements; i++) {
                IShellItem* pItem;

                pItems->GetItemAt(i, &pItem);
                LPOLESTR pwsz = NULL;

                if (pItem) {
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pwsz);

                    if (SUCCEEDED(hr)) {
                        name = utf8_t(pwsz);
                        std::replace(name.begin(), name.end(), '\\', '/');
                        CoTaskMemFree(pwsz);
                        out.push_back(name);
                    }
                }
            }

            if (state.orderBySelected && state.orderBySelected->checked && selectedFiles.size()) {
                std::vector<std::string> temp;

                for (auto& sSortedFile : selectedFiles) {
                    for(auto& sFile : out ) {
                        if (GUIKIT::String::findString(sFile, sSortedFile))
                            temp.push_back( sFile );
                    }
                }
                out = temp;
            }

        }
        if(pItems) pItems->Release();
    }

    if (pDialogEventHandler)
        delete pDialogEventHandler;
    pDialogEventHandler = nullptr;
    
    pDlg->Release();
    pDlg = nullptr;
    dialogHwnd = nullptr;
    
    return out;
}

// legacy mode
auto pBrowserWindow::fileMulti() -> std::vector<std::string> {
    std::vector<std::string> out;
    auto& state = browserWindow.state;

    if (pApplication::version >= WindowsVista)
        return fileVista(false, true);

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
    wchar_t wname[2048] = L"";

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

    OPENFILENAME ofn;
    memset(&ofn, 0, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = state.window->p.hwnd;
    ofn.lpstrFilter = wfilters;
    ofn.lpstrInitialDir = wpath;
    ofn.lpstrFile = wname;
    ofn.lpstrTitle = wtitle;
    ofn.nMaxFile = PATH_MAX;
    ofn.hInstance = GetModuleHandle(0);
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ENABLESIZING | OFN_ALLOWMULTISELECT;
    ofn.lpstrDefExt = L"";

    bool result = GetOpenFileName(&ofn);
    if(!result) return out;

    std::string append;
    p = wname;
    std::wstring pathList = p;
    p += lstrlenW(p) + 1;

    if ( *p == 0 ) {
        append = utf8_t( pathList.c_str() );
        std::replace( append.begin(), append.end(), '\\', '/');
        out.push_back( append );
    } else {
        for ( ; *p != 0; ) {
            std::wstring fileName = p;
            append = utf8_t( ( pathList + L"\\" + fileName ).c_str() );
            std::replace( append.begin(), append.end(), '\\', '/');
            out.push_back( append );
            p += fileName.size() + 1;
        }
    }

    dialogHwnd = nullptr;
    return out;
}

auto pBrowserWindow::file(bool save) -> std::string {
    
    auto& state = browserWindow.state;
    
    if (!state.contentView.id && (pApplication::version >= WindowsVista))
        return fileVista(save)[0];
    
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
    if(!result)
        return "";

    std::string name = utf8_t(wname);
    std::replace( name.begin(), name.end(), '\\', '/');
    dialogHwnd = nullptr;

    if (selectedButton) {
        auto _selB = selectedButton;
        selectedButton = nullptr;

        if (_selB->onClick( name, (name != selectedPath) ? 0 : contentViewSelection() ) )
            return "";
    }

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

auto pBrowserWindow::getSelectedPath(HWND dlg) -> std::string {
    std::string out = "";
    TCHAR wFilePath[_MAX_PATH] = { 0 };
    HWND hTrueDlg = GetParent(hDlg);

    if (SendMessage(hTrueDlg, CDM_GETFILEPATH, _MAX_PATH, (LPARAM)wFilePath) >= 0) {
        if (!(GetFileAttributes(wFilePath) & FILE_ATTRIBUTE_DIRECTORY)) {
            out = utf8_t(wFilePath);
            std::replace(out.begin(), out.end(), '\\', '/');
        }
    }

    return out;
}

auto CALLBACK pBrowserWindow::OfnHookProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) -> UINT_PTR {
    
    if (Application::isQuit)
        return false;

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
    
    if (!state)
        return false;

    context->hDlg = hDlg;
    HWND listBox = GetDlgItem(hDlg, state->contentView.id);
    
    static HFONT okFont = nullptr;    
    
    switch (uMsg) {
        case WM_INITDIALOG: {
            
            if (okFont) {
                pFont::free( okFont );
                okFont = nullptr;
            }
            
            context->inited = false;
            context->dialogHwnd = GetParent(hDlg);
            
            if (!state->textOk.empty()) {
                
                HWND hwndOk = GetDlgItem(context->dialogHwnd, IDOK);

                if (state->hideOkButton)
                    ShowWindow(hwndOk, SW_HIDE);
                else {
                    okFont = pFont::create(Font::system());

                    auto size = pFont::size(okFont, state->textOk);

                    RECT rect;

                    GetWindowRect(hwndOk, &rect);

                    if ((rect.right - rect.left) < size.width) {
                        pFont::free(okFont);

                        okFont = pFont::create(Font::system(7));

                        SendMessage(hwndOk, WM_SETFONT, (WPARAM) okFont, 0);

                        MoveWindow(hwndOk, 445, 325, 120, 24, TRUE);
                    }

                    SetDlgItemText(context->dialogHwnd, IDOK, (LPCWSTR) utf16_t(state->textOk));

                    context->setButtonTooltip(hwndOk, state->toolTip);
                }
            }
            
            if (!state->textCancel.empty())
                SetDlgItemText( context->dialogHwnd, IDCANCEL, (LPCWSTR)utf16_t(state->textCancel) );
            
            if (listBox) {
                if (state->contentView.font != "")
                    context->listFont = pFont::create( state->contentView.font );
                else
                    context->listFont = pFont::create( Font::system() );

                SendMessage(listBox, WM_SETFONT, (WPARAM)context->listFont, 0);
                
                auto size = pFont::size(context->listFont, " ");

                context->listItemHeight = size.height;
                
                SendMessage(listBox, LB_SETITEMHEIGHT, 0, size.height);
            }
            if (!context->listBgBrush) {
                auto colorBg = state->contentView.backgroundColor;

                if (state->contentView.overrideBackgroundColor)
                    context->listBgBrush = CreateSolidBrush(RGB((colorBg >> 16) & 0xff, (colorBg >> 8) & 0xff, colorBg & 0xff));
                else
                    context->listBgBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
            }

            if (!context->listHiBrush) {
                auto colorBg = state->contentView.selectionBackgroundColor;

                if (state->contentView.overrideSelectionColor)
                    context->listHiBrush = CreateSolidBrush(RGB((colorBg >> 16) & 0xff, (colorBg >> 8) & 0xff, colorBg & 0xff));
                else
                    context->listHiBrush = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
            }

			std::vector<unsigned> dlgButtonIds = {IDC_BUTTON, IDC_BUTTON1, IDC_BUTTON2};
			
            for(auto& button : state->buttons) {
                SetDlgItemText(hDlg, button.id, (LPCWSTR)utf16_t(button.text) );				
				context->setButtonTooltip(GetDlgItem(hDlg, button.id), button.toolTip);
				Vector::eraseVectorElement(dlgButtonIds, button.id);
            }
			
			for (auto dlgButtonId : dlgButtonIds) {
				ShowWindow(GetDlgItem(hDlg, dlgButtonId), SW_HIDE);
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

            if (state->contentView.overrideBackgroundColor) {
                return (LRESULT) context->listBgBrush;
            }
            break;
        }

        case WM_MEASUREITEM: {
            LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT)lParam;
            if ( (lpmis != NULL) && (lpmis->CtlType == ODT_LISTBOX) ) {
                lpmis->itemHeight = context->listItemHeight;
                return TRUE;
            }
            break;
        }

        case WM_DRAWITEM: {

            LPDRAWITEMSTRUCT lDraw = (LPDRAWITEMSTRUCT) lParam;

            if ((lDraw != NULL) && (lDraw->CtlType == ODT_LISTBOX)) {

                HBRUSH hBrush;
                COLORREF colorRef;
                BrowserWindow::State* state = &context->browserWindow.state;

                if (lDraw->itemID == -1) {
                    return TRUE;
                }

                switch (lDraw->itemAction) {
                    case ODA_SELECT:
                    case ODA_DRAWENTIRE: {

                        if (lDraw->itemState & ODS_SELECTED) {
                            if (state->contentView.overrideSelectionColor) {
                                auto colorFg = state->contentView.selectionForegroundColor;
                                colorRef = RGB((colorFg >> 16) & 0xff, (colorFg >> 8) & 0xff, colorFg & 0xff);
                            } else
                                colorRef = GetSysColor( COLOR_HIGHLIGHTTEXT );

                            hBrush = context->listHiBrush;

                        } else {
                            hBrush = context->listBgBrush;

                            if (state->contentView.overrideForegroundColor) {
                                auto colorFg = state->contentView.foregroundColor;
                                colorRef = RGB((colorFg >> 16) & 0xff, (colorFg >> 8) & 0xff, colorFg & 0xff);
                            } else
                                colorRef = GetSysColor( COLOR_WINDOWTEXT );
                        }

                        auto lRow = lDraw->rcItem;

                        FillRect(lDraw->hDC, &lRow, hBrush);

                        WCHAR lBuf[100];

                        int len = SendMessage(lDraw->hwndItem, LB_GETTEXTLEN, (WPARAM)lDraw->itemID, 0);

                        SendMessage(lDraw->hwndItem, LB_GETTEXT, (WPARAM)lDraw->itemID, (LPARAM)lBuf);

                        SetTextColor(lDraw->hDC, colorRef);

                        SetBkMode(lDraw->hDC, TRANSPARENT);

                        DrawText(lDraw->hDC, lBuf, len, &lRow, DT_LEFT | DT_NOPREFIX);

                    } break;
                }
                return TRUE;
            }
            break;
        }

        case WM_NOTIFY: {
            
            if (((OFNOTIFY*)lParam)->hdr.code == CDN_INITDONE) {
                context->inited = true;
                if (context)                    
                    context->resize(hDlg, true);                    
                
            } else if ( ((OFNOTIFY*)lParam)->hdr.code == CDN_SELCHANGE ) {
                
                if (listBox) {
                    SendMessage( listBox, WM_SETREDRAW, 0, 0);
                    SendMessage( listBox, LB_RESETCONTENT, 0, 0);
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

                                    maximumWidth = std::max<unsigned>(maximumWidth, pFont::size(context->listFont, row.entry).width);
                                    
                                    context->toolTips.push_back( row.tooltip );
                                }                                
                                SendMessage( listBox, LB_SETHORIZONTALEXTENT, maximumWidth + 8, 0);
                                SendMessage( listBox, WM_SETREDRAW, 1, 0);
                               // InvalidateRect(listBox, NULL, TRUE);
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
                        //if (button.onClick( context->selectedPath, context->contentViewSelection() ))
                          //  PostMessage(context->dialogHwnd, WM_COMMAND, IDCANCEL, 0); //end dialog
                        context->selectedButton = &button;
                        PostMessage(context->dialogHwnd, WM_COMMAND, IDOK, 0); //end dialog
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

auto pBrowserWindow::setListings( std::vector<BrowserWindow::Listing>& listings ) -> void {

    if (!browserWindow.state.contentView.id || !hDlg)
        return;

    HWND listBox = GetDlgItem(hDlg, browserWindow.state.contentView.id);

    if (!listBox)
        return;

    SendMessage( listBox, WM_SETREDRAW, 0, 0);
    SendMessage( listBox, LB_RESETCONTENT, 0, 0);
    toolTips.clear();

    unsigned maximumWidth = 0;

    for( auto& row : listings ) {
        SendMessage( listBox, LB_ADDSTRING, 0, (LPARAM)(wchar_t*)utf16_t(row.entry) );

        maximumWidth = std::max<unsigned>(maximumWidth, pFont::size(listFont, row.entry).width);

        toolTips.push_back( row.tooltip );
    }
    SendMessage( listBox, LB_SETHORIZONTALEXTENT, maximumWidth + 8, 0);
    SendMessage( listBox, WM_SETREDRAW, 1, 0);
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

auto pBrowserWindow::setButtonTooltip(HWND buttonHwnd, std::string tooltip) -> void {
    if(tooltip.empty())
        return;
    
	HWND toolTip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL,
		WS_POPUP | TTS_ALWAYSTIP | TTS_USEVISUALSTYLE,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		buttonHwnd, NULL, GetModuleHandle(0), 0);    
            
    utf16_t wtooltip(tooltip);

    TOOLINFO toolInfo = { 0 };
    toolInfo.cbSize = sizeof(toolInfo);
    toolInfo.hwnd = GetParent(buttonHwnd);
    toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    toolInfo.uId = (UINT_PTR)buttonHwnd;
    toolInfo.lpszText = wtooltip;

    SendMessage(toolTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
	SendMessage(toolTip, TTM_SETDELAYTIME, TTDT_INITIAL, 1500);
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
	unsigned buttonMargin = pFont::scale(20);
    
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
		int relativeX = -1;
		unsigned buttonBarWidth = 0;
		
        for (auto& button : browserWindow.state.buttons) {
            HWND hwnd = GetDlgItem(fileDialogView, button.id);
            if (!hwnd)
                continue;
            
            RECT rect;

            GetWindowRect(hwnd, &rect);
            MapWindowPoints(NULL, fileDialogView, (POINT*)&rect, 2);

            int width = std::abs(rect.right - rect.left);
            
            auto size = pFont::size(Font::system(), button.text);
            width = size.width + 10;
			if (!IsAppThemed())
				width += 10;
            
			if (relativeX == -1)
				relativeX = std::abs(rect.left - rCustomView.right);
					
			int height = std::abs(rect.bottom - rect.top);
            int relativeY = std::abs(rect.top - (listBox ? rListBox.bottom : rCustomView.top) );

            buttons.push_back({hwnd, width, height, relativeX, relativeY});
			
			relativeX += width + buttonMargin;
			
			buttonBarWidth += width + buttonMargin;
        }
		
		if (listBox && buttonBarWidth) { // center button Bar
			buttonBarWidth -= buttonMargin;
			if (buttonBarWidth < listWidth) {
				unsigned delta = (listWidth - buttonBarWidth) >> 1;
				for (auto& button : buttons) {
					button.relativeX += delta;
				}			
			}
		}
    }
	
    
    int dialogWidth = std::abs(rDialogView.right - rDialogView.left);
    int dialogHeight = std::abs(rDialogView.bottom - rDialogView.top);    
    int customX = dialogWidth - widthCustomView;

    int buttonTotalHeight = 0;
    
    for (auto& button : buttons) {
        
        buttonTotalHeight = std::max<int>(buttonTotalHeight, button.relativeY + button.height);
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

    if (listHiBrush)
        DeleteObject(listHiBrush);
    
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

    listHiBrush = nullptr;
    
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
