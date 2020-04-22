
TreeView* pTreeViewItem::parentTreeView() { return treeViewItem.state.parentTreeView; }

auto pTreeViewItem::append(TreeViewItem& item) -> void {
    if(!parentTreeView()) return;
    item.state.parentTreeView = parentTreeView();
    item.p.update(&treeViewItem);
}

auto pTreeViewItem::remove(TreeViewItem& item) -> void {
    if(!parentTreeView()) return;
    SendMessage(parentTreeView()->p.hwnd, TVM_DELETEITEM, (WPARAM)0, (LPARAM)item.p.hTreeItem);
}

auto pTreeViewItem::reset() -> void {
    if(!parentTreeView()) return;
    for(auto item : treeViewItem.state.items) {
        SendMessage(parentTreeView()->p.hwnd, TVM_DELETEITEM, (WPARAM)0, (LPARAM)item->p.hTreeItem);
    }
}

auto pTreeViewItem::setText(std::string text) -> void {
    if (!parentTreeView() || !hTreeItem) return;

    utf16_t wtext( text );
    TVITEMW item = {0};
    item.hItem = hTreeItem;
    item.cchTextMax = MAX_PATH;
    item.pszText = wtext;
    item.mask = TVIF_TEXT;
    SendMessage(parentTreeView()->p.hwnd, TVM_SETITEM, 0, (LPARAM)&item);
}

auto pTreeViewItem::setSelected() -> void {
    if (!parentTreeView() || !hTreeItem) return;

    SendMessage(parentTreeView()->p.hwnd, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hTreeItem);
}

auto pTreeViewItem::setExpanded(bool expanded) -> void {
    if (!parentTreeView() || !hTreeItem) return;

    SendMessage(parentTreeView()->p.hwnd, TVM_EXPAND, expanded ? TVE_EXPAND : TVE_COLLAPSE
        , (LPARAM)hTreeItem);
}

auto pTreeViewItem::addItem(TreeViewItem* parent) -> void {
    int imagePos = parentTreeView()->p.addToImageList(treeViewItem.state.image);
    int imagePosSelected = parentTreeView()->p.addToImageList(treeViewItem.state.imageSelected);

    utf16_t wtext( treeViewItem.text() );
    TVINSERTSTRUCT tvi;
    tvi.hParent = parent == nullptr ? TVI_ROOT : parent->p.hTreeItem;
    tvi.hInsertAfter = TVI_LAST;
    tvi.item.pszText = wtext;
    tvi.item.cchTextMax = MAX_PATH;
    if (imagePos > 0) {
        parentTreeView()->p.setImageList();
        tvi.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        tvi.item.iImage = imagePos;
        tvi.item.iSelectedImage = imagePosSelected > 0 ? imagePosSelected : imagePos;
    } else {
        tvi.item.mask = TVIF_TEXT;
    }

    hTreeItem = (HTREEITEM)SendMessage(parentTreeView()->p.hwnd, TVM_INSERTITEM, 0, (LPARAM)&tvi);
}

auto pTreeViewItem::update(TreeViewItem* parent) -> void {
    addItem( parent );

    for(auto& item : treeViewItem.state.items) {
        item->state.parentTreeView = parentTreeView();
        item->p.update( &treeViewItem );
    }
    setExpanded( treeViewItem.expanded() );
}

auto pTreeViewItem::setImage(Image& image) -> void {
    if (!parentTreeView() || !hTreeItem) return;
    setImage();
}

auto pTreeViewItem::setImageSelected(Image& image) -> void {
    if (!parentTreeView() || !hTreeItem) return;
    setImage();
}

auto pTreeViewItem::setImage() -> void {
    parentTreeView()->p.setImageList();
    int imagePos = parentTreeView()->p.addToImageList(treeViewItem.state.image);
    int imagePosSelected = parentTreeView()->p.addToImageList(treeViewItem.state.imageSelected);

    TVITEMW item = {0};
    item.hItem = hTreeItem;
    item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    item.iImage = imagePos;
    item.iSelectedImage = imagePosSelected > 0 ? imagePosSelected : imagePos;
    SendMessage(parentTreeView()->p.hwnd, TVM_SETITEM, 0, (LPARAM)&item);
}

auto pTreeViewItem::find( HTREEITEM _hTreeItem ) -> TreeViewItem* {
    if (_hTreeItem == this->hTreeItem) return &treeViewItem;

    for(auto child : treeViewItem.state.items) {
        TreeViewItem* item = child->p.find(_hTreeItem);
        if (item) return item;
    }
    return nullptr;
}

auto pTreeView::append(TreeViewItem& item) -> void {
    if(!hwnd) return;
    item.state.parentTreeView = &treeView;
    item.p.update( nullptr );
}

auto pTreeView::remove(TreeViewItem& item) -> void {
    if(hwnd) SendMessage(hwnd, TVM_DELETEITEM, (WPARAM)0, (LPARAM)item.p.hTreeItem);
}

auto pTreeView::reset() -> void {
    if (!hwnd) return;
    SendMessage(hwnd, TVM_DELETEITEM, (WPARAM)0, (LPARAM)TVI_ROOT);
    buildImageList();
}

auto CALLBACK pTreeView::subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    TreeView* treeView = (TreeView*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(treeView == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);
    Window* window = (Window*)treeView->Sizable::state.window;
    if(window == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);

    switch(msg) {
        case WM_GETDLGCODE:
            if (wparam == VK_RETURN) return DLGC_WANTALLKEYS;
    }
    return CallWindowProc(treeView->p.wndprocOrig, hwnd, msg, wparam, lparam);
}

auto pTreeView::create() -> void {
    destroy();
    destroy(hwndTip);
    
    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE, WC_TREEVIEW, L"",
        WS_CHILD | WS_TABSTOP | WS_BORDER | TVS_HASLINES | TVS_SHOWSELALWAYS,
        0, 0, 0, 0, treeView.window()->p.hwnd, (HMENU)(unsigned long long)treeView.id, GetModuleHandle(0), 0);

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&treeView);
    wndprocOrig = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)subclassWndProc);
}

auto pTreeView::rebuild() -> void {
    create();
	
	if (treeView.overrideBackgroundColor())
		setBackgroundColor( treeView.backgroundColor() );
	if (treeView.overrideForegroundColor())
		setForegroundColor( treeView.foregroundColor() );
	
    pWidget::setFont( widget.font() );
    buildImageList();
    update();
    if (treeView.selected()) treeView.selected()->p.setSelected();
    pWidget::rebuild();
}

auto pTreeView::update() -> void {
    for(auto& item : treeView.state.items) {
        item->state.parentTreeView = &treeView;
        item->p.update( nullptr );
    }
}

auto pTreeView::buildImageList() -> void {
    images.clear();

    ListView_SetImageList(hwnd, NULL, TVSIL_NORMAL);
    if(imageList) ImageList_Destroy(imageList);
    unsigned size = pFont::size(hfont, " ").height;
    imageList = ImageList_Create(size, size, ILC_COLOR32, 1, 0);

    uint8_t blank[4] = {255,255,255,1};
    auto blankoImg = new Image(1, 1, (uint8_t*)blank );
    addToImageList( blankoImg );
    delete blankoImg;
}

auto pTreeView::addToImageList(Image* image) -> int {
    if ( !imageList ) return -1;
    if ( !image || image->empty() ) return 0;

    for(unsigned z = 0; z < images.size(); z++) {
        if (image == images[z]) {
            return z; //already in list
        }
    }

    int size;
    ImageList_GetIconSize(imageList, &size, &size);

    images.push_back(image);
    image->scaleNearest(size, size);
    HBITMAP bitmap = CreateBitmap(*image);
    ImageList_Add(imageList, bitmap, NULL);
    DeleteObject(bitmap);
    return images.size() - 1;
}

auto pTreeView::setImageList() -> void {
    if(TreeView_GetImageList(hwnd, TVSIL_NORMAL) != imageList) {
        TreeView_SetImageList(hwnd, imageList, TVSIL_NORMAL);
    }
}

auto pTreeView::setFont(std::string font) -> void {
    pWidget::setFont(font);
    if (!hwnd) return;
    rebuild();
    setGeometry( widget.geometry() );
}

auto pTreeView::onActivate() -> void {
    if(!treeView.state.selected) return;
    if(treeView.onActivate) treeView.onActivate();
}

auto pTreeView::onChange() -> void {
    if(treeView.state.items.empty()) return;

    HTREEITEM hTreeItem = (HTREEITEM)SendMessage(hwnd, TVM_GETNEXTITEM, TVGN_CARET, (LPARAM)0);
    TreeViewItem* changed;

    for(auto item : treeView.state.items) {

        changed = item->p.find( hTreeItem );

        if (changed && (changed != treeView.state.selected) ) {
            treeView.state.selected = changed;
            if(treeView.onChange) treeView.onChange();
            return;
        }
    }
}

auto pTreeView::setBackgroundColor(unsigned color) -> void {	
	if (!hwnd) return;
	TreeView_SetBkColor( hwnd, RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff) );
}

auto pTreeView::setForegroundColor(unsigned color) -> void {
	if (!hwnd) return;
	TreeView_SetTextColor( hwnd, RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff) );
}