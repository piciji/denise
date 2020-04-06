
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
    gtk_list_store_clear(GTK_LIST_STORE(store));
    gtk_tree_view_set_model(GTK_TREE_VIEW(subWidget), GTK_TREE_MODEL(store));
    gtk_scrolled_window_set_hadjustment(GTK_SCROLLED_WINDOW(gtkWidget), 0);
    gtk_scrolled_window_set_vadjustment(GTK_SCROLLED_WINDOW(gtkWidget), 0);
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
		g_object_set(G_OBJECT(cell.text), "ypad", 0, nullptr);
		
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

    for(auto& cell : column) {
        gtk_tree_view_column_set_widget(GTK_TREE_VIEW_COLUMN(cell.column), cell.label);
        gtk_tree_view_append_column(GTK_TREE_VIEW(subWidget), cell.column);
        gtk_widget_show(cell.label);
    }

    //gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(subWidget), headerText.size() >= 2);
    gtk_tree_view_set_search_column(GTK_TREE_VIEW(subWidget), -1);

    g_signal_connect(G_OBJECT(subWidget), "cursor-changed", G_CALLBACK(pListView::onChange), (gpointer)&listView);
    g_signal_connect(G_OBJECT(subWidget), "row-activated", G_CALLBACK(pListView::onActivate), (gpointer)&listView);
    
    gtk_widget_show(subWidget);
}

auto pListView::destroy() -> void {
    if(subWidget) gtk_widget_destroy(subWidget);
    pWidget::destroy();
}

auto pListView::init() -> void {
    create();
    setHeaderVisible( listView.headerVisible() );

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
    pWidget::setFont(font);
    for(auto& cell : column) pFont::setFont(cell.label, pfont);
}

auto pListView::onActivate(GtkTreeView* treeView, GtkTreePath* path, GtkTreeViewColumn* column, ListView* self) -> void {
    char* pathname = gtk_tree_path_to_string(path);
    try {
        self->state.selection = std::stoi( pathname );
    } catch( ... ) { g_free(pathname); return; }

    g_free(pathname);
    if(self->onActivate) self->onActivate();
}

auto pListView::onChange(GtkTreeView* treeView, ListView* self) -> void {
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
        if(self->onChange) self->onChange();
    }
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
	
	pSystem::applyCss( subWidget, ".customBackgroundColor { background-color: " + pSystem::getColorCss( color ) + "; } " +
	"treeview:selected { border: 1px solid " + pSystem::getColorCss( color, true ) + ";} " );
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
