
auto pComboButton::append(std::string text) -> void {
    if(!hwnd)
        return;
    
    SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(wchar_t*)utf16_t(text));
    
    if(SendMessage(hwnd, CB_GETCOUNT, 0, 0) == 1)
        setSelection(0);
    
    calculatedMinimumSize.updated = false;
}

auto pComboButton::minimumSize() -> Size {

	static Size containerSize = pWidget::getScaledContainerSize( {24, 8} );	
	
    if (calculatedMinimumSize.updated)
        return calculatedMinimumSize.minimumSize; 
    
    unsigned maximumWidth = 0;
    for(auto& text : comboButton.state.rows)
        maximumWidth = std::max(maximumWidth, pFont::size(hfont, text).width);
    
    calculatedMinimumSize.updated = true;
    
    calculatedMinimumSize.minimumSize = {maximumWidth + containerSize.width,
											pFont::size(hfont, " ").height + 8}; // don't use scaled height
	
    return calculatedMinimumSize.minimumSize;
}

auto pComboButton::remove(unsigned selection) -> void {
    if(!hwnd)
        return;
    
    SendMessage(hwnd, CB_DELETESTRING, selection, 0);
    if(selection == comboButton.state.selection)
        comboButton.setSelection(0);
}

auto pComboButton::reset() -> void {
    if(hwnd)
        SendMessage(hwnd, CB_RESETCONTENT, 0, 0);
}

auto pComboButton::setGeometry(Geometry geometry) -> void {
    if(!hwnd)
        return;
    
    pWidget::setGeometry({geometry.x, geometry.y, geometry.width, 1});
    RECT rc;
    GetWindowRect(hwnd, &rc);
    unsigned adjustedHeight = geometry.height - ((rc.bottom - rc.top) - 
        SendMessage(hwnd, CB_GETITEMHEIGHT, (WPARAM)-1, 0));
    
    SendMessage(hwnd, CB_SETITEMHEIGHT, (WPARAM)-1, adjustedHeight);
}

auto pComboButton::setSelection(unsigned selection) -> void {
    if(hwnd)
        SendMessage(hwnd, CB_SETCURSEL, selection, 0);
}

auto pComboButton::setText(unsigned selection, std::string text) -> void {
    if(!hwnd)
        return;
    
    SendMessage(hwnd, CB_DELETESTRING, selection, 0);
    SendMessage(hwnd, CB_INSERTSTRING, selection, (LPARAM)(wchar_t*)utf16_t(text));
    setSelection(comboButton.state.selection);
    calculatedMinimumSize.updated = false;
}

auto pComboButton::create() -> void {
    destroy(hwnd);
    destroy(hwndTip);
    
    hwnd = CreateWindow(
        WC_COMBOBOX, L"",
        WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_HASSTRINGS,
        0, 0, 0, 0,
        getParentHandle(), (HMENU)(unsigned long long)comboButton.id, GetModuleHandle(0), 0
    );
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&comboButton);
    wndprocOrig = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)subclassWndProc);
}

auto CALLBACK pComboButton::subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    ComboButton* comboButton = (ComboButton*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(comboButton == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);

    switch(msg) {   
        case WM_ERASEBKGND:
            return 0;
    }
    
    return CallWindowProc(comboButton->p.wndprocOrig, hwnd, msg, wparam, lparam);
}

auto pComboButton::rebuild() -> void {
    if(!needRebuild())
        return;
        
    create();
    setFont( widget.font() );
    for(auto& text : comboButton.state.rows)
        append(text);
    
    setSelection(comboButton.state.selection);
    pWidget::rebuild();
}

auto pComboButton::onChange() -> void {
    unsigned selection = SendMessage(hwnd, CB_GETCURSEL, 0, 0);
    if(selection == comboButton.state.selection)
        return;
    
    comboButton.state.selection = selection;
    if(comboButton.onChange)
        comboButton.onChange();
}
