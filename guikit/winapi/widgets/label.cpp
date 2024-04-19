
auto pLabel::minimumSize() -> Size {
    Size size = getMinimumSize();
    return {size.width, size.height};
}

auto pLabel::setText(const std::string& text) -> void {
    pWidget::setText(text);
    
    if (hwnd)
        InvalidateRect(hwnd, 0, false);
}

auto pLabel::setFont(std::string font) -> void {
    pWidget::setFont( font );
    
    if (hwnd)
        InvalidateRect(hwnd, 0, false);
}

auto pLabel::setEnabled(bool enabled) -> void {
    pWidget::setEnabled( enabled );
    
    if (hwnd)
        InvalidateRect(hwnd, 0, false);
}

auto pLabel::setForegroundColor(unsigned color) -> void {
    
    if (hwnd)
        InvalidateRect(hwnd, 0, false);
}

auto pLabel::create() -> void {
    destroy(hwnd);
    destroy(hwndTip);
    
    hwnd = CreateWindow(WC_STATIC, L"", WS_CHILD | SS_NOTIFY,
        0, 0, 0, 0, getParentHandle(), (HMENU)(unsigned long long)label.id, GetModuleHandle(0), 0);

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&label);
    wndprocOrig = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)subclassWndProc);
}

auto pLabel::rebuild() -> void {
    if(!needRebuild())
        return;
    
    create();
    setFont( widget.font() );
    setText(widget.text());
    pWidget::rebuild();
}

auto pLabel::setAlign( Label::Align align ) -> void {
    if (hwnd)
        InvalidateRect(hwnd, 0, false);
}

auto CALLBACK pLabel::subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    Label* label = (Label*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(label == nullptr)
        return DefWindowProc(hwnd, msg, wparam, lparam);
    
    Window* window = (Window*)label->Sizable::state.window;
    
    if(window == nullptr)
        return DefWindowProc(hwnd, msg, wparam, lparam);

    switch(msg) {
        case WM_GETDLGCODE:
            return DLGC_STATIC | DLGC_WANTCHARS;
        case WM_ERASEBKGND:
            return 0;
        case WM_PAINT: {
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
			
			HBRUSH brush = label->p.getBackgroundBrush();
			if (brush)
				FillRect(memHdc, &rc, brush);
            
            SelectObject(memHdc, ((Widget*)label)->p.hfont);
            unsigned length = GetWindowTextLength(hwnd);
            wchar_t* text = new wchar_t[length + 1];
            GetWindowText(hwnd, text, length + 1);
            text[length] = 0;
            DrawText(memHdc, text, -1, &rc, DT_CALCRECT | DT_END_ELLIPSIS);
            unsigned _height = rc.bottom;
            GetClientRect(hwnd, &rc);
            rc.top = (rc.bottom - _height) / 2; //center vertically
            rc.bottom = rc.top + _height;
            if (!label->enabled())
                SetTextColor(memHdc, GetSysColor(COLOR_GRAYTEXT) );
			else if (label->overrideForegroundColor()) {
				unsigned color = label->foregroundColor();
				SetTextColor(memHdc, RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff) );
			}
				
            UINT format = label->state.align == Label::Align::Left ? DT_LEFT : DT_RIGHT;
            
            DrawText(memHdc, text, -1, &rc, format | DT_END_ELLIPSIS);
            
            BitBlt(hdc, 0, 0, width, height, memHdc, 0, 0, SRCCOPY);
            
            DeleteObject(memBitmap);
            DeleteDC    (memHdc);
            DeleteDC    (hdc);
            EndPaint(hwnd, &ps);
            delete[] text;
            return 0;
        }
    }
    //return DefWindowProc(hwnd, msg, wparam, lparam);
    return CallWindowProc(label->p.wndprocOrig, hwnd, msg, wparam, lparam);
}
