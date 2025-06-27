
auto pCheckBox::minimumSize() -> Size {
    Size size = getMinimumSize();
	
	static Size containerSize = pWidget::getScaledContainerSize( {18, 2} );
	
	return {size.width + containerSize.width, size.height + containerSize.height};
}

auto pCheckBox::setChecked(bool checked) -> void {
    if(hwnd)
        SendMessage(hwnd, BM_SETCHECK, (WPARAM)checked, 0);
}

auto pCheckBox::onToggle() -> void {
    checkBox.state.checked ^= 1;
    setChecked(checkBox.state.checked);
    
    if(checkBox.onToggle)
        checkBox.onToggle(checkBox.state.checked);
}

auto pCheckBox::create() -> void {
    destroy(hwnd);
    destroy(hwndTip);
    
    hwnd = CreateWindow(
        WC_BUTTON, L"",
        WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
        0, 0, 0, 0, getParentHandle(), (HMENU)(unsigned long long)checkBox.id, GetModuleHandle(0), 0
    );

    if (pApplication::useDark) {
        SetWindowTheme(hwnd, L"Explorer", nullptr);
        pApplication::pAllowDarkModeForWindow(hwnd, true);
        SendMessageW(hwnd, WM_THEMECHANGED, 0, 0);
    }

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&checkBox);
    wndprocOrig = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)subclassWndProc);    
}

auto pCheckBox::onCustomDraw(LPARAM lparam) -> LRESULT {
    LPNMCUSTOMDRAW lpcd = (LPNMCUSTOMDRAW)lparam;

    switch(lpcd->dwDrawStage) {
        case CDDS_PREPAINT:
            if (checkBox.overrideForegroundColor() || pApplication::useDark) {
                const int textLength = ::GetWindowTextLength(lpcd->hdr.hwndFrom);
                if (textLength > 0) {
                    TCHAR* buttonText = new TCHAR[textLength+1];
                    //SIZE dimensions = {0};
                    ::GetWindowText(lpcd->hdr.hwndFrom, buttonText, textLength+1);
                    //::GetTextExtentPoint32(lpcd->hdc, buttonText, textLength, &dimensions);
                    //const int xPos = (lpcd->rc.right - dimensions.cx) / 2;
                    //const int yPos = (lpcd->rc.bottom - dimensions.cy) / 2;
                    static Size containerSize = pWidget::getScaledContainerSize( {16, 1} );
                    ::SetBkMode(lpcd->hdc, TRANSPARENT);
                    auto color = checkBox.foregroundColor();
                    if (pApplication::useDark) {
                        if (checkBox.enabled())
                            ::SetTextColor(lpcd->hdc, DARK_FG_COL);
                        else
                            ::SetTextColor(lpcd->hdc, DARK_DISABLE_COL);
                    } else
                        ::SetTextColor(lpcd->hdc, RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff) );
                    ::TextOut(lpcd->hdc, containerSize.width, containerSize.height, buttonText, textLength);
                    delete[] buttonText;
                    return CDRF_SKIPDEFAULT;
                }
            }
    }
    return CDRF_DODEFAULT;
}

auto CALLBACK pCheckBox::subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    CheckBox* checkBox = (CheckBox*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(checkBox == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);

    switch(msg) {   
        case WM_ERASEBKGND:
            return 0;
    }
    
    return CallWindowProc(checkBox->p.wndprocOrig, hwnd, msg, wparam, lparam);
}

auto pCheckBox::rebuild() -> void {
    if(!needRebuild())
        return;
    
    create();
    setFont( widget.font() );
    setChecked(checkBox.state.checked);
    setText(widget.text());
    pWidget::rebuild();
}
