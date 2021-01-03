
auto pButton::minimumSize() -> Size {
    Size size = getMinimumSize();
    
    return {size.width + 20, size.height + 8};
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
    pWidget::rebuild();
}

auto pButton::onActivate() -> void {    
    if(button.onActivate)
        button.onActivate();
}

auto CALLBACK pButton::subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    Button* button = (Button*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(button == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);

    switch(msg) {   
        case WM_ERASEBKGND:
            return 0;
    }
    
    return CallWindowProc(button->p.wndprocOrig, hwnd, msg, wparam, lparam);
}