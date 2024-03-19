
auto pListView::focused() -> bool {
    return gtk_widget_has_focus(subWidget);
}

auto pListView::setFocused() -> void {
    gtk_widget_grab_focus(subWidget);
}

auto pListView::autoSizeColumns() -> void {
    gtk_tree_view_columns_autosize(GTK_TREE_VIEW(subWidget));
}

auto pListView::append(const std::vector<std::string>& list) -> void {
    GtkTreeIter iter;
    gtk_list_store_append(store, &iter);
    for(unsigned n = 0; n < list.size(); n++) {
        gtk_list_store_set(store, &iter, n * 2 + 1, list.at(n).c_str(), -1);
    }
}

auto pListView::remove(unsigned selection) -> void {
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(subWidget));
    GtkTreeIter iter;
    gtk_tree_model_get_iter_from_string(model, &iter, std::to_string(selection).c_str());
    gtk_list_store_remove(store, &iter);
}

auto pListView::reset() -> void {
    locked = true;
    gtk_list_store_clear(GTK_LIST_STORE(store));
    gtk_tree_view_set_model(GTK_TREE_VIEW(subWidget), GTK_TREE_MODEL(store));
    gtk_scrolled_window_set_hadjustment(GTK_SCROLLED_WINDOW(gtkWidget), 0);
    gtk_scrolled_window_set_vadjustment(GTK_SCROLLED_WINDOW(gtkWidget), 0);
    locked = false;
}

auto pListView::setHeaderText(std::vector<std::string> list) -> void { 
	init();
	pWidget::add();
	pWidget::setGeometry( widget.state.geometry );
}

auto pListView::setHeaderVisible(bool visible) -> void {
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(subWidget), visible);
}

auto pListView::setSelected(bool selected) -> void {
    if(!selected) {
        GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(subWidget));
        gtk_tree_selection_unselect_all(selection);
    } else {
        setSelection(listView.selection());
    }
}

auto pListView::setSelection(unsigned selection) -> void {
    if (!subWidget)
        return;

    GtkTreeSelection* treeSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW(subWidget));
    gtk_tree_selection_unselect_all(treeSelection);
    GtkTreePath* path = gtk_tree_path_new_from_string(std::to_string(selection).c_str());
    gtk_tree_selection_select_path(treeSelection, path);
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(subWidget), path, nullptr, false);
    gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(subWidget), path, nullptr, true, 0.5, 0.0);
    gtk_tree_path_free(path);
}

auto pListView::setText(unsigned selection, unsigned position, const std::string& text) -> void {
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(subWidget));
    GtkTreeIter iter;
    gtk_tree_model_get_iter_from_string(model, &iter, std::to_string(selection).c_str());
    gtk_list_store_set(store, &iter, position * 2 + 1, text.c_str(), -1);
}

auto pListView::create() -> void {
    destroy();
    gtkWidget = gtk_scrolled_window_new(0, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(gtkWidget), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(gtkWidget), GTK_SHADOW_ETCHED_IN);

    pSystem::applyCss( gtkWidget, "scrolledwindow undershoot.top, scrolledwindow undershoot.right, scrolledwindow undershoot.bottom, scrolledwindow undershoot.left { background-image: none; }");

    auto headerText = listView.state.header;

    if(headerText.size() == 0) headerText.push_back("");

    column.clear();
    std::vector<GType> gtype;
    for(auto& text : headerText) {
        GtkColumn cell;
        cell.label = gtk_label_new(text.c_str());
        cell.column = gtk_tree_view_column_new();
        gtk_tree_view_column_set_resizable(cell.column, true);
        gtk_tree_view_column_set_title(cell.column, "");

        cell.icon = gtk_cell_renderer_pixbuf_new();
        gtk_tree_view_column_pack_start(cell.column, cell.icon, false);
        gtk_tree_view_column_set_attributes(cell.column, cell.icon, "pixbuf", gtype.size(), nullptr);
        gtype.push_back(GDK_TYPE_PIXBUF);

        cell.text = gtk_cell_renderer_text_new();
		//g_object_set(G_OBJECT(cell.text), "ypad", 0, nullptr);
		gtk_cell_renderer_set_padding(cell.text, 0, 0);
		
        gtk_tree_view_column_pack_start(cell.column, cell.text, false);
        gtk_tree_view_column_set_attributes(cell.column, cell.text, "text", gtype.size(), nullptr);
        gtype.push_back(G_TYPE_STRING);

        column.push_back(cell);
    }

    if(store)
        g_object_unref (G_OBJECT (store));
    store = gtk_list_store_newv(gtype.size(), gtype.data());
    subWidget = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_container_add(GTK_CONTAINER(gtkWidget), subWidget);
	gtk_widget_set_has_tooltip(GTK_WIDGET(subWidget), TRUE);

    for(auto& cell : column) {
        gtk_tree_view_column_set_widget(GTK_TREE_VIEW_COLUMN(cell.column), cell.label);
        gtk_tree_view_append_column(GTK_TREE_VIEW(subWidget), cell.column);
        gtk_widget_show(cell.label);
    }

    //gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(subWidget), headerText.size() >= 2);
    gtk_tree_view_set_search_column(GTK_TREE_VIEW(subWidget), -1);

    g_signal_connect(G_OBJECT(subWidget), "cursor-changed", G_CALLBACK(pListView::onChange), (gpointer)&listView);
    g_signal_connect(G_OBJECT(subWidget), "button-press-event", G_CALLBACK(pListView::onPress), (gpointer)&listView);
    g_signal_connect(G_OBJECT(subWidget), "row-activated", G_CALLBACK(pListView::onActivate), (gpointer)&listView);	
    g_signal_connect(G_OBJECT(subWidget), "query-tooltip", G_CALLBACK(pListView::onTooltip), (gpointer)&listView);
	
    gtk_widget_show(subWidget);
}

auto pListView::destroy() -> void {
    if(subWidget)
		gtk_widget_destroy(subWidget);

	if (customTooltipLabel)
		delete customTooltipLabel;

	if (customTooltip)
		gtk_widget_destroy(GTK_WIDGET(customTooltip));
	
    pWidget::destroy();
}

auto pListView::init() -> void {
    create();
    setHeaderVisible( listView.headerVisible() );
    setForegroundColor( listView.foregroundColor() );
    setBackgroundColor( listView.backgroundColor() );

    for(unsigned i=0; i < listView.rowCount(); i++) {
        auto& row = listView.state.rows.at(i);
        append(row);
        for(unsigned j=0; j < row.size(); j++) {
            Image* img = listView.state.images.at(i).at(j);
            if (img) setImage(i, j, *img);
        }
    }
    if(listView.selected()) setSelection(listView.selection());
    autoSizeColumns();
}

auto pListView::setFont(std::string font) -> void {
	pSystem::removeCssClass(subWidget, "removeRowSpacing");
	
    pWidget::setFont(font);
    for(auto& cell : column)
		pFont::setFont(cell.label, pfont);
	
	if (customTooltipLabel)
		customTooltipLabel->setFont( listView.font() );
	
	if (listView.specialFont()) {
		pSystem::addCssClass(subWidget, "removeRowSpacing");

		pSystem::applyCss(subWidget, ".removeRowSpacing { padding: 0px; -GtkTreeView-expander-size: 0; -GtkTreeView-vertical-separator: 0; -GtkTreeView-horizontal-separator: 0; }");
	}	
}

auto pListView::onActivate(GtkTreeView* treeView, GtkTreePath* path, GtkTreeViewColumn* column, ListView* self) -> void {
    char* pathname = gtk_tree_path_to_string(path);
    try {
        self->state.selection = std::stoi( pathname );
    } catch( ... ) {
        g_free(pathname);
        return;
    }

    g_free(pathname);

    GList* cols = gtk_tree_view_get_columns(treeView);

    unsigned i = 0;
    while(cols != NULL) {
        if (column == GTK_TREE_VIEW_COLUMN(cols->data))
            break;

        cols = cols->next;
        i++;
    }

    self->state.column = i;

    g_list_free (cols);

    if(self->onActivate) self->onActivate();
}

auto pListView::onPress(GtkTreeView* treeView, GdkEventButton* event, ListView* self) -> gboolean {
    self->p.pressed = true;

    return false;
}

auto pListView::onChange(GtkTreeView* treeView, ListView* self) -> void {
//    if (!self->p.pressed)
//        return;
//
//    self->p.pressed = false;

    GtkTreeIter iter;
    if(!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(treeView), 0, &iter)) return;
    char* path = gtk_tree_model_get_string_from_iter(gtk_tree_view_get_model(treeView), &iter);
    unsigned selection;

    try {
        selection = std::stoi( path );
    } catch( ... ) { g_free(path); return; }

    g_free(path);

    if(!self->state.selected || self->state.selection != selection) {
        self->state.selected = true;
        self->state.selection = selection;
        if(self->onChange && !self->p.locked) self->onChange();
    }
}

auto pListView::onTooltip(GtkWidget* widget, gint x, gint y, gboolean keyboard_tip, GtkTooltip* tooltip, ListView* self) -> gboolean {

	if (!self->state.rowTooltips.size())
		return false;
			
	GtkTreeIter iter;
	GtkTreePath* path;
	GtkTreeModel* model;
	
	gboolean found = FALSE;
	std::string tooltipText = "";

	if (!gtk_tree_view_get_tooltip_context(GTK_TREE_VIEW(widget), &x, &y, keyboard_tip, &model, &path, &iter))
		return FALSE;

	char* pathname = gtk_tree_path_to_string(path);
	int hoverSelection = -1;
	
	try {
		hoverSelection = std::stoi(pathname);
	} catch (...) { }
	
	if (hoverSelection >= 0) {
		
		if (hoverSelection < self->state.rowTooltips.size())
			tooltipText = self->state.rowTooltips[hoverSelection];

		if (!tooltipText.empty()) {
			
			if (self->state.colorRowTooltips) {
				if (!self->p.customTooltip)
					self->p.createCustomTooltip();
				
				self->p.customTooltipLabel->setText( tooltipText );
				auto size = self->p.customTooltipLabel->minimumSize();
				gtk_window_resize(GTK_WINDOW(self->p.customTooltip), size.width, size.height );	
												
			} else
				gtk_tooltip_set_text(tooltip, tooltipText.c_str() );	
														
			gtk_tree_view_set_tooltip_row(GTK_TREE_VIEW(widget), tooltip, path);
			
			found = true;
		}
	}
	
	g_free(pathname);
	gtk_tree_path_free(path);
	
	return found;
}

auto pListView::createCustomTooltip() -> void {
	
	customTooltip = gtk_window_new( GTK_WINDOW_POPUP );

	auto verticalLayout = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(customTooltip), verticalLayout);
    gtk_widget_show(verticalLayout);
	
	customTooltipLabel = new Label;
	customTooltipLabel->setForegroundColor(listView.Widget::state.foregroundColor);
	customTooltipLabel->setBackgroundColor(listView.Widget::state.backgroundColor);
	customTooltipLabel->setFont( listView.font() );
	customTooltipLabel->setVisible();
	
	pSystem::addCssClass(customTooltipLabel->p.gtkWidget, "somePadding");	
	pSystem::applyCss( customTooltipLabel->p.gtkWidget, ".somePadding { padding: 4px;} " );
	
	gtk_container_add(GTK_CONTAINER(verticalLayout), customTooltipLabel->p.gtkWidget);
	
	gtk_widget_set_tooltip_window(subWidget, GTK_WINDOW(customTooltip));
}

auto pListView::setImage(unsigned selection, unsigned position, Image& image) -> void {
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(subWidget));
    GtkTreeIter iter;
    gtk_tree_model_get_iter_from_string(model, &iter, std::to_string(selection).c_str());

    if(!image.empty()) {
        unsigned size = pFont::size(pfont, " ").height;
        GdkPixbuf* pixbuf = CreatePixbuf(image, size > 2 ? size-2 : size);
		if (pixbuf) {
			gtk_list_store_set(store, &iter, position * 2, pixbuf, -1);
			g_object_unref( G_OBJECT( pixbuf ) );
		}
		
		for(auto& cell : column) {
			gtk_tree_view_column_set_spacing( cell.column, 4 );
		}
		
    } else {
        gtk_list_store_set(store, &iter, position * 2, nullptr, -1);
    }
}

auto pListView::setBackgroundColor(unsigned color) -> void {
	if (!subWidget)
		return;
	
	pSystem::removeCssClass(subWidget, "customBackgroundColor");
	
    if( !widget.overrideBackgroundColor() )
        return;	
	
	pSystem::addCssClass(subWidget, "customBackgroundColor");

    if ( !listView.overrideSelectionColor() ) {
        pSystem::applyCss(subWidget,
          ".customBackgroundColor { background-color: " + pSystem::getColorCss(color) + "; } " +
          "treeview:selected { border: 1px solid " + pSystem::getColorCss(color, true) + "; background-color: inherit; color: inherit;} ");
    } else {
        pSystem::applyCss(subWidget,
          ".customBackgroundColor { background-color: " + pSystem::getColorCss(color) + "; } " +
          "treeview:selected { border: 0px; background-color: " + pSystem::getColorCss( listView.selectionBackgroundColor() ) + "; color: " + pSystem::getColorCss( listView.selectionForegroundColor() ) + "; } ");
    }
}

auto pListView::setForegroundColor(unsigned color) -> void {
	if (!subWidget)
		return;
	
	pSystem::removeCssClass(subWidget, "customColor");
	
    if( !widget.overrideForegroundColor() )
        return;
	
	pSystem::addCssClass(subWidget, "customColor");
	
	pSystem::applyCss( subWidget, ".customColor { color: " + pSystem::getColorCss( color ) + "; }" );
}

auto pListView::setSelectionColor(unsigned foregroundColor, unsigned backgroundColor) -> void {
    if (!subWidget)
        return;

    if ( !listView.overrideSelectionColor() )
        pSystem::applyCss( subWidget, "treeview:selected { border: 1px solid " + pSystem::getColorCss( widget.backgroundColor(), true ) + "; background-color: inherit; color: inherit; } ");
    else
        pSystem::applyCss( subWidget, "treeview:selected { border: 0px; background-color: " + pSystem::getColorCss( backgroundColor ) + "; color: " + pSystem::getColorCss( foregroundColor ) + "; } ");
}