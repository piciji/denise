
auto pCheckBox::minimumSize() -> Size {
    Size size = pFont::size(pfont, widget.text());
	
	auto context = gtk_widget_get_style_context (gtkWidget);
    auto state = gtk_widget_get_state_flags (gtkWidget);
	GtkBorder padding;
	
	gtk_style_context_get_padding (context, state, &padding);
		
    return {size.width + padding.left + padding.right + 14,
		size.height + padding.top + padding.bottom + 2};
}

auto pCheckBox::setGeometry(Geometry geometry) -> void {
	geometry.x -= 2;
	pWidget::setGeometry( geometry );
}

auto pCheckBox::setChecked(bool checked) -> void {
    locked = true;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtkWidget), checked);
    locked = false;
}

auto pCheckBox::setText(std::string text) -> void {
    gtk_button_set_label(GTK_BUTTON(gtkWidget), text.c_str());
    setFont( widget.font() );
}

auto pCheckBox::onToggle(GtkToggleButton* toggleButton, CheckBox* self) -> void {
    self->state.checked = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->p.gtkWidget));
    if(!self->p.locked && self->onToggle) self->onToggle();
}

auto pCheckBox::create() -> void {
    destroy();
    gtkWidget = gtk_check_button_new_with_label("");
    g_signal_connect(G_OBJECT(gtkWidget), "toggled", G_CALLBACK(pCheckBox::onToggle), (gpointer)&checkBox);
}

auto pCheckBox::init() -> void {
    create();
    setChecked(checkBox.checked());
    setText(widget.text());
}
