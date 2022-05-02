
auto pSlider::minimumSize() -> Size {
	
//	GtkStyleContext* context = gtk_widget_get_style_context (gtkWidget);
//	gint minLength;	
//	gtk_style_context_get_style ( context, "slider-width", &minLength, NULL);
	
//	auto state = gtk_widget_get_state_flags (gtkWidget);
//	GtkBorder padding;
	
//	gtk_style_context_get_padding (context, state, &padding);
	
	Size size = getMinimumSize();
    if (slider.orientation == Slider::Orientation::VERTICAL)
		return {size.width, 0 };
		//return {(unsigned)(minLength + padding.left + padding.right), 0};
		
	
	return {0, size.height };
		
    //return {0, (unsigned)(minLength + padding.top + padding.bottom - 0) };
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
        gtkWidget = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, 0, 100, 1);
    } else {
        gtkWidget = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
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
