
#if !defined(BP_PUSHBUTTON)
    #define BP_PUSHBUTTON 1
#endif

#if !defined(PBS_NORMAL)
    #define PBS_NORMAL 1
#endif

#if !defined(PBS_HOT)
    #define PBS_HOT 2
#endif

#if !defined(PBS_PRESSED)
    #define PBS_PRESSED 3
#endif

#if !defined(PBS_DISABLED)
    #define PBS_DISABLED 4
#endif

auto pButton::minimumSize() -> Size {
    Size size = getMinimumSize();

    if (!button.image())
        return {size.width + 20, size.height + 8};

    if (button.text().empty())
        return {button.image()->width + 20, button.image()->height + 10};

    return {size.width + button.image()->width + 20, size.height + 8};
}

auto pButton::create() -> void {
    destroy(hwnd);
    destroy(hwndTip);
    hwnd = CreateWindow(WC_BUTTON, L"",
        WS_CHILD | WS_TABSTOP,
        0, 0, 0, 0, getParentHandle(), (HMENU)(unsigned long long)button.id, GetModuleHandle(0), 0);
    
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&button);
    wndprocOrig = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)subclassWndProc);    
}

auto pButton::rebuild() -> void {		
    if(!needRebuild())
        return;
    
    create();
    setFont( widget.font() );
    setText( widget.text() );
    setImage( button.image() );
    pWidget::rebuild();
}

auto pButton::onActivate() -> void {    
    if(button.onActivate)
        button.onActivate();
}

auto pButton::setText(const std::string& text) -> void {
    if (!button.image())
        pWidget::setText(text);
    else
        calculatedMinimumSize.updated = false;
}

auto pButton::setEnabled(bool enabled) -> void {
    pWidget::setEnabled(enabled);
    if(button.image())
        RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

auto pButton::setEnabledThreaded(bool enabled) -> void {
    setEnabled(enabled);
}

auto pButton::setImage(Image* image) -> void {
    if (!image)
        return;

    if (hbitmap)
        DeleteObject(hbitmap);

    hbitmap = CreateBitmapWithPremultipliedAlpha( *image );
}

auto pButton::customDraw(HWND hwnd, PAINTSTRUCT& ps) -> void {
    RECT rc;
    GetClientRect(hwnd, &rc);
    auto buttonState = Button_GetState(hwnd);
    auto minSize = minimumSize();

    if(auto theme = pApplication::openThemeData(hwnd, L"BUTTON")) {
        pApplication::drawThemeParentBackground(hwnd, ps.hdc, &rc);
        unsigned flags = 0;
        if(buttonState & BST_PUSHED ) flags = PBS_PRESSED;
        else if(buttonState & BST_HOT) flags = PBS_HOT;
        else flags = button.enabled() ? PBS_NORMAL : PBS_DISABLED;
        pApplication::drawThemeBackground(theme, ps.hdc, BP_PUSHBUTTON, flags, &rc, &ps.rcPaint);
        pApplication::closeThemeData(theme);

    } else {
        FillRect(ps.hdc, &rc, GetSysColorBrush(COLOR_3DFACE));
        unsigned flags = (buttonState & BST_PUSHED ) ? DFCS_PUSHED : 0;
        DrawFrameControl(ps.hdc, &rc, DFC_BUTTON, DFCS_BUTTONPUSH | flags | (button.enabled() ? 0 : DFCS_INACTIVE));
    }

    if(button.image() && hbitmap) {
        auto& img = *button.image();
        HDC hdcSource = CreateCompatibleDC(ps.hdc);

        SelectBitmap(hdcSource, hbitmap);
        BLENDFUNCTION blend{AC_SRC_OVER, 0, (BYTE)(IsWindowEnabled(hwnd) ? 255 : 128), AC_SRC_ALPHA};
        int _x, _y = 0;

        if(button.text().empty()) {
            _x = 10;
            _y = 5;
        } else {
            _x = 8;
            _y = (minSize.height - img.height) >> 1;
        }

        AlphaBlend(
            ps.hdc, _x, _y, img.width, img.height,
            hdcSource, 0, 0, img.width, img.height, blend
        );

        DeleteDC(hdcSource);
    }

    if(!button.text().empty()) {
        SetBkMode(ps.hdc, TRANSPARENT);
        SetTextColor(ps.hdc, GetSysColor(IsWindowEnabled(hwnd) ? COLOR_BTNTEXT : COLOR_GRAYTEXT));
        SelectObject(ps.hdc, hfont);

        rc.left += button.image()->width + 10;
        rc.top += 4;

        DrawText(ps.hdc, utf16_t(button.text().c_str()), -1, &rc, DT_NOPREFIX | DT_END_ELLIPSIS);
    }
}

auto CALLBACK pButton::subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    Button* button = (Button*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(button == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);

    switch(msg) {
        case WM_PAINT: {
            if (button->image()) {
                PAINTSTRUCT ps;
                BeginPaint(hwnd, &ps);
                button->p.customDraw(hwnd, ps);
                EndPaint(hwnd, &ps);
                return false;
            } break;
        }

        case WM_ERASEBKGND:
            return 0;
    }
    
    return CallWindowProc(button->p.wndprocOrig, hwnd, msg, wparam, lparam);
}