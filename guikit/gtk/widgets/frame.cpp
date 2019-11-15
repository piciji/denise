
auto pFrame::frameSize() -> Size {
    
    auto style = gtk_rc_get_style(gtkWidget);
    
    if (style) {
        return {style->xthickness, style->ythickness};
    }
    return {1,1};
}

auto pFrame::borderSize() -> unsigned {
    return frameSize().height;
}

auto pFrame::minimumSize() -> Size {
    Size size = pFont::size(pfont, widget.text());
    size.width += 4 + (frameSize().width << 1);
    size.height >>= 1;
    if (widget.text().empty()) size.height = 0;

    size.height += (frameSize().height << 1) + 1;
    return size;
}

auto pFrame::setGeometry(Geometry geometry) -> void {
    Size size = pFont::size(pfont, widget.text());
    size.height >>= 1;
    if (!widget.text().empty()) size.height = 0;
    geometry.y -= size.height;
    geometry.height += size.height;

    pWidget::setGeometry(geometry);
}

auto pFrame::getDisplacement() -> Position {
    Size size = pFont::size(pfont, widget.text());
    if (widget.text().empty()) size.height >>= 1, size.height += 1;
    return { frameSize().width, size.height };
}


auto pFrame::setText(std::string text) -> void {
    gtk_frame_set_label(GTK_FRAME(gtkWidget), text.c_str());
	setFont(widget.font());
}

auto pFrame::setEnabled(bool enabled) -> void {
	
	if(gtkWidget)
        gtk_widget_set_sensitive(gtk_frame_get_label_widget(GTK_FRAME(gtkWidget)), enabled);
}

auto pFrame::create() -> void {
    destroy();
    if(box) gtk_widget_destroy(box);
    gtkWidget = gtk_frame_new("");
    box = gtk_fixed_new();
    gtk_widget_show(box);
    gtk_container_add(GTK_CONTAINER(gtkWidget), box);
}

auto pFrame::init() -> void {  
    create();
    setText(widget.text());
}

auto pFrame::setFont(std::string font) -> void {
    pFont::free(pfont);
    pfont = gtkWidget ? pFont::setFont(gtk_frame_get_label_widget(GTK_FRAME(gtkWidget)), font) : pFont::create(font);
}

auto pFrame::getContainerWidget(int selection) -> GtkWidget* {
    return box;
}
