
auto pImageView::minimumSize() -> Size {
    if (!imageView.state.image)
        return {0u, 0u};

    auto image = imageView.state.image;

    return {image->width, image->height};
}

auto pImageView::create() -> void {
    destroy();
    gtkWidget = gtk_drawing_area_new();

    gtk_widget_add_events(gtkWidget, GDK_BUTTON_PRESS_MASK | GDK_EXPOSURE_MASK | GDK_POINTER_MOTION_MASK);

    g_signal_connect(G_OBJECT(gtkWidget), "motion-notify-event", G_CALLBACK(pImageView::mouseMove), (gpointer)this);
    g_signal_connect(G_OBJECT(gtkWidget), "button-press-event", G_CALLBACK(pImageView::mousePress), (gpointer)this);
    g_signal_connect(G_OBJECT(gtkWidget), "draw", G_CALLBACK(pImageView::expose), (gpointer)this);
}

auto pImageView::destroy() -> void {

    if (surface) {
        g_object_unref(surface);
        surface = nullptr;
    }
    pWidget::destroy();
}

auto pImageView::init() -> void {
    create();
    update();
}

auto pImageView::update() -> void {
    if (!imageView.state.image || imageView.state.image->empty())
        return;

    if (surface)
        g_object_unref(surface);

    surface = CreatePixbuf( *imageView.state.image );

    if (gtk_widget_get_realized(gtkWidget)) {
        gdk_window_invalidate_rect(gtk_widget_get_window(gtkWidget), nullptr, true);
    }
}

auto pImageView::setGeometry(Geometry geometry) -> void {

  //  update();
    pWidget::setGeometry( geometry );
}

auto pImageView::setImage(Image* image) -> void {
    update();
}

auto pImageView::expose(GtkWidget* widget, GdkEventExpose* event, pImageView* self) -> signed {
    if (self->surface == nullptr)
        return true;

    unsigned width = self->imageView.state.image->width;
    unsigned height = self->imageView.state.image->height;

    GdkDrawingContext* gdc = gdk_window_begin_draw_frame( gtk_widget_get_window( self->gtkWidget ), cairo_region_create());
    cairo_t* cr = gdk_drawing_context_get_cairo_context( gdc );

    gdk_cairo_set_source_pixbuf (cr, self->surface, 0, 0);
    cairo_paint (cr);
    gdk_window_end_draw_frame(gtk_widget_get_window(self->gtkWidget), gdc);

    return true;
}

auto pImageView::mousePress(GtkWidget* widget, GdkEventButton* event, pImageView* self) -> gboolean {

    GError *error = NULL;

    if (event->button == 1) {
        if(self->imageView.onClick)
            self->imageView.onClick();

        if (!self->imageView.state.uri.empty())
            gtk_show_uri_on_window (NULL, self->imageView.state.uri.c_str(), gtk_get_current_event_time(), &error);
    }

    return true;
}

auto pImageView::mouseMove(GtkWidget* widget, GdkEventButton* event, pImageView* self) -> gboolean {

    auto cursor = gdk_cursor_new_for_display( gdk_screen_get_display(gdk_screen_get_default()), GDK_HAND2 );

    if (cursor) {
        SetCursor(widget, cursor);
        g_object_unref( cursor );
    }

    return true;
}
