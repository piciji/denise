
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

auto pWidget::setText(const std::string& text) -> void {
    if(hwnd) {
        SetWindowText(hwnd, utf16_t(text));
        calculatedMinimumSize.updated = false;
    }
}

auto pWidget::setEnabledThreaded(bool enabled) -> void {
    setEnabled(enabled);
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
    
    if(hwnd)
        SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    
    setTooltip(widget.tooltip());    
}

auto pWidget::getParentHandle() -> HWND {

    if (!parentTabFrameLayout || !pApplication::hasAppThemed())
		return widget.window() ? widget.window()->p.hwnd : nullptr;
	
	return parentTabFrameLayout->frameWidget->p.hwnd;
}

auto pWidget::getParentTabWidget() -> Widget* {

    if (!parentTabFrameLayout || !pApplication::hasAppThemed())
		return nullptr;
	
	return parentTabFrameLayout->frameWidget;
}

auto pWidget::needRebuild() -> bool {
    
    auto parent = TabFrameLayout::getParentTabFrame((Sizable*)&widget);

    if (parent != parentTabFrameLayout) {

        parentTabFrameLayout = parent;

        if (hwnd) {
            SetParent(hwnd, getParentHandle());
            return false;
        }
        
        return true;
        
    } else if (hwnd)
        return false;

    return true;
}


auto pWidget::setGeometry(Geometry geometry) -> void {
    if (!hwnd)
        return;

    Widget* parent = getParentTabWidget();

    if (parent) {
        auto geo = parent->geometry();

        geometry.x -= geo.x;
        geometry.y -= geo.y;
    }
    
    SetWindowPos(hwnd, NULL, geometry.x, geometry.y, geometry.width, geometry.height, SWP_NOZORDER | SWP_NOCOPYBITS);
    if(widget.onSize)
        widget.onSize();        
}

auto pWidget::setTooltip(std::string tooltip) -> void {
    if(!hwnd)
        return;
    
    if (!hwndTip) {
        if (tooltip.empty())
            return;
        createTooltip();
    }
    
    utf16_t wtooltip(tooltip);

    TOOLINFO toolInfo = { 0 };
    toolInfo.cbSize = sizeof(toolInfo);
    toolInfo.hwnd = GetParent(hwnd);

    while ( SendMessage(hwndTip, TTM_ENUMTOOLS, 0, (LPARAM)&toolInfo) )
        SendMessage(hwndTip, TTM_DELTOOL, 0, (LPARAM)&toolInfo);

    if(tooltip.empty())
        return;

    toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    toolInfo.uId = (UINT_PTR)hwnd;
    toolInfo.lpszText = wtooltip;

    SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
    SendMessage(hwndTip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 25000);
    SendMessage(hwndTip, TTM_SETMAXTIPWIDTH, 0, 800);
}

auto pWidget::createTooltip(bool useBallon) -> void {

    hwndTip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL,
        WS_POPUP | TTS_ALWAYSTIP | TTS_USEVISUALSTYLE | ( useBallon ? TTS_BALLOON : 0),
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        hwnd, NULL, GetModuleHandle(0), 0);    
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

auto pWidget::getBackgroundBrush() -> HBRUSH {

	static HBRUSH baseBrush = CreateSolidBrush( GetSysColor( COLOR_3DFACE ) );
	
	HBRUSH brush = baseBrush;
	
	if ( parentTabFrameLayout ) {
        if (pApplication::hasAppThemed())
			brush = pTabFrame::getTabBackgroundForControl( parentTabFrameLayout->frameWidget->p.hwnd, hwnd );
		
	} else if (widget.window()->p.brush)
		brush = widget.window()->p.brush;
	
	return brush;
}

auto pWidget::getColor(WPARAM wparam) -> HBRUSH {
    
    static HBRUSH brush =  GetSysColorBrush(COLOR_WINDOW);
    
    if (!widget.overrideForegroundColor())
        return nullptr;
    
    SetBkColor((HDC) wparam, GetSysColor(COLOR_WINDOW));

    unsigned color = widget.foregroundColor();
    SetTextColor((HDC) wparam, RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff));    
    return brush;
}