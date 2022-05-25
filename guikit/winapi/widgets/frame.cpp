
// owner drawn groupbox widget to prevent flicker

auto pFrame::minimumSize() -> Size {
    Size size = getMinimumSize();
    
    size.width += 4 + (borderSize() << 1);
    size.height >>= 1;
    if (widget.text().empty()) size.height = 0;
	
    size.height += borderSize() << 1;
    return size;
}

auto pFrame::setGeometry(Geometry geometry) -> void {
    Size size = getMinimumSize();
	
    if (!hwnd)
        return;

    Widget* parent = getParentTabWidget();

    if (parent) {
        auto geo = parent->geometry();

        geometry.x -= geo.x;
        geometry.y -= geo.y;
    }
        
    unsigned headerSize = size.height;
    unsigned yVertical = geometry.y;
    unsigned hVertical = geometry.height;

    if (widget.text().empty()) {
        headerSize = roundedConrner ? 2 : 1;
    } else {
        yVertical += size.height >> 1;
        hVertical -= size.height >> 1;
    }
			
	if (roundedConrner) {
		yVertical += 2;
		hVertical -= 4;
	}
	
	SetWindowPos(hwnd, NULL, geometry.x + 1, geometry.y, geometry.width - 2, headerSize, SWP_NOZORDER | SWP_NOCOPYBITS);                          

    SetWindowPos(leftBorder, NULL, geometry.x, yVertical, 1, hVertical, SWP_NOZORDER | SWP_NOCOPYBITS);                

    SetWindowPos(rightBorder, NULL, geometry.x + geometry.width - 1, yVertical, 1, hVertical, SWP_NOZORDER | SWP_NOCOPYBITS);        

	if (roundedConrner)
		SetWindowPos(bottomBorder, NULL, geometry.x + 1, geometry.y + geometry.height - 2, geometry.width - 2, 2, SWP_NOZORDER | SWP_NOCOPYBITS);
	else
		SetWindowPos(bottomBorder, NULL, geometry.x + 1, geometry.y + geometry.height - 1, geometry.width - 2, 1, SWP_NOZORDER | SWP_NOCOPYBITS);   
}

auto pFrame::setVisible(bool visible) -> void {
    
    if (hwnd) {
        ShowWindow(hwnd, visible ? SW_SHOWNORMAL : SW_HIDE);
        ShowWindow(leftBorder, visible ? SW_SHOWNORMAL : SW_HIDE);
        ShowWindow(rightBorder, visible ? SW_SHOWNORMAL : SW_HIDE);
        ShowWindow(bottomBorder, visible ? SW_SHOWNORMAL : SW_HIDE);
    }
}

auto pFrame::create() -> void {
    destroy(hwnd);

    destroy(leftBorder);
    destroy(rightBorder);
    destroy(bottomBorder);
	
	if (pen)
		DeleteObject(pen);
	
	pen = CreatePen(PS_SOLID, 1, getBorderColor());		
	
	roundedConrner = pApplication::version <= Windows7;
    
    hwnd = CreateWindow(WC_STATIC, L"",
        WS_CHILD,
        0, 0, 0, 0, getParentHandle(), (HMENU)(unsigned long long)widget.id, GetModuleHandle(0), 0);

    leftBorder = CreateWindow(WC_STATIC, L"",
            WS_CHILD,
            0, 0, 0, 0, getParentHandle(), (HMENU) (unsigned long long) widget.id, GetModuleHandle(0), 0);
    
    rightBorder = CreateWindow(WC_STATIC, L"",
        WS_CHILD,
        0, 0, 0, 0, getParentHandle(), (HMENU)(unsigned long long)widget.id, GetModuleHandle(0), 0);
        
    bottomBorder = CreateWindow(WC_STATIC, L"",
        WS_CHILD,
        0, 0, 0, 0, getParentHandle(), (HMENU)(unsigned long long)widget.id, GetModuleHandle(0), 0);
    
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&widget);
    SetWindowLongPtr(leftBorder, GWLP_USERDATA, (LONG_PTR)&widget);
    SetWindowLongPtr(rightBorder, GWLP_USERDATA, (LONG_PTR)&widget);
    SetWindowLongPtr(bottomBorder, GWLP_USERDATA, (LONG_PTR)&widget);
    
    wndprocOrig = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)subclassWndProc);  
    SetWindowLongPtr(leftBorder, GWLP_WNDPROC, (LONG_PTR)subclassWndProcL);  
    SetWindowLongPtr(rightBorder, GWLP_WNDPROC, (LONG_PTR)subclassWndProcR);  
    SetWindowLongPtr(bottomBorder, GWLP_WNDPROC, (LONG_PTR)subclassWndProcB);  
}

auto CALLBACK pFrame::subclassWndProcL(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    Widget* widget = (Widget*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(widget == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);

    switch(msg) {   
        case WM_ERASEBKGND:
            return 0;
            
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
			pFrame* frame = (pFrame*)&(widget->p);

            RECT rc;
            GetClientRect(hwnd, &rc);
            unsigned width = 1;
            unsigned height = getHeight(rc);
            HDC memHdc = CreateCompatibleDC(hdc);
            HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
            SelectObject(memHdc, memBitmap);

            SelectObject(memHdc, frame->pen);
            MoveToEx(memHdc, rc.left, rc.top, NULL);
            LineTo(memHdc, rc.left, rc.bottom);

            BitBlt(hdc, 0, 0, width, height, memHdc, 0, 0, SRCCOPY);

            DeleteObject(memBitmap);
            DeleteDC(memHdc);
            DeleteDC(hdc);
            EndPaint(hwnd, &ps);
            
            return 0;
        }
    }
    
    return pApplication::wndProc(widget->p.wndprocOrig, hwnd, msg, wparam, lparam);
}

auto CALLBACK pFrame::subclassWndProcR(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    Widget* widget = (Widget*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(widget == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);

    switch(msg) {   
        case WM_ERASEBKGND:
            return 0;
            
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
			pFrame* frame = (pFrame*)&(widget->p);

            RECT rc;
            GetClientRect(hwnd, &rc);
            unsigned width = 1;
            unsigned height = getHeight(rc);
            HDC memHdc = CreateCompatibleDC(hdc);
            HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
            SelectObject(memHdc, memBitmap);

            SelectObject(memHdc, frame->pen);
            MoveToEx(memHdc, rc.left, rc.top, NULL);
            LineTo(memHdc, rc.left, rc.bottom);

            BitBlt(hdc, 0, 0, width, height, memHdc, 0, 0, SRCCOPY);

            DeleteObject(memBitmap);
            DeleteDC(memHdc);
            DeleteDC(hdc);
            EndPaint(hwnd, &ps);
            
            return 0;
        }
    }
    
    return pApplication::wndProc(widget->p.wndprocOrig, hwnd, msg, wparam, lparam);
}

auto CALLBACK pFrame::subclassWndProcB(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    Widget* widget = (Widget*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(widget == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);

    switch(msg) {   
        case WM_ERASEBKGND:
            return 0;
            
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
			pFrame* frame = (pFrame*)&(widget->p);

            RECT rc;
            GetClientRect(hwnd, &rc);
            unsigned width = getWidth(rc);
            unsigned height = frame->roundedConrner ? 2 : 1;
            HDC memHdc = CreateCompatibleDC(hdc);
            HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
            SelectObject(memHdc, memBitmap);

            SelectObject(memHdc, frame->pen);
			
			if (frame->roundedConrner) {
				HBRUSH brush = widget->p.getBackgroundBrush();
				
				if (brush) {
					FillRect(memHdc, &rc, brush);
				} else
					SetBkMode(memHdc, TRANSPARENT);

				rc.top += 1;
				rc.left += 1;
				rc.right -= 1;
				
				MoveToEx( memHdc, rc.left, rc.top, NULL );
				LineTo( memHdc, rc.right, rc.top );
				
				rc.top -= 1;
				rc.left -=1;
				rc.right += 1;

				MoveToEx( memHdc, rc.left, rc.top, NULL );
				LineTo( memHdc, rc.left+1, rc.top );				
				
				MoveToEx( memHdc, rc.right - 1, rc.top, NULL );
				LineTo( memHdc, rc.right, rc.top );				
				
			} else {
				MoveToEx(memHdc, rc.left, rc.top, NULL);
				LineTo(memHdc, rc.right, rc.top);				
			}
			
            BitBlt(hdc, 0, 0, width, height, memHdc, 0, 0, SRCCOPY);

            DeleteObject(memBitmap);
            DeleteDC(memHdc);
            DeleteDC(hdc);
            EndPaint(hwnd, &ps);
			
            return 0;
        }
    }
    
    return pApplication::wndProc(widget->p.wndprocOrig, hwnd, msg, wparam, lparam);
}

auto CALLBACK pFrame::subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    Widget* widget = (Widget*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(widget == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);

    switch(msg) {   
        case WM_ERASEBKGND:
            return 0;
            
        case WM_PAINT: {						
			bool useLabel = !widget->text().empty();			
			pFrame* frame = (pFrame*)&(widget->p);
            Size size = widget->p.getMinimumSize();
            
            RECT rc;
            GetClientRect(hwnd, &rc);
            unsigned width = getWidth(rc);
            unsigned height = getHeight(rc);			

            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            HDC memHdc = CreateCompatibleDC(hdc);
            HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
            SelectObject(memHdc, memBitmap);
			
			SetBkMode(memHdc, TRANSPARENT);
         //   if (widget->enabled())
           //     SetTextColor(memHdc, GetSysColor(COLOR_WINDOWTEXT) );
            //else
              //  SetTextColor(memHdc, GetSysColor(COLOR_GRAYTEXT) );

			HBRUSH brush = widget->p.getBackgroundBrush();
            
			if (brush && (useLabel || frame->roundedConrner))
				FillRect(memHdc, &rc, brush);
            
            RECT rcTemp = rc;
			if (useLabel)
				rcTemp.top += height >> 1;
			
			if (frame->roundedConrner) {
				rcTemp.left += 1;
				rcTemp.right -= 1;
			}            
            
            SelectObject(memHdc, frame->pen);
            MoveToEx(memHdc, rcTemp.left, rcTemp.top, NULL);
            LineTo(memHdc, rcTemp.right, rcTemp.top);
			
			if (frame->roundedConrner) {
				rcTemp.left -= 1;
				rcTemp.top += 1;
				rcTemp.right += 1;
				
				MoveToEx( memHdc, rcTemp.left, rcTemp.top, NULL );
				LineTo( memHdc, rcTemp.left + 1, rcTemp.top );

				MoveToEx( memHdc, rcTemp.right - 1, rcTemp.top, NULL );
				LineTo( memHdc, rcTemp.right, rcTemp.top );
			}
            
            if (useLabel) {
                rcTemp = rc;
                rcTemp.left += 6;
                rcTemp.right = rcTemp.left + size.width + 4;
                
				FillRect(memHdc, &rcTemp, brush);

                rcTemp = rc;
                rcTemp.left += 8;
                rcTemp.right = rcTemp.left + size.width;
                
                SelectObject(memHdc, widget->p.hfont);
                unsigned length = GetWindowTextLength(hwnd);
                wchar_t* text = new wchar_t[length + 1];
                GetWindowText(hwnd, text, length + 1);
                text[length] = 0;
                DrawText(memHdc, text, -1, &rcTemp, DT_LEFT | DT_END_ELLIPSIS);
                delete[] text;
            }
            
            BitBlt(hdc, 0, 0, width, height, memHdc, 0, 0, SRCCOPY);
            
            DeleteObject(memBitmap);
            DeleteDC    (memHdc);
            DeleteDC    (hdc);
            EndPaint(hwnd, &ps);
			
        } return 0;

    }
    
    //return CallWindowProc(widget->p.wndprocOrig, hwnd, msg, wparam, lparam);
    return pApplication::wndProc(widget->p.wndprocOrig, hwnd, msg, wparam, lparam);
}

auto pFrame::rebuild() -> void {
    if(!needRebuild())
        return;
        
    create();        
    pWidget::setFont( widget.font() );
    pWidget::setText( widget.text() );
    
    setEnabled(widget.enabled());
    setVisible(widget.visible());
    
    if(hwnd) {
        SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetWindowPos(leftBorder, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetWindowPos(rightBorder, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetWindowPos(bottomBorder, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
	
    setTooltip(widget.tooltip());    
}

inline auto pFrame::getBorderColor() -> COLORREF {
	
	if (!IsAppThemed() || (pApplication::version <= WindowsXP) )
		return RGB(0xff, 0xff, 0xff);
	
	if (pApplication::version > Windows7)
		return RGB(0xdc, 0xdc, 0xdc);
	
	return RGB(0xd5, 0xdf, 0xe5);
}

auto pFrame::setText(std::string text) -> void {
    pWidget::setText(text);
    
    if (hwnd)
        InvalidateRect(hwnd, 0, false);
}

auto pFrame::setEnabled(bool enabled) -> void {
    return;
    if (hwnd)
        InvalidateRect(hwnd, 0, false);
}

auto pFrame::setFont(std::string font) -> void {
    pWidget::setFont( font );
    
    if (hwnd)
        InvalidateRect(hwnd, 0, false);
}