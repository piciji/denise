
auto pCheckButton::minimumSize() -> Size {
    Size size = getMinimumSize();
	
	auto context = gtk_widget_get_style_context (gtkWidget);
    auto state = gtk_widget_get_state_flags (gtkWidget);
	GtkBorder padding;
	GtkBorder border;
	
	gtk_style_context_get_padding (context, state, &padding);
	gtk_style_context_get_border (context, state, &border);
		
    return {size.width + padding.left + padding.right + border.left + border.right + 10,
		size.height + padding.top + padding.bottom + border.top + border.bottom + 10};
}

auto pCheckButton::setChecked(bool checked) -> void {
    locked = true;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtkWidget), checked);
    locked = false;
}

auto pCheckButton::setText(std::string text) -> void {
    gtk_button_set_label(GTK_BUTTON(gtkWidget), text.c_str());
    setFont( widget.font() );
}

auto pCheckButton::create() -> void {
    destroy();
    gtkWidget = gtk_toggle_button_new();
    g_signal_connect(G_OBJECT(gtkWidget), "toggled", G_CALLBACK(pCheckButton::onToggle), (gpointer)&checkButton);
}

auto pCheckButton::init() -> void {
    create();
    setChecked(checkButton.checked());
    setText(widget.text());
}

auto pCheckButton::onToggle(GtkToggleButton* toggleButton, CheckButton* self) -> void {
    if(self->p.locked) return;
    self->state.checked = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->p.gtkWidget));
    if(self->onToggle) self->onToggle();
}
