
pMultiSquareCanvas::~pMultiSquareCanvas() {
    delete[] drawArea;
}

auto pMultiSquareCanvas::create() -> void {
    destroy();
    gtkWidget = gtk_scrolled_window_new(0, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(gtkWidget), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
 //   gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(gtkWidget), GTK_SHADOW_ETCHED_IN);

    pSystem::applyCss( gtkWidget, "scrolledwindow undershoot.top, scrolledwindow undershoot.right, scrolledwindow undershoot.bottom, scrolledwindow undershoot.left { background-image: none; }");

    pSystem::addCssClass(gtkWidget, "somePadding");
    pSystem::applyCss( gtkWidget, ".somePadding { padding-top: 4px;} " );

    subWidget = gtk_drawing_area_new();

    gtk_widget_add_events(subWidget, GDK_EXPOSURE_MASK);

    gtk_container_add(GTK_CONTAINER(gtkWidget), subWidget);

    g_signal_connect(G_OBJECT(subWidget), "draw", G_CALLBACK(pMultiSquareCanvas::expose), (gpointer)this);

    gtk_widget_show(subWidget);
}

auto pMultiSquareCanvas::destroy() -> void {
    if (surface) {
        g_object_unref(surface);
        surface = nullptr;
    }
    pWidget::destroy();
}

auto pMultiSquareCanvas::init() -> void {
    create();
    update();
}

auto pMultiSquareCanvas::expose(GtkWidget* widget, cairo_t* cr, pMultiSquareCanvas* self) -> gboolean {
    if (self->surface == nullptr)
        return FALSE;

    gdk_cairo_set_source_pixbuf (cr, self->surface, 0, 0);
    cairo_paint (cr);

    return FALSE;
}

auto pMultiSquareCanvas::update() -> void {
    buildDrawArea();
    redraw();

    if (gtk_widget_get_realized(subWidget)) {
        gdk_window_invalidate_rect(gtk_widget_get_window(subWidget), nullptr, true);
    }
}

auto pMultiSquareCanvas::setPadding(unsigned padding) -> void {
    update();
}

auto pMultiSquareCanvas::setGeometry(Geometry geometry) -> void {
    update();
    pWidget::setGeometry( geometry );
}

auto pMultiSquareCanvas::redraw() -> void {
    unsigned padding = multiSquareCanvas.padding();
    unsigned cols = multiSquareCanvas.cols();
    unsigned rows = multiSquareCanvas.rows();
    unsigned squareSize = multiSquareCanvas.squareSize();
    unsigned fullWidth = cols * (squareSize + padding);
    unsigned fullHeight = rows * (squareSize + padding);

    if (drawArea) {
        if (surface)
            g_object_unref(surface);

        surface = gdk_pixbuf_new(GDK_COLORSPACE_RGB, true, 8, fullWidth, fullHeight);
        uint8_t* buffer = (uint8_t*) gdk_pixbuf_get_pixels(surface);

        for (unsigned y = 0; y < fullHeight; y++)
            std::memcpy(buffer + y * fullWidth * 4, (uint8_t*)(drawArea + y * fullWidth), fullWidth * 4);

        gtk_widget_set_size_request(subWidget, fullWidth, fullHeight);
    }
}

#define DARK_BG_COL         (0x20 << 16 | 0x20 << 8 | 0x20)
#define DARK_BG_SOFTER_COL  (0x38 << 16 | 0x38 << 8 | 0x38)

auto pMultiSquareCanvas::buildDrawArea() -> void {
    unsigned* dots = multiSquareCanvas.getDotPtr();
    delete[] drawArea;
    drawArea = nullptr;

    if (!dots)
        return;

    unsigned cols = multiSquareCanvas.cols();
    unsigned rows = multiSquareCanvas.rows();
    unsigned squareSize = multiSquareCanvas.squareSize();
    unsigned padding = multiSquareCanvas.padding();

    unsigned width = cols * (squareSize + padding);
    unsigned height = rows * (squareSize + padding);

    drawArea = new unsigned[width * height];

    unsigned yPos = 0;
    unsigned* target;

    for (unsigned r = 0; r < rows; r++) {
        unsigned xPos = 0;

        for (unsigned c = 0; c < cols; c++) {
            unsigned color = *dots++;
            if (!color)
                color = DARK_BG_SOFTER_COL | (0xff << 24);
            else {
                uint8_t r = (color >> 16) & 0xff;
                uint8_t b = color & 0xff;
                color &= 0xff00ff00;
                color |= b << 16 | r;
            }

            for (unsigned y = 0; y < squareSize; y++) {
                target = drawArea + (yPos + y) * width + xPos;

                for (unsigned x = 0; x < padding; x++) {
                    *target++ = DARK_BG_COL | (0xff << 24);
                }

                for (unsigned x = 0; x < squareSize; x++) {
                    *target++ = color;
                }
            }

            xPos += squareSize + padding;
        }

        yPos += squareSize;

        for (unsigned y = 0; y < padding; y++) {
            target = drawArea + (yPos + y) * width;

            for (unsigned x = 0; x < width; x++) {
                *target++ = DARK_BG_COL | (0xff << 24);
            }
        }

        yPos += padding;
    }
}
