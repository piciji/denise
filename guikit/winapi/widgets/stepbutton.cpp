
auto pStepButton::setGeometry(Geometry geometry) -> void {
    
    if (buddyHwnd) {
        SetWindowPos(buddyHwnd, NULL, geometry.x, geometry.y, geometry.width - 10, geometry.height, SWP_NOZORDER);
    }
    
    geometry.x = geometry.x + geometry.width - 10;
    geometry.width = 10;
    
    pWidget::setGeometry( geometry );
}

auto pStepButton::setFont(std::string font) -> void {
    
    pWidget::setFont( font );

    if (buddyHwnd)
        SendMessage(buddyHwnd, WM_SETFONT, (WPARAM) hfont, 0);
}

auto pStepButton::minimumSize() -> Size {
    Size size = getMinimumSize();
    
    return {size.width + 20, size.height + 8};
}

auto pStepButton::create() -> void {
    destroy(hwnd);
    destroy(buddyHwnd);
    destroy(hwndTip);
        
    buddyHwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE, WC_EDIT, L"",
        WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_AUTOVSCROLL,
        0, 0, 0, 0, stepButton.window()->p.hwnd, (HMENU)(unsigned long long)( (1 << 24) | stepButton.id), GetModuleHandle(0), 0 );
    
    hwnd = CreateWindowEx( WS_EX_CLIENTEDGE, UPDOWN_CLASS, L"",
        WS_CHILD | WS_TABSTOP | UDS_WRAP | UDS_ARROWKEYS | UDS_ALIGNRIGHT | UDS_SETBUDDYINT,
        0, 0, 0, 0, stepButton.window()->p.hwnd, (HMENU)(unsigned long long)stepButton.id, GetModuleHandle(0), 0);
    
    SetWindowLongPtr(buddyHwnd, GWLP_USERDATA, (LONG_PTR)&stepButton);
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&stepButton);

    SendMessage(hwnd, UDM_SETBUDDY, (WPARAM)(HWND)buddyHwnd, 0L);        
}

auto pStepButton::rebuild() -> void {
    if (hwnd)
        return;
    
    create();
    setFont( widget.font() );    
    updateRange();
    setValue( stepButton.state.value );
    
    if(buddyHwnd)
        SetWindowPos(buddyHwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    
    pWidget::rebuild();
}

auto pStepButton::updateRange() -> void {    
    if (!hwnd)
        return;
    
    SendMessage(hwnd, UDM_SETRANGE, 0L, MAKELONG(stepButton.state.minValue, stepButton.state.maxValue));
}

auto pStepButton::setValue( int16_t value ) -> void {
    if (!hwnd)
        return;
        
    calculatedMinimumSize.updated = false;
    SendMessage(hwnd, UDM_SETPOS, 0L, MAKELONG((int) (value), 0));
}

auto pStepButton::onStep() -> void {
    
    LRESULT lr = SendMessage(hwnd, UDM_GETPOS, 0, 0);
    
    if(HIWORD(lr) == 0) {
        
        int16_t newValue = LOWORD(lr);
        stepButton.Widget::state.text = std::to_string( newValue );
        
        if ( newValue > stepButton.state.value) {
         
            stepButton.state.value = newValue;
            
            if(stepButton.onStepUp)
                stepButton.onStepUp();
            
        } else if ( newValue < stepButton.state.value) {
            stepButton.state.value = newValue;
            
            if(stepButton.onStepDown)
                stepButton.onStepDown();
        }                
    }    
    
}
