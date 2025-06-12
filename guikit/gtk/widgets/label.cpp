
auto pLabel::minimumSize() -> Size {
    Size size = getMinimumFontSize();
    return {size.width, size.height};
}

auto pLabel::setTextCall(gpointer data) -> gboolean {
    pLabel* self = (pLabel*)data;

    gtk_label_set_text(GTK_LABEL(self->gtkWidget), self->label.text().c_str());

    return G_SOURCE_REMOVE;
}

auto pLabel::setText(const std::string& text) -> void {
	calculatedMinimumSize.updated = false;
    gtk_label_set_text(GTK_LABEL(gtkWidget), text.c_str());
}

auto pLabel::setTextThreaded(const std::string& text) -> void {
    calculatedMinimumSize.updated = false;
    gdk_threads_add_idle(pLabel::setTextCall, this);
}

auto pLabel::setAlign( Label::Align align ) -> void {
	calculatedMinimumSize.updated = false;
	gtk_label_set_xalign(GTK_LABEL(gtkWidget), align == Label::Align::Left ? 0.0 : 1.0);
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
