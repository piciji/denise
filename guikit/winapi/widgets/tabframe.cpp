
HBRUSH pTabFrame::bkgndBrush = nullptr;

auto pTabFrame::borderSize() -> unsigned {
    return pApplication::hasAppThemed() ? 1 : 2;
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
    if (pApplication::hasAppThemed()) {
        geometry.width += 2;
        geometry.height += 1;
    }
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
    Window* window = (Window*)tabFrame->Sizable::state.window;
    if (window == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);

    auto* doubleBuffer = tabFrame->p.doubleBuffer;
    const auto& hMemDC = doubleBuffer->hMemDC;
    
    switch(msg) {
        case WM_ERASEBKGND: {
            if (!pApplication::useDark) {
                if (pApplication::hasAppThemed())
                    return 0;
                break;
            }

            const auto* hdc = reinterpret_cast<HDC>(wparam);
            if (hdc != hMemDC)        
                return FALSE;
        
            return TRUE;
        }

        case WM_PARENTNOTIFY: {
            if (LOWORD(wparam) == WM_CREATE) {
                if (pApplication::useDark) {
                    auto hUpDown = reinterpret_cast<HWND>(lparam);
                    SetWindowTheme(hUpDown, L"DarkMode_Explorer", nullptr);
                }
            }
            break;
        }

        case WM_PAINT: {
            if (!pApplication::useDark)
                break;

            const auto nStyle = ::GetWindowLongPtr(hwnd, GWL_STYLE);
            if ((nStyle & TCS_VERTICAL) == TCS_VERTICAL)
                break;

            PAINTSTRUCT ps{};
            HDC hdc = ::BeginPaint(hwnd, &ps);

            if (ps.rcPaint.right <= ps.rcPaint.left || ps.rcPaint.bottom <= ps.rcPaint.top) {
                EndPaint(hwnd, &ps);
                return 0;
            }

            RECT rcClient{};
            GetClientRect(hwnd, &rcClient);

            if (doubleBuffer->ensure(hdc, rcClient)) {
                const int savedState = ::SaveDC(hMemDC);
                IntersectClipRect(
                    hMemDC,
                    ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom
                );

                pTabFrame::paintTab(hwnd, hMemDC, rcClient);

                RestoreDC(hMemDC, savedState);

                BitBlt(hdc, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top,
                    hMemDC, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY
                );
            }

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_UPDATEUISTATE: {
            if ((HIWORD(wparam) & (UISF_HIDEACCEL | UISF_HIDEFOCUS)) != 0)
                InvalidateRect(hwnd, nullptr, FALSE);
        
            break;
        }

    }

    return pApplication::wndProc(tabFrame->p.wndprocOrig, hwnd, msg, wparam, lparam);
}

auto pTabFrame::paintTab(HWND hWnd, HDC hdc, const RECT& rect) -> void {
    FillRect(hdc, &rect, pApplication::darkBGTabBrush);

    auto holdPen = static_cast<HPEN>(SelectObject(hdc, pApplication::darkEdgePen));

    auto holdClip = ::CreateRectRgn(0, 0, 0, 0);
    if (GetClipRgn(hdc, holdClip) != 1) {
        DeleteObject(holdClip);
        holdClip = nullptr;
    }

    auto hFont = reinterpret_cast<HFONT>(::SendMessage(hWnd, WM_GETFONT, 0, 0));
    auto holdFont = SelectObject(hdc, hFont);

    POINT ptCursor{};
    GetCursorPos(&ptCursor);
    ScreenToClient(hWnd, &ptCursor);

    bool hasFocusRect = false;
    if (GetFocus() == hWnd) {
        const auto uiState = static_cast<DWORD>(::SendMessage(hWnd, WM_QUERYUISTATE, 0, 0));
        hasFocusRect = ((uiState & UISF_HIDEFOCUS) != UISF_HIDEFOCUS);
    }

    const int iSelTab = TabCtrl_GetCurSel(hWnd);
    const int nTabs = TabCtrl_GetItemCount(hWnd);

    for (int i = 0; i < nTabs; ++i) {
        RECT rcItem{};
        TabCtrl_GetItemRect(hWnd, i, &rcItem);
        RECT rcFrame{ rcItem };

        RECT rcIntersect{};
        if (IntersectRect(&rcIntersect, &rect, &rcItem) == TRUE) {
            const bool isHot = ::PtInRect(&rcItem, ptCursor) == TRUE;
            const bool isSelectedTab = (i == iSelTab);

            SetBkMode(hdc, TRANSPARENT);

            HRGN hClip = ::CreateRectRgnIndirect(&rcItem);
            SelectClipRgn(hdc, hClip);

            InflateRect(&rcItem, -1, -1);
            rcItem.right += 1;

            std::wstring label(MAX_PATH, L'\0');
            TCITEM tci{};
            tci.mask = TCIF_TEXT | TCIF_IMAGE | TCIF_STATE;
            tci.dwStateMask = TCIS_HIGHLIGHTED;
            tci.pszText = label.data();
            tci.cchTextMax = MAX_PATH - 1;

            TabCtrl_GetItem(hWnd, i, &tci);

            const auto nStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE);
            const bool isBtn = (nStyle & TCS_BUTTONS) == TCS_BUTTONS;
            if (isBtn) {
                const bool isHighlighted = (tci.dwState & TCIS_HIGHLIGHTED) == TCIS_HIGHLIGHTED;
                FillRect(hdc, &rcItem, isHighlighted ? pApplication::darkBGHotBrush : pApplication::darkBGSofterBrush);
                SetTextColor(hdc, isHighlighted ? DARK_FG_COL : DARK_FG_COL);
            } else {
                auto getBrush = [&]() -> HBRUSH {
                    if (isSelectedTab)
                        return  pApplication::darkBGTabBrush;

                    if (isHot)
                        return pApplication::darkBGHotBrush;

                    return pApplication::darkBGSofterBrush;
                };

                FillRect(hdc, &rcItem, getBrush());
                SetTextColor(hdc, (isHot || isSelectedTab) ? DARK_FG_COL : DARK_FG_COL);
            }

            RECT rcText{ rcItem };
            if (!isBtn) {
                if (isSelectedTab) {
                    //OffsetRect(&rcText, 0, -1);
                    InflateRect(&rcFrame, 0, 1);
                }

                if (i != nTabs - 1)
                    rcFrame.right += 1;
            }

            if (tci.iImage != -1) {
                int cx = 0;
                int cy = 0;
                auto hImagelist = TabCtrl_GetImageList(hWnd);
                static constexpr int offset = 3;
                ImageList_GetIconSize(hImagelist, &cx, &cy);
                ImageList_Draw(hImagelist, tci.iImage, hdc, rcText.left + offset, rcText.top + (((rcText.bottom - rcText.top) - cy) / 2), ILD_NORMAL);
                rcText.left += cx;
            }

            DrawText(hdc, label.c_str(), -1, &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            FrameRect(hdc, &rcFrame, pApplication::darkEdgeBrush);

            if (isSelectedTab && hasFocusRect) {
                ::InflateRect(&rcFrame, -2, -1);
                ::DrawFocusRect(hdc, &rcFrame);
            }

            SelectClipRgn(hdc, holdClip);
            DeleteObject(hClip);
        }
    }

    SelectObject(hdc, holdFont);
    SelectClipRgn(hdc, holdClip);
    if (holdClip != nullptr) {
        DeleteObject(holdClip);
        holdClip = nullptr;
    }
    SelectObject(hdc, holdPen);
}

auto pTabFrame::create() -> void {
    destroy();
    
    hwnd = CreateWindow(WC_TABCONTROL, L"",
        WS_CHILD | WS_TABSTOP | (pApplication::hasAppThemed() ? WS_CLIPCHILDREN : 0),
        0, 0, 0, 0, getParentHandle(), (HMENU)(unsigned long long)tabFrame.id, GetModuleHandle(0), 0);    

    if (!doubleBuffer)
        doubleBuffer = new DoubleBuffer();
    doubleBuffer->release();

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
    if (pApplication::useDark)
        return pApplication::darkBGTabBrush;

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
