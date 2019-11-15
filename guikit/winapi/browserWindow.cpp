
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

auto pBrowserWindow::file(BrowserWindow::State& state, bool save) -> std::string {
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

auto pBrowserWindow::directory(BrowserWindow::State& state) -> std::string {
    pApplication::currentWorkingDirectory();
    wchar_t wname[PATH_MAX + 1] = L"";
    utf16_t wtitle( state.title );

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
