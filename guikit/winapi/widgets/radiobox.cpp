
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
        0, 0, 0, 0, getParentHandle(), (HMENU)(unsigned long long)radioBox.id, GetModuleHandle(0), 0);

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&radioBox);
    wndprocOrig = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)subclassWndProc);    
}

auto CALLBACK pRadioBox::subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    RadioBox* radioBox = (RadioBox*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(radioBox == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);

    switch(msg) {   
        case WM_ERASEBKGND:
            return 0;
    }
    
    return CallWindowProc(radioBox->p.wndprocOrig, hwnd, msg, wparam, lparam);
}

auto pRadioBox::rebuild() -> void {
    if(!needRebuild())
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
