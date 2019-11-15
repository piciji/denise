
pMenuBase::~pMenuBase() { freeIcon(); }
pMenu::~pMenu() { 
    if(hmenu)
        DestroyMenu(hmenu);
}

auto pMenuBase::parentWindow() -> Window* {
    return menuBase.state.parentWindow;
}

auto pMenuBase::parentMenu() -> Menu* {
    return menuBase.state.parentMenu;
}

auto pMenuBase::freeIcon() -> void {
    if(hbitmap) DeleteObject(hbitmap);
	if(hicon) DestroyIcon(hicon);
    hbitmap = 0;
	hicon = 0;
}

auto pMenuBase::setEnabled(bool enabled) -> void {
    if(parentWindow())
        parentWindow()->p.updateMenu();
}

auto pMenuBase::setVisible(bool visible) -> void {
    if(parentWindow())
        parentWindow()->p.updateMenu();
}

auto pMenuBase::setText(const std::string& text) -> void {
    if(parentWindow())
        parentWindow()->p.updateMenu();
}

auto pMenuBase::setIcon(Image& icon) -> void {
    freeIcon();
	
    if (!icon.empty()) {
		Image iconTemp = icon; //deep copy, don't change raw data of original
		
		if(pApplication::version >= WindowsVista) { //Vista and later versions
            iconTemp.scaleNearest(15, 15);     
            // without premultiplied alpha, edges looks mangy in win32
            hbitmap = CreateBitmapWithPremultipliedAlpha( iconTemp );
            
            if (!hbitmap) {
                // fallback, alphablend to menu background color
                // works in Windows 7, in Windows 10 it looks good at first glance
                // but highlight color looks wrong
                // win xp inverts color of bitmap when selected ... looks ugly
                iconTemp = icon;
                iconTemp.alphaBlend( GetSysColor(COLOR_MENU) );
                iconTemp.scaleNearest(15, 15);
                hbitmap = CreateBitmap( iconTemp );
            }
  
		} else {
            // premultiplied alpha doesn't work with win xp
            // and the alpha blend fallback above looks mangy.
			// therefore drawing the icon within a callback
            // allows to use hicon which supports transparency
            // and it isn't owner drawn.
            // by supporting all OS's as best, image data is represented as rgb memory array.
            // creating an hicon from rgb or bgr raw data doesn't work reliable on xp. a win xp
            // virtual machine on a win 7 host works but a real installed xp doesn't.
            // converting a hbitmap to hicon doesn't work either.
            // maybe there is some reliable way by working with rgb raw data but for now we
            // use the windows prefered way. means including ico files within the resource compiler
            // and accessing them later (next line) by an resource id.
            hicon = 0;
            if ( iconTemp.resourceId >= 0 )
                hicon = (HICON)::LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(iconTemp.resourceId), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);        
		}
    }
    if(parentWindow()) parentWindow()->p.updateMenu();
}

auto pMenuItem::onActivate() -> void {
    if(menuItem.onActivate)
        menuItem.onActivate();
}

auto pMenuCheckItem::setChecked(bool checked) -> void {
    if(parentMenu())
        CheckMenuItem(parentMenu()->p.hmenu, menuCheckItem.id, checked ? MF_CHECKED : MF_UNCHECKED);
}

auto pMenuCheckItem::onToggle() -> void {
    menuCheckItem.state.checked ^= 1;
    setChecked(menuCheckItem.state.checked);
    if(menuCheckItem.onToggle) menuCheckItem.onToggle();
}

auto pMenuRadioItem::setChecked() -> void {
    for(auto& item : menuRadioItem.group) {
        if(item->state.parentMenu) CheckMenuRadioItem(item->state.parentMenu->p.hmenu, item->id, item->id, item->id + (menuRadioItem.id != item->id), MF_BYCOMMAND);
    }
}

auto pMenuRadioItem::onActivate() -> void {
    if(menuRadioItem.state.checked) return;
    menuRadioItem.setChecked();
    if(menuRadioItem.onActivate) menuRadioItem.onActivate();
}

auto pMenu::append(MenuBase& item) -> void {
    if(parentWindow())
        parentWindow()->p.updateMenu();
}

auto pMenu::remove(MenuBase& item) -> void {
    if(parentWindow())
        parentWindow()->p.updateMenu();
}

auto pMenu::update(Window& window) -> void {
    menu.state.parentWindow = &window;

    if(hmenu) DestroyMenu(hmenu);
    hmenu = CreatePopupMenu();

    for(auto& item : menu.childs) {
        item->state.parentWindow = &window;
        unsigned enabled = item->enabled() ? 0 : MF_GRAYED;
		
        if(dynamic_cast<Menu*>(item)) {
            if (item->visible()) {
                ((Menu*)item)->p.update(window);
                AppendMenu(hmenu, MF_STRING | MF_POPUP | enabled, (UINT_PTR)((Menu*)item)->p.hmenu, utf16_t(item->text()));
            }
        } else if(dynamic_cast<MenuItem*>(item)) {
            if (item->visible()) {
                AppendMenu(hmenu, MF_STRING | enabled, item->id, utf16_t(item->text()));
            }
        } else if(dynamic_cast<MenuSeparator*>(item)) {
            if (item->visible()) {
                AppendMenu(hmenu, MF_SEPARATOR | enabled, item->id, L"");
            }
        } else if(dynamic_cast<MenuCheckItem*>(item)) {
            if (item->visible()) {
                AppendMenu(hmenu, MF_STRING | enabled, item->id, utf16_t(item->text()));				
            }
            if(((MenuCheckItem*)item)->checked()) ((MenuCheckItem*)item)->setChecked();
        } else if(dynamic_cast<MenuRadioItem*>(item)) {
            if (item->visible()) {
                AppendMenu(hmenu, MF_STRING | enabled, item->id, utf16_t(item->text()));				
            }
            if(((MenuRadioItem*)item)->checked()) ((MenuRadioItem*)item)->setChecked();
        }
		
		item->p.setMenuItemInfo(hmenu);
    }
}

auto pMenuBase::setMenuInfo(HMENU parent) -> void {
	MENUINFO mnfo;
	mnfo.cbSize = sizeof (mnfo);
	mnfo.fMask = MIM_STYLE;
	mnfo.dwStyle = MNS_CHECKORBMP | MNS_AUTODISMISS;
	::SetMenuInfo(parent, &mnfo);
}

auto pMenuBase::setMenuItemInfo(HMENU parent) -> void {
	if(!menuBase.visible()) return;
		
	MENUITEMINFO mii = {sizeof(MENUITEMINFO)};
	mii.cbSize = sizeof(mii);

	if (pApplication::version < WindowsVista) {
		setMenuInfo( parent );

		mii.fMask = MIIM_FTYPE | MIIM_BITMAP;
		mii.hbmpItem = HBMMENU_CALLBACK;
	} else {
		mii.fMask = MIIM_CHECKMARKS;
		mii.hbmpUnchecked = hbitmap;
	}
	
	if (hbitmap || hicon)
		::SetMenuItemInfo(parent, dynamic_cast<pMenu*>(this) ? (UINT_PTR)((pMenu*)this)->hmenu : menuBase.id, FALSE, &mii);
}

auto pMenuBase::findMenu(unsigned id) -> MenuBase* {
	for (auto object : Base::objects) {
		if (dynamic_cast<Menu*>(object)) {
			if ((UINT_PTR)dynamic_cast<Menu*>(object)->p.hmenu == id) {
				return (MenuBase*) object;
			}
		}
	}
	return nullptr;
}

auto pMenuBase::measureItem( LPMEASUREITEMSTRUCT lpmis ) -> bool {	
	Base* base = Base::find(lpmis->itemID);
	
	if (base == nullptr) {
		base = pMenuBase::findMenu(lpmis->itemID);
		if (base == nullptr) return false;
	}
    
    lpmis->itemWidth = 15;
    lpmis->itemHeight = 15;

	HICON hIcon = ((MenuBase*) base)->p.hicon;
	if (!hIcon) return false;

	ICONINFO iconinfo;
	::GetIconInfo(hIcon, &iconinfo);

	BITMAP bitmap;
	::GetObject(iconinfo.hbmColor, sizeof (bitmap), &bitmap);

    ::DeleteObject(iconinfo.hbmColor);
	::DeleteObject(iconinfo.hbmMask);
    
	lpmis->itemWidth = bitmap.bmWidth + 5;
	lpmis->itemHeight = bitmap.bmHeight;
	
	return TRUE;
}

auto pMenuBase::drawItem( LPDRAWITEMSTRUCT lpdis ) -> bool {	
	Base* base = Base::find(lpdis->itemID);
	
	if (base == nullptr) {
		base = pMenuBase::findMenu(lpdis->itemID);
		if (base == nullptr) return false;
	}

	if (lpdis->itemState & ODS_SELECTED) {
		lpdis->rcItem.left++;
		lpdis->rcItem.right++;
	}

	HICON hIcon = ((MenuBase*) base)->p.hicon;
	if (!hIcon) return false;

	ICONINFO iconinfo;
	::GetIconInfo(hIcon, &iconinfo);

	BITMAP bitmap;
	::GetObject(iconinfo.hbmColor, sizeof (bitmap), &bitmap);

	::DeleteObject(iconinfo.hbmColor);
	::DeleteObject(iconinfo.hbmMask);

	::DrawIconEx(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top, hIcon, bitmap.bmWidth, bitmap.bmHeight, 0, NULL, DI_NORMAL);

	return TRUE;
}