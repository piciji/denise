
auto pButton::minimumSize() -> Size {
    Size size = getMinimumSize();
    return {size.width, size.height};
}

auto pButton::setText(const std::string& text) -> void {
    std::string _text = text;
    if (button.image())
        _text = " " + _text;

    gtk_button_set_label(GTK_BUTTON(gtkWidget), _text.c_str());
    setImage(button.image());
    //setFont( widget.font() );
    calculatedMinimumSize.updated = false;
}

auto pButton::setImage(Image* image) -> void {
    if (image) {
        GtkImage* gtkImage = CreateImage( *image ); // button takes ownership of image, so we don't need to destroy it
        gtk_button_set_always_show_image(GTK_BUTTON(gtkWidget), true);

        if (widget.text().empty())
            gtk_button_set_label(GTK_BUTTON(gtkWidget), NULL);

        gtk_button_set_image(GTK_BUTTON(gtkWidget), GTK_WIDGET(gtkImage));

    } else {
        gtk_button_set_always_show_image(GTK_BUTTON(gtkWidget), false);
        gtk_button_set_image(GTK_BUTTON(gtkWidget), NULL);
    }
}

auto pButton::create() -> void {
    destroy();
    gtkWidget = gtk_button_new();
    g_signal_connect_swapped(G_OBJECT(gtkWidget), "clicked", G_CALLBACK(pButton::onActivate), (gpointer)&button);	
}

auto pButton::init() -> void {
    create();
    setText(widget.text());
}

auto pButton::onActivate(Button* self) -> void {
    if(self->onActivate) self->onActivate();
}
