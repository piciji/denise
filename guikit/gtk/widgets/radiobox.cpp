
auto pRadioBox::minimumSize() -> Size {
	static bool initialized = false;
	static gint minimumHeight = 0;
	static gint minimumWidth = 0;
	Size size = getMinimumFontSize();
	
	if (!initialized) {
		initialized = true;
		gint natural;
		gtk_widget_get_preferred_height(gtkWidget, &minimumHeight, &natural);
		gtk_widget_get_preferred_width(gtkWidget, &minimumWidth, &natural);		
		
		if (minimumWidth > size.width)
			minimumWidth -= size.width;
	}	
    return {size.width + minimumWidth, std::max((unsigned)minimumHeight, size.height) };
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

auto pRadioBox::setText(const std::string& text) -> void {
    gtk_button_set_label(GTK_BUTTON(gtkWidget), text.c_str());
    //setFont( widget.font() );
    calculatedMinimumSize.updated = false;
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
    if (self->readonly()) {
        for(auto& item : self->state.group) {
            if (item->checked()) {
                item->setChecked();
                break;
            }
        }
        return;
    }
    bool wasChecked = self->checked();
    self->setChecked();
    if(wasChecked) return;
    if(self->onActivate) self->onActivate();
}

auto pRadioBox::parent() -> pRadioBox& {
    if(radioBox.state.group.size()) return radioBox.state.group.at(0)->p;
    return *this;
}
