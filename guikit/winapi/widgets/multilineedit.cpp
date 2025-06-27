
auto pMultilineEdit::create() -> void {
    destroy(hwnd);
    destroy(hwndTip);
    
    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE, WC_EDIT, L"",
        WS_CHILD | WS_TABSTOP | WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_WANTRETURN,
        0, 0, 0, 0, getParentHandle(), (HMENU)(unsigned long long)multilineEdit.id, GetModuleHandle(0), 0 );

    if (pApplication::useDark) {
        SetWindowTheme(hwnd, L"Explorer", NULL);
        pApplication::pAllowDarkModeForWindow(hwnd, true);
        SendMessageW(hwnd, WM_THEMECHANGED, 0, 0);
    }

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&multilineEdit);
    
    //wndprocOrig = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)subclassWndProc);    
}

auto pMultilineEdit::setEditable(bool editable) -> void {
    if(hwnd)
        SendMessage(hwnd, EM_SETREADONLY, !editable, 0);
}

auto pMultilineEdit::setForegroundColor(unsigned color) -> void {
    if (hwnd)
        InvalidateRect(hwnd, 0, false);
}

auto pMultilineEdit::setText(const std::string& text) -> void {
    locked = true;
    std::string _text = text;
    GUIKIT::String::replace(_text, "\n", "\r\n");
    pWidget::setText(_text);
    locked = false;
}

auto pMultilineEdit::text() -> std::string {
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

auto pMultilineEdit::onChange() -> void {
    if(!locked && multilineEdit.onChange)
        multilineEdit.onChange();
}

auto pMultilineEdit::onFocus() -> void {
    if(!locked && multilineEdit.onFocus)
        multilineEdit.onFocus();
}

auto pMultilineEdit::rebuild() -> void {
    if(!needRebuild())
        return;
    
    widget.state.text = text();
    create();
    setFont( widget.font() );
    
    setEditable(multilineEdit.editable());
    setText(widget.text());
    pWidget::rebuild();
}

auto CALLBACK pMultilineEdit::subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    MultilineEdit* multilineEdit = (MultilineEdit*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(multilineEdit == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);
    Window* window = (Window*)multilineEdit->Sizable::state.window;
    if(window == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);

    switch(msg) {
        case WM_ERASEBKGND: 
            return 0;

    }

    return CallWindowProc(multilineEdit->p.wndprocOrig, hwnd, msg, wparam, lparam);
}