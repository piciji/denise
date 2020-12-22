
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
        0, 0, 0, 0, checkButton.window()->p.hwnd, (HMENU)(unsigned long long)checkButton.id, GetModuleHandle(0), 0);
    
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&checkButton);    
}

auto pCheckButton::rebuild() -> void {
    if (hwnd)
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
