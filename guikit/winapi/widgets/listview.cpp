
auto pListView::autoSizeColumns() -> void {
    if (!hwnd) return;
    unsigned count = 0;
    LVCOLUMN column;
    column.mask = LVCF_WIDTH;
    while(ListView_GetColumn(hwnd, count++, &column));
    --count;

    for(unsigned i = 0; i < count; i++) {
        ListView_SetColumnWidth(hwnd, i, LVSCW_AUTOSIZE_USEHEADER);
    }
}

auto pListView::append(const std::vector<std::string>& list, bool preventColumnResizing) -> void {
    if (!hwnd) return;
    unsigned row = ListView_GetItemCount(hwnd);

    LVITEM item;
    item.mask = LVIF_IMAGE | LVIF_TEXT;
    item.iItem = row;
    item.iSubItem = 0;
    item.pszText = NULL;
    locked = true;
    ListView_InsertItem(hwnd, &item);
    locked = false;
    for(unsigned column = 0; column < list.size(); column++) {
        utf16_t wtext(list[column]);
        ListView_SetItemText(hwnd, row, column, wtext );
    }
    if (!preventColumnResizing)
        autoSizeColumns();
}

auto pListView::lockRedraw() -> void {
    if (hwnd)
        SendMessage( hwnd, WM_SETREDRAW, 0, 0);
}

auto pListView::unlockRedraw() -> void {
    if (hwnd) {
        SendMessage( hwnd, WM_SETREDRAW, 1, 0);
        InvalidateRect(hwnd, 0, false);
    }
}

auto pListView::remove(unsigned selection, bool preventColumnResizing) -> void {
    if (!hwnd) return;
    ListView_DeleteItem(hwnd, selection);
    if (!preventColumnResizing)
        autoSizeColumns();
}

auto pListView::reset() -> void {
    if (!hwnd) return;
    ListView_DeleteAllItems(hwnd);
    buildImageList();
}

auto pListView::setGeometry(Geometry geometry) -> void {
    pWidget::setGeometry(geometry);
    autoSizeColumns();
}

auto pListView::buildHeaderText(std::vector<std::string> list) -> void {
    if (!hwnd) return;
    while(ListView_DeleteColumn(hwnd, 0));

    if(list.size() == 0) list.push_back("");
    auto& aligns = listView.state.aligns;

    for(unsigned i = 0; i < list.size(); i++) {
        int fmt = LVCFMT_LEFT;
        if (i < aligns.size()) {
            switch (aligns[i]) {
                default:
                case ListView::Align::Left: fmt = LVCFMT_LEFT; break;
                case ListView::Align::Right: fmt = LVCFMT_RIGHT; break;
                case ListView::Align::Center: fmt = LVCFMT_CENTER; break;
            }
        }

        utf16_t wtext( list[i] );
        LVCOLUMN column;
        column.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM;
        column.fmt = fmt;
        column.iSubItem = i;
        column.pszText = wtext;
        ListView_InsertColumn(hwnd, i, &column);
    }

    if (pApplication::useDark) {
        HWND hHeader = ListView_GetHeader(hwnd);
        SetWindowTheme(hHeader, L"ItemsView", nullptr);
        pApplication::pAllowDarkModeForWindow(hHeader, true);
        SendMessageW(hHeader, WM_THEMECHANGED, 0, 0);
    }

    autoSizeColumns();
}

auto pListView::getFirstVisibleRow() -> unsigned {
    if (!hwnd)
        return 0;

    return ListView_GetTopIndex(hwnd);
}

auto pListView::setHeaderText(std::vector<std::string> list) -> void {
	if (!hwnd) return;
	reset();
	setContent();
	buildImageList();
}

auto pListView::setHeaderVisible(bool visible) -> void {
    if (!hwnd) return;
    SetWindowLong(
        hwnd, GWL_STYLE,
        (GetWindowLong(hwnd, GWL_STYLE) & ~LVS_NOCOLUMNHEADER) |
        (visible ? 0 : LVS_NOCOLUMNHEADER)
    );
}

auto pListView::setSelected(bool selected) -> void {
    locked = true;
    if (selected) {
        setSelection(listView.selection());
    } else {
        if (hwnd) ListView_SetItemState(hwnd, -1, 0, LVIS_FOCUSED | LVIS_SELECTED);
    }
    locked = false;
}

auto pListView::setSelection(unsigned selection) -> void {
    if (!hwnd) return;
    locked = true;
    ListView_SetItemState(hwnd, selection, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
    ListView_EnsureVisible(hwnd, selection, false);
    locked = false;
}

auto pListView::setText(unsigned selection, unsigned position, const std::string& text, bool preventColumnResizing) -> void {
    utf16_t wtext(text);
    if (hwnd) ListView_SetItemText(hwnd, selection, position, wtext);
    if (!preventColumnResizing)
        autoSizeColumns();
}

auto pListView::getThemeHeaderColors(HPEN& captionPen) -> HBRUSH {
    static bool initialized = false;
    static HBRUSH darkHeadBrush = nullptr;
    static HPEN pen = nullptr;

    if (!initialized && pApplication::pOpenThemeData) {
        HTHEME hTheme = pApplication::pOpenThemeData(NULL, L"ItemsView");
        if (hTheme) {
            COLORREF color;
            if (SUCCEEDED(GetThemeColor(hTheme, 0, 0, TMT_FILLCOLOR, &color)))
                darkHeadBrush = CreateSolidBrush( RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff) );

            if (SUCCEEDED(GetThemeColor(hTheme, 0, 0, pApplication::useDark ? TMT_EDGEDKSHADOWCOLOR : TMT_EDGEFILLCOLOR, &color)))
                pen = CreatePen(PS_SOLID, 1, color);

            pApplication::pCloseThemeData(hTheme);
        }
        initialized = true;
    }
    captionPen = pen;
    return darkHeadBrush;
}

auto CALLBACK pListView::subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    ListView* listView = (ListView*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(listView == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);
    Window* window = (Window*)listView->Sizable::state.window;
    if(window == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);

    switch(msg) {
        case WM_ERASEBKGND:
            return 0;
        case WM_GETDLGCODE:
            if (wparam != VK_TAB)
                return DLGC_WANTALLKEYS;
            break;

        case WM_NOTIFY: {
            if ((pApplication::useDark || listView->state.centerHeader) && (reinterpret_cast<LPNMHDR>(lparam)->code == NM_CUSTOMDRAW)) {
                LPNMCUSTOMDRAW nmcd = reinterpret_cast<LPNMCUSTOMDRAW>(lparam);
                switch (nmcd->dwDrawStage) {
                    case CDDS_PREPAINT:
                        return CDRF_NOTIFYITEMDRAW;

                    case CDDS_ITEMPREPAINT:
                        if (pApplication::useDark)
                            SetTextColor(nmcd->hdc, DARK_FG_COL);

                        if (listView->state.centerHeader) {
                            if (nmcd->dwItemSpec < listView->columnCount()) {
                                HPEN pen;
                                HBRUSH darkHeadBrush = getThemeHeaderColors(pen);

                                utf16_t wtext(listView->state.header[nmcd->dwItemSpec]);
                                SetBkMode(nmcd->hdc, TRANSPARENT);
                                if (pApplication::useDark && darkHeadBrush)
                                    FillRect(nmcd->hdc, &nmcd->rc, darkHeadBrush);

                                if (pen) {
                                    SelectObject( nmcd->hdc, pen);
                                    MoveToEx( nmcd->hdc, nmcd->rc.right - 1, nmcd->rc.top, NULL );
                                    LineTo( nmcd->hdc, nmcd->rc.right - 1, nmcd->rc.bottom );
                                }

                                auto rect = nmcd->rc;
                                int _h = rect.bottom - rect.top;
                                int _th = listView->p.getMinimumSize().height;
                                if (_h > _th)
                                    rect.top += (_h - _th) / 2;

                                DrawText(nmcd->hdc, wtext, -1, &rect, DT_CENTER | DT_NOPREFIX);
                                return CDRF_SKIPDEFAULT;
                            }
                        }
                        return CDRF_DODEFAULT;
                }
            }
        }
        break;

        case WM_MOUSEMOVE: {
            if (!listView->state.rowTooltips.size())
                break;
            
            if ((wparam & MK_LBUTTON) == 0) {
                LVHITTESTINFO ht = {0};
                ht.pt.x = LOWORD(lparam);
                ht.pt.y = HIWORD(lparam);
                ListView_SubItemHitTest(hwnd, &ht);                
                int curItem = ht.iItem;

                if (curItem >= 0) {
                    if ( listView->p.lastItem != curItem ) {
                        RECT rect;
                        ListView_GetItemRect(hwnd, curItem, &rect, LVIR_BOUNDS);
                        listView->p.updateRowToolTip( hwnd, curItem, rect );
                    }
                }

                listView->p.relayMesssageToToolTip(hwnd, msg, wparam, lparam);
            }
        } break;
        
    }
    //return CallWindowProc(listView->p.wndprocOrig, hwnd, msg, wparam, lparam);
    return pApplication::wndProc(listView->p.wndprocOrig, hwnd, msg, wparam, lparam);
}

auto pListView::relayMesssageToToolTip(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) -> void {
        
    if (!hwndTip)
        return;
    
    MSG msg;
    msg.hwnd    = hwnd;
    msg.message = umsg;
    msg.wParam  = wparam;
    msg.lParam  = lparam;
    msg.pt.x    = LOWORD(lparam);
    msg.pt.y    = HIWORD(lparam);
    
    SendMessage( hwndTip, TTM_RELAYEVENT, 0, (LPARAM)&msg);
}

auto pListView::updateRowToolTip(HWND hwnd, int curItem, RECT rect) -> void {
    
    if (!hwndTip)
        createTooltip();
    
    auto& toolTips = listView.state.rowTooltips;
    
    if (toolTips.size() <= curItem)
        return;     
   
    lastItem = curItem;
               
    TOOLINFO toolInfo = { 0 };
    toolInfo.cbSize = sizeof(toolInfo);
    toolInfo.hwnd = hwnd;
    
    while ( SendMessage(hwndTip, TTM_ENUMTOOLS, 0, (LPARAM)&toolInfo) ) {
        SendMessage(hwndTip, TTM_DELTOOL, 0, (LPARAM)&toolInfo);
    }
    
    if (toolTips[curItem].empty())
        return;
    
    utf16_t wtooltip( toolTips[curItem] );
    
    toolInfo.uFlags = 0;
    toolInfo.uId = curItem;
    toolInfo.rect.left    = rect.left;
    toolInfo.rect.top     = rect.top;
    toolInfo.rect.right   = rect.right;
    toolInfo.rect.bottom  = rect.bottom;         
    toolInfo.lpszText = wtooltip;
    
    SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
    SendMessage(hwndTip, TTM_SETDELAYTIME, TTDT_INITIAL, 1000);
}

auto pListView::clearBrush() -> void {
    if (bgBrush)
        DeleteObject(bgBrush);
    if (hiBrush)
        DeleteObject(hiBrush);
                
    bgBrush = nullptr;
    hiBrush = nullptr;

    for (auto& pair : rowBrushes)
        DeleteObject(pair.second);
    rowBrushes.clear();
}

auto pListView::create() -> void {
    destroy();
    destroy(hwndTip); 
    clearBrush();

    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE, WC_LISTVIEW, L"",
        WS_CHILD | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | ( !pApplication::useDark ? LVS_SHOWSELALWAYS : 0) | LVS_NOSORTHEADER | LVS_NOCOLUMNHEADER | WS_HSCROLL |
            ( listView.spacing() >= 0 ? LVS_OWNERDRAWFIXED : 0),
        0, 0, 0, 0, getParentHandle(), (HMENU)(unsigned long long)listView.id, GetModuleHandle(0), 0);  

    if (pApplication::pSetWindowTheme) {
        pApplication::pSetWindowTheme(hwnd, L"Explorer", nullptr);
        if (pApplication::useDark) {
            pApplication::pAllowDarkModeForWindow(hwnd, true);
        }
        SendMessageW(hwnd, WM_THEMECHANGED, 0, 0);
    }

    ListView_SetExtendedListViewStyle(hwnd, LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES | LVS_EX_DOUBLEBUFFER);

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&listView);
    wndprocOrig = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)subclassWndProc);  
    
    lastItem = -1;
}

auto pListView::createTooltip(bool useBallon) -> void {
    
    pWidget::createTooltip( useBallon );

    if (pApplication::pSetWindowTheme)
        pApplication::pSetWindowTheme(hwndTip, L" ", L" "); // to make coloring work

    RECT rectSetMargin = {5, 5, 5, 3};
    SendMessage(hwndTip, TTM_SETMARGIN, 0, (LPARAM)&rectSetMargin);  
    
    auto& widgetState = listView.Widget::state;
    
    if (listView.state.colorRowTooltips && widgetState.overrideBackgroundColor) {
        unsigned color = widgetState.backgroundColor;
        SendMessage(hwndTip, TTM_SETTIPBKCOLOR, RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff), 0);    
    } else if (pApplication::useDark)
        SendMessage(hwndTip, TTM_SETTIPBKCOLOR, DARK_BG_SOFTER_COL, 0);


    if (listView.state.colorRowTooltips && widgetState.overrideForegroundColor) {
        unsigned color = widgetState.foregroundColor;
        SendMessage(hwndTip, TTM_SETTIPTEXTCOLOR, RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff), 0);
    } else if (pApplication::useDark)
        SendMessage(hwndTip, TTM_SETTIPTEXTCOLOR, DARK_FG_COL, 0);
    
    if (hfont)
        SendMessage(hwndTip, WM_SETFONT, (WPARAM)hfont, 0);

    if (listView.spacing() == -1) {
        SendMessage(hwndTip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 25000);
        SendMessage(hwndTip, TTM_SETMAXTIPWIDTH, 0, 800);
    }
}

auto pListView::rebuild() -> void {
    if(!needRebuild())
        return;
        
    create();

    if (listView.overrideBackgroundColor())
        setBackgroundColor(listView.backgroundColor());
    else if (pApplication::useDark)
        setDarkBackground();

	if (listView.overrideForegroundColor())
		setForegroundColor( listView.foregroundColor() );
    else if (pApplication::useDark)
        setDarkForeground();
	
    pWidget::setFont( widget.font() );
    setContent();
    buildImageList();
    pWidget::rebuild();
}

auto pListView::updateSpacing() -> void {
    if (hwnd) {
        destroy(hwnd);
        rebuild();
        setGeometry( widget.geometry() );
    }
}

auto pListView::setFont(std::string font) -> void {
    pWidget::setFont(font);    
    reset();
    setContent();
    buildImageList();           
}

auto pListView::setContent() -> void {
    buildHeaderText(listView.state.header);
    setHeaderVisible(listView.state.headerVisible);
    for(auto& row : listView.state.rows)
        append(row, true);

    autoSizeColumns();
    if(listView.selected()) setSelection(listView.selection());

}

auto pListView::onChange(LPARAM lparam) -> void {
    LPNMLISTVIEW nmlistview = (LPNMLISTVIEW)lparam;
    if(!(nmlistview->uChanged & LVIF_STATE)) return;

    unsigned selection = nmlistview->iItem;
   // unsigned column = nmlistview->iSubItem;

    if((nmlistview->uOldState & LVIS_FOCUSED) && !(nmlistview->uNewState & LVIS_FOCUSED)) {
        listView.state.selected = false;
    } else if(!(nmlistview->uOldState & LVIS_SELECTED) && (nmlistview->uNewState & LVIS_SELECTED)) {
        listView.state.selected = true;
        listView.state.selection = selection;
     //   listView.state.column = column;
        if(!locked && listView.onChange) listView.onChange();
    } else if (listView.selected() && (ListView_GetSelectedCount(hwnd) == 0) ) {
        listView.state.selected = false;
        if(!locked && listView.onChange) listView.onChange();
    }
}

auto pListView::onClick(LPARAM lparam, bool rightClick) -> void {
    if (locked)
        return;

    LPNMLISTVIEW nmlistview = (LPNMLISTVIEW)lparam;
    unsigned selection = nmlistview->iItem;
    unsigned column = nmlistview->iSubItem;
    POINT pt;
    GetCursorPos(&pt);

    if (!rightClick) {
        if (listView.onClick)
            listView.onClick(selection, column, {static_cast<signed int>(pt.x), static_cast<signed int>(pt.y)});
    } else if (listView.onContext) {
        listView.onContext(selection, column, {static_cast<signed int>(pt.x), static_cast<signed int>(pt.y)});
    }
}

auto pListView::onActivate(LPARAM lparam) -> void {
    LPNMLISTVIEW nmlistview = (LPNMLISTVIEW)lparam;
    listView.state.column = nmlistview->iSubItem;

    if(listView.state.rows.empty() || !listView.state.selected) return;
    if(listView.onActivate) listView.onActivate();
}

auto pListView::onCustomDraw(LPARAM lparam) -> LRESULT {
    LPNMLVCUSTOMDRAW lvcd = (LPNMLVCUSTOMDRAW)lparam;

    switch(lvcd->nmcd.dwDrawStage) {
        case CDDS_PREPAINT:
            return CDRF_NOTIFYITEMDRAW;

        case CDDS_ITEMPREPAINT | CDDS_SUBITEM: {
            auto rowColor = listView.rowForegroundColor( lvcd->nmcd.dwItemSpec, lvcd->iSubItem );
            if (rowColor.has_value()) {
                unsigned color = rowColor.value();
                lvcd->clrText = RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff);
            } else {
                if (pApplication::useDark)
                    lvcd->clrText = DARK_FG_COL;
                else
                    lvcd->clrText = GetSysColor(COLOR_WINDOWTEXT);
            }
            return CDRF_NEWFONT;
        } break;

        case CDDS_ITEMPREPAINT: {
            if((listView.columnCount() >= 2) && (lvcd->nmcd.dwItemSpec % 2) ) {
                lvcd->clrTextBk = pApplication::useDark ? DARK_BG_COL ^ 0x060606 : 0xfff8f0 ^ 0x0f0f0f;
            }

            auto rowColor = listView.rowBackgroundColor( lvcd->nmcd.dwItemSpec );
            if (rowColor.has_value()) {
                unsigned color = rowColor.value();
                lvcd->clrTextBk = RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff);
            }

            for (auto& entry : listView.state.rowForegroundColor) {
                if (entry.col != std::nullopt)
                    return CDRF_NOTIFYSUBITEMDRAW;
            }

            rowColor = listView.rowForegroundColor(lvcd->nmcd.dwItemSpec);
            if (rowColor.has_value()) {
                unsigned color = rowColor.value();
                lvcd->clrText = RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff);
            }

        } break;
    }
    return CDRF_DODEFAULT;
}

//images
auto pListView::setImage(unsigned selection, unsigned position, int imageListPos) -> void {
    if(ListView_GetImageList(hwnd, LVSIL_SMALL) != imageList) {
        ListView_SetImageList(hwnd, imageList, LVSIL_SMALL);
    }

    LVITEM item;
    item.mask = LVIF_IMAGE;
    item.iItem = selection;
    item.iSubItem = position;
    item.iImage = imageListPos;
    ListView_SetItem(hwnd, &item);
}

auto pListView::setImage(unsigned selection, unsigned position, Image& image, bool preventColumnResizing) -> void {
    unsigned size;
    if(!hwnd)
        return;

    if(image.empty()) {
        setImage(selection, position, -1);
        goto End;
    }

    for(unsigned n = 0; n < images.size(); n++) {
        if(images[n] == &image) {
            setImage(selection, position, n);
            goto End;
        }
    }

    size = pFont::size(hfont, " ").height;
    addToImageList(image, size);
    setImage(selection, position, images.size()-1);

End:
    if (!preventColumnResizing)
        autoSizeColumns();

    InvalidateRect(hwnd, 0, false);
}

auto pListView::addToImageList(Image& image, unsigned size) -> void {
    images.push_back(&image);
    image.scaleLinear( size, size);
    HBITMAP bitmap = CreateBitmap(image);
    ImageList_Add(imageList, bitmap, NULL);
    DeleteObject(bitmap);
}

auto pListView::buildImageList() -> void {
    images.clear();

    ListView_SetImageList(hwnd, NULL, LVSIL_SMALL);
    if(imageList) ImageList_Destroy(imageList);
    unsigned size = pFont::size(hfont, " ").height;
    imageList = ImageList_Create(size, size, ILC_COLOR32, 1, 0);
    
    if (listView.countImages() == 0) return;
    auto& list = listView.state.images;

    for(unsigned y = 0; y < list.size(); y++) {
        for(unsigned x = 0; x < list[y].size(); x++) {
            Image* img = list[y][x];
            if(!img || img->empty()) {
                setImage(y, x, -1);
                continue;
            }

            bool found = false;
            for(unsigned z = 0; z < images.size(); z++) {
                if(img == images[z]) {
                    found = true;
                    setImage(y, x, z);
                    break;
                }
            }
            if (found) continue;

            addToImageList(*img, size);
            setImage(y, x, images.size()-1);
        }
    }
    autoSizeColumns();
}

auto pListView::setBackgroundColor(unsigned color) -> void {
	if (!hwnd) return;
	ListView_SetBkColor( hwnd, RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff) );
	ListView_SetTextBkColor( hwnd, RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff) ); 
    
    clearBrush();
    destroy(hwndTip); 
}

auto pListView::setDarkBackground() -> void {
    if (!hwnd) return;

    ListView_SetBkColor(hwnd, DARK_BG_COL);
    ListView_SetTextBkColor(hwnd, DARK_BG_COL);

    clearBrush();
    destroy(hwndTip);    
}

auto pListView::setDarkForeground() -> void {
    if (!hwnd) return;
    ListView_SetTextColor(hwnd, DARK_FG_COL);
    destroy(hwndTip);
}

auto pListView::setForegroundColor(unsigned color) -> void {
	if (!hwnd) return;
	ListView_SetTextColor( hwnd, RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff) );
    destroy(hwndTip); 
}

auto pListView::setSelectionColor(unsigned foregroundColor, unsigned backgroundColor) -> void {
    if (!hwnd) return;
    clearBrush();
}

auto pListView::updateRowColors() -> void {
    if (!hwnd) return;
    clearBrush();
    InvalidateRect(hwnd, 0, false);
}

auto pListView::measureItem(LPMEASUREITEMSTRUCT lpmis) -> void {
    
    if (!hfont)
        return;
    
    auto size = getMinimumSize();

    int spacing = listView.spacing();
    
    lpmis->itemHeight = size.height + 2 * spacing;
}

auto pListView::drawItem(LPDRAWITEMSTRUCT lDraw) -> void {
    HBRUSH hBrush = nullptr;
    COLORREF colorRef;
    
    if (lDraw->itemState & ODS_SELECTED) {
        if (listView.state.overrideSelectionColor) {
            unsigned color = listView.state.selectionForegroundColor;
            colorRef = RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff);
        } else
            colorRef = GetSysColor( COLOR_HIGHLIGHTTEXT );

        if (!hiBrush) {
            if (listView.state.overrideSelectionColor) {
                unsigned color =  listView.state.selectionBackgroundColor;
                hiBrush = CreateSolidBrush( RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff) );
            } else
                hiBrush = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
        }

        hBrush = hiBrush;
    } else {
        auto rowBackgroundColor = listView.rowBackgroundColor( lDraw->itemID );
        if (rowBackgroundColor.has_value()) {
            hBrush = findRowBrush(lDraw->itemID);
            if (!hBrush) {
                unsigned color = rowBackgroundColor.value();
                hBrush = CreateSolidBrush( RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff) );
                rowBrushes.push_back( {lDraw->itemID, hBrush} );
            }
        }

        if (!bgBrush) {
            if (listView.overrideBackgroundColor()) {
                unsigned color = listView.backgroundColor();
                bgBrush = CreateSolidBrush( RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff) );
            } else if (pApplication::useDark)
                bgBrush = CreateSolidBrush( DARK_BG_COL );
            else
                bgBrush = CreateSolidBrush( GetSysColor( COLOR_WINDOW ) );                                    
        }

        if (!hBrush)
            hBrush = bgBrush;

        auto rowColor = listView.rowForegroundColor( lDraw->itemID );

        if (rowColor.has_value()) {
            unsigned color = rowColor.value();
            colorRef = RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff);
        } else if (listView.overrideForegroundColor()) {
            unsigned color = listView.foregroundColor();

            colorRef = RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff);
        } else if (pApplication::useDark)
            colorRef = DARK_FG_COL;
        else
            colorRef = GetSysColor( COLOR_WINDOWTEXT );
    }
        
    auto lRow = lDraw->rcItem;
    
    FillRect(lDraw->hDC, &lRow, hBrush);

    SetTextColor(lDraw->hDC, colorRef);

    int spacing = listView.spacing();

    for (int i = 0; i < listView.columnCount(); i++) {
        RECT prc;

        LVITEM lvItem = {0};
        lvItem.mask = LVIF_IMAGE | LVIF_TEXT;
        lvItem.iItem = lDraw->itemID;
        lvItem.iSubItem = i;

        auto image = listView.getImage(lDraw->itemID, i);

        if (image && !image->empty() && ListView_GetItem(lDraw->hwndItem, &lvItem) && (lvItem.iImage >= 0)) {
            ListView_GetSubItemRect(lDraw->hwndItem, lDraw->itemID, i, LVIR_ICON, &prc);

            if (spacing > 0) {
                prc.left += spacing;
                prc.top += spacing;
                prc.bottom -= spacing;
            }

            ImageList_Draw(listView.p.imageList, lvItem.iImage, lDraw->hDC, prc.left,prc.top, ILD_TRANSPARENT);
        } else {
            ListView_GetSubItemRect(lDraw->hwndItem, lDraw->itemID, i, LVIR_LABEL, &prc);
            wchar_t lBuf[100];
            ListView_GetItemText(lDraw->hwndItem, lDraw->itemID, i, (LPTSTR) lBuf, 100);

            if (spacing > 0) {
                prc.left += spacing;
                prc.top += spacing;
                prc.bottom -= spacing;
            }

            DrawText(lDraw->hDC, lBuf, -1, &prc, DT_LEFT | DT_NOPREFIX);
        }
    }
}

auto pListView::findRowBrush(unsigned row) -> HBRUSH {
    for (auto& pair : rowBrushes) {
        if (pair.first == row) {
            return pair.second;
        }
    }
    return nullptr;
}