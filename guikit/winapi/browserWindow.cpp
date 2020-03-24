
STDMETHODIMP FileDialogEventHandler::OnFileOk ( IFileDialog* pfd )
{
    return S_OK;    // allow the dialog to close
}

STDMETHODIMP FileDialogEventHandler::OnFolderChanging ( IFileDialog* pfd, IShellItem* psiFolder )
{
    return S_OK;    // allow the change
}

STDMETHODIMP FileDialogEventHandler::OnFolderChange ( IFileDialog* pfd )
{
    return S_OK;
}

STDMETHODIMP FileDialogEventHandler::OnSelectionChange ( IFileDialog* pfd )
{                
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
        
        if (!name.empty() && !path.empty() && path != filePath) {
            if ( state && state->onSelectionChange)
                state->onSelectionChange(path);
            
            filePath = path;
        }
        
        CoTaskMemFree ( pwsz );
    }               

    return S_OK;
}

auto FileDialogEventHandler::getFilePath() -> std::string {
    LPOLESTR pwsz = NULL;
    IShellItem* pItem;
    std::string path = "";
            
    HRESULT hr = pDlg->GetFolder(&pItem);
        
    if ( SUCCEEDED(hr)) {
        
        hr = pItem->GetDisplayName ( SIGDN_FILESYSPATH, &pwsz );

        if ( SUCCEEDED(hr) ) {
            
            path = utf8_t(pwsz);
            std::replace( path.begin(), path.end(), '\\', '/');
            CoTaskMemFree ( pwsz );
        }
    }
    
    hr = pDlg->GetFileName( &pwsz );    
        
    if ( SUCCEEDED(hr)) {
        
        std::string name = utf8_t(pwsz);       

        path += "/" + name;
        
        CoTaskMemFree ( pwsz );
    }    
    
    return path;
}

STDMETHODIMP FileDialogEventHandler::OnShareViolation (
    IFileDialog* pfd, IShellItem* psi, FDE_SHAREVIOLATION_RESPONSE* pResponse )
{
    return S_OK;
}

STDMETHODIMP FileDialogEventHandler::OnTypeChange ( IFileDialog* pfd )
{
    
    return S_OK;
}

STDMETHODIMP FileDialogEventHandler::OnOverwrite (
    IFileDialog* pfd, IShellItem* psi, FDE_OVERWRITE_RESPONSE* pResponse )
{
    return S_OK;
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


STDMETHODIMP FileDialogEventHandler::OnButtonClicked ( IFileDialogCustomize* pfdc, DWORD dwIDCtl ) {
    
    auto id = dwIDCtl - 1000;
    
    if (id < state->buttons.size()) {
        
        auto button = state->buttons[id];
        
        if (button.onClick)
            if ( button.onClick( getFilePath() ) )
                pDlg->Close( S_OK );
    }
    
    return S_OK;
}


static auto CALLBACK BrowserWindowCallbackProc(HWND hwnd, UINT msg, LPARAM lparam, LPARAM lpdata) -> int {
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
                //pDlgc->MakeProminent(1000 + i++);
            }
        }    
    }    
    
    pDlg->Advise(pDialogEventHandler, &cookie);  

    if (!state.window) {        
        dummyParent = CreateWindow(L"fileDialogDummy", L"fileDialogDummy", WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, nullptr, nullptr, nullptr, nullptr);

        SetWindowLongPtr(dummyParent, GWLP_USERDATA, (LONG_PTR)this);
    }
        
    hr = pDlg->Show ( state.window ? state.window->p.hwnd : dummyParent );
    pDlg->Unadvise(cookie);    
    
    if ( SUCCEEDED(hr) ) {
        
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
    }
    
    delete pDialogEventHandler;
    pDialogEventHandler = nullptr;
    
    pDlg->Release();
    pDlg = nullptr;
    
    return name;
}

auto pBrowserWindow::file(bool save) -> std::string {
    
    auto& state = browserWindow.state;
    
    if (pApplication::version >= WindowsVista) 
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

    OPENFILENAME ofn;
    memset(&ofn, 0, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = state.window ? state.window->p.hwnd : 0;
    ofn.lpstrFilter = wfilters;
    ofn.lpstrInitialDir = wpath;
    ofn.lpstrFile = wname;
    ofn.lpstrTitle = wtitle;
    ofn.nMaxFile = PATH_MAX;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = L"";

    bool result = !save ? GetOpenFileName(&ofn) : GetSaveFileName(&ofn);
    if(!result) return "";
    std::string name = utf8_t(wname);
    std::replace( name.begin(), name.end(), '\\', '/');
    return name;
}

auto pBrowserWindow::directory() -> std::string {
    auto& state = browserWindow.state;
    
    pApplication::currentWorkingDirectory();
    wchar_t wname[PATH_MAX + 1] = L"";
    utf16_t wtitle( browserWindow.state.title );

    BROWSEINFO bi;
    bi.hwndOwner = state.window ? state.window->p.hwnd : 0;
    bi.pidlRoot = NULL;
    bi.pszDisplayName = wname;
    bi.lpszTitle = wtitle;
    bi.ulFlags = BIF_NEWDIALOGSTYLE | BIF_RETURNONLYFSDIRS;
    bi.lpfn = BrowserWindowCallbackProc;
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

auto pBrowserWindow::getHwnd() -> HWND {
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

auto pBrowserWindow::setForeground() -> void {
    auto hwnd = getHwnd();
    
    if (hwnd)
        SetForegroundWindow( hwnd );
}


auto pBrowserWindow::close() -> void {    
    // close from external
    auto hwnd = getHwnd();
    
    if (hwnd)
        PostMessage(hwnd, WM_COMMAND, IDCANCEL, 0); //end dialog
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
        
        initialized = true;
    }
}

pBrowserWindow::~pBrowserWindow() {
    
    if (pDlg) {
        pDlg->Release();
    }
    
    if (pDialogEventHandler) {
        delete pDialogEventHandler;
    }
    
    pDialogEventHandler = nullptr;
    
    pDlg = nullptr;
    
    if (dummyParent)
        DestroyWindow(dummyParent);
}