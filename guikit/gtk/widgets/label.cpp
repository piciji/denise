
auto pLabel::minimumSize() -> Size {
    Size size = pFont::size(pfont, widget.text());
    return {size.width, size.height};
}

auto pLabel::setText(std::string text) -> void {
    gtk_label_set_text(GTK_LABEL(gtkWidget), text.c_str());
}

auto pLabel::setAlign( Label::Align align ) -> void {
	
	gtk_label_set_xalign(GTK_LABEL(gtkWidget), align == Label::Align::Left ? 0.0 : 1.0);
}

auto pLabel::setForegroundColor(unsigned color) -> void {
    if( !gtkWidget || !widget.overrideForegroundColor() )
        return;
   // GdkColor gdkColor = CreateColor( (color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff );
   // gtk_widget_modify_fg(gtkWidget, GTK_STATE_NORMAL, &gdkColor);
  //  gtk_widget_modify_text(gtkWidget, GTK_STATE_NORMAL, &gdkColor);
	
	std::string _color = "rgb(" + std::to_string( (color >> 16) & 0xff ) + ", " + std::to_string( (color >> 8) & 0xff )
		+ ", " + std::to_string( color & 0xff ) + ")";
	
	pSystem::applyCss( gtkWidget, "label { color: " + _color + " }" );
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
