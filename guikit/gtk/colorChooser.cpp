
auto pColorChooser::choose(ColorChooser::State& state) -> std::optional<unsigned> {
    std::optional<unsigned> result = std::nullopt;

    GtkWidget* dialog = gtk_color_chooser_dialog_new(NULL,
        state.window ? GTK_WINDOW(state.window->p.widget) : (GtkWindow*)nullptr);

    g_signal_connect(dialog, "notify::rgba", G_CALLBACK(pColorChooser::onColorChanged), NULL);

    GdkRGBA color;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        gtk_color_chooser_get_rgba(
            GTK_COLOR_CHOOSER(dialog),
            &color
        );

        result = (uint8_t)(color.blue * 255.0) | ((uint8_t)(color.green * 255.0) << 8) | ((uint8_t)(color.red * 255.0) << 16);
    }

    gtk_widget_destroy(dialog);

    return result;
}

auto pColorChooser::onColorChanged(GObject* chooser, GParamSpec* pspec, gpointer user_data) -> void {
    GdkRGBA color;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(chooser), &color);

    if (ColorChooser::onChoose)
        ColorChooser::onChoose( (uint8_t)(color.blue * 255.0) | ((uint8_t)(color.green * 255.0) << 8) | ((uint8_t)(color.red * 255.0) << 16) );
}
