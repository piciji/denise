
auto pLabel::minimumSize() -> Size {
    Size size = getMinimumSize();
    return {size.width, size.height};
}

auto pLabel::setText(std::string text) -> void {
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
        0, 0, 0, 0, label.window()->p.hwnd, (HMENU)(unsigned long long)label.id, GetModuleHandle(0), 0);

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&label);
    wndprocOrig = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)subclassWndProc);
}

auto pLabel::rebuild() -> void {
    if (hwnd)
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
        case WM_GETDLGCODE: return DLGC_STATIC | DLGC_WANTCHARS;
        case WM_ERASEBKGND: return TRUE;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);
            SetBkMode(ps.hdc, TRANSPARENT);
            TabFrameLayout* tabFrameLayout = TabFrameLayout::getParentTabFrame( (Widget*)label );
            if (tabFrameLayout) {
                if (IsAppThemed()) {
                    pTabFrame::updateTabBackgroundForControl( tabFrameLayout->frameWidget->p.hwnd, ((Widget*)label)->p.hwnd);
                    FillRect(ps.hdc, &rc, pTabFrame::bkgndBrush);
                } else {
                    HBRUSH hbrush = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
                    FillRect(ps.hdc, &rc, hbrush);
                    DeleteObject(hbrush);
                }
            }
            SelectObject(ps.hdc, ((Widget*)label)->p.hfont);
            unsigned length = GetWindowTextLength(hwnd);
            wchar_t text[length + 1];
            GetWindowText(hwnd, text, length + 1);
            text[length] = 0;
            DrawText(ps.hdc, text, -1, &rc, DT_CALCRECT | DT_END_ELLIPSIS);
            unsigned height = rc.bottom;
            GetClientRect(hwnd, &rc);
            rc.top = (rc.bottom - height) / 2; //center vertically
            rc.bottom = rc.top + height;
            if (!label->enabled())
                SetTextColor(ps.hdc, GetSysColor(COLOR_GRAYTEXT) );
			else if (label->overrideForegroundColor()) {
				unsigned color = label->foregroundColor();
				SetTextColor(ps.hdc, RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff) );
			}
				
            UINT format = label->state.align == Label::Align::Left ? DT_LEFT : DT_RIGHT;
            
            DrawText(ps.hdc, text, -1, &rc, format | DT_END_ELLIPSIS);
            EndPaint(hwnd, &ps);
            return false;
        }
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}
