
auto pStepButton::setGeometry(Geometry geometry) -> void {
    static Size containerSize = pWidget::getScaledContainerSize( {20, 8} );	
    
    if (!hwnd)
        return;
    
    Widget* parent = getParentTabWidget();

    if (parent) {
        auto geo = parent->geometry();

        geometry.x -= geo.x;
        geometry.y -= geo.y;
    }
        
    SetWindowPos(buddyHwnd, NULL, geometry.x, geometry.y, geometry.width - containerSize.width, geometry.height, SWP_NOZORDER | SWP_NOCOPYBITS);    
    
    geometry.x += geometry.width - containerSize.width;
    geometry.width = containerSize.width;
    
    SetWindowPos(hwnd, NULL, geometry.x, geometry.y, geometry.width, geometry.height, SWP_NOZORDER | SWP_NOCOPYBITS);
    if(widget.onSize)
        widget.onSize(); 
}

auto pStepButton::setFont(std::string font) -> void {
    
    pWidget::setFont( font );

    if (buddyHwnd)
        SendMessage(buddyHwnd, WM_SETFONT, (WPARAM) hfont, 0);
}

auto pStepButton::setVisible(bool visible) -> void {
    if(hwnd) {
        ShowWindow(buddyHwnd, visible ? SW_SHOWNORMAL : SW_HIDE);
        ShowWindow(hwnd, visible ? SW_SHOWNORMAL : SW_HIDE);
    }
}

auto pStepButton::setEnabled(bool enabled) -> void {
    if(hwnd) {
        EnableWindow(buddyHwnd, enabled);
		EnableWindow(hwnd, enabled);
    }
}

auto pStepButton::minimumSize() -> Size {
    static Size containerSize = pWidget::getScaledContainerSize( {36, 8} );	
    
    Size size = getMinimumSize();
    
    return {size.width + containerSize.width, size.height + 8};
}

auto pStepButton::setForegroundColor(unsigned color) -> void {
    if (hwnd)
        InvalidateRect(hwnd, 0, false);
}

auto pStepButton::create() -> void {
    destroy(hwnd);
    destroy(buddyHwnd);
    destroy(hwndTip);
        
    buddyHwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE, WC_EDIT, L"",
        WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_AUTOVSCROLL,
        0, 0, 0, 0, getParentHandle(), (HMENU)(unsigned long long)stepButton.id, GetModuleHandle(0), 0 );
    
    hwnd = CreateWindowEx( WS_EX_CLIENTEDGE, UPDOWN_CLASS, L"",
        WS_CHILD | WS_TABSTOP | UDS_WRAP | UDS_ARROWKEYS | UDS_ALIGNRIGHT | UDS_SETBUDDYINT,
        0, 0, 0, 0, getParentHandle(), (HMENU)(unsigned long long)stepButton.id, GetModuleHandle(0), 0);
    
    SetWindowLongPtr(buddyHwnd, GWLP_USERDATA, (LONG_PTR)&stepButton);
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&stepButton);
    
    //wndprocOrig = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)subclassWndProc);    
    wndprocOrig = (WNDPROC)SetWindowLongPtr(buddyHwnd, GWLP_WNDPROC, (LONG_PTR)subclassWndProcBuddy);    

    SendMessage(hwnd, UDM_SETPOS, 0L, MAKELONG(0, 0));
    SendMessage(hwnd, UDM_SETBUDDY, (WPARAM)(HWND)buddyHwnd, 0L);            
}

auto pStepButton::rebuild() -> void {
    if(!needRebuild())
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
    
    SendMessage(hwnd, UDM_SETRANGE, 0L, MAKELONG(stepButton.state.maxValue, stepButton.state.minValue));
}

auto pStepButton::setValue( int16_t value ) -> void {
    if (!hwnd)
        return;
        
    calculatedMinimumSize.updated = false;
    locked = true;
    SetWindowText(buddyHwnd, utf16_t(std::to_string(value)));
    SendMessage(hwnd, UDM_SETPOS, 0L, MAKELONG((int) (value), 0));
    locked = false;
}

auto pStepButton::onStep(LPARAM lparam) -> void {
    lockChangeWhileStepping = true;
    NMUPDOWN* lpnmud = (LPNMUPDOWN) lparam;
    
    int16_t newValue = lpnmud->iPos + lpnmud->iDelta;

    if (newValue < stepButton.state.minValue)
        newValue = stepButton.state.maxValue;
    else if (newValue > stepButton.state.maxValue)
        newValue = stepButton.state.minValue;

    stepButton.state.value = newValue;
    stepButton.Widget::state.text = std::to_string( newValue );

    if (stepButton.onChange)
        stepButton.onChange();                 
}

auto pStepButton::onChange() -> void {
    LRESULT lr = SendMessage(hwnd, UDM_GETPOS, 0, 0);
    
    if(HIWORD(lr) == 0) {
        stepButton.state.value = LOWORD(lr);
        stepButton.Widget::state.text = std::to_string( stepButton.state.value );
    }
        
    if (!locked && !lockChangeWhileStepping && stepButton.onChange)
        stepButton.onChange();
    
    lockChangeWhileStepping = false;
}

auto CALLBACK pStepButton::subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    StepButton* stepButton = (StepButton*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(stepButton == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);

    switch(msg) {
        case WM_ERASEBKGND: 
            return 0;
        
    }

    //return pApplication::wndProc(stepButton->p.wndprocOrig, hwnd, msg, wparam, lparam);
    return CallWindowProc(stepButton->p.wndprocOrig, hwnd, msg, wparam, lparam);
}

auto CALLBACK pStepButton::subclassWndProcBuddy(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    StepButton* stepButton = (StepButton*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(stepButton == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);

    switch(msg) {
        case WM_ERASEBKGND: 
            return 0;                    
    }

    //return pApplication::wndProc(stepButton->p.wndprocOrig, hwnd, msg, wparam, lparam);
    return CallWindowProc(stepButton->p.wndprocOrig, hwnd, msg, wparam, lparam);
}