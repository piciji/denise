
auto pButton::minimumSize() -> Size {
    Size size = getMinimumSize();
    
    return {size.width + 20, size.height + 8};
}

auto pButton::create() -> void {
    destroy(hwnd);
    destroy(hwndTip);
    hwnd = CreateWindow(L"BUTTON", L"",
        WS_CHILD | WS_TABSTOP,
        0, 0, 0, 0, button.window()->p.hwnd, (HMENU)(unsigned long long)button.id, GetModuleHandle(0), 0);
    
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&button);
}

auto pButton::rebuild() -> void {
    create();
    setFont( widget.font() );
    setText(widget.text());
    pWidget::rebuild();
}

auto pButton::onActivate() -> void {    
    if(button.onActivate)
        button.onActivate();
}
