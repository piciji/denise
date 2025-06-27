
auto pCheckButton::minimumSize() -> Size {
    Size size = getMinimumSize();
	
    return {size.width + 20, size.height + 8};
}

auto pCheckButton::setChecked(bool checked) -> void {
    if(hwnd) 
        SendMessage(hwnd, BM_SETCHECK, (WPARAM)checked, 0);
}

auto pCheckButton::create() -> void {
    destroy(hwnd);
    destroy(hwndTip);
    
    hwnd = CreateWindow(WC_BUTTON, L"",
        WS_CHILD | WS_TABSTOP | BS_CHECKBOX | BS_PUSHLIKE,
        0, 0, 0, 0, getParentHandle(), (HMENU)(unsigned long long)checkButton.id, GetModuleHandle(0), 0);

    if (pApplication::useDark) {
        SetWindowTheme(hwnd, L"Explorer", NULL);
        pApplication::pAllowDarkModeForWindow(hwnd, true);
        SendMessageW(hwnd, WM_THEMECHANGED, 0, 0);
    }
    
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&checkButton);  
    wndprocOrig = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)subclassWndProc);    
}

auto CALLBACK pCheckButton::subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    CheckButton* checkButton = (CheckButton*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(checkButton == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);

    switch(msg) {   
        case WM_ERASEBKGND:
            return 0;
    }
    
    return CallWindowProc(checkButton->p.wndprocOrig, hwnd, msg, wparam, lparam);
}

auto pCheckButton::rebuild() -> void {
    if(!needRebuild())
        return;
        
    create();
    setFont( widget.font() );
    setChecked(checkButton.state.checked);
    setText(widget.text());
    pWidget::rebuild();
}

auto pCheckButton::onToggle() -> void {
    checkButton.state.checked ^= 1;
    setChecked(checkButton.state.checked);
    
    if(checkButton.onToggle)
        checkButton.onToggle();
}
