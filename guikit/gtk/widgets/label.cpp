
auto pLabel::minimumSize() -> Size {
    Size size = getMinimumSize();
    return {size.width, size.height};
}

auto pLabel::setText(std::string text) -> void {
	calculatedMinimumSize.updated = false;
    gtk_label_set_text(GTK_LABEL(gtkWidget), text.c_str());
}

auto pLabel::setAlign( Label::Align align ) -> void {
	calculatedMinimumSize.updated = false;
	gtk_label_set_xalign(GTK_LABEL(gtkWidget), align == Label::Align::Left ? 0.0 : 1.0);
}

auto pLabel::setForegroundColor(unsigned color) -> void {
	pSystem::removeCssClass(gtkWidget, "customColor");
	
    if( !gtkWidget || !widget.overrideForegroundColor() )
        return;
	
	std::string _color = "rgb(" + std::to_string( (color >> 16) & 0xff ) + ", " + std::to_string( (color >> 8) & 0xff )
		+ ", " + std::to_string( color & 0xff ) + ")";
	
	pSystem::addCssClass(gtkWidget, "customColor");
	
	pSystem::applyCss( gtkWidget, ".customColor { color: " + _color + " }" );
}

auto pLabel::create() -> void {
    destroy();
    gtkWidget = gtk_label_new("");
}

auto pLabel::init() -> void {
    create();
    setAlign( label.align() );
    setText(widget.text());
}
