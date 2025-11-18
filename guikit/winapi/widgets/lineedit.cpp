
auto pLineEdit::create() -> void {
    destroy(hwnd);
    destroy(hwndTip);
    
    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE, WC_EDIT, L"",
        WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_AUTOVSCROLL,
        0, 0, 0, 0, getParentHandle(), (HMENU)(unsigned long long)lineEdit.id, GetModuleHandle(0), 0 );

    if (pApplication::useDark) {
        SetWindowTheme(hwnd, L"CFD", NULL);
        pApplication::pAllowDarkModeForWindow(hwnd, true);
        SendMessageW(hwnd, WM_THEMECHANGED, 0, 0);
    }

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&lineEdit);
    wndprocOrig = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)subclassWndProc);    
}

auto pLineEdit::setDroppable(bool droppable) -> void {
    if (hwnd)
        DragAcceptFiles(hwnd, droppable);
}

auto pLineEdit::minimumSize() -> Size {
    Size size = getMinimumSize();
    return {size.width + 16, size.height + 6};
}

auto pLineEdit::setEditable(bool editable) -> void {
    if(hwnd)
        SendMessage(hwnd, EM_SETREADONLY, !editable, 0);
}

auto pLineEdit::setPlaceholder(const std::string& placeholder) -> void {
    if(hwnd)
        SendMessage(hwnd, EM_SETCUEBANNER, false, (LPARAM)(wchar_t*)utf16_t(placeholder));
}

auto pLineEdit::setText(const std::string& text) -> void {
    locked = true;
    pWidget::setText(text);
    locked = false;
}

auto pLineEdit::text() -> std::string {
    if (!hwnd)
        return widget.text();
    
    unsigned length = GetWindowTextLength(hwnd);
    wchar_t* text = new wchar_t[length + 1];
    GetWindowText(hwnd, text, length + 1);
    text[length] = 0;
    std::string out = utf8_t(text);
    delete[] text;
    return out;
}

auto pLineEdit::setMaxLength( unsigned maxLength ) -> void {
    if(hwnd)
        SendMessage(hwnd, EM_SETLIMITTEXT, maxLength, 0);
}

auto pLineEdit::setAlign( LineEdit::Align align ) -> void {
    if (!hwnd)
        return;

    if (align == LineEdit::Align::Right)
        SetWindowLong(hwnd, GWL_STYLE, (GetWindowLong(hwnd, GWL_STYLE) | ES_RIGHT));
    else
        SetWindowLong(hwnd, GWL_STYLE, (GetWindowLong(hwnd, GWL_STYLE) & ~ES_RIGHT));
}

auto pLineEdit::onChange() -> void {
    if(!locked && lineEdit.onChange)
        lineEdit.onChange();
}

auto pLineEdit::onFocus() -> void {
    if(!locked && lineEdit.onFocus)
        lineEdit.onFocus();
}

auto pLineEdit::setForegroundColor(unsigned color) -> void {
    if (hwnd)
        InvalidateRect(hwnd, 0, false);
}

auto pLineEdit::rebuild() -> void {
    if(!needRebuild())
        return;
    
    widget.state.text = text();
    create();
    setFont( widget.font() );
    setEditable(lineEdit.editable());
    setDroppable(lineEdit.droppable());
    setAlign( lineEdit.align() );
    setText(widget.text());
    setPlaceholder(lineEdit.placeholder());
    setMaxLength( lineEdit.maxLength() );
    pWidget::rebuild();
}

auto CALLBACK pLineEdit::subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    LineEdit* lineEdit = (LineEdit*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(lineEdit == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);
    Window* window = (Window*)lineEdit->Sizable::state.window;
    if(window == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);

    switch(msg) {
        case WM_ERASEBKGND: 
            return 0;
        case WM_DROPFILES: {
            std::vector<std::string> paths = getDropPaths(wparam);
        
            if(!paths.empty() && lineEdit->onDrop)
                lineEdit->onDrop(paths);        
            
            return false;
        }
        case WM_KEYUP:
            if (wparam == VK_RETURN) {
                if (lineEdit->onReturn)
                    lineEdit->onReturn();
            } break;
    }

    return CallWindowProc(lineEdit->p.wndprocOrig, hwnd, msg, wparam, lparam);
}