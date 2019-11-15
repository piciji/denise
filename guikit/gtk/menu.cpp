
//base
pMenuBase::~pMenuBase() { destroy(); }

auto pMenuBase::destroy() -> void { 
	gtk_widget_destroy(widget);
	gtk_widget_destroy(cwidget);
}
auto pMenuBase::destroyImage() -> void {
	if(gtkImage) gtk_widget_destroy((GtkWidget*)gtkImage);
	if(cgtkImage) gtk_widget_destroy((GtkWidget*)cgtkImage);
}

auto pMenuBase::setEnabled(bool enabled) -> void {
    gtk_widget_set_sensitive(widget, enabled);
	gtk_widget_set_sensitive(cwidget, enabled);
}

auto pMenuBase::setVisible(bool visible) -> void {
    gtk_widget_set_visible(widget, visible);
	gtk_widget_set_visible(cwidget, visible);
}

auto pMenuBase::setText(const std::string& text) -> void {
    gtk_menu_item_set_label(GTK_MENU_ITEM(widget), text.c_str());
	gtk_menu_item_set_label(GTK_MENU_ITEM(cwidget), text.c_str());
}

auto pMenuBase::setIcon(Image& icon) -> void {
    if (dynamic_cast<pMenuCheckItem*>(this) || dynamic_cast<pMenuRadioItem*>(this)) return;

    if(!icon.empty()) {
        destroyImage();
        gtkImage = CreateImage(icon, 15);
		cgtkImage = CreateImage(icon, 15);
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(widget), (GtkWidget*)gtkImage);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(cwidget), (GtkWidget*)cgtkImage);
    } else {
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(widget), nullptr);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(cwidget), nullptr);
    }
}

auto pMenuBase::rebuild() -> void {
    destroy();
    init();
}

//menu
pMenu::pMenu(Menu& menu) : pMenuBase(menu), menu(menu) { }
pMenu::~pMenu() { destroy(); }

auto pMenu::init() -> void {
    gtkMenu = gtk_menu_new();
	cgtkMenu = gtk_menu_new();
    widget = gtk_image_menu_item_new_with_mnemonic("");
	cwidget = gtk_image_menu_item_new_with_mnemonic("");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(widget), gtkMenu);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(cwidget), cgtkMenu);
    setText( menuBase.text() );
    setIcon( *menuBase.state.icon );
}

auto pMenu::destroy() -> void {
    gtk_widget_destroy(gtkMenu);
	gtk_widget_destroy(cgtkMenu);
    pMenuBase::destroy();
}

auto pMenu::rebuild() -> void {
    for(auto& child : menu.childs) child->p.rebuild();
    destroy();
    init();
    for(auto& child : menu.childs) append(*child);
}

auto pMenu::append(MenuBase& item) -> void {
    item.state.parentWindow = menu.state.parentWindow;
    gtk_menu_shell_append(GTK_MENU_SHELL(gtkMenu), item.p.widget);
	gtk_menu_shell_append(GTK_MENU_SHELL(cgtkMenu), item.p.cwidget);
    gtk_widget_show(item.p.widget);
	gtk_widget_show(item.p.cwidget);
}

auto pMenu::remove(MenuBase& item) -> void {
    item.p.rebuild();
    item.state.parentWindow = nullptr;
}

//item
pMenuItem::pMenuItem(MenuItem& menuItem) : pMenuBase(menuItem), menuItem(menuItem) { }

auto pMenuItem::activate(MenuItem* self) -> void {
    if(self->onActivate) self->onActivate();
}

auto pMenuItem::init() -> void {
    widget = gtk_image_menu_item_new_with_mnemonic("");
	cwidget = gtk_image_menu_item_new_with_mnemonic("");
    g_signal_connect_swapped(G_OBJECT(widget), "activate", G_CALLBACK(pMenuItem::activate), (gpointer)&menuItem);
	g_signal_connect_swapped(G_OBJECT(cwidget), "activate", G_CALLBACK(pMenuItem::activate), (gpointer)&menuItem);
    setText( menuBase.text() );
    setIcon( *menuBase.state.icon );
}

//check item
pMenuCheckItem::pMenuCheckItem(MenuCheckItem& menuCheckItem) : pMenuBase(menuCheckItem), menuCheckItem(menuCheckItem) { }

auto pMenuCheckItem::toggle(GtkCheckMenuItem* gtkCheckMenuItem, MenuCheckItem* self) -> void {
	if(self->p.locked) return;
			
    self->state.checked = gtk_check_menu_item_get_active(gtkCheckMenuItem);
	self->p.setChecked( self->checked() );
	
    if(self->onToggle) self->onToggle();		
}

auto pMenuCheckItem::init() -> void {
    widget = gtk_check_menu_item_new_with_mnemonic("");
	cwidget = gtk_check_menu_item_new_with_mnemonic("");
    setChecked(menuCheckItem.checked());
    setText( menuBase.text() );
    g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(pMenuCheckItem::toggle), (gpointer)&menuCheckItem);
	g_signal_connect(G_OBJECT(cwidget), "toggled", G_CALLBACK(pMenuCheckItem::toggle), (gpointer)&menuCheckItem);
}

auto pMenuCheckItem::setChecked(bool checked) -> void {
    locked = true;
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget), checked);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(cwidget), checked);
    locked = false;
}

//radio item
pMenuRadioItem::pMenuRadioItem(MenuRadioItem& menuRadioItem) : pMenuBase(menuRadioItem), menuRadioItem(menuRadioItem) { }

auto pMenuRadioItem::activate(GtkCheckMenuItem* gtkCheckMenuItem, MenuRadioItem* self) -> void {
    if(self->p.parent().locked) return;
    bool wasChecked = self->checked();
    self->setChecked();
    if(wasChecked) return;
    if(self->onActivate) self->onActivate();
}

auto pMenuRadioItem::init() -> void {
    widget = gtk_radio_menu_item_new_with_mnemonic(0, "");
	cwidget = gtk_radio_menu_item_new_with_mnemonic(0, "");

    setGroup(menuRadioItem.group);
    setText( menuBase.text() );
    for(auto& item : menuRadioItem.group) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item->p.widget), item->checked());
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item->p.cwidget), item->checked());
    }
    g_signal_connect(G_OBJECT(widget), "activate", G_CALLBACK(activate), (gpointer)&menuRadioItem);
	g_signal_connect(G_OBJECT(cwidget), "activate", G_CALLBACK(activate), (gpointer)&menuRadioItem);
}

auto pMenuRadioItem::setGroup(const std::vector<MenuRadioItem*>& group) -> void {
    parent().locked = true;
    for(auto& item : group) {
        if(&item == &group.at(0)) continue;
        GSList* currentGroup = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(group.at(0)->p.widget));
        if(currentGroup != gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item->p.widget))) {
            gtk_radio_menu_item_set_group(GTK_RADIO_MENU_ITEM(item->p.widget), currentGroup);
        }
		currentGroup = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(group.at(0)->p.cwidget));
        if(currentGroup != gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item->p.cwidget))) {
            gtk_radio_menu_item_set_group(GTK_RADIO_MENU_ITEM(item->p.cwidget), currentGroup);
        }
    }
    parent().locked = false;
}

auto pMenuRadioItem::setChecked() -> void {
    parent().locked = true;
    for(auto& item : menuRadioItem.group) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item->p.widget), false);
	for(auto& item : menuRadioItem.group) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item->p.cwidget), false);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget), true);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(cwidget), true);
    parent().locked = false;
}

auto pMenuRadioItem::parent() -> pMenuRadioItem& {
    if(menuRadioItem.group.size()) return menuRadioItem.group.at(0)->p;
    return *this;
}

//separator
pMenuSeparator::pMenuSeparator(MenuSeparator& menuSeparator) : pMenuBase(menuSeparator), menuSeparator(menuSeparator) { }

auto pMenuSeparator::init() -> void {
    widget = gtk_separator_menu_item_new();
	cwidget = gtk_separator_menu_item_new();
}
