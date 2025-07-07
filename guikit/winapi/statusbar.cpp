
#define TRACKBAR_SLIDER 10000

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
    
	//if (pWindow::XPOrBelowOrWin7InXPMode())
	//	hwnd = CreateWindowEx( WS_EX_COMPOSITED, STATUSCLASSNAME, L"", WS_CHILD, 0, 0, 0, 0, statusBar.window()->p.hwnd, (HMENU)(unsigned long long)statusBar.id, GetModuleHandle(0), 0);
	//else
		hwnd = CreateWindow( STATUSCLASSNAME, L"", WS_CHILD, 0, 0, 0, 0, statusBar.window()->p.hwnd, (HMENU)(unsigned long long)statusBar.id, GetModuleHandle(0), 0);
    

	// SendMessage( hwnd, SB_SETBKCOLOR, 0, GetSysColor(COLOR_MENU));
    
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&statusBar);
    
    wndprocOrig = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)subclassWndProc);
    
//	hwndTip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL,
//		WS_POPUP | TTS_ALWAYSTIP | TTS_USEVISUALSTYLE,
//		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
//		hwnd, NULL, GetModuleHandle(0), 0);
    
    hoverPart = nullptr;

    //brush = GetSysColorBrush(COLOR_MENU);
    
    unsigned partCount = statusBar.state.parts.size();
    
    if (hfont)
        SendMessage(hwnd, WM_SETFONT, (WPARAM)hfont, 0);
    
    if (partCount == 0)
        setText( statusBar.text() );
    else
        update();
}

auto pStatusBar::setComposited(bool state) -> void {
    if (!hwnd || !pWindow::XPOrBelowOrWin7InXPMode())
        return;

    if (state)
        SetWindowLong(hwnd, GWL_EXSTYLE, (GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_COMPOSITED));
    else
        SetWindowLong(hwnd, GWL_EXSTYLE, (GetWindowLong(hwnd, GWL_EXSTYLE) & ~WS_EX_COMPOSITED));
}

auto pStatusBar::subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    StatusBar* statusBar = (StatusBar*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(statusBar == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);
	static bool xpMode = pWindow::XPOrBelowOrWin7InXPMode();

    switch(msg) {

        case WM_MOUSEMOVE: {
            auto& p = statusBar->p;

            StatusBar::Part* part = p.getHoverPart( (int)(short) LOWORD(lparam) );

            if (part && (part->popupMenu || part->onClick))
                SetCursor(statusBar->p.hCursor);

//			if (part != p.hoverPart) {
//				p.hoverPart = part;
//
//				p.setTooltip( part );
//
//              return 0;
//			}

        } break;
        case WM_ERASEBKGND:
            //RECT rc;
            //GetClientRect(hwnd, &rc);
            //PAINTSTRUCT ps;
            //BeginPaint(hwnd, &ps);
            //FillRect(ps.hdc, &rc, statusBar->p.brush);
            //EndPaint(hwnd, &ps);
            //return 1;

            if (pApplication::useDark) {
                HDC hdc = (HDC)wparam;
                RECT rect;
                GetClientRect(hwnd, &rect);
                FillRect(hdc, &rect, pApplication::darkBGBrush);
                return TRUE;
            } break;

        case WM_CTLCOLORSTATIC:
            if (pApplication::useDark) {
                SetBkMode((HDC)(wparam), TRANSPARENT);
                return (INT_PTR)pApplication::darkBGBrush;
            } break;

        case WM_PAINT: {
            auto& p = statusBar->p;
			if (xpMode && statusBar->window()->fullScreen() );
			else {
                if (p.locked )
                    p.locked = false;
                else
                    SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOREDRAW);
            }
            break;
        }

        case WM_HSCROLL:
        case WM_VSCROLL: {
            auto& p = statusBar->p;

            for(auto& trackBar : p.trackBars) {
                if (trackBar.hwnd == (HWND)lparam) {
                    trackBar.part.sliderPosition =  SendMessage(trackBar.hwnd, TBM_GETPOS, 0, 0);
                    if (trackBar.part.onChange)
                        trackBar.part.onChange( trackBar.part.sliderPosition );
                    break;
                }
            }
        } break;

        case WM_NOTIFY: {
            NMHDR* nmhdr = (NMHDR*)lparam;
            if (!nmhdr)
                break;
            auto& p = statusBar->p;

            for(auto& trackBar : p.trackBars) {
                if (trackBar.hwnd == nmhdr->hwndFrom) {
                    if (nmhdr->code == NM_CUSTOMDRAW) {
                        NMCUSTOMDRAW* lpNMCustomDraw = (NMCUSTOMDRAW*)lparam;
                        lpNMCustomDraw->uItemState &= ~CDIS_FOCUS;
                        LPNMCUSTOMDRAW lpcd = (LPNMCUSTOMDRAW)lparam;

                        switch (lpcd->dwDrawStage) {
                            case CDDS_PREPAINT:
                                if (pApplication::useDark)
                                    return CDRF_NOTIFYITEMDRAW;
                                break;

                            case CDDS_ITEMPREPAINT:
                                if (lpcd->dwItemSpec == TBCD_CHANNEL) {
                                  //  InflateRect(&lpcd->rc, 0, -1);
                                    FillRect(lpcd->hdc, &lpcd->rc, pApplication::darkEdgeBrush);
                                    return CDRF_SKIPDEFAULT;

                                } break;
                        }
                        return CDRF_DODEFAULT;
                    }
                    break;
                }
            }

        } break;

        case WM_CONTEXTMENU:
            return 0;
    }
    //return CallWindowProc(statusBar->p.wndprocOrig, hwnd, msg, wparam, lparam);
    return pApplication::wndProc(statusBar->p.wndprocOrig, hwnd, msg, wparam, lparam);
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
    for(auto& trackBar : trackBars)
        DestroyWindow(trackBar.hwnd);

    trackBars.clear();

    if (hwnd)
        DestroyWindow(hwnd);
    
    if (hwndTip)
        DestroyWindow(hwndTip);
    
    hwnd = 0;
    hwndTip = 0;
}

auto pStatusBar::setFont(const std::string& font) -> void {
    
    pFont::free(hfont);
    hfont = pFont::create(font);    
    
    if (hwnd)
        SendMessage(hwnd, WM_SETFONT, (WPARAM)hfont, 0);
}

auto pStatusBar::setText(const std::string& text) -> void {
    if (hwnd)
        SendMessage(hwnd, SB_SETTEXT, 0 | SBT_NOBORDERS, (LPARAM)(wchar_t*)utf16_t(text));
    
    update();
}

auto pStatusBar::updatePosition() -> void {

   // if (hwnd)
     //   SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_FRAMECHANGED);

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

auto pStatusBar::getWidth(const std::string& text) -> unsigned {
    
    if (text == "")
        return 0;
    
    if (!hfont)
        setFont( statusBar.font().empty() ? Font::system() : statusBar.font() );
    
    return pFont::size( hfont, text ).width + 8;
}

auto pStatusBar::updatePart( StatusBar::Part& part ) -> void {
    if (part.position < 0) {
        statusBar.state.updatePending = true;
        return;
    }

    if (hwnd) {
        if (part.sliderLength) {
            HWND trackBar = getSlider(part);
            if (trackBar && (SendMessage(trackBar, TBM_GETPOS, 0, 0) != part.sliderPosition) )
                SendMessage(trackBar, TBM_SETPOS, (WPARAM)true, (LPARAM)part.sliderPosition);
        }

//		if (part.image) {
//			// alpha blend transparent part, because of winapi draws some 3D effect
//			HICON hIcon = CreateHIconWithAlphaBlend( *part.image, GetSysColor(COLOR_MENU) );
//			SendMessage(hwnd, SB_SETICON, part.position, (LPARAM) hIcon);
//			DestroyIcon(hIcon);
//		} else {
            bool _border = pApplication::hasAppThemed() && part.appendSeparator && ((part.position + 1) < usedParts.size());

            SendMessage(hwnd, SB_SETTEXT, part.position | SBT_OWNERDRAW | (_border ? 0 : SBT_NOBORDERS), 0);
//      }
	}
}

auto pStatusBar::getSlider(StatusBar::Part& part) -> HWND {
    for(auto& trackBar : trackBars) {
        if (trackBar.part.id == part.id)
            return trackBar.hwnd;
    }
    return nullptr;
}

auto pStatusBar::update() -> void {
    int i;
    RECT rect;
    int pos = 2;
    int* widths;
    usedParts.clear();
    
    if (!statusBar.window() || !hwnd)
        return;    
        
    auto& parts = statusBar.state.parts;
    
    GetWindowRect(hwnd, &rect);

    std::vector<int> _widths;

    unsigned countVisible = 0;

    HWND trackBar;
        
    for( i = 0; i < parts.size(); i++ ) {
        auto& part = parts[i];

        if (part.sliderLength) {
            trackBar = getSlider(part);

            if (!trackBar) {
                trackBar = CreateWindow(
                    TRACKBAR_CLASS, L"", WS_CHILD | TBS_NOTICKS | TBS_BOTH | TBS_HORZ,
                    0, 0, 0, 0, hwnd, (HMENU)(long long)(part.id + TRACKBAR_SLIDER), GetModuleHandle(0), 0);

                SendMessage(trackBar, TBM_SETRANGE, (WPARAM)true, (LPARAM)MAKELONG(0, part.sliderLength - 1));
                //SendMessage(trackBar, TBM_SETPAGESIZE, 0, (LPARAM)(part.sliderLength >> 3));

                trackBars.push_back({part, trackBar});
            }

            if (SendMessage(trackBar, TBM_GETPOS, 0, 0) != part.sliderPosition)
                SendMessage(trackBar, TBM_SETPOS, (WPARAM)true, (LPARAM)part.sliderPosition);
        } else
            trackBar = nullptr;

        if (!part.visible) {
            if (trackBar && IsWindowVisible(trackBar))
                ShowWindow(trackBar, SW_HIDE);
            continue;
        }

        if (trackBar) {
            MoveWindow(trackBar, pos, 3, part.width - 1, getHeight() - 2, true);

            if (!IsWindowVisible(trackBar))
                ShowWindow(trackBar, SW_SHOWNORMAL);
        }

        countVisible++;
        
        // first part width doesn't matter. always use remaining space
        if (part.image)
            pos += part.image->width + 7;
        else if (part.width)
            pos += part.width;
		else
			continue;

        _widths.push_back( pos );
    }

   // if (_widths.size() == 0) {
     //   SendMessage(hwnd, SB_SETPARTS, 0, 0);
       // return;
    //}
	
	_widths.push_back( rect.right - rect.left );
        
    unsigned partCount = _widths.size();
    
    widths = new int[partCount];
    
    i = 0;
    for( auto& width : _widths )
        widths[i++] = width;
            
    SendMessage(hwnd, SB_SETPARTS, partCount, (LPARAM)widths );        
    
    i = 0;
    for(auto& part : parts) {
                
        if (part.visible) {
            part.position = i;
            usedParts.push_back( &part );

            bool _border = pApplication::hasAppThemed() && part.appendSeparator && ((i + 1) < countVisible);

            SendMessage(hwnd, SB_SETTEXT, i++ | SBT_OWNERDRAW | (_border ? 0 : SBT_NOBORDERS), 0);
        }
    }
    // clear right margin area
   // SendMessage(hwnd, SB_SETTEXT, i | SBT_NOBORDERS, (LPARAM)"" );
    
    delete[] widths;

    locked = false;
    SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_FRAMECHANGED);
}

auto pStatusBar::drawItem(WPARAM wparam, LPARAM lparam) -> void {

    RECT rect = ((DRAWITEMSTRUCT*)lparam)->rcItem;
    
    HDC hDC = ((DRAWITEMSTRUCT*)lparam)->hDC;
    UINT itemID = ((DRAWITEMSTRUCT*)lparam)->itemID;
    auto _size = usedParts.size();
    if (_size == 0)
        return;

    if (itemID >= _size) {
        itemID = _size - 1;
    }      

    auto& part = *usedParts[itemID];
    
    if (part.image) {
		// use SB_SETICON for updates, but following for initialisation
        unsigned yPos = rect.bottom - rect.top;
        
        Image* image = part.image;
        
        HICON hIcon = CreateHIcon( *image );

        yPos = rect.top + (unsigned)((yPos - image->height) / 2);
        
        DrawIconEx( hDC, rect.left + (!part.position ? 3 : 0), yPos - ((getVersionNew() > Windows7) ? 1 : 0), hIcon, image->width, image->height, 0, NULL, DI_NORMAL);
        
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
    } else if (part.sliderLength) {

    } else {
		
        if (part.overrideForegroundColor != -1) {
            unsigned color = part.overrideForegroundColor;
            SetTextColor(hDC, RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff));
        } else if (pApplication::useDark)
            SetTextColor(hDC, DARK_FG_COL);

        if (pApplication::hasAppThemed() && (getVersionNew() <= Windows7))
            rect.top += 2;
        else
            rect.top += 1;

		rect.left += 1;
        if (part.alignRight)
            rect.right -= 4;
        else if ( !part.width )
            rect.left += 4;
        else if (itemID == 0)
            rect.left += 4;

        SetBkMode(hDC, TRANSPARENT);
        // SetBkColor(hDC, GetSysColor(COLOR_MENU));
        DrawText(hDC, utf16_t(part.text.c_str()), -1, &rect, DT_SINGLELINE | DT_NOCLIP | (part.alignRight ? DT_RIGHT : 0) );
    }
    locked = !disableLock;
}

auto pStatusBar::setLockDisabled(bool state) -> void {
	disableLock = state;
	locked = false;
    //if (!state && !hasAppThemed())
      //  SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOREDRAW);
}

auto pStatusBar::onClick(LPARAM lparam) -> void {
    LPNMMOUSE lpnm = (LPNMMOUSE) lparam;
    
    StatusBar::Part* part = getHoverPart( lpnm->pt.x );
        
    if (!part)
        return;
    
    if (part->popupMenu) {
        POINT pt;
        GetCursorPos(&pt);
        int adjust = 0;
        if (statusBar.window()->fullScreen())
            adjust = -15;

        int mid = TrackPopupMenuEx(part->popupMenu->p.hmenu, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y + adjust, statusBar.window()->p.hwnd, NULL);
        if (mid) SendMessage(statusBar.window()->p.hwnd, WM_COMMAND, mid, 0);
    }

    if (part->onClick)
        part->onClick();                   
}

auto pStatusBar::getHoverPart(int xPos) -> StatusBar::Part* {
    StatusBar::Part* part;

    RECT rect;
    GetWindowRect(hwnd, &rect);
    int pos = 2;

    unsigned partCount = usedParts.size();

   for(unsigned i = 0; i < partCount; i++) {
        part = usedParts[i];

        if (part->image)
            pos += part->image->width + 7;
        else
            pos += part->width;

        if (xPos < pos)
            return part;        
    }
    
    return nullptr;
}
