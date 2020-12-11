
pStatusBar::pStatusBar(StatusBar& statusBar) : statusBar(statusBar) {
    
    hwnd = nullptr;    
    hfont = nullptr;
}

pStatusBar::~pStatusBar() {
    destroy();
    pFont::free(hfont);
}

auto pStatusBar::create() -> void {    
    
    hwnd = CreateWindow(STATUSCLASSNAME, L"", WS_CHILD, 0, 0, 0, 0, statusBar.window()->p.hwnd, (HMENU)(unsigned long long)statusBar.id, GetModuleHandle(0), 0);
    
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&statusBar);
    
    unsigned partCount = statusBar.state.parts.size();
    
    if (hfont)
        SendMessage(hwnd, WM_SETFONT, (WPARAM)hfont, 0);
    
    if (partCount == 0)
        setText( statusBar.text() );
    else
        update();
}

auto pStatusBar::destroy() -> void {
    
    if (hwnd)
        DestroyWindow(hwnd);
    
    hwnd = 0;
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

auto pStatusBar::updatePart( StatusBar::Part& part ) -> void {
    if (hwnd)
        SendMessage(hwnd, SB_SETTEXT, part.position | SBT_OWNERDRAW, 0);
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
    
    for( i = parts.size() - 1; i >= 0; i-- ) {
        if (!parts[i].visible)
            continue;
            
        _widths.push_back( pos );
        
        // first part width doesn't matter. always use remaining space
        pos -= parts[i].width;
    }

    if (_widths.size() == 0) {
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

            if (part.popupMenu) {

                part.popupMenu->p.update(*statusBar.window());            
            }
        }
    }    
    
    delete[] widths;
}

auto pStatusBar::drawItem(WPARAM wparam, LPARAM lparam) -> void {

    RECT rect = ((DRAWITEMSTRUCT*)lparam)->rcItem;
    
    HDC hDC = ((DRAWITEMSTRUCT*)lparam)->hDC;
    UINT itemID = ((DRAWITEMSTRUCT*)lparam)->itemID;
        
    auto& part = *usedParts[itemID];
    
    bool useImage = part.image && !part.image->empty();
    
    if (useImage) {
        unsigned yPos = rect.bottom - rect.top;
        
        Image* image = part.image;
        
        HICON hIcon = CreateHIcon( *image );
        
        yPos = rect.top + (unsigned)((yPos - image->height) / 2);
        
        DrawIconEx( hDC,  rect.left, yPos - 1, hIcon, image->width, image->height, 0, NULL, DI_NORMAL);
        
        if(hIcon)
            DestroyIcon(hIcon);
        
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
    }
    
    if (!part.text.empty()) {
        
        if (useImage)
            rect.left += part.image->width + 5;

        if (part.overrideForegroundColor) {
            unsigned color = part.foregroundColor;
            SetTextColor(hDC, RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff));
        }
        
        rect.top += 1;
        
        if ( !part.width ) {
            rect.left -= 5;
            rect.right -= 5;
            DrawText(hDC, utf16_t(part.text.c_str()), -1, &rect, DT_RIGHT | DT_END_ELLIPSIS );
        } else
            DrawText(hDC, utf16_t(part.text.c_str()), -1, &rect, DT_END_ELLIPSIS);
    }
}

auto pStatusBar::onClick(LPARAM lparam) -> void {
    LPNMMOUSE lpnm = (LPNMMOUSE) lparam;

    RECT rect;
    GetWindowRect(hwnd, &rect);
    
    int x = lpnm->pt.x;
    int pos = rect.right - rect.left;
        
    unsigned partCount = usedParts.size();
        
    for( int i = partCount - 1; i >= 0; i-- ) {
        auto& part = *usedParts[i];
        
        pos -= part.width;

        if (x > pos) {
            if (part.popupMenu) {
                POINT pt;
                GetCursorPos(&pt);
                int mid = TrackPopupMenuEx(part.popupMenu->p.hmenu, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, statusBar.window()->p.hwnd, NULL);
                if (mid) SendMessage(statusBar.window()->p.hwnd, WM_COMMAND, mid, 0);
            }
            
            if (part.onClick)
                part.onClick();
            
            break;
        }
    }       
}

