
auto pCheckButton::minimumSize() -> Size {
    Size size = getMinimumSize();
    return {size.width + 8, size.height}; // GTK + BSD need some padding
}

auto pCheckButton::setChecked(bool checked) -> void {
    locked = true;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtkWidget), checked);
    locked = false;
}

auto pCheckButton::setText(const std::string& text) -> void {
    gtk_button_set_label(GTK_BUTTON(gtkWidget), text.c_str());
    //setFont( widget.font() );
    calculatedMinimumSize.updated = false;
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
