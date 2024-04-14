
auto pCheckBox::minimumSize() -> Size {
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

auto pCheckBox::setGeometry(Geometry geometry) -> void {
	//geometry.x -= 2;
	pWidget::setGeometry( geometry );
}

auto pCheckBox::setChecked(bool checked) -> void {
    locked = true;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtkWidget), checked);
    locked = false;
}

auto pCheckBox::setText(const std::string& text) -> void {
    gtk_button_set_label(GTK_BUTTON(gtkWidget), text.c_str());
    //setFont( widget.font() );
    calculatedMinimumSize.updated = false;
}

auto pCheckBox::onToggle(GtkToggleButton* toggleButton, CheckBox* self) -> void {
    self->state.checked = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->p.gtkWidget));
    if(!self->p.locked && self->onToggle) self->onToggle( self->state.checked );
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
