
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
        L"BUTTON", L"",
        WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
        0, 0, 0, 0, checkBox.window()->p.hwnd, (HMENU)(unsigned long long)checkBox.id, GetModuleHandle(0), 0
    );
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&checkBox);
}

auto pCheckBox::rebuild() -> void {
    create();
    setFont( widget.font() );
    setChecked(checkBox.state.checked);
    setText(widget.text());
    pWidget::rebuild();
}
