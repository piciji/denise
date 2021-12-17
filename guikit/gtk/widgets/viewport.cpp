
auto pViewport::create() -> void {
    destroy();
    gtkWidget = gtk_drawing_area_new();

    gtk_widget_add_events(gtkWidget,
    GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_POINTER_MOTION_MASK);

    g_signal_connect(G_OBJECT(gtkWidget), "drag-data-received", G_CALLBACK(pViewport::dropEvent), (gpointer)&viewport);
    g_signal_connect(G_OBJECT(gtkWidget), "button-press-event", G_CALLBACK(pViewport::mousePress), (gpointer)this);
    g_signal_connect(G_OBJECT(gtkWidget), "button-release-event", G_CALLBACK(pViewport::mouseRelease), (gpointer)this);
    g_signal_connect(G_OBJECT(gtkWidget), "leave-notify-event", G_CALLBACK(pViewport::mouseLeave), (gpointer)this);
    g_signal_connect(G_OBJECT(gtkWidget), "motion-notify-event", G_CALLBACK(pViewport::mouseMove), (gpointer)this);
	g_signal_connect(G_OBJECT(gtkWidget), "draw", G_CALLBACK(pViewport::drawEvent), (gpointer)this);
}

auto pViewport::init() -> void {
    create();
    setDroppable(viewport.droppable());
}

auto pViewport::setDroppable(bool droppable) -> void {
    gtk_drag_dest_set(gtkWidget, GTK_DEST_DEFAULT_ALL, nullptr, 0, GDK_ACTION_COPY);
    if(droppable) gtk_drag_dest_add_uri_targets(gtkWidget);
}

auto pViewport::handle() -> uintptr_t {
    //return GDK_WINDOW_XID(gtk_widget_get_window(gtkWidget));
    return (uintptr_t)gtk_widget_get_window(gtkWidget);
}

auto pViewport::dropEvent(GtkWidget* widget, GdkDragContext* context, gint x, gint y,
GtkSelectionData* data, guint type, guint timestamp, Viewport* viewport) -> void {
    if(!viewport->state.droppable) return;
    auto paths = getDropPaths(data);
    if(paths.empty()) return;
    if(viewport->onDrop) viewport->onDrop(paths);
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

static auto Window_onButtonPressed(GtkWidget* widget, GdkEventButton* event, Window* window) -> gboolean;

auto pViewport::mousePress(GtkWidget* widget, GdkEventButton* event, pViewport* self) -> gboolean {
    if(self->viewport.onMousePress) switch(event->button) {
        case 1: self->viewport.onMousePress(Mouse::Button::Left); break;
        case 2: self->viewport.onMousePress(Mouse::Button::Middle); break;
        case 3: self->viewport.onMousePress(Mouse::Button::Right); break;
    }
	
	if (event->button == 3) Window_onButtonPressed(widget, event, self->widget.window());
	
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