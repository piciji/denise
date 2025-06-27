
auto pLabel::minimumSize() -> Size {
    Size size = getMinimumSize();
    return {size.width, size.height};
}

auto pLabel::setText(const std::string& text) -> void {
    pWidget::setText(text);
    
    if (hwnd)
        InvalidateRect(hwnd, 0, false);
}

auto pLabel::setTextThreaded(const std::string& text) -> void {
    pWidget::setText(text);
}

auto pLabel::setFont(std::string font) -> void {
    pWidget::setFont( font );    
    
    if (hwnd)
        InvalidateRect(hwnd, 0, false);
}

auto pLabel::setEnabled(bool enabled) -> void {
    pWidget::setEnabled( enabled );
    
    if (hwnd)
        InvalidateRect(hwnd, 0, true);
}

auto pLabel::setForegroundColor(unsigned color) -> void {
    
    if (hwnd)
        InvalidateRect(hwnd, 0, false);
}

auto pLabel::setForegroundColorThreaded(unsigned color) -> void {
    setForegroundColor(color);
}

auto pLabel::create() -> void {
    destroy(hwnd);
    destroy(hwndTip);
    
    hwnd = CreateWindow(WC_STATIC, L"", WS_CHILD | SS_OWNERDRAW | SS_NOTIFY,
        0, 0, 0, 0, getParentHandle(), (HMENU)(unsigned long long)label.id, GetModuleHandle(0), 0);

    if (pApplication::useDark) {
        SetWindowTheme(hwnd, L"Explorer", NULL);
        pApplication::pAllowDarkModeForWindow(hwnd, true);
        SendMessageW(hwnd, WM_THEMECHANGED, 0, 0);
    }

    if (!doubleBuffer)
        doubleBuffer = new DoubleBuffer();
    doubleBuffer->release();

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

    auto* doubleBuffer = label->p.doubleBuffer;
    const auto& hMemDC = doubleBuffer->hMemDC;

    switch(msg) {
        case WM_GETDLGCODE:
            return DLGC_STATIC | DLGC_WANTCHARS;

        case WM_ERASEBKGND: {
            const auto* hdc = reinterpret_cast<HDC>(wparam);
            if (hdc != hMemDC)
                return 0;

            return 1;
        }
        case WM_PAINT: {
            break;
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            if (ps.rcPaint.right <= ps.rcPaint.left || ps.rcPaint.bottom <= ps.rcPaint.top) {
                EndPaint(hwnd, &ps);
                return 0;
            }

            RECT rcClient{};
            ::GetClientRect(hwnd, &rcClient);

            if (doubleBuffer->ensure(hdc, rcClient)) {
                const int savedState = ::SaveDC(hMemDC);
                IntersectClipRect(
                    hMemDC,
                    ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom
                );

                label->p.buildLabel(hwnd, hMemDC, rcClient);

                RestoreDC(hMemDC, savedState);

                BitBlt(
                    hdc, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right - ps.rcPaint.left,
                    ps.rcPaint.bottom - ps.rcPaint.top,
                    hMemDC, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY );
            }

            EndPaint(hwnd, &ps);
            return 0;
        }
    }

    return CallWindowProc(label->p.wndprocOrig, hwnd, msg, wparam, lparam);
}

auto pLabel::drawItem(LPDRAWITEMSTRUCT lDraw) -> void {
    auto hdc = lDraw->hDC;
    auto rc = lDraw->rcItem;

    buildLabel(hwnd, hdc, rc);
}

inline auto pLabel::buildLabel(HWND hwnd, HDC hdc, RECT& rc) -> void {
    SetBkMode(hdc, TRANSPARENT);

    HBRUSH brush = getBackgroundBrush();
    if (brush)
        FillRect(hdc, &rc, brush);

    SelectObject(hdc, hfont);
    unsigned length = GetWindowTextLength(hwnd);
    wchar_t* text = new wchar_t[length + 1];
    GetWindowText(hwnd, text, length + 1);
    text[length] = 0;
    DrawText(hdc, text, -1, &rc, DT_CALCRECT | DT_END_ELLIPSIS);
    unsigned _height = rc.bottom;
    GetClientRect(hwnd, &rc);
    rc.top = (rc.bottom - _height) / 2; //center vertically
    rc.bottom = rc.top + _height;
   
    if (!IsWindowEnabled(hwnd))
        SetTextColor(hdc, pApplication::useDark ? DARK_DISABLE_COL : GetSysColor(COLOR_GRAYTEXT));
    else if (label.overrideForegroundColor()) {
        unsigned color = label.foregroundColor();
        SetTextColor(hdc, RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff));
    } else if (pApplication::useDark)
        SetTextColor(hdc, DARK_FG_COL);

    UINT format = label.state.align == Label::Align::Left ? DT_LEFT : DT_RIGHT;

    DrawText(hdc, text, -1, &rc, format | DT_END_ELLIPSIS);

    delete[] text;
}
