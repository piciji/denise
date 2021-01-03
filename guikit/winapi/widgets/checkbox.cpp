
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
        checkBox.onToggle();
}

auto pCheckBox::create() -> void {
    destroy(hwnd);
    destroy(hwndTip);
    
    hwnd = CreateWindow(
        WC_BUTTON, L"",
        WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
        0, 0, 0, 0, getParentHandle(), (HMENU)(unsigned long long)checkBox.id, GetModuleHandle(0), 0
    );
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&checkBox);
    wndprocOrig = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)subclassWndProc);    
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
