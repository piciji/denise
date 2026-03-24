
#define DMA_SLOT_WIDTH 70u

pLogicViewer::~pLogicViewer() {

}

auto pLogicViewer::init() -> void {
    create();
    update();
}

auto pLogicViewer::create() -> void {
    destroy();
    gtkWidget = gtk_scrolled_window_new(0, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(gtkWidget), GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);

    pSystem::applyCss( gtkWidget, "scrolledwindow undershoot.top, scrolledwindow undershoot.right, scrolledwindow undershoot.bottom, scrolledwindow undershoot.left { background-image: none; }");

    pSystem::addCssClass(gtkWidget, "somePadding");
    pSystem::applyCss( gtkWidget, ".somePadding { padding-top: 0px;} " );

    subWidget = gtk_drawing_area_new();

    gtk_widget_add_events(subWidget, GDK_EXPOSURE_MASK);

    gtk_container_add(GTK_CONTAINER(gtkWidget), subWidget);

    g_signal_connect(G_OBJECT(subWidget), "draw", G_CALLBACK(pLogicViewer::expose), (gpointer)this);

    GtkAdjustment* hadj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(gtkWidget));
    g_signal_connect(hadj, "value-changed", G_CALLBACK(pLogicViewer::scrolled), (gpointer)this);

    g_signal_connect(G_OBJECT(gtkWidget), "scroll-event", G_CALLBACK(pLogicViewer::onScroll), (gpointer)this);

    gtk_widget_show(subWidget);

    scrollTimer.onFinished = [this]() {
        scrollToActive();
    };
    scrollTimer.setInterval(20);
    scrollTimer.setData(0);
}

auto pLogicViewer::scrolled(GtkAdjustment* adj, pLogicViewer* self) -> void {
    if (gtk_widget_get_realized(self->subWidget)) {
        gdk_window_invalidate_rect(gtk_widget_get_window(self->subWidget), nullptr, true);
    }
}

auto pLogicViewer::onScroll(GtkWidget* widget, GdkEventScroll* event, pLogicViewer* self) -> gboolean {
    if (event->direction == GDK_SCROLL_SMOOTH) {
        GtkScrolledWindow *sw = GTK_SCROLLED_WINDOW(widget);
        GtkAdjustment *hadj = gtk_scrolled_window_get_hadjustment(sw);

        gdouble value = gtk_adjustment_get_value(hadj);
        gtk_adjustment_set_value(hadj, value + event->delta_y * 30);

        return TRUE;
    }
    return FALSE;
}

auto pLogicViewer::expose(GtkWidget* widget, cairo_t* cr, pLogicViewer* self) -> gboolean {
    double _col = self->getColorComponent(0x38);
    cairo_set_source_rgb(cr, _col, _col, _col);
   // cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint (cr);

    self->redraw(cr);
    return FALSE;
}

auto pLogicViewer::setGeometry(Geometry geometry) -> void {
    update();
    pWidget::setGeometry( geometry );
}

#define SCROLL_STEPS 8

auto pLogicViewer::scrollToActive() -> void {
    GtkAdjustment* hadj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(gtkWidget));
    int scrollPos = (int)gtk_adjustment_get_value(hadj);

    unsigned scrollSlot = 0;
    for (auto& logicState : logicViewer.state.logics) {
        if (!logicState.active)
            break;

        scrollSlot++;
    }

    unsigned targetPos = scrollSlot * (DMA_SLOT_WIDTH + 1);
    unsigned width = logicViewer.geometry().width >> 1;

    if (targetPos > width)
        targetPos -= width;
    else
        targetPos = 0;

    unsigned counter = scrollTimer.data();

    if ( ((counter + 1) >= SCROLL_STEPS) || (targetPos == scrollPos) ) {
        scrollPos = targetPos;
        scrollTimer.setEnabled( false );
        scrollTimer.setData(0);
    } else {
        if (targetPos < scrollPos)
            scrollPos -= (scrollPos - targetPos) / (SCROLL_STEPS - counter);
        else
            scrollPos += (targetPos - scrollPos) / (SCROLL_STEPS - counter);

        scrollTimer.setData(counter + 1);
        scrollTimer.setEnabled( true );
    }

    gtk_adjustment_set_value(hadj, (double)scrollPos);

    if (gtk_widget_get_realized(subWidget)) {
        gdk_window_invalidate_rect(gtk_widget_get_window(subWidget), nullptr, true);
    }
}

auto pLogicViewer::update() -> void {
    unsigned maxSlots = logicViewer.state.logics.size();
    unsigned neededWidth = maxSlots * (DMA_SLOT_WIDTH + 1);
    const auto& geometry = logicViewer.geometry();
    gtk_widget_set_size_request(subWidget, neededWidth, geometry.height);

    if (gtk_widget_get_realized(subWidget)) {
        gdk_window_invalidate_rect(gtk_widget_get_window(subWidget), nullptr, true);
    }
}

auto pLogicViewer::redraw(cairo_t* cr) -> void {
    unsigned maxSlots = logicViewer.state.logics.size();
    unsigned neededWidth = maxSlots * (DMA_SLOT_WIDTH + 1);
    const auto& geometry = logicViewer.geometry();

    unsigned width = geometry.width;
    unsigned height = geometry.height;

    GtkAdjustment* hadj = gtk_scrolled_window_get_hadjustment( GTK_SCROLLED_WINDOW(gtkWidget));
    double _scrollOffset = gtk_adjustment_get_value(hadj);

    unsigned offset = 0;
    unsigned scrollPos = (unsigned)_scrollOffset;

    //fprintf(stderr, "%i ", scrollPos);

    if (neededWidth <= width)
        width = neededWidth;
    else {
        offset = neededWidth - width;

        if (offset >= scrollPos)
            offset = scrollPos;
    }

    unsigned firstSlot = offset / (DMA_SLOT_WIDTH + 1);
    offset = offset % (DMA_SLOT_WIDTH + 1);
    unsigned buildSlots = (width / (DMA_SLOT_WIDTH + 1)) + 2;

    if (firstSlot >= maxSlots)
        firstSlot = 0;

    buildSlots = firstSlot + buildSlots;
    if (buildSlots > maxSlots)
        buildSlots = maxSlots;

    int _startX = firstSlot * (DMA_SLOT_WIDTH + 1);

    Geometry geo = {_startX, 0, DMA_SLOT_WIDTH, height};

    cairo_set_font_size(cr, 11);

    for (unsigned i = firstSlot; i < buildSlots; i++) {
        auto& logicState = logicViewer.state.logics[i];
        buildDmaSlot(cr,logicState, geo);
        geo.x += DMA_SLOT_WIDTH + 1;
    }
}

auto pLogicViewer::buildDmaSlot(cairo_t* cr, LogicState& logicState, Geometry geo) -> void {
    int addrLength = logicViewer.addrAs24bit() ? 6 : 4;
    double _col = getColorComponent(0x54);
    cairo_set_source_rgb(cr, _col, _col, _col);
    cairo_set_line_width(cr, 1.0);

    cairo_move_to(cr, pg(geo.x + geo.width), pg(geo.y));
    cairo_line_to(cr, pg(geo.x + geo.width), pg(geo.y + geo.height));
    cairo_stroke(cr);

    geo.height = 20;
    geo.y += 5;

    if (logicState.active) {
        _col = getColorComponent(0xe0);
    } else {
        _col = getColorComponent(0x80);
    }

    cairo_set_source_rgb(cr, _col, _col, _col);
    drawText(cr, geo, std::to_string(logicState.position));

    geo.y += geo.height + 5;

    if (logicState.display != LogicState::Display::EmptyBlock) {
        unsigned& sCol = logicState.color;
        cairo_set_source_rgb(cr, getColorComponent((sCol >> 16) & 0xff), getColorComponent((sCol >> 8) & 0xff), getColorComponent((sCol >> 0) & 0xff));
        cairo_set_line_width(cr, 5.0);
        cairo_move_to(cr, geo.x, geo.y);
        cairo_line_to(cr, geo.x + geo.width, geo.y);
        cairo_stroke(cr);
    }

    cairo_set_line_width(cr, 1.0);

    setBox(geo, (int)LogicState::Offset::Usage1);

    cairo_set_source_rgb(cr, _col, _col, _col);

    if (logicState.display == LogicState::Display::EmptyBlock) {
        drawText(cr, geo, "-");
        setBox(geo, (int)LogicState::Offset::Addr1);
        drawLine(cr, geo);
        setBox(geo, (int)LogicState::Offset::Data1);
        drawLine(cr, geo);

    } else {
        drawText(cr, geo, logicState.usage);
        setBox(geo, (int)LogicState::Offset::Addr1);
        std::string _addr = logicState.symbolicAddr.empty() ? String::convertToHex(logicState.addr, addrLength) : logicState.symbolicAddr;
        drawRectRounded(cr, geo, _addr, 5);

        setBox(geo, (int)LogicState::Offset::Data1);
        drawRectRounded(cr, geo, String::convertToHex(logicState.data), 10);
    }

    setBox(geo, (int)LogicState::Offset::OpCode);
    if (logicState.opCode) {
        std::string _opCode = logicState.opCode;
        String::toUpperCase( _opCode );
        if (logicState.hilight == LogicState::Hilight::Opcode) {
            cairo_set_source_rgb(cr, getColorComponent(0x87), getColorComponent(0xce), getColorComponent(0xfa));
            drawText(cr, geo, _opCode);
            cairo_set_source_rgb(cr, _col, _col, _col);
        } else
            drawText(cr, geo, _opCode);
    } else
        drawText(cr, geo, "");

    setBox(geo, (int)LogicState::Offset::Usage2);

    if (logicState.display2 == LogicState::Display::EmptyBlock) {
        drawText(cr, geo, "-");
        setBox(geo, (int)LogicState::Offset::Addr2);
        drawLine(cr, geo);
        setBox(geo, (int)LogicState::Offset::Data2);
        drawLine(cr, geo);

    } else {
        drawText(cr, geo, logicState.usage2);
        setBox(geo, (int)LogicState::Offset::Addr2);
        drawRectRounded(cr, geo, String::convertToHex(logicState.addr2, addrLength), 5);

        setBox(geo, (int)LogicState::Offset::Data2);

        if (logicState.hilight == LogicState::Hilight::Write) {
            drawRectRounded(cr, geo, 10);
            cairo_stroke(cr);
            cairo_set_source_rgb(cr, getColorComponent(0xff), getColorComponent(0x6f), getColorComponent(0x61));
            drawText(cr, geo, String::convertToHex(logicState.data2) );
            cairo_set_source_rgb(cr, _col, _col, _col);
        } else
            drawRectRounded(cr, geo, String::convertToHex(logicState.data2), 10);
    }

    int i = 0;
    for (auto& watch : logicState.watches) {
        setBox(geo, (int)(LogicState::Offset::Watch1) + i++);
        drawRect(cr, watch.first, geo, String::convertToHex(watch.second), 10);
    }
}

auto pLogicViewer::drawRect(cairo_t* cr, LogicState::Display display, Geometry& geo, const std::string& text, unsigned padding) -> void {
    switch (display) {
        default:
        case LogicState::Display::EmptyBlock:
            drawLine(cr, geo);
            break;
        case LogicState::Display::SingleBlock:
            drawRectRounded(cr, geo, text, padding);
            break;
        case LogicState::Display::BeginBlock:
            drawRectLeftRounded(cr, geo, text, padding);
            break;
        case LogicState::Display::KeepBlock:
            drawRect(cr, geo, text);
            break;
        case LogicState::Display::EndBlock:
            drawRectRightRounded(cr, geo, text, padding);
            break;
    }
}

auto pLogicViewer::drawRect(cairo_t* cr, Geometry& geo, const std::string& text) -> void {
    cairo_move_to(cr, pg(geo.x), pg(geo.y));
    cairo_line_to(cr, pg(geo.x + geo.width), pg(geo.y));

    cairo_move_to(cr, pg(geo.x), pg(geo.y + geo.height));
    cairo_line_to(cr, pg(geo.x + geo.width), pg(geo.y + geo.height));

    drawText(cr, geo, text);
}

auto pLogicViewer::drawLine(cairo_t* cr, Geometry& geo) -> void {
    unsigned center = geo.y + (geo.height / 2);
    cairo_move_to(cr, pg(geo.x), pg(center));
    cairo_line_to(cr, pg(geo.x + geo.width), pg(center));
    cairo_stroke(cr);
}

auto pLogicViewer::drawText(cairo_t* cr, Geometry& geo, const std::string& text) -> void {
    cairo_text_extents_t te;
    cairo_text_extents(cr, text.c_str(), &te);

    int textX = geo.x;
    int textY = geo.y + te.height;

    if (te.width < geo.width)
        textX += (geo.width - (int)te.width) / 2;

    if (te.height < geo.height)
        textY += (geo.height - (int)te.height) / 2;

    cairo_move_to(cr, textX, textY + 1);
    cairo_show_text(cr, text.c_str());
    cairo_stroke(cr);
}

auto pLogicViewer::drawRectRounded(cairo_t* cr, Geometry& geo, const std::string& text, unsigned padding) -> void {
    drawRectRounded(cr, geo, padding);
    drawText(cr, geo, text );
}

inline auto pLogicViewer::drawRectRounded(cairo_t* cr, Geometry& geo, unsigned padding) -> void {
    unsigned center = geo.y + (geo.height / 2);

    cairo_move_to(cr, pg(geo.x), pg(center));
    cairo_line_to(cr, pg(geo.x + padding), pg(center));

    cairo_move_to(cr, pg(geo.x + geo.width - padding), pg(center));
    cairo_line_to(cr, pg(geo.x + geo.width), pg(center));

    auto _geo = geo;
    _geo.x += padding;
    _geo.width -= padding * 2;
    getRoundedPath(cr, _geo);
}

auto pLogicViewer::drawRectLeftRounded(cairo_t* cr, Geometry& geo, const std::string& text, unsigned padding) -> void {
    unsigned center = geo.y + (geo.height / 2);

    cairo_move_to(cr, pg(geo.x), pg(center));
    cairo_line_to(cr, pg(geo.x + padding), pg(center));
    cairo_stroke(cr);

    auto _geo = geo;
    _geo.x += padding;
    _geo.width -= padding;
    getLeftRoundedPath(cr, _geo);

    drawText(cr, _geo, text );
}

auto pLogicViewer::drawRectRightRounded(cairo_t* cr, Geometry& geo, const std::string& text, unsigned padding) -> void {
    unsigned center = geo.y + (geo.height / 2);

    cairo_move_to(cr, pg(geo.x + geo.width - padding), pg(center));
    cairo_line_to(cr, pg(geo.x + geo.width), pg(center));
    cairo_stroke(cr);

    auto _geo = geo;
    _geo.width -= padding;
    getRightRoundedPath(cr, _geo);

    drawText(cr, _geo, text );
}

auto pLogicViewer::getRightRoundedPath(cairo_t* cr, Geometry& geo) -> void {
    static constexpr double r = 5.0;
    double x = geo.x;
    double y = geo.y;
    double width = geo.width;
    double height = geo.height;

    cairo_new_path(cr);
    cairo_move_to(cr, pg(x), pg(y));
    cairo_line_to(cr, pg(x + width - r), pg(y));
    cairo_arc(cr,pg(x + width - r), pg(y + r),r,3 * G_PI / 2, 0);

    cairo_line_to(cr, pg(x + width), pg(y + height - r));
    cairo_arc(cr,pg(x + width - r), pg(y + height - r),r,0, G_PI / 2);

    cairo_line_to(cr, pg(x), pg(y + height));
}

auto pLogicViewer::getLeftRoundedPath(cairo_t* cr, Geometry& geo) -> void {
    static constexpr double r = 5.0;
    double x = geo.x;
    double y = geo.y;
    double width = geo.width;
    double height = geo.height;

    cairo_new_path(cr);
    cairo_move_to(cr, pg(x + width), pg(y));
    cairo_line_to(cr, pg(x + r), pg(y));
    cairo_arc_negative(cr,pg(x + r), pg(y + r),r,3 * G_PI / 2, G_PI);

    cairo_line_to(cr, pg(x), pg(y + height - r));
    cairo_arc_negative(cr,pg(x + r), pg(y + height - r),r,G_PI, G_PI / 2);
    cairo_line_to(cr, pg(x + width), pg(y + height));
}

auto pLogicViewer::getRoundedPath(cairo_t* cr, Geometry& geo) -> void {
    static double degrees = M_PI / 180.0;
    static constexpr double radius = 5.0;

    double x = geo.x;
    double y = geo.y;
    double width = geo.width;
    double height = geo.height;

    cairo_new_sub_path(cr);
    cairo_arc(cr, pg(x + width - radius), pg(y + radius), radius, -90 * degrees, 0 * degrees);
    cairo_arc(cr, pg(x + width - radius), pg(y + height - radius), radius, 0 * degrees, 90 * degrees);
    cairo_arc(cr, pg(x + radius), pg(y + height - radius), radius, 90 * degrees, 180 * degrees);
    cairo_arc(cr, pg(x + radius), pg(y + radius), radius, 180 * degrees, 270 * degrees);
    cairo_close_path(cr);
}

inline auto pLogicViewer::setBox(Geometry& geo, int offset) -> void {
    auto o = logicViewer.state.offsets;
    unsigned y = o[offset];
    geo.y = y > 23 ? y - 23 : y;
}

inline auto pLogicViewer::getColorComponent(uint8_t component) -> double {
    return (double)component / 255.0;
}

inline auto pLogicViewer::pg(int val) -> double { // align to pixel grid
    return double(val) + 0.5;
}