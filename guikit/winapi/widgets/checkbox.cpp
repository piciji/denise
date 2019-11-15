
auto pCheckBox::minimumSize() -> Size {
    //Size size = pFont::size(hfont, widget.text());
    Size size = getMinimumSize();
    return {size.width + 18, size.height + 4};
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
        0, 0, 0, 0, checkBox.window()->p.hwnd, (HMENU)checkBox.id, GetModuleHandle(0), 0
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
