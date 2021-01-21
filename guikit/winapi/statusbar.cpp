
pStatusBar::pStatusBar(StatusBar& statusBar) : statusBar(statusBar) {
    
    hwnd = nullptr;    
    hfont = nullptr;
    hCursor = LoadCursor(0, IDC_HAND); 
    hoverPart = nullptr;
    hwndTip = nullptr;
}

pStatusBar::~pStatusBar() {
    destroy();
    pFont::free(hfont);
    if (hCursor)
        DestroyCursor( hCursor );
}

auto pStatusBar::create() -> void {    
    
    hwnd = CreateWindow(STATUSCLASSNAME, L"", WS_CHILD, 0, 0, 0, 0, statusBar.window()->p.hwnd, (HMENU)(unsigned long long)statusBar.id, GetModuleHandle(0), 0);
	
	SendMessage( hwnd, SB_SETBKCOLOR, 0, GetSysColor(COLOR_MENU));
    
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&statusBar);
    
    wndprocOrig = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)subclassWndProc);
    
	hwndTip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL,
		WS_POPUP | TTS_ALWAYSTIP | TTS_USEVISUALSTYLE,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		hwnd, NULL, GetModuleHandle(0), 0);
    
    hoverPart = nullptr;
    
    unsigned partCount = statusBar.state.parts.size();
    
    if (hfont)
        SendMessage(hwnd, WM_SETFONT, (WPARAM)hfont, 0);
    
    if (partCount == 0)
        setText( statusBar.text() );
    else
        update();
}

auto CALLBACK pStatusBar::subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    StatusBar* statusBar = (StatusBar*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(statusBar == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);

    switch(msg) {   
        case WM_MOUSEMOVE: {            
            auto& p = statusBar->p;
            
            StatusBar::Part* part = p.getHoverPart( (int)(short) LOWORD(lparam) );

            if (part && (part->popupMenu || part->onClick))
                SetCursor(statusBar->p.hCursor);
            
			if (part != p.hoverPart) {
				p.hoverPart = part;

				p.setTooltip( part );

                return 0;
			}

        } break;
            
    }
    return CallWindowProc(statusBar->p.wndprocOrig, hwnd, msg, wparam, lparam);
}

auto pStatusBar::setTooltip(StatusBar::Part* part) -> void {

	TOOLINFO toolInfo;
	memset(&toolInfo, 0, sizeof(TOOLINFO));
	toolInfo.cbSize = sizeof (toolInfo);
	toolInfo.hwnd = GetParent(hwnd);
	toolInfo.uId = (UINT_PTR) hwnd;

	while (SendMessage(hwndTip, TTM_ENUMTOOLS, 0, (LPARAM)&toolInfo))
		SendMessage(hwndTip, TTM_DELTOOL, 0, (LPARAM)&toolInfo);

	if (part && !part->tooltip.empty()) {
		utf16_t wtooltip(part->tooltip);

		toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;                
		toolInfo.lpszText = wtooltip;

		SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
	}
}

auto pStatusBar::destroy() -> void {
    
    if (hwnd)
        DestroyWindow(hwnd);
    
    if (hwndTip)
        DestroyWindow(hwndTip);
    
    hwnd = 0;
    hwndTip = 0;
}

auto pStatusBar::setFont(std::string font) -> void {
    
    pFont::free(hfont);
    hfont = pFont::create(font);    
    
    if (hwnd)
        SendMessage(hwnd, WM_SETFONT, (WPARAM)hfont, 0);
}

auto pStatusBar::setText(std::string text) -> void {
    if (hwnd)
        SendMessage(hwnd, SB_SETTEXT, 0, (LPARAM)(wchar_t*)utf16_t(text));
    
    update();
}

auto pStatusBar::updatePosition() -> void {
    
    if (hwnd)
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_FRAMECHANGED);
    
    update();
}

auto pStatusBar::setStatusVisible(bool visible) -> void {
    if (hwnd)
        ShowWindow(hwnd, visible ? SW_SHOWNORMAL : SW_HIDE);
    
    update();
}

auto pStatusBar::getHeight() -> unsigned {
    if (!hwnd)
        return 0;
    
    RECT src;
    GetClientRect(hwnd, &src);
    return src.bottom - src.top;
}

auto pStatusBar::getWidth(std::string text) -> unsigned {
    
    if (text == "")
        return 0;
    
    if (!hfont)
        setFont( statusBar.font().empty() ? Font::system() : statusBar.font() );
    
    return pFont::size( hfont, text ).width + 8;
}

auto pStatusBar::updatePart( StatusBar::Part& part ) -> void {
    if (hwnd) {		
		if (part.image) {
			// alpha blend transparent part, because of winapi draws some 3D effect
			HICON hIcon = CreateHIconWithAlphaBlend( *part.image, GetSysColor(COLOR_MENU) );
			SendMessage(hwnd, SB_SETICON, part.position, (LPARAM) hIcon);
			DestroyIcon(hIcon);
		} else
			SendMessage(hwnd, SB_SETTEXT, part.position | SBT_OWNERDRAW, 0);
	}
}

auto pStatusBar::update() -> void {
    int i;
    RECT rect;
    int pos;
    int* widths;
    usedParts.clear();
    
    if (!statusBar.window() || !hwnd)
        return;        
        
    auto& parts = statusBar.state.parts;
    
    GetWindowRect(hwnd, &rect);
    pos = rect.right - rect.left;

    std::vector<int> _widths;
    _widths.push_back( pos );
    pos -= 11;
    
    for( i = parts.size() - 1; i >= 0; i-- ) {
        auto& part = parts[i];
        
        if (!part.visible)
            continue;
            
        _widths.push_back( pos );
        
        // first part width doesn't matter. always use remaining space
        if (part.image)
            pos -= part.image->width + 7;
        else
            pos -= part.width;
    }

    if (_widths.size() == 1) {
        SendMessage(hwnd, SB_SETPARTS, 0, 0);
        return;
    }
        
    unsigned partCount = _widths.size();
    
    widths = new int[partCount];
    
    i = partCount;
    for( auto& width : _widths )
        widths[--i] = width;
            
    SendMessage(hwnd, SB_SETPARTS, partCount, (LPARAM)widths );        
    
    i = 0;    
    
    for(auto& part : parts) {
                
        if (part.visible) {                    
            part.position = i;
            usedParts.push_back( &part );
            
            SendMessage(hwnd, SB_SETTEXT, i++ | SBT_OWNERDRAW, 0);

            if (part.popupMenu && !part.popupMenu->state.parentWindow)
                part.popupMenu->p.update(*statusBar.window());            
        }
    }
    // clear right margin area
    SendMessage(hwnd, SB_SETTEXT, i, (LPARAM)"" );
    
    delete[] widths;
}

auto pStatusBar::drawItem(WPARAM wparam, LPARAM lparam) -> void {

    RECT rect = ((DRAWITEMSTRUCT*)lparam)->rcItem;
    
    HDC hDC = ((DRAWITEMSTRUCT*)lparam)->hDC;
    UINT itemID = ((DRAWITEMSTRUCT*)lparam)->itemID;
    auto& part = *usedParts[itemID];
    
    if (part.image) {
		// use SB_SETICON for updates, but following for initialisation
        unsigned yPos = rect.bottom - rect.top;
        
        Image* image = part.image;
        
        HICON hIcon = CreateHIcon( *image );

        yPos = rect.top + (unsigned)((yPos - image->height) / 2);
        
        DrawIconEx( hDC,  rect.left, yPos - 1, hIcon, image->width, image->height, 0, NULL, DI_NORMAL);
        
        if(hIcon)
            DestroyIcon(hIcon);
// same with bitmaps
//        if (!image->alphaBlendApplied)
//            image->alphaBlend( GetSysColor(COLOR_MENU) );
//        
//        HBITMAP hbitmap = CreateBitmap( *image );      
//        HDC hdcMem = CreateCompatibleDC(hDC);
//        
//        SelectObject(hdcMem, hbitmap);
//        
//        StretchBlt( hDC, rect.left, yPos - 1, image->width, image->height, hdcMem, 0, 0, image->width, image->height, SRCCOPY );
//                   
//        DeleteObject(hbitmap);
//        DeleteDC( hdcMem );
        
    } else {
		SetBkMode(hDC, TRANSPARENT);
		
        if (part.overrideForegroundColor != -1) {
            unsigned color = part.overrideForegroundColor;
            SetTextColor(hDC, RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff));
        }
        
        rect.top += 1;

		rect.left += 1;
        if (part.alignRight)
            rect.right -= 4;
        else if ( !part.width )
            rect.left += 4;

        DrawText(hDC, utf16_t(part.text.c_str()), -1, &rect, (part.alignRight ? DT_RIGHT : 0)  );
    }
}

auto pStatusBar::onClick(LPARAM lparam) -> void {
    LPNMMOUSE lpnm = (LPNMMOUSE) lparam;
    
    StatusBar::Part* part = getHoverPart( lpnm->pt.x );
        
    if (!part)
        return;
    
    if (part->popupMenu) {
        POINT pt;
        GetCursorPos(&pt);
        int mid = TrackPopupMenuEx(part->popupMenu->p.hmenu, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, statusBar.window()->p.hwnd, NULL);
        if (mid) SendMessage(statusBar.window()->p.hwnd, WM_COMMAND, mid, 0);
    }

    if (part->onClick)
        part->onClick();                   
}

auto pStatusBar::getHoverPart(int xPos) -> StatusBar::Part* {
    StatusBar::Part* part;

    RECT rect;
    GetWindowRect(hwnd, &rect);

    int pos = rect.right - rect.left;
    pos -= 11;

    unsigned partCount = usedParts.size();

    for (int i = partCount - 1; i >= 0; i--) {
        part = usedParts[i];

        if (part->image)
            pos -= part->image->width + 7;
        else
            pos -= part->width;

        if (xPos > pos)
            return part;        
    }
    
    return nullptr;
}
