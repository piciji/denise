
#include "main.h"
#include "../tools/crc32.h"

namespace GUIKIT {

#include "tools.cpp"
#include "menu.cpp"
#include "browserWindow.cpp"
#include "messageWindow.cpp"
#include "statusbar.cpp"
#include "display.cpp"
    
#include "widgets/widget.cpp"   
#include "widgets/button.cpp"   
#include "widgets/stepbutton.cpp"
#include "widgets/lineedit.cpp"
#include "widgets/multilineedit.cpp"
#include "widgets/label.cpp"
#include "widgets/hyperlink.cpp"
#include "widgets/checkbutton.cpp"
#include "widgets/checkbox.cpp"
#include "widgets/combobutton.cpp"
#include "widgets/slider.cpp"
#include "widgets/radiobox.cpp"
#include "widgets/progressbar.cpp"
#include "widgets/frame.cpp"
#include "widgets/tabframe.cpp"
#include "widgets/viewport.cpp"
#include "widgets/listview.cpp"
#include "widgets/treeview.cpp"
#include "widgets/squareCanvas.cpp"
#include "widgets/imageView.cpp"

pApplication::DesktopSession pApplication::desktopSession = pApplication::DesktopSession::Unknown;

auto pApplication::run() -> void {

    if(Application::loop) {
        while(!Application::isQuit) {
            Application::loop();
            processEvents();
        }
    } else if (!Application::isQuit)
        gtk_main();
}

auto pApplication::processEvents() -> void {
    while( gtk_events_pending() ) gtk_main_iteration_do(false);
}

auto pApplication::quit() -> void {
    if(gtk_main_level())
		gtk_main_quit();

    pMonitor::resetSetting();
    pMonitor::disconnect();
}

auto pApplication::initialize() -> void {
    XInitThreads();
    gdk_set_allowed_backends("x11");
    gtk_init(nullptr, nullptr);

// don't apply global CSS because of micro stutter
//    #include "css.cpp"
//    GtkCssProvider* cssProvider = gtk_css_provider_new();
//    gtk_css_provider_load_from_data(cssProvider, css.c_str(), -1, NULL);
//    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
//                                              GTK_STYLE_PROVIDER(cssProvider),
//                                              GTK_STYLE_PROVIDER_PRIORITY_USER);

    fetchDesktopSession();
}  

auto pApplication::fetchDesktopSession() -> void {
    desktopSession = DesktopSession::Unknown;

    const char* currentDesktop = getenv("XDG_CURRENT_DESKTOP");

    if (currentDesktop == nullptr)
        return;

    if (String::findString(currentDesktop, "Cinnamon"))
        desktopSession = DesktopSession::Cinnamon;
    else if (String::findString(currentDesktop, "GNOME"))
        desktopSession = DesktopSession::Gnome;
    else if (String::findString(currentDesktop, "KDE"))
        desktopSession = DesktopSession::KDE;
    else if (String::findString(currentDesktop, "MATE"))
        desktopSession = DesktopSession::Mate;
    else if (String::findString(currentDesktop, "XFCE"))
        desktopSession = DesktopSession::XFCE;
    else if (String::findString(currentDesktop, "Unity"))
        desktopSession = DesktopSession::Unity;
}

auto pApplication::pasteClipboardCallback(GtkClipboard* clipboard, const gchar* text, gpointer data) -> void {
	
	if (text && Application::onClipboardRequest)
		Application::onClipboardRequest( (char*)text );
}

auto pApplication::requestClipboardText() -> void {
	
	gtk_clipboard_request_text(gtk_clipboard_get(GDK_NONE), pasteClipboardCallback, nullptr);
}

auto pApplication::setClipboardText( std::string text ) -> void {
	
	GdkDisplay* display = gdk_display_get_default();
	
	GtkClipboard* clipboard = gtk_clipboard_get_for_display(display, GDK_SELECTION_CLIPBOARD);
	
	gtk_clipboard_set_text(clipboard, text.c_str(), -1);
	
	if (gdk_display_supports_clipboard_persistence(display))
		gtk_clipboard_store(clipboard);
}

//window

auto pWindow::onRealize(GtkWidget* widget, pWindow* self) -> void {
    if (self->window.onRealize)
        self->window.onRealize();
}

auto pWindow::mouseMove(GtkWidget* widget, GdkEventButton* event, pWindow* self) -> gboolean {
    if (self->viewport)
        return pViewport::mouseMove( widget, event, self->viewport );

    return false;
}

auto pWindow::mousePress(GtkWidget* widget, GdkEventButton* event, pWindow* self) -> gboolean {
    if (self->viewport)
        return pViewport::mousePress( widget, event, self->viewport );

    return false;
}

auto pWindow::mouseRelease(GtkWidget* widget, GdkEventButton* event, pWindow* self) -> gboolean {
    if (self->viewport)
        return pViewport::mouseRelease( widget, event, self->viewport );

    return false;
}

auto pWindow::monitorsChanged(GdkScreen* screen, pWindow* self) -> void {
    if (self->viewport) {
        pViewport::monitorsChanged(screen, self->viewport);
    }
}

auto pWindow::drawMain(GtkWidget* widget, cairo_t* context, Window* window) -> gboolean {
	
    if(!window->p.overrideBackgroundColor) {		
		auto style = gtk_widget_get_style_context(widget);
		GtkAllocation allocation;
		gtk_widget_get_allocation(widget, &allocation);
		gtk_render_background(style, context, 0, 0, allocation.width, allocation.height);
		return false;
	}

    unsigned color = window->p.backgroundColor;
    double red   = (double)((color >> 16) & 0xff) / 255.0;
    double green = (double)((color >> 8) & 0xff) / 255.0;
    double blue  = (double)((color >> 0) & 0xff) / 255.0;
	
    if(gdk_screen_is_composited(gdk_screen_get_default())
    && gdk_screen_get_rgba_visual(gdk_screen_get_default())
    ) {
		cairo_set_source_rgba(context, red, green, blue, 1.0);
    } else {
		cairo_set_source_rgb(context, red, green, blue);
    }

	cairo_set_operator(context, CAIRO_OPERATOR_SOURCE);
    cairo_paint(context);
		
    return false;
}

auto pWindow::draw(GtkWidget* widget, cairo_t* context, Window* window) -> gboolean {
	  	
	auto style = gtk_widget_get_style_context(widget);
	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);
	gtk_render_background(style, context, 0, 0, allocation.width, allocation.height);
	return false;
}

auto pWindow::close(GtkWidget* widget, GdkEvent* event, Window* window) -> gint {
    if(window->onClose) window->onClose();
    else window->setVisible(false);
    return true;
}

auto pWindow::drop(GtkWidget* widget, GdkDragContext* context, gint x, gint y, GtkSelectionData* data, guint type, guint timestamp, Window* window) -> void {
    if( !window->state.droppable ) return;
    auto paths = getDropPaths(data);
    if(paths.empty()) return;
    if(window->onDrop) window->onDrop(paths);
}

auto pWindow::dragMove(GtkWidget* widget, GdkDragContext* context, gint x, gint y, guint time, pWindow* self) -> gboolean {
    if (self->viewport && self->viewport->viewport.droppable()) {
        if (!self->viewport->dragAnalyzed && self->viewport->viewport.onDragEnter) {
            GtkTargetList* targetlist = gtk_drag_dest_get_target_list(widget);
            GList* list = gdk_drag_context_list_targets(context);
            if (list) {
                while (list) {
                    GdkAtom atom = (GdkAtom)g_list_nth_data(list, 0);
                    if (gtk_target_list_find(targetlist, GDK_POINTER_TO_ATOM(g_list_nth_data(list, 0)), NULL)) {
                        gtk_drag_get_data(widget, context, atom, time);
                        break;
                    }
                    list = g_list_next(list);
                }
            }
           return true;
        }

        int scale = gtk_widget_get_scale_factor(self->widget);
        return pViewport::dragMove(widget, context, x * scale, (y - self->menuHeight) * scale, time, &self->viewport->viewport);
    }
    return false;
}

auto pWindow::dragEnd(GtkWidget* widget, GdkDragContext* context, guint time, pWindow* self) -> void {
    if (self->viewport && self->viewport->viewport.droppable())
        pViewport::dragEnd( widget, context, &self->viewport->viewport );
}

auto pWindow::dragDataReceived(GtkWidget* widget, GdkDragContext* context, gint x, gint y, GtkSelectionData* data, guint type, guint timestamp, pWindow* self) -> void {
    if (self->viewport && self->viewport->viewport.droppable())
        pViewport::dragDataReceived( widget, context, x, y, data, type, timestamp, &self->viewport->viewport );
    else {
        drop(widget, context, x, y, data, type, timestamp, &self->window);
    }
}

auto pWindow::dragDrop(GtkWidget* widget, GdkDragContext* context, gint x, gint y, guint time, pWindow* self) -> gboolean {
    if(self->viewport && self->viewport->viewport.onDrop && self->viewport->viewport.onDragEnter)
        self->viewport->viewport.onDrop(self->viewport->paths);

    return TRUE;
}

auto pWindow::configure(GtkWidget* widget, GdkEvent* event, pWindow* p) -> gboolean {
	p->moveWindow( event );
	return false;
}

auto pWindow::sizeAllocate(GtkWidget* widget, GtkAllocation* allocation, pWindow* p) -> void {
    if (allocation->height < 0)
        return;

	p->sizeWindow( allocation );
}

auto pWindow::getPreferredWidth(GtkWidget* widget, int* minimalWidth, int* naturalWidth) -> void {
  
	if(auto p = (pWindow*)g_object_get_data(G_OBJECT(widget), "window")) {		
		*minimalWidth = 1;
		*naturalWidth = p->window.state.geometry.width;  
	}	
}

auto pWindow::getPreferredHeight(GtkWidget* widget, int* minimalHeight, int* naturalHeight) -> void {
  
	if(auto p = (pWindow*)g_object_get_data(G_OBJECT(widget), "window")) {		
		*minimalHeight = 1;
		*naturalHeight = p->window.state.geometry.height;
	}
}

auto pWindow::onButtonPressed(GtkWidget* widget, GdkEventButton* event, Window* window) -> gboolean {
		
	if (event->type == GDK_BUTTON_PRESS && event->button == 3) {		
		if (!window->onContext) return false;
		if (!window->onContext()) return false;
		gtk_menu_popup_at_pointer(GTK_MENU(window->p.contextMenu), nullptr);
	}
	return true;
}

auto pWindow::stateChange(GtkWidget* widget, GdkEventWindowState* event, Window* window) -> gboolean {

	if(event->new_window_state & GDK_WINDOW_STATE_ICONIFIED) {
		window->p.isMinimized = true;
		if(window->onMinimize)
			window->onMinimize();
	} else if(event->new_window_state & GDK_WINDOW_STATE_FOCUSED) {
		if (window->onFocus)
			window->onFocus();

        if (window->p.isMinimized) {
            if(window->onUnminimize)
                window->onUnminimize();

            window->p.isMinimized = false;
        }
	}

	return false;
}

pWindow::pWindow(Window& window, Window::Hints hints) : window(window) {

    lastAllocation.width  = lastAllocation.height = 0;
    widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    viewport = nullptr;
    
    setIcon( pSystem::getIconFolder() );

//	auto visual = gdk_screen_get_rgba_visual(gdk_screen_get_default());
//	if(!visual) visual = gdk_screen_get_system_visual(gdk_screen_get_default());
	//if(visual) gtk_widget_set_visual(widget, visual);
	
	gtk_widget_add_events(widget, (gint)GDK_BUTTON_PRESS_MASK | (gint)GDK_CONFIGURE);
    gtk_widget_set_app_paintable(widget, true);

    verticalLayout = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(widget), verticalLayout);
    gtk_widget_show(verticalLayout);

    menu = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(verticalLayout), menu, false, false, 0);
	contextMenu = gtk_menu_new();

    if (hints == Window::Hints::Video) {
        mainDisplay = gtk_drawing_area_new();
        gtk_widget_add_events(mainDisplay, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);
    } else
        mainDisplay = gtk_fixed_new();

    gtk_box_pack_start(GTK_BOX(verticalLayout), mainDisplay, true, true, 0);
    gtk_widget_show(mainDisplay);

    statusContainer = gtk_event_box_new();
	
    gtk_box_pack_start(GTK_BOX(verticalLayout), statusContainer, false, false, 0);
    gtk_widget_show(statusContainer);
		
    setResizable(window.resizable());

    g_signal_connect(G_OBJECT(widget), "delete-event", G_CALLBACK(pWindow::close), (gpointer)&window);
	g_signal_connect(G_OBJECT(widget), "draw", G_CALLBACK(pWindow::draw), (gpointer)&window);
	g_signal_connect(G_OBJECT(mainDisplay), "draw", G_CALLBACK(pWindow::drawMain), (gpointer)&window);

    g_signal_connect(G_OBJECT(widget), "configure-event", G_CALLBACK(pWindow::configure), (gpointer)this);
	g_signal_connect(G_OBJECT(mainDisplay), "size-allocate", G_CALLBACK(pWindow::sizeAllocate), (gpointer)this);

    g_signal_connect(G_OBJECT(widget), "drag-data-received", G_CALLBACK(pWindow::dragDataReceived), (gpointer)this);
    dragMotionId = g_signal_connect(G_OBJECT(widget), "drag-motion", G_CALLBACK(pWindow::dragMove), (gpointer)this);
    g_signal_connect(G_OBJECT(widget), "drag-leave", G_CALLBACK(pWindow::dragEnd), (gpointer)this);
    g_signal_connect(G_OBJECT(widget), "drag-drop", G_CALLBACK(pWindow::dragDrop), (gpointer)this);

	g_signal_connect(G_OBJECT(widget), "button-press-event", G_CALLBACK(pWindow::onButtonPressed), (gpointer)&window);
	g_signal_connect(G_OBJECT(widget), "window-state-event", G_CALLBACK(pWindow::stateChange), (gpointer)&window);
    g_signal_connect(G_OBJECT(mainDisplay), "realize", G_CALLBACK (pWindow::onRealize), (gpointer)this);

    if (hints == Window::Hints::Video) {
        g_signal_connect(G_OBJECT(mainDisplay), "motion-notify-event", G_CALLBACK(pWindow::mouseMove), (gpointer)this);
        g_signal_connect(G_OBJECT(mainDisplay), "button-press-event", G_CALLBACK(pWindow::mousePress), (gpointer)this);
        g_signal_connect(G_OBJECT(mainDisplay), "button-release-event", G_CALLBACK(pWindow::mouseRelease), (gpointer)this);
        g_signal_connect(gtk_widget_get_screen(mainDisplay), "monitors_changed", G_CALLBACK(pWindow::monitorsChanged), (gpointer)this);
    }
	
	auto widgetClass = GTK_WIDGET_GET_CLASS(mainDisplay);
	widgetClass->get_preferred_width  = pWindow::getPreferredWidth;
	widgetClass->get_preferred_height = pWindow::getPreferredHeight;

	g_object_set_data(G_OBJECT(widget), "window", (gpointer)this);
	g_object_set_data(G_OBJECT(mainDisplay), "window", (gpointer)this);

	
    timer.setInterval(150);
    timer.onFinished = [this]() {
        timer.setEnabled(false);
        locked = false;
    };
	
	timerResize.setInterval(100);
	timerResize.onFinished = [this]() {
        timerResize.setEnabled(false);

		if(this->window.state.layout) {
            this->window.state.layout->resetSynchronisation();
			Geometry layoutGeometry = this->window.geometry();
			layoutGeometry.x = layoutGeometry.y = 0;
			this->window.state.layout->setGeometry(layoutGeometry);
		}

        // update layout, otherwise status container is placed at wrong position when switching fullscreen
        gtk_box_set_child_packing (GTK_BOX(verticalLayout), statusContainer, false, false, 0, GTK_PACK_START);

        if (requestFullscreenToggle)
            if( this->window.onSize) this->window.onSize(Window::SIZE_MODE::Default);

        if (resizing) {
            resizing = false;
            if (this->window.onResizeEnd && !this->window.fullScreen())
                this->window.onResizeEnd();
        }

        requestFullscreenToggle = false;
    };

    timerFullscreen.setInterval( 150 );
    timerFullscreen.onFinished = [this]() {
        timerFullscreen.setEnabled(false);
        locked = false;
        setGeometry(this->window.state.geometry);
    };
}

auto pWindow::applyAspectRatio() -> void {

    updateGeometryHint();
}

auto pWindow::updateGeometryHint() -> void {

    unsigned hints = GDK_HINT_MIN_SIZE /*| GDK_HINT_RESIZE_INC*/;
    GdkGeometry geom;
    unsigned statusHeight = 0;
    auto aspect = window.aspectRatio();

    if (!window.fullScreen() && aspect.width && aspect.height) {

        aspect.height = (unsigned)((float)window.state.geometry.width * ((float)aspect.height / (float)aspect.width) + 0.5);
        aspect.width = window.state.geometry.width;

        if (window.statusBar())
            statusHeight = window.statusBar()->p.getHeight();

        aspect.height += statusHeight + menuHeight;
        if (pApplication::desktopSession == pApplication::DesktopSession::KDE) {
            // todo: don't understand why this is necessary in KDE Plasma, works as expected in Cinnamon, GNOME, Mate or XFCE
            if (aspect.height > 21)
                aspect.height -= 21;
        }

        double ratio = (double)aspect.width / (double)aspect.height;
        if (pApplication::desktopSession == pApplication::DesktopSession::Cinnamon || pApplication::desktopSession == pApplication::DesktopSession::Mate)            
            geom.min_aspect = ratio; // blocks resizing in KDE, GNOME and XFCE, works in Cinnamon or Mate, need to test others
            
        geom.max_aspect = ratio;
        hints |= GdkWindowHints::GDK_HINT_ASPECT;
    }

    geom.min_width = 100;
    geom.min_height = 100;

    gtk_window_set_geometry_hints(GTK_WINDOW(widget), nullptr, &geom, (GdkWindowHints)hints);
}

auto pWindow::handle() -> uintptr_t {
    //return GDK_WINDOW_XID(gtk_widget_get_window(widget));
    return (uintptr_t)gtk_widget_get_window(widget);
    //return (uintptr_t)widget;
}

auto pWindow::setDroppable(bool droppable) -> void {
    if (viewport && viewport->viewport.onDragEnter)
        gtk_drag_dest_set(widget, (GtkDestDefaults)(GTK_DEST_DEFAULT_MOTION ), nullptr, 0, GDK_ACTION_COPY);
    else
        gtk_drag_dest_set(widget, (GtkDestDefaults)(GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP ), nullptr, 0, GDK_ACTION_COPY);

    if(droppable) gtk_drag_dest_add_uri_targets(widget);
}

auto pWindow::setFocused() -> void {
    gtk_window_present(GTK_WINDOW(widget));
}

auto pWindow::setResizable(bool resizable) -> void {
    gtk_window_set_resizable(GTK_WINDOW(widget), resizable);

    updateGeometryHint();
}

auto pWindow::setTitle(std::string text) -> void {
    gtk_window_set_title(GTK_WINDOW(widget), text.c_str());
}

auto pWindow::setBackgroundColor(unsigned color) -> void {
    backgroundColor = color;
    overrideBackgroundColor = true;
}

auto pWindow::focused() -> bool {
    return gtk_window_is_active(GTK_WINDOW(widget));
}

auto pWindow::append(Menu& menu) -> void {
	gtk_menu_shell_append(GTK_MENU_SHELL(this->contextMenu), menu.p.elementC.widget);
    if (!menu.contextOnly())
        gtk_menu_shell_append(GTK_MENU_SHELL(this->menu), menu.p.element.widget);
	gtk_widget_show(menu.p.element.widget);
	gtk_widget_show(menu.p.elementC.widget);
}

auto pWindow::remove(Menu& menu) -> void {
    menu.p.rebuild();
}

auto pWindow::append(Widget& widget) -> void {
    if (dynamic_cast<Viewport*>(&widget))
        viewport = dynamic_cast<pViewport*>(&widget.p);

    widget.p.add();
}

auto pWindow::remove(Widget& widget) -> void {
    widget.p.init();
}

auto pWindow::append(StatusBar& statusBar) -> void {
	statusBar.p.create();
}

auto pWindow::remove(StatusBar& statusBar) -> void {
	statusBar.p.destroy();
}

auto pWindow::append(Layout& layout) -> void {
    Geometry geometry = this->geometry();
    geometry.x = geometry.y = 0;
    layout.setGeometry(geometry);
}

auto pWindow::addCustomFont( CustomFont* customFont ) -> bool {

    return pFont::add( customFont );
}

auto pWindow::changeCursor( Image& image, unsigned hotSpotX, unsigned hotSpotY ) -> void {
    
    if(image.empty()) {
        
        setDefaultCursor();
    
        return;
    } 
    
    if (cursor)
        g_object_unref( cursor );
    
    cursor = nullptr;

    auto pixBuf = CreatePixbuf( image );

    if (!pixBuf)
        return;

    cursor = CreateCursor( widget, pixBuf, hotSpotX, hotSpotY );            
        
    if (cursor)
        SetCursor( widget, cursor );
}

auto pWindow::setDefaultCursor() -> void {
    
    if (cursor)
        g_object_unref( cursor );
    
    cursor = gdk_cursor_new_for_display( gdk_screen_get_display(gdk_screen_get_default()), GDK_ARROW );
    
    if (cursor)
        SetCursor( widget, cursor );
}
    
auto pWindow::setPointerCursor() -> void {
        
    if (cursor)
        g_object_unref( cursor );
            
    cursor = gdk_cursor_new_for_display( gdk_screen_get_display(gdk_screen_get_default()), GDK_HAND2 );
            
    if (cursor)
        SetCursor( widget, cursor );
}

auto pWindow::setIcon( std::string path ) -> bool {
   
    File file( path + String::toLowerCase( Application::name ) + ".png" );    
        
    if ( file.open() ) {
        
        Image image;
        image.loadPng( file.read(), file.getSize() );
        
        auto pixbuf = CreatePixbuf(image);
        gtk_window_set_icon(GTK_WINDOW(widget), pixbuf);
        g_object_unref(G_OBJECT(pixbuf));
        return true;
    }

    return false;    
}

auto pWindow::moveWindow(GdkEvent* event) -> void {
    if( !gtk_widget_get_realized(widget) ) return;
    if( !window.visible() ) return;
    if( window.fullScreen() ) return;

    GdkWindow* gdkWindow = gtk_widget_get_window(widget);
    GdkRectangle border;
    gdk_window_get_frame_extents(gdkWindow, &border);

    if(border.x != window.state.geometry.x || border.y != window.state.geometry.y) {
        window.state.geometry.x = border.x;
        window.state.geometry.y = border.y;
        if(window.onMove) window.onMove();
    }
}

auto pWindow::setVisible(bool visible) -> bool {
    
	if (visible)
		setGeometry( geometry() );	
	
	if (!window.menuVisible()) //dirty hack:
		/* if menu is not enabled when showing window, first calculation of menu size gives wrong results, so do it stealthy */
		gtk_widget_set_visible(menu, true);
	
    gtk_widget_set_visible(widget, visible);
		
	if (!window.menuVisible()) // dirty hack tail
		setMenuVisible(false);
		
	else if (visible)
		setMenuVisible(true);
	
	return false; // wait for realize event
}

auto pWindow::setMenuVisible(bool visible) -> void {
    gtk_widget_set_visible(menu, visible);
	
    if (!visible) menuHeight = 0;
    if (!gtk_widget_get_visible(widget)) return;

    calcMenuHeight();

    //if (window.fullScreen()) gtk_window_fullscreen(GTK_WINDOW(widget));
		
    resize( geometry() );
    updateGeometryHint();
}

auto pWindow::calcMenuHeight() -> void {
    menuHeight = 0;

    if(gtk_widget_get_visible(menu)) {
        GtkAllocation allocation;
        gtk_widget_get_allocation(menu, &allocation);
        menuHeight = allocation.height;				
    }
}

auto pWindow::setStatusVisible(bool visible) -> void {    
    if (!window.statusBar())
        return;
        
    window.statusBar()->p.setVisible( visible );
	
    if (!gtk_widget_get_visible(widget)) return;

    //if (window.fullScreen()) gtk_window_fullscreen(GTK_WINDOW(widget));

	// Why?
	// switching language between asian and european changes menu height
	calcMenuHeight();
	
    resize( geometry() );

    updateGeometryHint();
}

auto pWindow::sizeWindow(GtkAllocation* allocation) -> void {

    if( !window.visible() ) return;
	if (!gtk_widget_get_realized(mainDisplay)) return;
	
    if( allocation->width == lastAllocation.width
    && allocation->height == lastAllocation.height ) return;
	
    if( !window.fullScreen() && !locked ) {
        window.state.geometry.width  = allocation->width;
        window.state.geometry.height = allocation->height;
    }

    if (!resizing) {
        resizing = true;
        if (window.onResizeStart && !window.fullScreen())
            window.onResizeStart();
    }

	timerResize.setEnabled();
    if (window.aspectRatio().width)
        updateGeometryHint();

    if(this->window.state.layout) {
        this->window.state.layout->resetSynchronisation();
        Geometry layoutGeometry = this->window.geometry();
        layoutGeometry.x = layoutGeometry.y = 0;
        this->window.state.layout->setGeometry(layoutGeometry);
    }

    if (!requestFullscreenToggle)
        if( this->window.onSize) this->window.onSize(Window::SIZE_MODE::Default);
	
    lastAllocation = *allocation;
}

auto pWindow::resize(Geometry geo) -> void {
	statusHeight = 0;
	if (window.statusBar())
		statusHeight = window.statusBar()->p.getHeight();
		
	gtk_window_resize(GTK_WINDOW(widget), geo.width, geo.height + statusHeight + menuHeight );	
}

auto pWindow::setGeometry(Geometry geometry) -> void {

	if (!window.state.visible) return;
    calcMenuHeight();

    gtk_window_move(GTK_WINDOW(widget), geometry.x , geometry.y );

    resize(geometry);

    if(window.state.layout) {
        window.state.layout->resetSynchronisation();
        Geometry layoutGeometry = this->geometry();
        layoutGeometry.x = layoutGeometry.y = 0;
        window.state.layout->setGeometry(layoutGeometry);
    }
}

auto pWindow::geometry() -> Geometry {
    if(window.fullScreen()) {
        GtkAllocation allocation;
        gtk_widget_get_allocation(mainDisplay, &allocation);
        return {0, 0, (unsigned)allocation.width, (unsigned)allocation.height};
    }
    return window.state.geometry;
}

auto pWindow::fullScreenToggleDelayed() -> bool {
    if(locked) timer.setEnabled();
    return locked;
}

auto pWindow::minimized() -> bool {
	return isMinimized;
}

auto pWindow::restore() -> void {
	
	gtk_window_deiconify(GTK_WINDOW(widget));
	
	setFocused();
}

auto pWindow::setForeground() -> void {
   // gtk_window_set_focus( GTK_WINDOW(widget), nullptr );
	setFocused();
}

auto pWindow::setFullScreen(bool fullScreen) -> void {
    //if (!window.resizable()) return;
    locked = true;
    requestFullscreenToggle = true;
    timer.setEnabled();

    if(!fullScreen) {

        if (pMonitor::resetSetting())
            timerFullscreen.setInterval(1000);
        else
            timerFullscreen.setInterval(150);

        timer.setEnabled(false);
        locked = true;
        timerFullscreen.setEnabled();
        gtk_window_unfullscreen(GTK_WINDOW(widget));

    } else {
        if (window.fullscreenSetting.inUse)
            pMonitor::setSetting( window.fullscreenSetting.displayId, window.fullscreenSetting.settingId );

        gtk_window_fullscreen(GTK_WINDOW(widget));
        if (dragMotionId)
            g_signal_handler_disconnect(G_OBJECT(widget), dragMotionId);

        if (window.droppable()) {
            dragMotionId = g_signal_connect(G_OBJECT(widget), "drag-motion", G_CALLBACK(pWindow::dragMove), (gpointer)this);
            setDroppable(true);
        }
    }
}

}
