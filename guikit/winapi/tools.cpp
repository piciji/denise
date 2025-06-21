
//timer
std::vector<pTimer*> pTimer::timers;
bool pSystem::offscreen = true;
std::vector<RECT> pSystem::clientRects;

auto CALLBACK pTimer::timeoutProc(HWND hwnd, UINT msg, UINT_PTR timerID, DWORD time) -> void {
    for(auto& instance : timers) {
        if(instance->htimer == timerID) {
            if(instance->timer.onFinished) instance->timer.onFinished();
            return;
        }
    }
}

auto pTimer::setEnabled(bool enabled) -> void {
    killTimer();
    if(!enabled) return;
    htimer = SetTimer(NULL, 0u, timer.state.interval, timeoutProc);
}

auto pTimer::setInterval(unsigned interval) -> void {
    setEnabled(timer.enabled());
}

auto pTimer::killTimer() -> void {
    if(htimer) KillTimer(NULL, htimer);
    htimer = 0;
}

pTimer::pTimer(Timer& timer) : timer(timer) {
    timers.push_back(this);
    htimer = 0;
}

pTimer::~pTimer() {
    killTimer();
    Vector::eraseVectorElement<pTimer*>(timers, this);
}
//font
auto pFont::system(unsigned size, std::string style, bool monospaced) -> std::string {
    static float dpiX = dpi().x;
    NONCLIENTMETRICS metrics;
    metrics.cbSize = sizeof(NONCLIENTMETRICS);
    ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &metrics, 0);

    std::string family = utf8_t(metrics.lfMessageFont.lfFaceName);
    
    if (monospaced)
        family = "Lucida Console";

    if (size == 0) {
		size = float(std::abs(metrics.lfMessageFont.lfHeight)) * 72.0 / dpiX;
    }

    if(style == "") style = "Normal";

    return family + ", " + std::to_string(size) + ", " + style;
}

auto pFont::systemFontFile() -> std::string {
    NONCLIENTMETRICS metrics;
    metrics.cbSize = sizeof(NONCLIENTMETRICS);
    ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &metrics, 0);
    std::wstring wsFaceName(metrics.lfMessageFont.lfFaceName);

    HKEY hKey;
    LONG result;

    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts", 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) {
        return "";
    }

    DWORD maxValueNameSize, maxValueDataSize;
    result = RegQueryInfoKey(hKey, 0, 0, 0, 0, 0, 0, 0, &maxValueNameSize, &maxValueDataSize, 0, 0);
    if (result != ERROR_SUCCESS) {
        return "";
    }

    DWORD valueIndex = 0;
    LPWSTR valueName = new WCHAR[maxValueNameSize];
    LPBYTE valueData = new BYTE[maxValueDataSize];
    DWORD valueNameSize, valueDataSize, valueType;
    std::wstring wsFontFile;

    do {
        wsFontFile.clear();
        valueDataSize = maxValueDataSize;
        valueNameSize = maxValueNameSize;

        result = RegEnumValue(hKey, valueIndex, valueName, &valueNameSize, 0, &valueType, valueData, &valueDataSize);

        valueIndex++;

        if (result != ERROR_SUCCESS || valueType != REG_SZ) {
            continue;
        }

        std::wstring wsValueName(valueName, valueNameSize);

        if (_wcsnicmp(wsFaceName.c_str(), wsValueName.c_str(), wsFaceName.length()) == 0) {

            wsFontFile.assign((LPWSTR)valueData, valueDataSize);
            break;
        }
    }
    while (result != ERROR_NO_MORE_ITEMS);

    delete[] valueName;
    delete[] valueData;

    RegCloseKey(hKey);

    if (wsFontFile.empty()) {
        return "";
    }

    WCHAR winDir[MAX_PATH];
    GetWindowsDirectory(winDir, MAX_PATH);

    std::wstringstream ss;
    ss << winDir << "\\Fonts\\" << wsFontFile;
    wsFontFile = ss.str();

    std::string str(wsFontFile.length(), 0);
    std::transform(wsFontFile.begin(), wsFontFile.end(), str.begin(), [](wchar_t c) {
        return (char)c;
    });

    return str;
}

auto pFont::add(CustomFont& customFont) -> bool {
    DWORD nFonts;
    File file(customFont.filePath);
    bool success = false;

    if (file.open()) {
        if (AddFontMemResourceEx( file.read(), file.getSize(), NULL, &nFonts ))
            success = true;
        file.unload();
    }

    return success;
    /* The following function should only be used for fonts that are not distributed by the APP, as the font will be locked by Windows.
     * The font file cannot be overwritten or deleted for this session, and cannot even be transferred to GIT.
     * The font can be unregistered when the APP is terminated, but in case of a crash, the font is locked.
     * I would only use this function with fonts that are installed directly in the Windows Fonts folder.
     */

    // return AddFontResourceW( utf16_t(customFont.filePath.c_str()) );
}

auto pFont::create(const std::string& desc) -> HFONT {
    static float dpiX = dpi().x;
    std::vector<std::string> tokens = String::split(desc, ',');

    std::string family = "Default";
    unsigned size = 8u;
    bool bold = false, italic = false;

    if(tokens.at(0) != "") family = tokens.at(0);
    if(tokens.size() >= 2  && String::isNumber(tokens.at(1))) size = std::stoi(tokens.at(1));

    if(tokens.size() >= 3) {
        for(unsigned i = 2; i < tokens.size(); i++) {
            std::string style = String::toLowerCase( tokens.at( i ) );
            bold |= String::foundSubStr(style, "bold");
            italic |= String::foundSubStr(style, "italic");
        }
    }

    std::string _desc = desc;
    String::toLowerCase(_desc);

    if (!bold) {
        bold |= String::foundSubStr(_desc, "bold");
    }

    if (!italic) {
        italic |= String::foundSubStr(_desc, "italic");
    }
	
    HFONT _hfont = CreateFont(
        -((float)size * dpiX / 72.0 + 0.5),
        0, 0, 0, !bold ? FW_DONTCARE : FW_BOLD, italic,
        0, 0, 0, 0, 0, 0, 0, utf16_t(family) );
		
	return _hfont;	
}

auto pFont::dpi() -> Position {
    HDC hdc = GetDC(0);
    auto dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
    auto dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(0, hdc);
    return {dpiX, dpiY};
}

auto pFont::free(HFONT& hfont) -> void {
	
    if(hfont)
		DeleteObject(hfont);
    hfont = 0;
}

auto pFont::size(const std::string& font, const std::string& text) -> Size {
    HFONT hfont = pFont::create(font);
    Size size = pFont::size(hfont, text);
    free(hfont);
    return size;
}

auto pFont::size(HFONT hfont, std::string text) -> Size {
    if(text.empty()) text = " ";
    HDC hdc = GetDC(0);
    SelectObject(hdc, hfont);
    RECT rc = {0, 0, 0, 0};
    DrawText(hdc, utf16_t(text.c_str()), -1, &rc, DT_CALCRECT);
    ReleaseDC(0, hdc);
    return {(unsigned)rc.right, (unsigned)rc.bottom};
}

auto pFont::scale( unsigned pixel ) -> unsigned {
    static float dpiX = dpi().x;

    return (float)pixel * dpiX / 96.0 + 0.5;
}

//UTF-8 <> UTF-16 string converter
utf16_t::utf16_t(const std::string& str, unsigned CodePage) {
    unsigned length = MultiByteToWideChar(CodePage, 0, str.c_str(), -1, nullptr, 0);
    buffer = new wchar_t[length + 1]();
    MultiByteToWideChar(CodePage, 0, str.c_str(), -1, buffer, length);
}
utf8_t::utf8_t(const wchar_t* s) {
    if(!s) s = L"";
    unsigned length = WideCharToMultiByte(CP_UTF8, 0, s, -1, nullptr, 0, nullptr, nullptr);
    buffer = new char[length + 1]();
    WideCharToMultiByte(CP_UTF8, 0, s, -1, buffer, length, nullptr, nullptr);
}

//bitmap
auto CreateBitmap(Image& image, bool structureOnly ) -> HBITMAP {
    if (image.format == Image::Format::RGBA)
		image.switchBetweenBGRandRGB();
	
    HDC hdc = GetDC(0);
    BITMAPINFO bitmapInfo;
    memset(&bitmapInfo, 0, sizeof(BITMAPINFO));
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = image.width;
    bitmapInfo.bmiHeader.biHeight = -((long)image.height);
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    bitmapInfo.bmiHeader.biSizeImage = image.width * image.height * 4;
    void* bits = nullptr;
    HBITMAP hbitmap = CreateDIBSection(hdc, &bitmapInfo, DIB_RGB_COLORS, &bits, NULL, 0);
    if(bits && !structureOnly)
        memcpy(bits, image.data, image.width * image.height * 4);
    ReleaseDC(0, hdc);
    return hbitmap;
}

auto CreateBitmapWithPremultipliedAlpha(Image& image) -> HBITMAP {
    // coudn't load dll
    if ( !pApplication::pfnBeginBufferedPaint || !pApplication::pfnEndBufferedPaint )
        return NULL;
    
    HICON hicon = CreateHIcon( image );
    
    if ( !hicon )
        return NULL;
        
    RECT rcIcon;
    SetRect(&rcIcon, 0, 0, image.width, image.height);     
    HBITMAP hbitmap = NULL;
    bool ok = false;
    
    HDC hdcDest = CreateCompatibleDC(NULL);
    
    if (hdcDest) {

        hbitmap = CreateBitmap( image, true );
        
        HBITMAP hbmpOld = (HBITMAP)SelectObject(hdcDest, hbitmap);

        if (hbmpOld) {
            BLENDFUNCTION bfAlpha = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
            BP_PAINTPARAMS paintParams = {0};
            paintParams.cbSize = sizeof(paintParams);
            paintParams.dwFlags = BPPF_ERASE;
            paintParams.pBlendFunction = &bfAlpha;

            HDC hdcBuffer;
            HPAINTBUFFER hPaintBuffer = pApplication::pfnBeginBufferedPaint(hdcDest, &rcIcon, BPBF_DIB, &paintParams, &hdcBuffer);
            
            if (hPaintBuffer) {
                DrawIconEx(hdcBuffer, 0, 0, hicon, image.width, image.height, 0, NULL, DI_NORMAL);
                pApplication::pfnEndBufferedPaint(hPaintBuffer, TRUE);
                ok = true;
            }

            SelectObject(hdcDest, hbmpOld);
        }
        
        DeleteDC(hdcDest);
    }

    DestroyIcon(hicon);
    
    if (!ok && hbitmap)
        DeleteObject(hbitmap);
    
    return ok ? hbitmap : NULL;
}

auto CreateHIconWithAlphaBlend(Image& image, unsigned color) -> HICON {
    if (image.format == Image::Format::RGBA)
        image.switchBetweenBGRandRGB();

    if (!image.alphaBlendApplied)
        image.alphaBlend( color );

    HICON hicon = CreateIcon(0, image.width, image.height, 1, 32, 0, image.data);

    return hicon;
}

auto CreateHIcon(Image& image) -> HICON {
	if (image.format == Image::Format::RGBA)
		image.switchBetweenBGRandRGB();

	HICON hicon = CreateIcon(0, image.width, image.height, 1, 32, 0, image.data);

	return hicon;
}

// cursor
auto CreateHCursor( HBITMAP hBitmap, unsigned hotSpotX, unsigned hotSpotY ) -> HCURSOR {
    
	HDC hDC					= ::GetDC(NULL);
	HDC hMainDC				= ::CreateCompatibleDC(hDC); 
	HDC hAndMaskDC			= ::CreateCompatibleDC(hDC); 
	HDC hXorMaskDC			= ::CreateCompatibleDC(hDC); 

	BITMAP bm;
	::GetObject( hBitmap, sizeof(BITMAP),&bm );
	
	HBITMAP hAndMaskBitmap	= ::CreateCompatibleBitmap( hDC,bm.bmWidth,bm.bmHeight );
	HBITMAP hXorMaskBitmap	= ::CreateCompatibleBitmap( hDC,bm.bmWidth,bm.bmHeight );

	HBITMAP hOldMainBitmap = (HBITMAP)::SelectObject(hMainDC,hBitmap);
	HBITMAP hOldAndMaskBitmap = (HBITMAP)::SelectObject(hAndMaskDC,hAndMaskBitmap);
	HBITMAP hOldXorMaskBitmap = (HBITMAP)::SelectObject(hXorMaskDC,hXorMaskBitmap);

	COLORREF MainBitPixel;
    
	for(int x=0; x < bm.bmWidth; ++x) {
		for(int y=0; y < bm.bmHeight; ++y) {
			MainBitPixel = ::GetPixel(hMainDC,x,y);
            
			if(MainBitPixel == RGB(0,0,0)) {
				::SetPixel(hAndMaskDC,x,y,RGB(255,255,255));
				::SetPixel(hXorMaskDC,x,y,RGB(0,0,0));
                
			} else {
                
				::SetPixel(hAndMaskDC,x,y,RGB(0,0,0));
				::SetPixel(hXorMaskDC,x,y,MainBitPixel);
			}
		}
	}
	
	::SelectObject(hMainDC,hOldMainBitmap);
	::SelectObject(hAndMaskDC,hOldAndMaskBitmap);
	::SelectObject(hXorMaskDC,hOldXorMaskBitmap);

	::DeleteDC(hXorMaskDC);
	::DeleteDC(hAndMaskDC);
	::DeleteDC(hMainDC);

	::ReleaseDC(NULL,hDC);
    
    ICONINFO iconinfo = {0};
    iconinfo.fIcon		= FALSE;
    iconinfo.xHotspot	= hotSpotX;
    iconinfo.yHotspot	= hotSpotY;
    iconinfo.hbmMask	= hAndMaskBitmap;
    iconinfo.hbmColor	= hXorMaskBitmap;

    return ::CreateIconIndirect( &iconinfo );
}

// drag'n'drop
auto getDropPaths(WPARAM wparam) -> std::vector<std::string> {
    std::vector<std::string> paths;
    auto dropList = HDROP(wparam);
    auto fileCount = DragQueryFile(dropList, ~0u, nullptr, 0);

    for(unsigned n = 0; n < fileCount; n++) {
        auto length = DragQueryFile(dropList, n, nullptr, 0);
        auto buffer = new wchar_t[length + 1];

        if(DragQueryFile(dropList, n, buffer, length + 1)) {
            std::string path = utf8_t(buffer);
            std::replace( path.begin(), path.end(), '\\', '/');

            if (File::isDir(path) && path.back() != '/') path.push_back('/');
            paths.push_back(path);
        }

        delete[] buffer;
    }
    return paths;
}

//system
auto CALLBACK MonitorEnumProc2(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) -> BOOL {
    MONITORINFO monitorInfo;
    monitorInfo.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo( hMonitor, &monitorInfo );
    RECT clientRect = monitorInfo.rcMonitor;
    pSystem::clientRects.push_back(clientRect);
    return TRUE;
}

auto CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) -> BOOL {
    MONITORINFO monitorInfo;
    
    monitorInfo.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo( hMonitor, &monitorInfo );
    
    Geometry* geometry = (Geometry*)dwData;
    
    RECT clientRect = monitorInfo.rcMonitor;

    if (pSystem::clientRects.size() < 4)
        pSystem::clientRects.push_back(clientRect);
    
    int xOff = geometry->width / 2;
    int yOff = geometry->height / 2;
    
    if ( (geometry->x + xOff) < clientRect.left )    
        return TRUE;
    
    if ( (geometry->x + xOff) > clientRect.right )    
        return TRUE;
    
    if ( (geometry->y + yOff) < clientRect.top ) 
        return TRUE;
    
    if ( (geometry->y + yOff) > clientRect.bottom ) 
        return TRUE;
    
    pSystem::offscreen = false;
    
    return FALSE;
}

auto pSystem::getUserDataFolder() -> std::string {
    std::string out;
    wchar_t path[PATH_MAX] = L"";
    SHGetFolderPathW(nullptr, CSIDL_APPDATA | CSIDL_FLAG_CREATE, nullptr, 0, path);
    out = utf8_t(path);
    return File::beautifyPath(out);
}

auto pSystem::getResourceFolder(std::string appIdent) -> std::string {
    return getExecutableDirectory();
}

auto pSystem::getWorkingDirectory() -> std::string {
    return pApplication::currentWorkingDirectory();
}

auto pSystem::getExecutableDirectory() -> std::string {
    std::string out;
    wchar_t path[MAX_PATH] = { 0 };
    GetModuleFileNameW(NULL, path, MAX_PATH);
    out = utf8_t(path);
    std::replace( out.begin(), out.end(), '\\', '/');
    out = File::getPath(out);
    return out;
}

auto pSystem::getDesktopSize() -> Size {
    Size out = { 0,0 };

    if (!clientRects.size())
        EnumDisplayMonitors(NULL, NULL, MonitorEnumProc2, 0 );

    for (auto& clientRect : clientRects) {
        if (clientRect.right > out.width)
            out.width = clientRect.right;

        if (clientRect.bottom > out.height)
            out.height = clientRect.bottom;
    }

    if (!out.width)
        out.width = (unsigned)GetSystemMetrics(SM_CXSCREEN);

    if (!out.height)
        out.height = (unsigned)GetSystemMetrics(SM_CYSCREEN);

    return out;
}

auto pSystem::sleep(unsigned milliSeconds) -> void {
    Sleep( milliSeconds );
}

auto pSystem::isOffscreen( Geometry geometry ) -> bool {
    offscreen = true;
    
    if (!EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)&geometry ))
        return false;
    
    return offscreen;
}

auto pSystem::getOSLang() -> System::Language {
    
    auto langID = GetUserDefaultUILanguage();
    
    if (langID == 0x0C07 || langID == 0x0407 || langID == 0x1407 || langID == 0x0807 || langID == 0x1007)
        return System::Language::DE;
    
    if (langID == 0x0409 || langID == 0x0475 || langID == 0x540A)
        return System::Language::US;
    
    if (langID == 0x080c || langID == 0x0C0C || langID == 0x040c || langID == 0x140C || langID == 0x180C || langID == 0x100C)
        return System::Language::FR;

    return System::Language::UK;
}

auto pSystem::printToCmd( std::string str ) -> void {
    
    static bool isAttached = false;

    if (!isAttached) {
        HWND consoleWnd = GetConsoleWindow();

        if (!consoleWnd) {
            if (AttachConsole(ATTACH_PARENT_PROCESS)) {
                freopen("CON", "w", stdout);
                isAttached = true;
            }
        } else
            isAttached = true;
    }
    
    if (!isAttached)
        return;
    
    fwprintf(stdout, utf16_t( str ) );
}



typedef LONG NTSTATUS, *PNTSTATUS;
typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

static auto getVersionNew() -> unsigned {
    static unsigned version = 0;

    if (version)
        return version;

    HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
    if (hMod) {
        RtlGetVersionPtr fxPtr = (RtlGetVersionPtr)::GetProcAddress(hMod, "RtlGetVersion");
        if (fxPtr != nullptr) {
            RTL_OSVERSIONINFOW rovi = {0};
            rovi.dwOSVersionInfoSize = sizeof (rovi);
            if (0x00000000 == fxPtr(&rovi)) {
                FreeLibrary(hMod);
                version = (rovi.dwMajorVersion << 8) | rovi.dwMinorVersion;
                if ((version >= 0xa00) && (rovi.dwBuildNumber >= 22000))
                    version += 1; // Win 11
                return version;
            }
        }
    }

    if (hMod)
        FreeLibrary(hMod);

    OSVERSIONINFO versionInfo{0};
    versionInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
    GetVersionEx(&versionInfo);

    version = (versionInfo.dwMajorVersion << 8) | versionInfo.dwMinorVersion;
    return version;
}

auto pThreadPriority::setPriority( ThreadPriority::Mode mode, float minProcessingTimeInMilliSeconds, float maxProcessingTimeInMilliSeconds ) -> bool {

    int prio = 0;
    switch(mode) {
        default:
        case ThreadPriority::Mode::Normal:
            prio = THREAD_PRIORITY_NORMAL;
            break;
        case ThreadPriority::Mode::High:
            prio = THREAD_PRIORITY_HIGHEST;
            break;
        case ThreadPriority::Mode::Realtime:
            prio = THREAD_PRIORITY_TIME_CRITICAL;
            break;
    }

    return SetThreadPriority(GetCurrentThread(), prio) != 0;
}

inline auto getDim( RECT& rect ) -> Size {

    Size size;
    size.width = rect.right - rect.left;
    size.height = rect.bottom - rect.top;
    
    return size;
}

inline auto getHeight( RECT& rect ) -> int {
    return rect.bottom - rect.top;
}

inline auto getWidth( RECT& rect ) -> int {
    return rect.right - rect.left;
}

