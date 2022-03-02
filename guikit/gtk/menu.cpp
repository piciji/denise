
//base
pMenuBase::~pMenuBase() {
	destroy();
}

auto pMenuBase::destroy() -> void {
	destroy(element);
	destroy(elementC);
}

auto pMenuBase::destroy(Element& el) -> void { 

	if(el.gtkImage)
		gtk_widget_destroy((GtkWidget*)el.gtkImage);
	
	el.gtkImage = nullptr;
	
	if (el.label)
		gtk_widget_destroy(el.label);
	
	el.label = nullptr;
	
	if (el.box)
		gtk_widget_destroy(el.box);
	
	el.box = nullptr;
	
	if (el.widget)
		gtk_widget_destroy(el.widget);
	
	el.widget = nullptr;
}

auto pMenuBase::setEnabled(bool enabled) -> void {
    gtk_widget_set_sensitive(element.widget, enabled);
	gtk_widget_set_sensitive(elementC.widget, enabled);
}

auto pMenuBase::setVisible(bool visible) -> void {
    gtk_widget_set_visible(element.widget, visible);
	gtk_widget_set_visible(elementC.widget, visible);
}

auto pMenuBase::setText(const std::string& text) -> void {
	
	if (!element.box) {
		gtk_menu_item_set_label(GTK_MENU_ITEM(element.widget), text.c_str());
		gtk_menu_item_set_label(GTK_MENU_ITEM(elementC.widget), text.c_str());
	} else {
		updateItemBox(element);
		updateItemBox(elementC);
	}
}

auto pMenuBase::setIcon(Image& icon) -> void {
    if (dynamic_cast<pMenuCheckItem*>(this) || dynamic_cast<pMenuRadioItem*>(this)) return;

	updateItemBox(element);
	updateItemBox(elementC);
}

auto pMenuBase::updateItemBox(Element& el) -> void {
		
	if (el.label)
		gtk_container_remove (GTK_CONTAINER (el.box), el.label);
	
	if (el.gtkImage)
		gtk_container_remove (GTK_CONTAINER (el.box), GTK_WIDGET(el.gtkImage));
	
	el.gtkImage = nullptr;
	
	if(!menuBase.state.icon->empty()) {
		el.gtkImage = CreateImage(*menuBase.state.icon, 15);
		
		gtk_container_add (GTK_CONTAINER (el.box), GTK_WIDGET(el.gtkImage) );
	}
	
	el.label = gtk_label_new ( menuBase.state.text.c_str() );	
	
	gtk_container_add (GTK_CONTAINER (el.box), el.label);
	
	gtk_widget_show_all(el.widget);
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
		
    element.widget = gtk_menu_item_new();
	elementC.widget = gtk_menu_item_new();
	element.box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	elementC.box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_container_add (GTK_CONTAINER (element.widget), element.box);
	gtk_container_add (GTK_CONTAINER (elementC.widget), elementC.box);
	
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(element.widget), gtkMenu);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(elementC.widget), cgtkMenu);
	updateItemBox(element);
	updateItemBox(elementC);	
}

auto pMenu::destroy() -> void {
	
	if (gtkMenu)
		gtk_widget_destroy(gtkMenu);
	gtkMenu = nullptr;
	
	if (cgtkMenu)
		gtk_widget_destroy(cgtkMenu);    
	cgtkMenu = nullptr;
	
	pMenuBase::destroy();
}

auto pMenu::rebuild() -> void {
    for(auto& child : menu.childs) child->p.rebuild();
    destroy();
    init();
    for(auto& child : menu.childs) append(*child);
}

auto pMenu::append(MenuBase& item) -> void {
    item.state.parentWindow = menu.parentWindow();
    gtk_menu_shell_append(GTK_MENU_SHELL(gtkMenu), item.p.element.widget);
	gtk_menu_shell_append(GTK_MENU_SHELL(cgtkMenu), item.p.elementC.widget);
    gtk_widget_show(item.p.element.widget);
	gtk_widget_show(item.p.elementC.widget);
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
    element.widget = gtk_menu_item_new();
	elementC.widget = gtk_menu_item_new();	
	
	element.box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	elementC.box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_container_add (GTK_CONTAINER (element.widget), element.box);
	gtk_container_add (GTK_CONTAINER (elementC.widget), elementC.box);
	
    g_signal_connect_swapped(G_OBJECT(element.widget), "activate", G_CALLBACK(pMenuItem::activate), (gpointer)&menuItem);
	g_signal_connect_swapped(G_OBJECT(elementC.widget), "activate", G_CALLBACK(pMenuItem::activate), (gpointer)&menuItem);
	updateItemBox(element);
	updateItemBox(elementC);
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
    element.widget = gtk_check_menu_item_new_with_mnemonic("");
	elementC.widget = gtk_check_menu_item_new_with_mnemonic("");

    pSystem::addCssClass(element.widget, "paddingMenuItem");
    pSystem::applyCss( element.widget, ".paddingMenuItem { padding-left: 25px; }");
    pSystem::addCssClass(elementC.widget, "paddingMenuItem");
    pSystem::applyCss( elementC.widget, ".paddingMenuItem { padding-left: 25px; }");

    setChecked(menuCheckItem.checked());
    setText( menuBase.text() );
    g_signal_connect(G_OBJECT(element.widget), "toggled", G_CALLBACK(pMenuCheckItem::toggle), (gpointer)&menuCheckItem);
	g_signal_connect(G_OBJECT(elementC.widget), "toggled", G_CALLBACK(pMenuCheckItem::toggle), (gpointer)&menuCheckItem);
}

auto pMenuCheckItem::setChecked(bool checked) -> void {
    locked = true;
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(element.widget), checked);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(elementC.widget), checked);
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
    element.widget = gtk_radio_menu_item_new_with_mnemonic(0, "");
	elementC.widget = gtk_radio_menu_item_new_with_mnemonic(0, "");

    pSystem::addCssClass(element.widget, "paddingMenuItem");
    pSystem::applyCss( element.widget, ".paddingMenuItem { padding-left: 25px; }");
    pSystem::addCssClass(elementC.widget, "paddingMenuItem");
    pSystem::applyCss( elementC.widget, ".paddingMenuItem { padding-left: 25px; }");

    setGroup(menuRadioItem.group);
    setText( menuBase.text() );
    for(auto& item : menuRadioItem.group) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item->p.element.widget), item->checked());
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item->p.elementC.widget), item->checked());
    }
    g_signal_connect(G_OBJECT(element.widget), "activate", G_CALLBACK(activate), (gpointer)&menuRadioItem);
	g_signal_connect(G_OBJECT(elementC.widget), "activate", G_CALLBACK(activate), (gpointer)&menuRadioItem);
}

auto pMenuRadioItem::setGroup(const std::vector<MenuRadioItem*>& group) -> void {
    parent().locked = true;
    for(auto& item : group) {
        if(&item == &group.at(0)) continue;
        GSList* currentGroup = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(group.at(0)->p.element.widget));
        if(currentGroup != gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item->p.element.widget))) {
            gtk_radio_menu_item_set_group(GTK_RADIO_MENU_ITEM(item->p.element.widget), currentGroup);
        }
		currentGroup = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(group.at(0)->p.elementC.widget));
        if(currentGroup != gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item->p.elementC.widget))) {
            gtk_radio_menu_item_set_group(GTK_RADIO_MENU_ITEM(item->p.elementC.widget), currentGroup);
        }
    }
    parent().locked = false;
}

auto pMenuRadioItem::setChecked() -> void {
    parent().locked = true;
    for(auto& item : menuRadioItem.group) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item->p.element.widget), false);
	for(auto& item : menuRadioItem.group) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item->p.elementC.widget), false);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(element.widget), true);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(elementC.widget), true);
    parent().locked = false;
}

auto pMenuRadioItem::parent() -> pMenuRadioItem& {
    if(menuRadioItem.group.size()) return menuRadioItem.group.at(0)->p;
    return *this;
}

//separator
pMenuSeparator::pMenuSeparator(MenuSeparator& menuSeparator) : pMenuBase(menuSeparator), menuSeparator(menuSeparator) { }

auto pMenuSeparator::init() -> void {
    element.widget = gtk_separator_menu_item_new();
	elementC.widget = gtk_separator_menu_item_new();
}
