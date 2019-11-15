
auto pSlider::minimumSize() -> Size {
    if (slider.orientation == Slider::Orientation::VERTICAL) return {20, 0};
    return {0, 20};
}

auto pSlider::setLength(unsigned length) -> void {
    length += length == 0;
    gtk_range_set_range(GTK_RANGE(gtkWidget), 0, std::max(1u, length - 1));
    gtk_range_set_increments(GTK_RANGE(gtkWidget), 1, length >> 3);
}

auto pSlider::setPosition(unsigned position) -> void {
    gtk_range_set_value(GTK_RANGE(gtkWidget), position);
}

auto pSlider::create() -> void {
    destroy();

    if (slider.orientation == Slider::Orientation::VERTICAL) {
        gtkWidget = gtk_vscale_new_with_range(0, 100, 1);
    } else {
        gtkWidget = gtk_hscale_new_with_range(0, 100, 1);
    }

    gtk_scale_set_draw_value(GTK_SCALE(gtkWidget), false);
    g_signal_connect(G_OBJECT(gtkWidget), "value-changed", G_CALLBACK(pSlider::onChange), (gpointer)&slider);
}

auto pSlider::init() -> void {
    create();
    setLength(slider.length());
    setPosition(slider.position());
}

auto pSlider::onChange(GtkRange* gtkRange, Slider* self) -> void {
    unsigned position = (unsigned)gtk_range_get_value(gtkRange);
    if(self->state.position == position) return;
    self->state.position = position;
    if(self->onChange) self->onChange();
}
