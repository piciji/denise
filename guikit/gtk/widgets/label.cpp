
auto pLabel::minimumSize() -> Size {
    Size size = pFont::size(pfont, widget.text());
    return {size.width, size.height};
}

auto pLabel::setText(std::string text) -> void {
    gtk_label_set_text(GTK_LABEL(gtkWidget), text.c_str());
}

auto pLabel::setAlign( Label::Align align ) -> void {
    gtk_misc_set_alignment(GTK_MISC(gtkWidget), align == Label::Align::Left ? 0.0 : 1.0, 0.5);
}

auto pLabel::create() -> void {
    destroy();
    gtkWidget = gtk_label_new("");
  //  gtk_misc_set_alignment(GTK_MISC(gtkWidget), 0.0, 0.5);
}

auto pLabel::init() -> void {
    create();
    setAlign( label.align() );
    setText(widget.text());
}
