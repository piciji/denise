
auto pTabFrame::minimumSize() -> Size {
    std::string text = tabFrame.text(0);
    Size size = pFont::size(pfont, text);
    return {size.width + (borderSize() << 1) + 55, size.height + (borderSize() << 1) + 10 };
}

auto pTabFrame::getDisplacement() -> Position {
    
//    auto style = gtk_rc_get_style (gtkWidget);        
//	
//    if (style) {
//        return { (signed)style->xthickness, (signed)minimumSize().height };
//    }    
    
    return {1, (signed)minimumSize().height};
}

auto pTabFrame::append(std::string text, Image* image) -> void {
    unsigned selection = tabFrame.state.header.size() - 1;

    Tab tab;
    tab.child = gtk_fixed_new();
    tab.container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    tab.image = gtk_image_new();
    tab.title = gtk_label_new( text.c_str() );
    tabs.push_back(tab);

    gtk_widget_show(tab.child);
    gtk_widget_show(tab.container);
    gtk_widget_show(tab.image);
    gtk_widget_show(tab.title);
    gtk_box_pack_start(GTK_BOX(tab.container), tab.image, false, false, 0);
    gtk_box_pack_start(GTK_BOX(tab.container), tab.title, false, false, 2);
    locked = true;
    gtk_notebook_append_page(GTK_NOTEBOOK(gtkWidget), tab.child, tab.container);
    locked = false;
    if(image && !image->empty()) setImage(selection, *image);
}

auto pTabFrame::remove(unsigned selection) -> void {
    gtk_widget_destroy(tabs[selection].image);
    gtk_widget_destroy(tabs[selection].child);
    tabs.erase(tabs.begin() + selection);
    locked = true;
    gtk_notebook_remove_page(GTK_NOTEBOOK(gtkWidget), selection);
    locked = false;
}

auto pTabFrame::setFont(std::string font) -> void {
    pFont::free(pfont);
    pfont = pFont::create(font);

    for(auto& tab : tabs) pFont::setFont(tab.title, pfont);
}

auto pTabFrame::setImage(unsigned selection, Image& image) -> void {
    if(!image.empty()) {
        unsigned size = pFont::size(pfont, " ").height;
        GdkPixbuf* pixbuf = CreatePixbuf(image, size > 2 ? size-2 : size);
		if (pixbuf) {
			gtk_image_set_from_pixbuf(GTK_IMAGE(tabs[selection].image), pixbuf);
			g_object_unref( G_OBJECT( pixbuf ) );
		}
    } else {
        gtk_image_clear(GTK_IMAGE(tabs[selection].image));
    }
}

auto pTabFrame::setText(unsigned selection, std::string text) -> void {
    gtk_label_set_text(GTK_LABEL(tabs[selection].title), text.c_str());
}

auto pTabFrame::setSelection(unsigned selection) -> void {
    locked = true;
    gtk_notebook_set_current_page(GTK_NOTEBOOK(gtkWidget), selection);
    locked = false;
}

auto pTabFrame::onChange(GtkNotebook* notebook, GtkWidget* page, unsigned selection, TabFrameLayout::TabFrame* self) -> void {
    if (self->p.locked) return;
    self->state.selection = selection;
    if(self->onChange) self->onChange();
}

auto pTabFrame::create() -> void {
    destroy();
    gtkWidget = gtk_notebook_new();
    gtk_notebook_set_show_border(GTK_NOTEBOOK(gtkWidget), true);
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(gtkWidget), GTK_POS_TOP);
    g_signal_connect(G_OBJECT(gtkWidget), "switch-page", G_CALLBACK(pTabFrame::onChange), (gpointer)&tabFrame);
}

auto pTabFrame::init() -> void {
    while(tabs.size()) remove(0);
    create();

    for(unsigned i=0; i < tabFrame.tabs(); i++) {
        append(tabFrame.state.header.at(i), nullptr);
        Image* img = tabFrame.state.images.at(i);
        if(img) setImage(i, *img);
    }
    setSelection(tabFrame.selection());
}

auto pTabFrame::getContainerWidget(int selection) -> GtkWidget* {
    
    if (selection > -1 && tabs.size() > selection) {        
        return tabs[selection].child;        
    }
    
    return nullptr;
}
