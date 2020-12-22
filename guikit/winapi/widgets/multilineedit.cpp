
auto pMultilineEdit::create() -> void {
    destroy(hwnd);
    destroy(hwndTip);
    
    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE, WC_EDIT, L"",
        WS_CHILD | WS_TABSTOP | ES_AUTOVSCROLL | WS_VSCROLL | ES_MULTILINE,
        0, 0, 0, 0, multilineEdit.window()->p.hwnd, (HMENU)(unsigned long long)multilineEdit.id, GetModuleHandle(0), 0 );

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&multilineEdit);
}

auto pMultilineEdit::minimumSize() -> Size {
    Size size = getMinimumSize();
    return {size.width + 16, size.height + 6};
}

auto pMultilineEdit::setEditable(bool editable) -> void {
    if(hwnd)
        SendMessage(hwnd, EM_SETREADONLY, !editable, 0);
}

auto pMultilineEdit::setText(std::string text) -> void {
    locked = true;
    pWidget::setText(text);
    locked = false;
}

auto pMultilineEdit::text() -> std::string {
    if (!hwnd)
        return widget.text();
    
    unsigned length = GetWindowTextLength(hwnd);
    wchar_t text[length + 1];
    GetWindowText(hwnd, text, length + 1);
    text[length] = 0;
    return utf8_t(text);
}

auto pMultilineEdit::setMaxLength( unsigned maxLength ) -> void {
    if(hwnd)
        SendMessage(hwnd, EM_SETLIMITTEXT, maxLength, 0);
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
    if (hwnd)
        return;
    
    widget.state.text = text();
    create();
    setFont( widget.font() );
    setEditable(multilineEdit.editable());
    setText(widget.text());
    setMaxLength( multilineEdit.maxLength() );
    pWidget::rebuild();
}
