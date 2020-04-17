
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

auto pListView::append(const std::vector<std::string>& list) -> void {
    if (!hwnd) return;
    unsigned row = ListView_GetItemCount(hwnd);

    LVITEM item;
    item.mask = LVIF_TEXT;
    item.iItem = row;
    item.iSubItem = 0;
    utf16_t wtext("");
    item.pszText = wtext;
    locked = true;
    ListView_InsertItem(hwnd, &item);
    locked = false;
    for(unsigned column = 0; column < list.size(); column++) {
        utf16_t wtext(list.at(column));
        ListView_SetItemText(hwnd, row, column, wtext );
    }
    autoSizeColumns();
}

auto pListView::remove(unsigned selection) -> void {
    if (hwnd) ListView_DeleteItem(hwnd, selection);
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

    for(unsigned i = 0; i < list.size(); i++) {
        utf16_t wtext( list.at(i) );
        LVCOLUMN column;
        column.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM;
        column.fmt = LVCFMT_LEFT;
        column.iSubItem = i;
        column.pszText = wtext;
        ListView_InsertColumn(hwnd, i, &column);
    }
    autoSizeColumns();
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
    SendMessage(hwnd, LVM_ENSUREVISIBLE, (WPARAM)selection, false);
    locked = false;
}

auto pListView::setText(unsigned selection, unsigned position, const std::string& text) -> void {
    utf16_t wtext(text);
    if (hwnd) ListView_SetItemText(hwnd, selection, position, wtext);
    autoSizeColumns();
}

auto CALLBACK pListView::subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    ListView* listView = (ListView*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(listView == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);
    Window* window = (Window*)listView->Sizable::state.window;
    if(window == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);

    switch(msg) {        
        case WM_GETDLGCODE:
            if (wparam != VK_TAB)
                return DLGC_WANTALLKEYS;
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
    return CallWindowProc(listView->p.wndprocOrig, hwnd, msg, wparam, lparam);
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
}

auto pListView::create() -> void {
    destroy();
    destroy(hwndTip);
    
    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE, WC_LISTVIEW, L"",
        WS_CHILD | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER | LVS_NOCOLUMNHEADER,
        0, 0, 0, 0, listView.window()->p.hwnd, (HMENU)(unsigned long long)listView.id, GetModuleHandle(0), 0);

    ListView_SetExtendedListViewStyle(hwnd, LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);
    
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&listView);
    wndprocOrig = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)subclassWndProc);  
    
    lastItem = -1;
}

auto pListView::createTooltip(bool useBallon) -> void {
    
    pWidget::createTooltip( useBallon );

    SetWindowTheme(hwndTip, L" ", L" "); // to make coloring work
    RECT rectSetMargin = {5, 5, 5, 3};
    SendMessage(hwndTip, TTM_SETMARGIN, 0, (LPARAM)&rectSetMargin);  
    
    auto& widgetState = listView.Widget::state;
    
    if (listView.state.colorRowTooltips && widgetState.overrideBackgroundColor) {
        unsigned color = widgetState.backgroundColor;
        SendMessage(hwndTip, TTM_SETTIPBKCOLOR, RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff), 0);    
    }

    if (listView.state.colorRowTooltips && widgetState.overrideForegroundColor) {
        unsigned color = widgetState.foregroundColor;
        SendMessage(hwndTip, TTM_SETTIPTEXTCOLOR, RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff), 0);
    }
    
    if (hfont)
        SendMessage(hwndTip, WM_SETFONT, (WPARAM)hfont, 0);     
}

auto pListView::rebuild() -> void {
    create();

	if (listView.overrideBackgroundColor())
		setBackgroundColor( listView.backgroundColor() );
	if (listView.overrideForegroundColor())
		setForegroundColor( listView.foregroundColor() );
	
    pWidget::setFont( widget.font() );
    setContent();
    buildImageList();
    pWidget::rebuild();
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
    for(auto& row : listView.state.rows) append(row);
    if(listView.selected()) setSelection(listView.selection());
}

auto pListView::onChange(LPARAM lparam) -> void {
    LPNMLISTVIEW nmlistview = (LPNMLISTVIEW)lparam;
    if(!(nmlistview->uChanged & LVIF_STATE)) return;

    unsigned selection = nmlistview->iItem;

    if((nmlistview->uOldState & LVIS_FOCUSED) && !(nmlistview->uNewState & LVIS_FOCUSED)) {
        listView.state.selected = false;
    } else if(!(nmlistview->uOldState & LVIS_SELECTED) && (nmlistview->uNewState & LVIS_SELECTED)) {
        listView.state.selected = true;
        listView.state.selection = selection;
        if(!locked && listView.onChange) listView.onChange();
    } else if (listView.selected() && (ListView_GetSelectedCount(hwnd) == 0) ) {
        listView.state.selected = false;
        if(!locked && listView.onChange) listView.onChange();
    }
}

auto pListView::onActivate() -> void {
    if(listView.state.rows.empty() || !listView.state.selected) return;
    if(listView.onActivate) listView.onActivate();
}

auto pListView::onCustomDraw(LPARAM lparam) -> LRESULT {
    LPNMLVCUSTOMDRAW lvcd = (LPNMLVCUSTOMDRAW)lparam;

    switch(lvcd->nmcd.dwDrawStage) {
        case CDDS_PREPAINT:
            return CDRF_NOTIFYITEMDRAW;
        case CDDS_ITEMPREPAINT:
            if( (listView.columnCount() >= 2) && (lvcd->nmcd.dwItemSpec % 2) )
                lvcd->clrTextBk = 0xfff8f0 ^ 0x0f0f0f;

            break;
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

auto pListView::setImage(unsigned selection, unsigned position, Image& image) -> void {
    if(!hwnd) return;
    if(image.empty()) {
        setImage(selection, position, -1);
        return autoSizeColumns();
    }

    for(unsigned n = 0; n < images.size(); n++) {
        if(images[n] == &image) {
            setImage(selection, position, n);
            return autoSizeColumns();
        }
    }

    unsigned size = pFont::size(hfont, " ").height;
    addToImageList(image, size);
    setImage(selection, position, images.size()-1);
    autoSizeColumns();
}

auto pListView::addToImageList(Image& image, unsigned size) -> void {
    images.push_back(&image);
    image.scaleNearest(size, size);
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
        for(unsigned x = 0; x < list.at(y).size(); x++) {
            Image* img = list.at(y).at(x);
            if(!img || img->empty()) {
                setImage(y, x, -1);
                continue;
            }

            bool found = false;
            for(unsigned z = 0; z < images.size(); z++) {
                if(img == images.at(z)) {
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
}

auto pListView::setForegroundColor(unsigned color) -> void {
	if (!hwnd) return;
	ListView_SetTextColor( hwnd, RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff) );
}