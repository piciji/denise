
auto pViewport::create() -> void {
    destroy();
    gtkWidget = gtk_drawing_area_new();

    gtk_widget_add_events(gtkWidget,
    GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_POINTER_MOTION_MASK);

    g_signal_connect(G_OBJECT(gtkWidget), "button-press-event", G_CALLBACK(pViewport::mousePress), (gpointer)this);
    g_signal_connect(G_OBJECT(gtkWidget), "button-release-event", G_CALLBACK(pViewport::mouseRelease), (gpointer)this);
    g_signal_connect(G_OBJECT(gtkWidget), "leave-notify-event", G_CALLBACK(pViewport::mouseLeave), (gpointer)this);
    g_signal_connect(G_OBJECT(gtkWidget), "motion-notify-event", G_CALLBACK(pViewport::mouseMove), (gpointer)this);
    //g_signal_connect(G_OBJECT(gtkWidget), "draw", G_CALLBACK(pViewport::drawEvent), (gpointer)this);

    g_signal_connect(gtk_widget_get_screen(gtkWidget), "monitors_changed", G_CALLBACK(pViewport::monitorsChanged), (gpointer)this);
    dragAnalyzed = false;
}

auto pViewport::init() -> void {
    create();
    setDroppable(viewport.droppable());
}

auto pViewport::add() -> void {
    // putting the viewport directly in an expanding box performs better
    if (viewport.window()->hints == Window::Hints::Video) {
        destroy();
        return;
    }

    pWidget::add();
}

auto pViewport::setGeometry(Geometry geometry) -> void {
    if (viewport.window()->hints == Window::Hints::Video)
        return;

    pWidget::setGeometry( geometry );
}

auto pViewport::setDroppable(bool droppable) -> void {


}

auto pViewport::handle() -> uintptr_t {
    if (viewport.window()->hints == Window::Hints::Video)
        return (uintptr_t) gtk_widget_get_window(viewport.window()->p.mainDisplay);
    //return GDK_WINDOW_XID(gtk_widget_get_window(gtkWidget));
    return (uintptr_t)gtk_widget_get_window(gtkWidget);
}

auto pViewport::dragEnd(GtkWidget* widget, GdkDragContext* context, Viewport* viewport) -> void {
    if( viewport->onDragLeave) {
        viewport->onDragLeave();
    }

    viewport->p.dragAnalyzed = false;
}

auto pViewport::dragDataReceived(GtkWidget* widget, GdkDragContext* context, gint x, gint y, GtkSelectionData* data, guint type, guint timestamp, Viewport* viewport) -> void {
    if (viewport->onDragEnter) {
        if(!viewport->p.dragAnalyzed) {
            viewport->p.dragAnalyzed = true;
            viewport->p.paths = getDropPaths(data);

            if (!viewport->p.paths.empty() && viewport->onDragEnter) {
                viewport->onDragEnter(viewport->p.paths);
            }
        }
        return;
    }

    viewport->p.dragAnalyzed = false;
    if(!viewport->state.droppable) return;
    auto paths = getDropPaths(data);
    if(paths.empty()) return;
    if(viewport->onDrop) viewport->onDrop(paths);
}

auto pViewport::dragMove(GtkWidget* widget, GdkDragContext* context, gint x, gint y, guint time, Viewport* viewport) -> gboolean {
    if( viewport->onDragMove) {
        viewport->onDragMove(x, y);
        return true;
    }
    return false;
}

auto pViewport::mouseLeave(GtkWidget* widget, GdkEventButton* event, pViewport* self) -> gboolean {
    if(self->viewport.onMouseLeave) self->viewport.onMouseLeave();
    return true;
}

auto pViewport::mouseMove(GtkWidget* widget, GdkEventButton* event, pViewport* self) -> gboolean {
        
    self->viewport.state.mousePos.x = event->x;
    self->viewport.state.mousePos.y = event->y;
	
	if(self->viewport.onMouseMove) 
        self->viewport.onMouseMove(self->viewport.state.mousePos);
        
    return true;
}

auto pViewport::mousePress(GtkWidget* widget, GdkEventButton* event, pViewport* self) -> gboolean {
    if(self->viewport.onMousePress) switch(event->button) {
        case 1: self->viewport.onMousePress(Mouse::Button::Left); break;
        case 2: self->viewport.onMousePress(Mouse::Button::Middle); break;
        case 3: self->viewport.onMousePress(Mouse::Button::Right); break;
    }
	
	if (event->button == 3) pWindow::onButtonPressed(widget, event, self->widget.window());
	
    return true;
}

auto pViewport::mouseRelease(GtkWidget* widget, GdkEventButton* event, pViewport* self) -> gboolean {
    if(self->viewport.onMouseRelease) switch(event->button) {
        case 1: self->viewport.onMouseRelease(Mouse::Button::Left); break;
        case 2: self->viewport.onMouseRelease(Mouse::Button::Middle); break;
        case 3: self->viewport.onMouseRelease(Mouse::Button::Right); break;
    }
    return true;
}

auto pViewport::drawEvent(GtkWidget* widget, cairo_t* context, pViewport* self) -> gboolean {
	cairo_set_source_rgba(context, 0.0, 0.0, 0.0, 1.0);
	cairo_paint(context);
	return true;
}

auto pViewport::monitorsChanged(GdkScreen* screen, pViewport* self) -> void {

    pMonitor::fetchDisplays();

    if (Application::onDisplayChange)
        Application::onDisplayChange();
}