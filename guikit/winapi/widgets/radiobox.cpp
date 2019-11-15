
auto pRadioBox::minimumSize() -> Size {
    //Size size = pFont::size(hfont, widget.text());
    Size size = getMinimumSize();
    return {size.width + 18, size.height + 4};
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
        L"BUTTON", L"",
        WS_CHILD | WS_TABSTOP | BS_RADIOBUTTON,
        0, 0, 0, 0, radioBox.window()->p.hwnd, (HMENU)radioBox.id, GetModuleHandle(0), 0);

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&radioBox);
}

auto pRadioBox::rebuild() -> void {
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
