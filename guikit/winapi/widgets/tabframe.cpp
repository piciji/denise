
HBRUSH pTabFrame::bkgndBrush = nullptr;

auto pTabFrame::borderSize() -> unsigned {
    return hasAppThemed() ? 1 : 2;
}

auto pTabFrame::minimumSize() -> Size {
    if (calculatedMinimumSize.updated)
        return calculatedMinimumSize.minimumSize; 
    
    std::string text = tabFrame.text(0);
    Size size = pFont::size(hfont, text);
    
    calculatedMinimumSize.updated = true;
    
    calculatedMinimumSize.minimumSize = {size.width + (borderSize() << 1) + 55, size.height + (borderSize() << 1) + 7 };
    
    return calculatedMinimumSize.minimumSize;
}

auto pTabFrame::setGeometry(Geometry geometry) -> void {
    if(hasAppThemed()) { geometry.width += 2; geometry.height += 1; }
    pWidget::setGeometry(geometry);
}

auto pTabFrame::append(std::string text, Image* image) -> void {
    if(!hwnd) return;
    unsigned selection = TabCtrl_GetItemCount(hwnd);
    TCITEM item;
    item.mask = TCIF_TEXT;
    utf16_t wtext("");
    item.pszText = wtext;
    TabCtrl_InsertItem(hwnd, selection, &item);
    setText(selection, text);
    if(image && !image->empty()) setImage(selection, *image);
}

auto pTabFrame::remove(unsigned selection) -> void {
    if(hwnd) TabCtrl_DeleteItem(hwnd, selection);
    buildImageList();
}

auto pTabFrame::setFont(std::string font) -> void {
    pWidget::setFont(font);
    buildImageList();
}

auto pTabFrame::setImage(unsigned selection, Image& image) -> void {
    buildImageList();
}

auto pTabFrame::buildImageList() -> void {
    if(!hwnd) return;
    if(imageList) ImageList_Destroy(imageList);
    unsigned size = pFont::size(hfont, " ").height;
    imageList = ImageList_Create(size, size, ILC_COLOR32, 1, 0);
    TabCtrl_SetImageList(hwnd, imageList);

    for(unsigned n = 0; n < tabFrame.state.images.size(); n++) {
        Image* image = tabFrame.state.images[n];
        if (!image || image->empty()) continue;

        image->scaleNearest(size, size);
        HBITMAP bitmap = CreateBitmap(*image);
        ImageList_Add(imageList, bitmap, NULL);
        DeleteObject(bitmap);

        TCITEM item;
        item.mask = TCIF_IMAGE;
        item.iImage = ImageList_GetImageCount(imageList) - 1;
        TabCtrl_SetItem(hwnd, n, &item);
    }
}

auto pTabFrame::setText(unsigned selection, const std::string& text) -> void {
    calculatedMinimumSize.updated = false;
    utf16_t wtext(text);
    TCITEM item;
    item.mask = TCIF_TEXT;
    item.pszText = wtext;
    if(hwnd) TabCtrl_SetItem(hwnd, selection, &item);
}

auto pTabFrame::setSelection(unsigned selection) -> void {
    if(hwnd) TabCtrl_SetCurSel(hwnd, selection);
}

auto pTabFrame::onChange() -> void {
    if(!hwnd) return;
    tabFrame.state.selection = TabCtrl_GetCurSel(hwnd);
    if(tabFrame.onChange) tabFrame.onChange();
}

auto CALLBACK pTabFrame::subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    TabFrameLayout::TabFrame* tabFrame = (TabFrameLayout::TabFrame*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(tabFrame == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);
    
    switch(msg) {   
        case WM_ERASEBKGND:
			if (hasAppThemed())
				return 0;
			break;
    }

   return pApplication::wndProc(tabFrame->p.wndprocOrig, hwnd, msg, wparam, lparam);
    
   // return CallWindowProc(tabFrame->p.wndprocOrig, hwnd, msg, wparam, lparam);
}

auto pTabFrame::create() -> void {
    destroy();
    
    hwnd = CreateWindow(WC_TABCONTROL, L"",
        WS_CHILD | WS_TABSTOP | (hasAppThemed( ) ? WS_CLIPCHILDREN : 0),
        0, 0, 0, 0, getParentHandle(), (HMENU)(unsigned long long)tabFrame.id, GetModuleHandle(0), 0);

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&tabFrame);
    wndprocOrig = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)subclassWndProc);
}

auto pTabFrame::rebuild() -> void {
    if(!needRebuild())
        return;
    
    create();
    setFont( widget.font() );
    for(auto& text : tabFrame.state.header) append(text, nullptr);
    buildImageList();
    setSelection(tabFrame.selection());
    pWidget::rebuild();
}

auto pTabFrame::getTabBackgroundForControl(HWND tab, HWND control) -> HBRUSH {
    if (bkgndBrush)
        return bkgndBrush;
    
    HDC controlDC = GetDC(control);
    HDC copyControlDC = CreateCompatibleDC(controlDC);
    RECT r;
    GetClientRect(control, &r);
    HBITMAP tmpControlBitmap = CreateCompatibleBitmap(controlDC, r.right - r.left, r.bottom - r.top);
    SelectObject(copyControlDC, tmpControlBitmap);

    POINT pt = {0, 0};
    MapWindowPoints(control, tab, &pt, 1);
    SetViewportOrgEx(copyControlDC, -pt.x, -pt.y, &pt);
    SendMessage(tab, WM_PRINTCLIENT, (WPARAM)copyControlDC, PRF_CLIENT);
    SetViewportOrgEx(copyControlDC, pt.x, pt.y, NULL);

	if(bkgndBrush != nullptr) DeleteObject(bkgndBrush);
    bkgndBrush = CreatePatternBrush(tmpControlBitmap);

    DeleteObject(copyControlDC);
    DeleteObject(tmpControlBitmap);
    ReleaseDC(control, controlDC);
    
    return bkgndBrush;
}
