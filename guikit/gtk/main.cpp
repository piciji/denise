
#include "main.h"

namespace GUIKIT {

#include "tools.cpp"
#include "menu.cpp"
#include "browserWindow.cpp"
#include "messageWindow.cpp"    
    
#include "widgets/widget.cpp"   
#include "widgets/button.cpp"   
#include "widgets/lineedit.cpp"
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

auto pApplication::run() -> void {
    if(Application::loop) {
        while(!Application::isQuit) {            
            Application::loop();
			processEvents();
        }
    } else gtk_main();
}

auto pApplication::processEvents() -> void {
    while( gtk_events_pending() ) gtk_main_iteration_do(false);
}

auto pApplication::quit() -> void {
    if(gtk_main_level()) gtk_main_quit();
}

auto pApplication::initialize() -> void {
    gtk_init(nullptr, nullptr);
}

//window
static auto Window_expose(GtkWidget* widget, GdkEvent* event, Window* window) -> gboolean {
    if(!window->p.overrideBackgroundColor) return false;
    cairo_t* context = gdk_cairo_create(widget->window);

    unsigned color = window->p.backgroundColor;
    double red   = (double)((color >> 16) & 0xff) / 255.0;
    double green = (double)((color >> 8) & 0xff) / 255.0;
    double blue  = (double)((color >> 0) & 0xff) / 255.0;

    cairo_set_source_rgb(context, red, green, blue);

    cairo_set_operator(context, CAIRO_OPERATOR_SOURCE);
    cairo_paint(context);
    cairo_destroy(context);
    return false;
}

static auto Window_close(GtkWidget* widget, GdkEvent* event, Window* window) -> gint {
    if(window->onClose) window->onClose();
    else window->setVisible(false);
    return true;
}

static auto Window_drop(GtkWidget* widget, GdkDragContext* context, gint x, gint y, GtkSelectionData* data, guint type, guint timestamp, Window* window) -> void {
    if( !window->state.droppable ) return;
    auto paths = getDropPaths(data);
    if(paths.empty()) return;
    if(window->onDrop) window->onDrop(paths);
}

static auto Window_configure(GtkWidget* widget, GdkEvent* event, Window* window) -> gboolean {
    if( !gtk_widget_get_realized(window->p.widget) ) return false;
    if( !window->visible() ) return false;
    if( window->fullScreen() ) return false;

    GdkWindow* gdkWindow = gtk_widget_get_window(widget);
    GdkRectangle border;
    gdk_window_get_frame_extents(gdkWindow, &border);

    if(border.x != window->state.geometry.x || border.y != window->state.geometry.y) {
        window->state.geometry.x = border.x;
        window->state.geometry.y = border.y;
        if(window->onMove) window->onMove();
    }
    return false;
}

static auto Window_sizeAllocate(GtkWidget* widget, GtkAllocation* allocation, Window* window) -> void {
    if( !window->visible() ) return;

    if( allocation->width == window->p.lastAllocation.width
    && allocation->height == window->p.lastAllocation.height ) return;

    if( !window->fullScreen() ) {
        window->state.geometry.width  = allocation->width;
        window->state.geometry.height = allocation->height;
    }

    if(window->state.layout) {
        Geometry layoutGeometry = window->geometry();
        layoutGeometry.x = layoutGeometry.y = 0;
        window->state.layout->setGeometry(layoutGeometry);
    }
    if( window->onSize) window->onSize();
    window->p.lastAllocation = *allocation;
}

static auto Window_sizeRequest(GtkWidget* widget, GtkRequisition* requisition, Window* window) -> void {
    requisition->width  = window->state.geometry.width;
    requisition->height = window->state.geometry.height;
}

static auto Window_onButtonPressed(GtkWidget* widget, GdkEventButton* event, Window* window) -> gboolean {
		
	if (event->type == GDK_BUTTON_PRESS && event->button == 3) {		
		if (!window->onContext) return false;
		if (!window->onContext()) return false;
		gtk_menu_popup(GTK_MENU(window->p.contextMenu), nullptr, nullptr, nullptr, nullptr, 0, gtk_get_current_event_time());
	}
	return true;
}

pWindow::pWindow(Window& window) : window(window) {
    lastAllocation.width  = lastAllocation.height = 0;
    widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    
    setIcon( pSystem::getIconFolder() );

	gtk_widget_add_events(widget, GDK_BUTTON_PRESS_MASK | GDK_CONFIGURE);
    gtk_widget_set_app_paintable(widget, true);

    verticalLayout = gtk_vbox_new(false, 0);
    gtk_container_add(GTK_CONTAINER(widget), verticalLayout);
    gtk_widget_show(verticalLayout);

    menu = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(verticalLayout), menu, false, false, 0);
	contextMenu = gtk_menu_new();
    
    mainDisplay = gtk_fixed_new();		
    gtk_box_pack_start(GTK_BOX(verticalLayout), mainDisplay, true, true, 0);
    gtk_widget_show(mainDisplay);

    statusContainer = gtk_event_box_new();
    status = gtk_statusbar_new();

    gtk_container_add(GTK_CONTAINER(statusContainer), status);
    gtk_box_pack_start(GTK_BOX(verticalLayout), statusContainer, false, false, 0);
    gtk_widget_show(statusContainer);

    setResizable(window.resizable());
    setGeometry(geometry());
    setStatusFont(Font::system());

    g_signal_connect(G_OBJECT(widget), "delete-event", G_CALLBACK(Window_close), (gpointer)&window);
    g_signal_connect(G_OBJECT(mainDisplay), "expose-event", G_CALLBACK(Window_expose), (gpointer)&window);
    g_signal_connect(G_OBJECT(widget), "configure-event", G_CALLBACK(Window_configure), (gpointer)&window);
    g_signal_connect(G_OBJECT(widget), "drag-data-received", G_CALLBACK(Window_drop), (gpointer)&window);

    g_signal_connect(G_OBJECT(mainDisplay), "size-allocate", G_CALLBACK(Window_sizeAllocate), (gpointer)&window);
    g_signal_connect(G_OBJECT(mainDisplay), "size-request", G_CALLBACK(Window_sizeRequest), (gpointer)&window);
	g_signal_connect(G_OBJECT(widget), "button-press-event", G_CALLBACK(Window_onButtonPressed), (gpointer)&window);

    timer.setInterval(50);
    timer.onFinished = [this]() {
        timer.setEnabled(false);
        locked = false;
    };
}

uintptr_t pWindow::handle() {
    //return GDK_WINDOW_XID(gtk_widget_get_window(widget));
    return (uintptr_t)gtk_widget_get_window(widget);
    //return (uintptr_t)widget;
}

void pWindow::setDroppable(bool droppable) {
    gtk_drag_dest_set(widget, GTK_DEST_DEFAULT_ALL, nullptr, 0, GDK_ACTION_COPY);
    if(droppable) gtk_drag_dest_add_uri_targets(widget);
}

void pWindow::setFocused() {
    gtk_window_present(GTK_WINDOW(widget));
}

void pWindow::setVisible(bool visible) {
    if (!window.menuVisible()) //dirty hack:
        /* if menu is not enabled when showing window, first calculation of menu size gives wrong results
         */
        gtk_widget_set_visible(menu, true);
    
    gtk_widget_set_visible(widget, visible);
    
    if (!window.menuVisible()) //dirty hack (tail)
        gtk_widget_set_visible(menu, false);
    
    if (!visible) return;
    setGeometry( geometry() );
}

void pWindow::setResizable(bool resizable) {
    gtk_window_set_resizable(GTK_WINDOW(widget), resizable);
    gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(status), resizable);
}

void pWindow::setStatusFont(std::string font) {
    PangoFontDescription* pStatusfont = pFont::setFont(status, font);
    statusHeight = pFont::size(pStatusfont, " ").height + 2;
    pFont::free(pStatusfont);
}

void pWindow::setTitle(std::string text) {
    gtk_window_set_title(GTK_WINDOW(widget), text.c_str());
}

void pWindow::setStatusText(std::string text) {
    gtk_statusbar_pop(GTK_STATUSBAR(status), 1);
    gtk_statusbar_push(GTK_STATUSBAR(status), 1, text.c_str());
}

void pWindow::setMenuVisible(bool visible) {
    gtk_widget_set_visible(menu, visible);
    if (!visible) menuHeight = 0;
    if (!gtk_widget_get_visible(widget)) return;

    calcMenuHeight();

    if (window.fullScreen()) gtk_window_fullscreen(GTK_WINDOW(widget));
    resize( geometry() );
}

void pWindow::calcMenuHeight() {
    menuHeight = 0;

    if(gtk_widget_get_visible(menu)) {
        GtkAllocation allocation;
        gtk_widget_get_allocation(menu, &allocation);
        menuHeight = allocation.height;
    }
}

void pWindow::setStatusVisible(bool visible) {
    gtk_widget_set_visible(status, visible);
    if (!gtk_widget_get_visible(widget)) return;

    if (window.fullScreen()) gtk_window_fullscreen(GTK_WINDOW(widget));
    resize( geometry() );
}

void pWindow::setBackgroundColor(unsigned color) {
    backgroundColor = color;
    overrideBackgroundColor = true;
}

bool pWindow::focused() {
    return gtk_window_is_active(GTK_WINDOW(widget));
}

void pWindow::resize(Geometry geo) {
    gtk_window_resize(GTK_WINDOW(widget), geo.width, geo.height + (gtk_widget_get_visible(status) ?  statusHeight : 0) + menuHeight );
    gtk_widget_set_size_request(mainDisplay, geo.width, geo.height);
}

void pWindow::setGeometry(Geometry geometry) {
    if (!gtk_widget_get_visible(widget)) return;
    calcMenuHeight();

    gtk_window_move(GTK_WINDOW(widget), geometry.x , geometry.y );

    GdkGeometry geom;
    geom.min_width  = window.resizable() ? 1 : geometry.width;
    geom.min_height = window.resizable() ? 1 : geometry.height;
    gtk_window_set_geometry_hints(GTK_WINDOW(widget), GTK_WIDGET(mainDisplay), &geom, GDK_HINT_MIN_SIZE);

    resize(geometry);

    if(window.state.layout) {
        Geometry layoutGeometry = this->geometry();
        layoutGeometry.x = layoutGeometry.y = 0;
        window.state.layout->setGeometry(layoutGeometry);
    }
}

Geometry pWindow::geometry() {
    if(window.fullScreen()) {
        GtkAllocation allocation;
        gtk_widget_get_allocation(mainDisplay, &allocation);
        return {0, 0, allocation.width, allocation.height};
    }
    return window.state.geometry;
}

bool pWindow::fullScreenToggleDelayed() {
    if(locked) timer.setEnabled();
    return locked;
}

void pWindow::setFullScreen(bool fullScreen) {
    if (!window.resizable()) return;
    locked = true;
    timer.setEnabled();

    if(!fullScreen) {
        gtk_window_unfullscreen(GTK_WINDOW(widget));
        setGeometry(window.state.geometry);
    } else {
        gtk_window_fullscreen(GTK_WINDOW(widget));
    }
}

void pWindow::append(Menu& menu) {
	gtk_menu_shell_append(GTK_MENU_SHELL(this->contextMenu), menu.p.cwidget);	
    gtk_menu_shell_append(GTK_MENU_SHELL(this->menu), menu.p.widget);    	
	gtk_widget_show(menu.p.widget);
	gtk_widget_show(menu.p.cwidget);
}

void pWindow::remove(Menu& menu) {
    menu.p.rebuild();
}
void pWindow::append(Widget& widget) {
    widget.p.add();
}

void pWindow::remove(Widget& widget) {
    widget.p.init();
}

void pWindow::append(Layout& layout) {
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
        gdk_cursor_unref( cursor );
    
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
        gdk_cursor_unref( cursor );
    
    cursor = gdk_cursor_new( GDK_ARROW );
    
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

}
