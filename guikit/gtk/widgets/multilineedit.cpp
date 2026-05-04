
auto pMultilineEdit::destroy() -> void {
    if(subWidget)
		gtk_widget_destroy(subWidget);
	
	subWidget = nullptr;
	
    pWidget::destroy();
}

auto pMultilineEdit::setEditable(bool editable) -> void {
    
    gtk_text_view_set_editable(GTK_TEXT_VIEW(subWidget), editable);
}

auto pMultilineEdit::setText(const std::string& text) -> void {
    locked = true;
	calculatedMinimumSize.updated = false;
    gtk_text_buffer_set_text( buffer, text.c_str(), -1 );
    locked = false;
}

auto pMultilineEdit::appendText(const std::string& text, unsigned maxSize) -> void {
	locked = true;
	GtkTextIter endIter;

	auto& s = multilineEdit.textRef();
	s += text;

	if (s.size() > maxSize ) {
		unsigned fivePercent = 5 * maxSize / 100;
		s = s.substr(s.size() - fivePercent);
		gtk_text_buffer_set_text( buffer, "", -1 );
		gtk_text_buffer_get_end_iter(buffer, &endIter);
		gtk_text_buffer_insert(buffer, &endIter, s.c_str(), -1);
	} else {
		gtk_text_buffer_get_end_iter(buffer, &endIter);
		gtk_text_buffer_insert(buffer, &endIter, text.c_str(), -1);
	}

	GtkTextMark* mark = gtk_text_buffer_create_mark(buffer, NULL, &endIter, TRUE);
	gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(subWidget), mark, 0.0, false, 0.0, 0.0);
	gtk_text_buffer_delete_mark(buffer, mark);

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
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(gtkWidget), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(gtkWidget), GTK_SHADOW_ETCHED_IN);
	
    subWidget = gtk_text_view_new();
	gtk_text_view_set_left_margin( GTK_TEXT_VIEW(subWidget), 3);
	gtk_text_view_set_right_margin( GTK_TEXT_VIEW(subWidget), 3);
	gtk_text_view_set_top_margin( GTK_TEXT_VIEW(subWidget), 3);
	gtk_text_view_set_bottom_margin( GTK_TEXT_VIEW(subWidget), 3);
	gtk_text_view_set_wrap_mode( GTK_TEXT_VIEW(subWidget), GTK_WRAP_WORD_CHAR );
	
	buffer = gtk_text_view_get_buffer( GTK_TEXT_VIEW(subWidget) );
	
	gtk_container_add(GTK_CONTAINER(gtkWidget), subWidget);

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
