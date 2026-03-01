
auto pRadioBox::minimumSize() -> Size {
	Size size = getMinimumSize();
	
	static Size containerSize = pWidget::getScaledContainerSize( {18, 2} );
	
	return {size.width + containerSize.width, size.height + containerSize.height};
}

auto pRadioBox::setChecked() -> void {
    for(auto& item : radioBox.state.group) {
        if(!item->p.hwnd)
            continue;
        
        SendMessage(item->p.hwnd, BM_SETCHECK, (WPARAM)(item == &radioBox), 0);
    }
}

auto pRadioBox::create() -> void {
    destroy(hwnd);
    destroy(hwndTip);
    
    hwnd = CreateWindow(
        WC_BUTTON, L"",
        WS_CHILD | WS_TABSTOP | BS_RADIOBUTTON,
        0, 0, 0, 0, getParentHandle(), (HMENU)(unsigned long long)radioBox.id, GetModuleHandle(0), 0);

    if (pApplication::useDark) {
        SetWindowTheme(hwnd, L"Explorer", nullptr);
        pApplication::pAllowDarkModeForWindow(hwnd, true);
        SendMessageW(hwnd, WM_THEMECHANGED, 0, 0);
    }

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&radioBox);
    wndprocOrig = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)subclassWndProc);    
}

auto CALLBACK pRadioBox::subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    RadioBox* radioBox = (RadioBox*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(radioBox == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);

    switch(msg) {   
        case WM_ERASEBKGND:
            return 0;
    }
    
    return CallWindowProc(radioBox->p.wndprocOrig, hwnd, msg, wparam, lparam);
}

auto pRadioBox::onCustomDraw(LPARAM lparam) -> LRESULT {
    LPNMCUSTOMDRAW lpcd = (LPNMCUSTOMDRAW)lparam;

    switch (lpcd->dwDrawStage) {
    case CDDS_PREPAINT:
        if (radioBox.overrideForegroundColor() || pApplication::useDark) {
            const int textLength = ::GetWindowTextLength(lpcd->hdr.hwndFrom);
            if (textLength > 0) {
                TCHAR* buttonText = new TCHAR[textLength + 1];
                ::GetWindowText(lpcd->hdr.hwndFrom, buttonText, textLength + 1);
                static Size containerSize = pWidget::getScaledContainerSize({ 16, 1 });
                ::SetBkMode(lpcd->hdc, TRANSPARENT);
                auto color = radioBox.foregroundColor();
                if (radioBox.overrideForegroundColor())
                    ::SetTextColor(lpcd->hdc, RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff));
                else {
                    if (radioBox.enabled())
                        ::SetTextColor(lpcd->hdc, DARK_FG_COL);
                    else
                        ::SetTextColor(lpcd->hdc, DARK_DISABLE_COL);
                }

                ::TextOut(lpcd->hdc, containerSize.width, containerSize.height, buttonText, textLength);
                delete[] buttonText;
                return CDRF_SKIPDEFAULT;
            }
        }
    }
    return CDRF_DODEFAULT;
}

auto pRadioBox::setForegroundColor(unsigned color) -> void {
    if (hwnd)
        InvalidateRect(hwnd, 0, false);
}

auto pRadioBox::rebuild() -> void {
    if(!needRebuild())
        return;
    
    create();
    setFont( widget.font() );
    if(radioBox.state.checked)
        setChecked();
    
    setText(widget.text());
    pWidget::rebuild();
}

auto pRadioBox::onActivate() -> void {
    if(radioBox.state.checked)
        return;
    
    radioBox.setChecked();
    
    if(radioBox.onActivate)
        radioBox.onActivate();
}
