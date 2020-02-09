
pWidget::pWidget(Widget& widget) : widget(widget), hwnd(0), hwndTip(0) {
    setFont(Font::system());
}

pWidget::~pWidget() {
    destroy(hwndTip);
    destroy(hwnd);
    destroyImageList();
    pFont::free(hfont);
}

auto pWidget::destroy(HWND& handle) -> void {
    if(handle)
        DestroyWindow(handle);
    
    handle = 0;
}

auto pWidget::destroyImageList() -> void {
    if(imageList)
        ImageList_Destroy(imageList);
    
    imageList = nullptr;
}

auto pWidget::focused() -> bool {
    return GetFocus() == hwnd;
}

auto pWidget::setFocused() -> void {
    if(hwnd) {
        locked = true;
        SetFocus(hwnd);
        locked = false;
    }
}

auto pWidget::setFont(std::string font) -> void {
    pFont::free(hfont);
    hfont = pFont::create(font);
    calculatedMinimumSize.updated = false;
    if(hwnd)
        SendMessage(hwnd, WM_SETFONT, (WPARAM)hfont, 0);
    
    if(hwndTip)
        SendMessage(hwndTip, WM_SETFONT, (WPARAM)hfont, 0);
}

inline auto pWidget::getMinimumSize() -> Size {
    if (calculatedMinimumSize.updated)
        return calculatedMinimumSize.minimumSize;        
    
    calculatedMinimumSize.minimumSize = pFont::size(hfont, widget.text());
    
    calculatedMinimumSize.updated = true;
    
    return calculatedMinimumSize.minimumSize;
}

auto pWidget::setText(std::string text) -> void {
    if(hwnd) {
        SetWindowText(hwnd, utf16_t(text));
        calculatedMinimumSize.updated = false;
    }
}

auto pWidget::setEnabled(bool enabled) -> void {
    if(hwnd)
		EnableWindow(hwnd, enabled);
}

auto pWidget::setVisible(bool visible) -> void {
    if(hwnd)
        ShowWindow(hwnd, visible ? SW_SHOWNORMAL : SW_HIDE);
}

auto pWidget::rebuild() -> void {
    widget.setEnabled(widget.enabled());
    widget.setVisible(widget.visible());
    
    if(!dynamic_cast<pFrame*>(this)) {
        if(hwnd)
            SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
    setTooltip(widget.tooltip());
}

auto pWidget::setGeometry(Geometry geometry) -> void {
    if (!hwnd)
        return;
    
    SetWindowPos(hwnd, NULL, geometry.x, geometry.y, geometry.width, geometry.height, SWP_NOZORDER);
    if(widget.onSize)
        widget.onSize();
}

auto pWidget::setTooltip(std::string tooltip) -> void {
    if(!hwnd || tooltip.empty())
        return;
    
    if (!hwndTip)
        createTooltip();
    
    utf16_t wtooltip(tooltip);

    TOOLINFO toolInfo = { 0 };
    toolInfo.cbSize = sizeof(toolInfo);
    toolInfo.hwnd = GetParent(hwnd);
    toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    toolInfo.uId = (UINT_PTR)hwnd;
    toolInfo.lpszText = wtooltip;

    SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
}

auto pWidget::createTooltip() -> void {

    hwndTip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL,
        WS_POPUP |TTS_ALWAYSTIP | TTS_BALLOON,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        GetParent(hwnd), NULL, GetModuleHandle(0), 0);
}

auto pWidget::getScaledContainerSize( Size size ) -> Size {
	static float dpiX = pFont::dpi().x;
	static float dpiY = pFont::dpi().y;
	
	if (size.width == Size::Minimum || size.width == Size::Maximum);
	else
		size.width = (unsigned)((float)(size.width) * dpiX / 96.0);
	
	if (size.height == Size::Minimum || size.height == Size::Maximum);
	else
		size.height = (unsigned)((float)(size.height) * dpiY / 96.0);
	
	return size;
}

auto pWidget::getScaledDim( unsigned value ) -> unsigned {
	static float dpiY = pFont::dpi().y;
	
	return (unsigned)((float)(value) * dpiY / 96.0);
}