
pWidget::pWidget(Widget& widget) : widget(widget) { 
    setFont( Font::system() );
}

pWidget::~pWidget() {
    destroy();
    pFont::free(pfont);
}

auto pWidget::destroy() -> void {
    if(gtkWidget) gtk_widget_destroy(gtkWidget);
    gtkWidget = nullptr;
}

auto pWidget::focused() -> bool { 
    return gtkWidget && GTK_WIDGET_HAS_FOCUS(gtkWidget);
}

auto pWidget::setFocused() -> void {
    if(gtkWidget) gtk_widget_grab_focus(gtkWidget);
}

auto pWidget::setFont(std::string font) -> void {
    pFont::free(pfont);
    pfont = gtkWidget ? pFont::setFont(gtkWidget, font) : pFont::create(font);
}

auto pWidget::setEnabled(bool enabled) -> void {
    if(gtkWidget)
        gtk_widget_set_sensitive(gtkWidget, enabled);
}

auto pWidget::setVisible(bool visible) -> void {
    if(gtkWidget)
        gtk_widget_set_visible(gtkWidget, visible);
}

auto pWidget::setGeometry(Geometry geometry) -> void {
    if (!widget.window() || !gtkWidget)
        return;
       
    int selection = -1;
    Widget* parent = Layout::getParentWidget( &widget, selection);
    GtkWidget* parentGtk = nullptr;
        
    if (parent) {
        parentGtk = parent->p.getContainerWidget(selection);
        auto geo = parent->geometry();
        auto dis = parent->p.getDisplacement();
        
        geometry.x -= geo.x + dis.x;
        geometry.y -= geo.y + dis.y;        
    }
    
    if (!parentGtk) {
        parentGtk = widget.window()->p.mainDisplay;
    }
    
    gtk_fixed_move(GTK_FIXED(parentGtk), gtkWidget, geometry.x, geometry.y);
    gtk_widget_set_size_request(gtkWidget, geometry.width, geometry.height);
    if(widget.onSize)
        widget.onSize();
}

auto pWidget::setTooltip(std::string tooltip) -> void {
    if(tooltip.empty() || !gtkWidget) return;
    gtk_widget_set_tooltip_text((GtkWidget*)gtkWidget, tooltip.c_str());
 
}

auto pWidget::add() -> void {
    setTooltip( widget.tooltip() );
    
    if (gtkWidget && widget.window()) {
       
        int selection = -1;
        Widget* parent = Layout::getParentWidget( &widget, selection);
        
        GtkWidget* parentGtk = nullptr;
        
        if (parent) {           
            parentGtk = parent->p.getContainerWidget(selection);                        
        }
        
        if (!parentGtk) {
            parentGtk = widget.window()->p.mainDisplay;
        }
        
        gtk_container_add(GTK_CONTAINER( parentGtk), gtkWidget);                        
    }        
    
    setFont( widget.font() );
    widget.setEnabled(widget.enabled());
    widget.setVisible(widget.visible());   
    setBackgroundColor( widget.backgroundColor() );
    setForegroundColor( widget.foregroundColor() );
}

auto pWidget::setBackgroundColor(unsigned color) -> void {
    if( !gtkWidget || !widget.overrideBackgroundColor() )
        return;
    GdkColor gdkColor = CreateColor( (color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff );
    gtk_widget_modify_bg(gtkWidget, GTK_STATE_NORMAL, &gdkColor);
    gtk_widget_modify_base(gtkWidget, GTK_STATE_NORMAL, &gdkColor);
}

auto pWidget::setForegroundColor(unsigned color) -> void {
    if( !gtkWidget || !widget.overrideForegroundColor() )
        return;
    GdkColor gdkColor = CreateColor( (color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff );
    gtk_widget_modify_fg(gtkWidget, GTK_STATE_NORMAL, &gdkColor);
    gtk_widget_modify_text(gtkWidget, GTK_STATE_NORMAL, &gdkColor);
}
