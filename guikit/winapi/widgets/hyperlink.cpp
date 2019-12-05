
auto pHyperlink::minimumSize() -> Size {
    Size size = getMinimumSize();
    return {size.width, size.height};
}

auto pHyperlink::create() -> void {
    destroy(hwnd);
    destroy(hwndTip);
    
    hwnd = CreateWindowEx(0, WC_LINK, 
        utf16_t( generate() ),
        WS_CHILD | WS_TABSTOP, 
        0, 0, 0, 0, 
        hyperlink.window()->p.hwnd, (HMENU)hyperlink.id, GetModuleHandle(0), 0);
    
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&hyperlink);
}

auto pHyperlink::setText(std::string text) -> void {
    calculatedMinimumSize.updated = false;
    rebuild();
}

auto pHyperlink::rebuild() -> void {
    create();
    setFont( widget.font() );    
    pWidget::rebuild();
}

auto pHyperlink::setUri( std::string uri, std::string wrap ) -> void {
    rebuild();
}

auto pHyperlink::generate() -> std::string {
    std::string link = "";
    std::string text = hyperlink.text();
    std::string uri = hyperlink.uri();
    std::string wrap = hyperlink.wrap();

    if (wrap.empty())
        wrap = uri;

    if (text.empty())
        link = "<A HREF=\"" + uri + "\">" + wrap + "</A>";
    else {

        if (String::foundSubStr(text, wrap))
            link = String::replace(text, wrap, "<A HREF=\"" + uri + "\">" + wrap + "</A>");
        else
            link = "<A HREF=\"" + uri + "\">" + text + "</A>";
    }
    
    return link;
}
