
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
        0, 0, 0, 0, radioBox.window()->p.hwnd, (HMENU)(unsigned long long)radioBox.id, GetModuleHandle(0), 0);

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&radioBox);
}

auto pRadioBox::rebuild() -> void {
    if (hwnd)
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
