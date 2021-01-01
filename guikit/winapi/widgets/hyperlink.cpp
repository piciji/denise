
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
        getParentHandle(), (HMENU)(unsigned long long)hyperlink.id, GetModuleHandle(0), 0);
    
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&hyperlink);
    wndprocOrig = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)subclassWndProc);    
}

auto CALLBACK pHyperlink::subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    Hyperlink* hyperlink = (Hyperlink*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(hyperlink == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);

    switch(msg) {   
        case WM_ERASEBKGND:
            return 0;
    }
    
    return CallWindowProc(hyperlink->p.wndprocOrig, hwnd, msg, wparam, lparam);
}

auto pHyperlink::setText(std::string text) -> void {
    calculatedMinimumSize.updated = false;
    destroy(hwnd);
    rebuild();
    setGeometry( widget.geometry() );
}

auto pHyperlink::rebuild() -> void {
    if(!needRebuild())
        return;
    
    create();
    setFont( widget.font() );    
    pWidget::rebuild();
}

auto pHyperlink::setUri( std::string uri, std::string wrap ) -> void {
    destroy(hwnd);
    rebuild();
    setGeometry( widget.geometry() );
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
