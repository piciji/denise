
auto pComboButton::append(const ComboButton::Entry& entry) -> void {
    gint count;

    if (comboButton.hintMultiFonts) {
        GtkTreeIter iter;

        gtk_list_store_append(store, &iter);

        gtk_list_store_set(store, &iter,
                           0, entry.text.c_str(),
                           1, entry.font.c_str(),
                           -1);

        count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL);
    } else {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(gtkWidget), entry.text.c_str());
        count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(gtk_combo_box_get_model(GTK_COMBO_BOX(gtkWidget))), NULL);
    }

    if(count == 1) setSelection(0);
	calculatedMinimumSize.updated = false;
}

auto pComboButton::appendMulti(std::vector<ComboButton::Entry>& rows) -> void {
    for (auto& entry : rows)
        append( entry );
}

auto pComboButton::minimumSize() -> Size {
    Size size = getMinimumSize();
    return {size.width, size.height};
}

auto pComboButton::remove(unsigned selection) -> void {
    locked = true;
    if (comboButton.hintMultiFonts) {
        GtkTreeIter iter;

        if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, selection))
            gtk_list_store_remove(store, &iter);
    } else
        gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(gtkWidget), selection);

    locked = false;

    if(selection == comboButton.selection()) comboButton.setSelection(0);
}

auto pComboButton::reset() -> void {
    locked = true;
    if (comboButton.hintMultiFonts)
        gtk_list_store_clear(store);
    else
        gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(gtkWidget))));
    locked = false;
}

auto pComboButton::setSelection(unsigned selection) -> void {
    locked = true;    
    gtk_combo_box_set_active(GTK_COMBO_BOX(gtkWidget), selection == ~0 ? -1 : selection);
    locked = false;
}

auto pComboButton::setText(unsigned selection, const std::string& text) -> void {
    locked = true;
    if (comboButton.hintMultiFonts) {
        GtkTreeIter iter;

        if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, selection))
            gtk_list_store_set(store, &iter, 0, text.c_str(), -1);
    } else {
        gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(gtkWidget), selection);
        gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(gtkWidget), selection, text.c_str());
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX(gtkWidget), comboButton.selection());
	calculatedMinimumSize.updated = false;
    locked = false;
}

auto pComboButton::create() -> void {
    destroy();

    if (comboButton.hintMultiFonts) {
        store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);

        gtkWidget = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));

        GtkCellRenderer* renderer = gtk_cell_renderer_text_new();

        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(gtkWidget), renderer, TRUE);

        gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(gtkWidget), renderer,
                                           "text", 0,
                                           "font", 1,
                                           NULL);
    } else
        gtkWidget = gtk_combo_box_text_new();

    g_signal_connect_swapped(G_OBJECT(gtkWidget), "changed", G_CALLBACK(pComboButton::onChange), (gpointer)&comboButton);
    g_signal_connect(G_OBJECT(gtkWidget), "drag-data-received", G_CALLBACK(pComboButton::dropEvent), (gpointer)&comboButton);
}

auto pComboButton::init() -> void {
    create();
    locked = true;

    for( auto& entry : comboButton.rows())
        append(entry);

    locked = false;
    setSelection(comboButton.selection());
    setDroppable(comboButton.droppable());
}

auto pComboButton::onChange(ComboButton* self) -> void {
    if(!self->p.locked) {
        self->state.selection = gtk_combo_box_get_active(GTK_COMBO_BOX(self->p.gtkWidget));
        if(self->onChange) self->onChange();
    }
}

auto pComboButton::dropEvent(GtkWidget* widget, GdkDragContext* context, gint x, gint y,
GtkSelectionData* data, guint type, guint timestamp, ComboButton* comboButton) -> void {
    if(!comboButton->droppable()) return;
    auto paths = getDropPaths(data);
    if(paths.empty()) return;
    if(comboButton->onDrop) comboButton->onDrop(paths);
}

auto pComboButton::setDroppable(bool droppable) -> void {
    gtk_drag_dest_set(gtkWidget, GTK_DEST_DEFAULT_ALL, nullptr, 0, GDK_ACTION_COPY);
    if(droppable) gtk_drag_dest_add_uri_targets(gtkWidget);
}
