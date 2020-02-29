
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
	parentWidget = nullptr;
}

auto pWidget::focused() -> bool { 
    return gtkWidget && gtk_widget_has_focus(gtkWidget);
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
    
    if (!parentGtk)
        parentGtk = widget.window()->p.mainDisplay; 
    
	if (!widget.visible()) 
		gtk_widget_set_visible(gtkWidget, true);
	
    gtk_fixed_move(GTK_FIXED(parentGtk), gtkWidget, geometry.x, geometry.y);		
    gtk_widget_set_size_request(gtkWidget, geometry.width, geometry.height);	
	
	if (!widget.visible()) 
		gtk_widget_set_visible(gtkWidget, false);
	
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
		
		if (parentGtk != parentWidget)
			gtk_container_add(GTK_CONTAINER( parentGtk), gtkWidget);   
		
		parentWidget = parentGtk;
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
	
}

auto pWidget::setForegroundColor(unsigned color) -> void {
    if( !gtkWidget || !widget.overrideForegroundColor() )
        return;
}
