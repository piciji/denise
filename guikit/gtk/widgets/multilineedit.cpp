
auto pMultilineEdit::destroy() -> void {
    if(subWidget)
		gtk_widget_destroy(subWidget);
	
	subWidget = nullptr;
	
    pWidget::destroy();
}

auto pMultilineEdit::setEditable(bool editable) -> void {
    
    gtk_text_view_set_editable(GTK_TEXT_VIEW(subWidget), editable);
}

auto pMultilineEdit::setText(std::string text) -> void {
    locked = true;
	calculatedMinimumSize.updated = false;
    gtk_text_buffer_set_text( buffer, text.c_str(), -1 );
    locked = false;
}

auto pMultilineEdit::text() -> std::string {
	
	GtkTextIter startSel, endSel;
	gtk_text_buffer_get_bounds ( buffer, &startSel, &endSel);
	
    return (std::string)gtk_text_buffer_get_text(buffer, &startSel, &endSel, false);
}

auto pMultilineEdit::onChange(MultilineEdit* self) -> void {
    
    ((Widget*)self)->state.text = ((Widget*)self)->text();
    if(!self->p.locked && self->onChange) self->onChange();
}

auto pMultilineEdit::onFocus(MultilineEdit* self) -> bool {
    if(!self->p.locked && self->onFocus) self->onFocus();
    return false;
}

auto pMultilineEdit::create() -> void {
    destroy();
	gtkWidget = gtk_scrolled_window_new(0, 0);
	
    subWidget = gtk_text_view_new();
	buffer = gtk_text_view_get_buffer( GTK_TEXT_VIEW(subWidget) );
	
	gtk_container_add(GTK_CONTAINER(gtkWidget), subWidget);
	
	//pSystem::addCssClass(gtkWidget, "scrollBox");
	
    g_signal_connect_swapped(G_OBJECT(buffer), "changed", G_CALLBACK(pMultilineEdit::onChange), (gpointer)&multilineEdit);
	g_signal_connect_swapped(G_OBJECT(subWidget), "focus-in-event", G_CALLBACK(pMultilineEdit::onFocus), (gpointer)&multilineEdit);
	
	gtk_widget_show(subWidget);
}

auto pMultilineEdit::init() -> void {
    create();
    setEditable(multilineEdit.editable());
    setText(widget.state.text);
}

auto pMultilineEdit::setForegroundColor(unsigned color) -> void {
	if (!subWidget)
		return;
	
	pSystem::removeCssClass(subWidget, "customColor");
	
    if( !widget.overrideForegroundColor() )
        return;
	
	pSystem::addCssClass(subWidget, "customColor");
	
	pSystem::applyCss( subWidget, ".customColor text { color: " + pSystem::getColorCss( color ) + "; }" );
}
