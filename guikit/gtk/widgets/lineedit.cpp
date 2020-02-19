
auto pLineEdit::minimumSize() -> Size {
    Size size = pFont::size(pfont, lineEdit.text());
	
	auto context = gtk_widget_get_style_context (gtkWidget);
    auto state = gtk_widget_get_state_flags (gtkWidget);
	GtkBorder padding;
	GtkBorder border;
	
	gtk_style_context_get_padding (context, state, &padding);
	gtk_style_context_get_border (context, state, &border);
	
    return {size.width + padding.left + padding.right + border.left + border.right + 2,
		size.height + padding.top + padding.bottom + border.top + border.bottom + 10};
}

auto pLineEdit::setEditable(bool editable) -> void {
    
    if (lineEdit.droppable() )
        // drag'n'drop doesn't work if entry isn't editable
        editable = true;
    
    gtk_editable_set_editable(GTK_EDITABLE(gtkWidget), editable);
}

auto pLineEdit::setText(std::string text) -> void {
    locked = true;
    gtk_entry_set_text(GTK_ENTRY(gtkWidget), text.c_str());
	gtk_entry_set_width_chars(GTK_ENTRY(gtkWidget), text.size());
    locked = false;
}

auto pLineEdit::text() -> std::string {
    return gtk_entry_get_text(GTK_ENTRY(gtkWidget));
}

auto pLineEdit::onChange(LineEdit* self) -> void {
    if (!self->editable() && self->droppable() ) {
        // dirty hack
        self->p.setText( ((Widget*)self)->state.text );
        return;
    }
    
    ((Widget*)self)->state.text = ((Widget*)self)->text();
    if(!self->p.locked && self->onChange) self->onChange();
}

auto pLineEdit::onFocus(LineEdit* self) -> bool {
    if(!self->p.locked && self->onFocus) self->onFocus();
    return false;
}

auto pLineEdit::create() -> void {
    destroy();
    gtkWidget = gtk_entry_new();    
    g_signal_connect_swapped(G_OBJECT(gtkWidget), "changed", G_CALLBACK(pLineEdit::onChange), (gpointer)&lineEdit);
    g_signal_connect_swapped(G_OBJECT(gtkWidget), "focus-in-event", G_CALLBACK(pLineEdit::onFocus), (gpointer)&lineEdit);
    g_signal_connect(G_OBJECT(gtkWidget), "drag-data-received", G_CALLBACK(pLineEdit::dropEvent), (gpointer)&lineEdit);
}

auto pLineEdit::init() -> void {
    create();
    setEditable(lineEdit.editable());
    setDroppable(lineEdit.droppable());
    setText(widget.state.text);
}

auto pLineEdit::dropEvent(GtkWidget* widget, GdkDragContext* context, gint x, gint y,
GtkSelectionData* data, guint type, guint timestamp, LineEdit* lineEdit) -> void {
    if(!lineEdit->state.droppable) return;
    auto paths = getDropPaths(data);
    if(paths.empty()) return;
    if(lineEdit->onDrop) lineEdit->onDrop(paths);
}

auto pLineEdit::setDroppable(bool droppable) -> void {
    gtk_drag_dest_set(gtkWidget, GTK_DEST_DEFAULT_ALL, nullptr, 0, GDK_ACTION_COPY);
    if(droppable) gtk_drag_dest_add_uri_targets(gtkWidget);
    
    if (droppable && !lineEdit.editable())
        setEditable( true );
}

auto pLineEdit::setMaxLength( unsigned maxLength ) -> void {
	
    gtk_entry_set_max_length(GTK_ENTRY(gtkWidget), maxLength);
}
