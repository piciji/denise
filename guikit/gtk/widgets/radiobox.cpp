
auto pRadioBox::minimumSize() -> Size {
    Size size = getMinimumSize();
	
	return {size.width + 16 + 4, size.height + 4};
}

auto pRadioBox::setGeometry(Geometry geometry) -> void {
	//geometry.x -= 2;
	pWidget::setGeometry( geometry );
}

auto pRadioBox::setChecked() -> void {
    parent().locked = true;
    for(auto& item : radioBox.state.group) item->state.checked = false;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtkWidget), radioBox.state.checked = true);
    parent().locked = false;
}

auto pRadioBox::setText(std::string text) -> void {
    gtk_button_set_label(GTK_BUTTON(gtkWidget), text.c_str());
    setFont( widget.font() );
}

auto pRadioBox::setGroup(const std::vector<RadioBox*>& group) -> void {
    if(&parent() == this) return;
    parent().locked = true;
    gtk_radio_button_set_group( GTK_RADIO_BUTTON(gtkWidget), gtk_radio_button_get_group(GTK_RADIO_BUTTON(parent().gtkWidget)));

    for(auto& item : radioBox.state.group) {
        if(item->checked()) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(item->p.gtkWidget), true);
            break;
        }
    }
    parent().locked = false;
}

auto pRadioBox::create() -> void {
    destroy();
    gtkWidget = gtk_radio_button_new_with_label(nullptr, "");
    g_signal_connect(G_OBJECT(gtkWidget), "toggled", G_CALLBACK(pRadioBox::onActivate), (gpointer)&radioBox);
}

auto pRadioBox::init() -> void {
    create();
    setGroup(radioBox.state.group);
    setText(widget.text());
}

auto pRadioBox::onActivate(GtkToggleButton* toggleButton, RadioBox* self) -> void {
    if(self->p.parent().locked) return;
    bool wasChecked = self->checked();
    self->setChecked();
    if(wasChecked) return;
    if(self->onActivate) self->onActivate();
}

auto pRadioBox::parent() -> pRadioBox& {
    if(radioBox.state.group.size()) return radioBox.state.group.at(0)->p;
    return *this;
}
