
auto pBrowserWindow::file(BrowserWindow::State& state, bool save) -> std::string {
    std::string name;

    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        !state.title.empty() ? state.title.c_str() : (save ? "Save File" : "Open File"),
        state.window ? GTK_WINDOW(state.window->p.widget) : (GtkWindow*)nullptr,
        save ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        save ? GTK_STOCK_SAVE : GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
        (const gchar*)nullptr );

    if(!state.path.empty())
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), state.path.c_str());

    for(auto& filter : state.filters) {
        std::vector<std::string> tokens = String::split(filter, '(');
        if(tokens.size() != 2) continue;
        GtkFileFilter* gtkFilter = gtk_file_filter_new();
        gtk_file_filter_set_name(gtkFilter, filter.c_str());
        std::string part = tokens.at(1);
        part.pop_back();
        String::delSpaces(part);
        tokens = String::split(part, ',');
        for(auto& token : tokens) {
            gtk_file_filter_add_pattern(gtkFilter, token.c_str());
            gtk_file_filter_add_pattern(gtkFilter, String::toUpperCase( token ).c_str());
        }
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), gtkFilter);
    }

    if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* temp = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        name = temp;
        g_free(temp);
    }

    gtk_widget_destroy(dialog);
    
    return name;
}

auto pBrowserWindow::directory(BrowserWindow::State& state) -> std::string {
    std::string name = "";

    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        !state.title.empty() ? state.title.c_str() : "Select Directory",
        state.window ? GTK_WINDOW(state.window->p.widget) : (GtkWindow*)nullptr,
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
        (const gchar*)nullptr
    );

    if(!state.path.empty())
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), state.path.c_str());

    if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* temp = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        name = temp;
        g_free(temp);
    }

    gtk_widget_destroy(dialog);
    if(!name.empty() && (name.back() != '/')) name.push_back('/');
    
    return name;
}
