
auto pComboButton::append(std::string text, const std::string& _font) -> void {
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(gtkWidget), text.c_str());
    if(comboButton.rows() == 1) comboButton.setSelection(0);
	calculatedMinimumSize.updated = false;
}

auto pComboButton::minimumSize() -> Size {
    Size size = getMinimumSize();
    return {size.width, size.height};
}

auto pComboButton::remove(unsigned selection) -> void {
    locked = true;
    gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(gtkWidget), selection);
    locked = false;

    if(selection == comboButton.selection()) comboButton.setSelection(0);
}

auto pComboButton::reset() -> void {
    locked = true;
    gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(gtkWidget))));
    locked = false;
}

auto pComboButton::setSelection(unsigned selection) -> void {
    locked = true;
    gtk_combo_box_set_active(GTK_COMBO_BOX(gtkWidget), selection);
    locked = false;
}

auto pComboButton::setText(unsigned selection, const std::string& text) -> void {
    locked = true;
    gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(gtkWidget), selection);
    gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(gtkWidget), selection, text.c_str());
    gtk_combo_box_set_active(GTK_COMBO_BOX(gtkWidget), comboButton.selection());
	calculatedMinimumSize.updated = false;
    locked = false;
}

auto pComboButton::create() -> void {
    destroy();
    gtkWidget = gtk_combo_box_text_new();
    g_signal_connect_swapped(G_OBJECT(gtkWidget), "changed", G_CALLBACK(pComboButton::onChange), (gpointer)&comboButton);
}

auto pComboButton::init() -> void {
    create();
    locked = true;
    for (int i = 0; i < comboButton.rows(); i++)
        append(comboButton.state.rows[i], comboButton.state.fonts[i]);
    locked = false;
    setSelection(comboButton.selection());
}

auto pComboButton::onChange(ComboButton* self) -> void {
    if(!self->p.locked) {
        self->state.selection = gtk_combo_box_get_active(GTK_COMBO_BOX(self->p.gtkWidget));
        if(self->onChange) self->onChange();
    }
}
