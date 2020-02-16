
auto pSquareCanvas::create() -> void {
    destroy();
    gtkWidget = gtk_drawing_area_new();
    
    gtk_widget_add_events(gtkWidget, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_EXPOSURE_MASK);        

    g_signal_connect(G_OBJECT(gtkWidget), "button-press-event", G_CALLBACK(pSquareCanvas::mousePress), (gpointer)this);
    g_signal_connect(G_OBJECT(gtkWidget), "button-release-event", G_CALLBACK(pSquareCanvas::mouseRelease), (gpointer)this);
    //g_signal_connect(G_OBJECT(gtkWidget), "expose-event", G_CALLBACK(pSquareCanvas::expose), (gpointer)this);
	g_signal_connect(G_OBJECT(gtkWidget), "draw", G_CALLBACK(pSquareCanvas::expose), (gpointer)this);
}

auto pSquareCanvas::destroy() -> void {
    
    if (surface) {
        g_object_unref(surface);
        surface = nullptr;
    }
    pWidget::destroy();
}

auto pSquareCanvas::init() -> void {
    create();
    update();
}

auto pSquareCanvas::setBackgroundColor( unsigned color ) -> void {
    update();
}

auto pSquareCanvas::setBorderColor(unsigned borderSize, unsigned borderColor) -> void {
    update();
}

auto pSquareCanvas::update() -> void {
    redraw();
    
    if (gtk_widget_get_realized(gtkWidget)) {
        gdk_window_invalidate_rect(gtk_widget_get_window(gtkWidget), nullptr, true);
    }
}

auto pSquareCanvas::redraw() -> void {
    unsigned width = squareCanvas.Widget::state.geometry.width;
    unsigned height = squareCanvas.Widget::state.geometry.height;
    unsigned color = squareCanvas.Widget::state.backgroundColor;
    unsigned borderColor = squareCanvas.state.borderColor;
    unsigned borderSize = squareCanvas.state.borderSize;
    
	if (!width || !height)
		return;
	
    uint8_t r = (color >> 16) & 0xff;
    uint8_t g = (color >> 8) & 0xff;
    uint8_t b = (color >> 0) & 0xff;

    uint8_t bR = (borderColor >> 16) & 0xff;
    uint8_t bG = (borderColor >> 8) & 0xff;
    uint8_t bB = (borderColor >> 0) & 0xff;

    if (surface) g_object_unref(surface);
    
    surface = gdk_pixbuf_new(GDK_COLORSPACE_RGB, true, 8, width, height);
    auto buffer = (uint32_t*) gdk_pixbuf_get_pixels(surface);

    for (unsigned y = 0; y < height; y++) {
        bool borderYPixel = 0;
        if (y < borderSize)
            borderYPixel = 1;
        else if (y >= (height - borderSize))
            borderYPixel = 1;

        for (unsigned x = 0; x < width; x++) {
            bool borderXPixel = 0;

            if (!borderYPixel) {
                if (x < borderSize)
                    borderXPixel = 1;
                else if (x >= (width - borderSize))
                    borderXPixel = 1;
            }

            if (borderYPixel || borderXPixel)            
                *buffer++ = 0xff << 24 | bB << 16 | bG << 8 | bR;
            else
                *buffer++ = 0xff << 24 | b << 16 | g << 8 | r;
        }
    }
}

auto pSquareCanvas::setGeometry(Geometry geometry) -> void {

	update();
	pWidget::setGeometry( geometry );
}

auto pSquareCanvas::expose(GtkWidget* widget, GdkEventExpose* event, pSquareCanvas* self) -> signed {
    if (self->surface == nullptr)
        return true;
    
    unsigned width = self->squareCanvas.Widget::state.geometry.width;
    unsigned height = self->squareCanvas.Widget::state.geometry.height;

    //gdk_draw_pixbuf(gtk_widget_get_window(self->gtkWidget), nullptr, self->surface, 0, 0, 0, 0, width, height, GDK_RGB_DITHER_NONE, 0, 0);
	
//	cairo_t* cr = gdk_cairo_create ( gtk_widget_get_window(self->gtkWidget) );
	
	GdkDrawingContext* gdc = gdk_window_begin_draw_frame( gtk_widget_get_window( self->gtkWidget ), cairo_region_create());
	cairo_t* cr = gdk_drawing_context_get_cairo_context( gdc );	
	
	gdk_cairo_set_source_pixbuf (cr, self->surface, 0, 0);
	cairo_paint (cr);
	//cairo_destroy (cr);
	gdk_window_end_draw_frame(gtk_widget_get_window(self->gtkWidget), gdc);
    
    return true;
}

auto pSquareCanvas::mousePress(GtkWidget* widget, GdkEventButton* event, pSquareCanvas* self) -> gboolean {
    if(self->squareCanvas.onMousePress) switch(event->button) {
        case 1: self->squareCanvas.onMousePress(Mouse::Button::Left); break;
        case 2: self->squareCanvas.onMousePress(Mouse::Button::Middle); break;
        case 3: self->squareCanvas.onMousePress(Mouse::Button::Right); break;
    }
	
    return true;
}

auto pSquareCanvas::mouseRelease(GtkWidget* widget, GdkEventButton* event, pSquareCanvas* self) -> gboolean {
    if(self->squareCanvas.onMouseRelease) switch(event->button) {
        case 1: self->squareCanvas.onMouseRelease(Mouse::Button::Left); break;
        case 2: self->squareCanvas.onMouseRelease(Mouse::Button::Middle); break;
        case 3: self->squareCanvas.onMouseRelease(Mouse::Button::Right); break;
    }
    
    return true;
}