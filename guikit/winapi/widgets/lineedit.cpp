
auto pLineEdit::create() -> void {
    destroy(hwnd);
    destroy(hwndTip);
    
    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_AUTOVSCROLL,
        0, 0, 0, 0, lineEdit.window()->p.hwnd, (HMENU)lineEdit.id, GetModuleHandle(0), 0 );

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&lineEdit);
    wndprocOrig = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)subclassWndProc);    
}

auto pLineEdit::setDroppable(bool droppable) -> void {
    if (hwnd)
        DragAcceptFiles(hwnd, droppable);
}

auto pLineEdit::minimumSize() -> Size {
    //Size size = pFont::size(hfont, widget.text());
    Size size = getMinimumSize();
    return {size.width + 14, size.height + 6};
}

auto pLineEdit::setEditable(bool editable) -> void {
    if(hwnd)
        SendMessage(hwnd, EM_SETREADONLY, !editable, 0);
}

auto pLineEdit::setText(std::string text) -> void {
    locked = true;
    pWidget::setText(text);
    locked = false;
}

auto pLineEdit::text() -> std::string {
    if (!hwnd)
        return widget.text();
    
    unsigned length = GetWindowTextLength(hwnd);
    wchar_t text[length + 1];
    GetWindowText(hwnd, text, length + 1);
    text[length] = 0;
    return utf8_t(text);
}

auto pLineEdit::setMaxLength( unsigned maxLength ) -> void {
    if(hwnd)
        SendMessage(hwnd, EM_SETLIMITTEXT, maxLength, 0);
}

auto pLineEdit::onChange() -> void {
    if(!locked && lineEdit.onChange)
        lineEdit.onChange();
}

auto pLineEdit::onFocus() -> void {
    if(!locked && lineEdit.onFocus)
        lineEdit.onFocus();
}

auto pLineEdit::rebuild() -> void {
    widget.state.text = text();
    create();
    setFont( widget.font() );
    setEditable(lineEdit.editable());
    setDroppable(lineEdit.droppable());
    setText(widget.text());
    setMaxLength( lineEdit.maxLength() );
    pWidget::rebuild();
}

auto CALLBACK pLineEdit::subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    LineEdit* lineEdit = (LineEdit*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(lineEdit == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);
    Window* window = (Window*)lineEdit->Sizable::state.window;
    if(window == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);

    if(msg == WM_DROPFILES) {
        std::vector<std::string> paths = getDropPaths(wparam);
        
        if(!paths.empty() && lineEdit->onDrop)
            lineEdit->onDrop(paths);        
        
        return false;
    }

    return CallWindowProc(lineEdit->p.wndprocOrig, hwnd, msg, wparam, lparam);
}