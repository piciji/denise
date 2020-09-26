
auto pFrame::minimumSize() -> Size {
    Size size = getMinimumSize();
    
    size.width += 4 + (borderSize() << 1);
    size.height >>= 1;
    if (widget.text().empty()) size.height = 0;
	
    size.height += borderSize() << 1;
    return size;
}

auto pFrame::setGeometry(Geometry geometry) -> void {
    Size size = pFont::size(hfont, widget.text());
    size.height >>= 1;
    if (!widget.text().empty()) size.height = 0;
    geometry.y -= size.height; geometry.height += size.height;

    pWidget::setGeometry(geometry);
}

auto pFrame::setText(std::string text) -> void {
    pWidget::setText( text );
    if (widget.visible()) { //to update frame border display
        widget.setVisible(false);
        widget.setVisible(true);
    }
}

auto pFrame::create() -> void {
    destroy(hwnd);
    hwnd = CreateWindow(L"BUTTON", L"",
        WS_CHILD | BS_GROUPBOX,
        0, 0, 0, 0, widget.window()->p.hwnd, (HMENU)(unsigned long long)widget.id, GetModuleHandle(0), 0);

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&widget);
}

auto pFrame::rebuild() -> void {
    if (hwnd)
        return;
        
    create();        
    setFont( widget.font() );
    pWidget::setText(widget.text());
    pWidget::rebuild();
}
