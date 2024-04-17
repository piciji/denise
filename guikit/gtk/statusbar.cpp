
pStatusBar::pStatusBar(StatusBar& statusBar) : statusBar(statusBar) {    
    gridWidget = nullptr;    
    pfont = nullptr;
	statusHeight = 0;
}

pStatusBar::~pStatusBar() {
	for (auto widget : usedWidgets)
		delete widget;	
	
    destroy();
    pFont::free(pfont);
}

auto pStatusBar::destroy() -> void {
		
	if (gridWidget) {
		delete gridWidget;
		gridWidget = nullptr;
	}
}

auto pStatusBar::create() -> void {
	
	destroy();
	
	gridWidget = gtk_grid_new();
	
	gtk_widget_set_margin_top( gridWidget, 3);
	gtk_widget_set_margin_bottom( gridWidget, 3);
	gtk_widget_set_margin_start( gridWidget, 4);
	gtk_widget_set_margin_end( gridWidget, 8);
	
	gtk_container_add(GTK_CONTAINER( statusBar.window()->p.statusContainer), gridWidget);
			
	update();	
	
	setFont( statusBar.font().empty() ? Font::system() : statusBar.font() );		
}

auto pStatusBar::setFont(const std::string& font) -> void {
	if (!gridWidget)
		return;
	
	pFont::free(pfont);
    pfont = pFont::setFont(gridWidget, font);
	statusHeight = pFont::size(pfont, "|").height + 2;
	gtk_widget_set_size_request( GTK_WIDGET(gridWidget), -1, statusHeight  );
	statusHeight += 6; // add top and bottom margin
}

auto pStatusBar::setText(const std::string& text) -> void {
	
	if (!gridWidget)
		return;
	
	gtk_label_set_text(GTK_LABEL(usedWidgets[0]), text.c_str());
}

auto pStatusBar::setVisible(bool visible) -> void {
	
	if (gridWidget)		
		gtk_widget_set_visible(gridWidget, visible);	
}

auto pStatusBar::getHeight() -> unsigned {
	
	if (!gridWidget || !gtk_widget_get_visible(gridWidget))
		return 0;

	return statusHeight;
}

auto pStatusBar::update() -> void {
	
	if (!statusBar.window() || !gridWidget)
        return;      
	
	gtk_grid_remove_row( GTK_GRID(gridWidget), 1 );
	
	gtk_widget_set_vexpand( gridWidget, false );
	
	for( auto widget : usedWidgets )
		delete widget;

    for( auto helper : helpers )
        gtk_widget_destroy( helper );
	
	usedWidgets.clear();
    helpers.clear();
	
	unsigned partCount = statusBar.state.parts.size();
	
	if (partCount == 0) { // simple status view
		Label* label = new Label;		
		label->setText( statusBar.text() );
		gtk_widget_set_hexpand( label->p.gtkWidget, true );
		usedWidgets.push_back( label );
		gtk_grid_attach_next_to (GTK_GRID(gridWidget), label->p.gtkWidget, nullptr, GtkPositionType::GTK_POS_LEFT, 1, 1);
		gtk_widget_show_now( label->p.gtkWidget );
		return;
	}
	
	auto& parts = statusBar.state.parts;
    unsigned countVisible = 0;
    for(auto& part : parts) {
        if (part.visible)
            countVisible++;
    }
	
	gtk_grid_set_column_spacing( GTK_GRID(gridWidget), 5 );
	
	gtk_widget_set_halign( gridWidget, GTK_ALIGN_START );
	
	for(auto& part : parts) {
		
		if (!part.visible)
			continue;
		
		part.position = usedWidgets.size();
		
		GtkWidget* gtkWidget;
		GtkWidget* gtkEventBox = nullptr;
		
		if (part.image) {
			Widget* widget = new Widget();
			
			widget->p.gtkWidget = (GtkWidget*)CreateImage( *part.image );
				
			gtkWidget = widget->p.gtkWidget;

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#else
            if (pApplication::desktopSession == pApplication::DesktopSession::XFCE || pApplication::desktopSession == pApplication::DesktopSession::KDE)
                gtk_widget_set_margin_top(gtkWidget, 2);
            else // Cinnamon, Mate, Gnome, untested
                gtk_widget_set_margin_top(gtkWidget, 1);
#endif
			usedWidgets.push_back( widget );
			
		} else {
			Label* label = new Label;

            label->setAlign( part.alignRight ? Label::Align::Right : Label::Align::Left );
			label->setText( part.text );

			if (part.overrideForegroundColor != -1)
				label->setForegroundColor( part.overrideForegroundColor );

			gtkWidget = label->p.gtkWidget;

			usedWidgets.push_back( label );
		}
		
		if (part.onClick || part.popupMenu) {
			gtkEventBox = gtk_event_box_new();

			gtk_widget_add_events(gtkEventBox, GDK_BUTTON_PRESS_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);

			g_signal_connect(G_OBJECT(gtkEventBox), "button-press-event", G_CALLBACK(pStatusBar::onClick), (gpointer)&part);

			g_signal_connect(G_OBJECT(gtkEventBox), "enter-notify-event", G_CALLBACK(pStatusBar::onEnter), (gpointer)&part);

			g_signal_connect(G_OBJECT(gtkEventBox), "leave-notify-event", G_CALLBACK(pStatusBar::onLeave), (gpointer)&part);

			gtk_container_add(GTK_CONTAINER(gtkEventBox), gtkWidget);

			gtkWidget = gtkEventBox;

            helpers.push_back( gtkEventBox );
		}
				
		gtk_grid_attach_next_to (GTK_GRID(gridWidget), gtkWidget, nullptr, GtkPositionType::GTK_POS_RIGHT, 1, 1);
		if (!part.tooltip.empty())
			gtk_widget_set_tooltip_text(gtkWidget, part.tooltip.c_str());
		gtk_widget_set_vexpand( gtkWidget, true );
		gtk_widget_show_all( gtkWidget );

        if (part.appendSeparator && (usedWidgets.size() < countVisible) ) {
		    GtkWidget* separator = gtk_separator_new( GTK_ORIENTATION_VERTICAL );
            gtk_grid_attach_next_to (GTK_GRID(gridWidget), separator, nullptr, GtkPositionType::GTK_POS_RIGHT, 1, 1);
            gtk_widget_show_all( separator );
            helpers.push_back( separator );
		}
	}
}

//auto pStatusBar::updateTooltip( StatusBar::Part& part ) -> void {
//    if (part.tooltip.empty())
//        return;
//
//    if (part.position >= usedWidgets.size())
//        return;
//
//    if (!statusBar.window() || !gridWidget)
//        return;
//
//    Widget* widget = usedWidgets[ part.position ];
//
//    gtk_widget_set_tooltip_text(widget->p.gtkWidget, part.tooltip.c_str());
//}

auto pStatusBar::updatePart( StatusBar::Part& part ) -> void {

    if (part.position < 0) {
        statusBar.state.updatePending = true;
        return;
    }

	if (part.position >= usedWidgets.size())
		return;

	if (!statusBar.window() || !gridWidget)
		return;
	
	Widget* widget = usedWidgets[ part.position ];
	
	if ( part.image ) {
		
		setImage( GTK_IMAGE(widget->p.gtkWidget), *part.image );
		
	} else {
		
		Label* label = (Label*)widget;
		Label::Align align = part.alignRight ? Label::Align::Right : Label::Align::Left;

		if (label->align() != align)
            label->setAlign( align );

		label->setText( part.text );

		if (part.overrideForegroundColor != -1)
			label->setForegroundColor( part.overrideForegroundColor );
		else
			label->resetForegroundColor();
	}
}

auto pStatusBar::onClick(GtkWidget* widget, GdkEventButton* event, StatusBar::Part* part) -> void {
	
	if(part->popupMenu)
		gtk_menu_popup_at_pointer(GTK_MENU(part->popupMenu->p.gtkMenu), nullptr);
	
    if(part->onClick)
		part->onClick();
}

auto pStatusBar::onEnter(GtkWidget* widget, GdkEventButton* event, StatusBar::Part* part) -> void {

	GdkWindow* window = gtk_widget_get_window(widget);
	
	auto cursor = gdk_cursor_new_for_display( gdk_screen_get_display(gdk_screen_get_default()), GDK_HAND2 );
	
	if (window)
		gdk_window_set_cursor(window, cursor);
}

auto pStatusBar::onLeave(GtkWidget* widget, GdkEventButton* event, StatusBar::Part* part) -> void {
		
	GdkWindow* window = gtk_widget_get_window(widget);
	
	auto cursor = gdk_cursor_new_for_display( gdk_screen_get_display(gdk_screen_get_default()), GDK_ARROW );

	if (window)
		gdk_window_set_cursor(window, cursor);
}
