
auto pButton::minimumSize() -> Size {
    Size size = getMinimumSize();
	
	auto context = gtk_widget_get_style_context (gtkWidget);
    auto state = gtk_widget_get_state_flags (gtkWidget);
	GtkBorder border;
	
	gtk_style_context_get_border (context, state, &border);
		
    return {size.width + 4 + border.left + border.right + 10,
		size.height + border.top + border.bottom + 10};		
	
}

auto pButton::setText(std::string text) -> void {
    gtk_button_set_label(GTK_BUTTON(gtkWidget), text.c_str());
    //setFont( widget.font() );
    calculatedMinimumSize.updated = false;
}

auto pButton::create() -> void {
    destroy();
    gtkWidget = gtk_button_new();
    g_signal_connect_swapped(G_OBJECT(gtkWidget), "clicked", G_CALLBACK(pButton::onActivate), (gpointer)&button);	
	
	pSystem::addCssClass(gtkWidget, "removePadding");	
	pSystem::applyCss( gtkWidget, ".removePadding { padding-left: 2px; padding-right: 2px; padding-top: 0px; padding-bottom: 0px;}" );
}

auto pButton::init() -> void {
    create();
    setText(widget.text());
}

auto pButton::onActivate(Button* self) -> void {
    if(self->onActivate) self->onActivate();
}
