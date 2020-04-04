
pBrowserWindow::pBrowserWindow(BrowserWindow& browserWindow) : browserWindow(browserWindow) {
	
}

auto pBrowserWindow::responseHandler(GtkDialog* dialog, gint responseId, gpointer data) -> void {
  
	pBrowserWindow* instance = (pBrowserWindow*)data;

	auto& state = instance->browserWindow.state;
	
	for(auto& button : state.buttons) {
		
		if (button.id == responseId) {
			
			if (button.onClick) {
				if ( button.onClick( instance->selectedPath, instance->contentSelection ) )					
					gtk_window_close( (GtkWindow*)instance->dialog );
					
			}
			break;
		}		        
	}
}

auto pBrowserWindow::selectionHandler(GtkFileChooser* chooser, gpointer data) -> void {
  
	pBrowserWindow* instance = (pBrowserWindow*)data;
	
	auto& state = instance->browserWindow.state;
	
	auto fileName = gtk_file_chooser_get_filename(chooser);
	
	std::string path = (std::string)fileName;
	
	if (!path.empty() && path != instance->selectedPath) {
        if (state.onSelectionChange)
            state.onSelectionChange(path);

        instance->selectedPath = path;
    }  
}

auto pBrowserWindow::file(bool save) -> std::string {
    std::string name  = "";
	auto& state = browserWindow.state;

    dialog = gtk_file_chooser_dialog_new(
        !state.title.empty() ? state.title.c_str() : (save ? "Save File" : "Open File"),
        state.window ? GTK_WINDOW(state.window->p.widget) : (GtkWindow*)nullptr,
        save ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN,
        g_dgettext("gtk30", "_Cancel"), GTK_RESPONSE_CANCEL,
        save ? g_dgettext("gtk30", "_Save") : g_dgettext("gtk30", "_Open"), GTK_RESPONSE_ACCEPT,
        (const gchar*)nullptr );

    if(!state.path.empty())
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), state.path.c_str());

//	for(auto& button : state.buttons) {
//		
//		gtk_dialog_add_button( (GtkDialog*)dialog, button.text.c_str(), button.id );
//	}
//	
//	if (state.buttons.size())
//		g_signal_connect(dialog, "response", G_CALLBACK(pBrowserWindow::responseHandler), (gpointer)this);
	
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
        gtk_file_chooser_add_filter((GtkFileChooser*)dialog, gtkFilter);
    }
	
//	g_signal_connect(dialog, "selection-changed", G_CALLBACK(pBrowserWindow::selectionHandler), (gpointer)this);	
	
    if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* temp = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        name = temp;
        g_free(temp);
    }   
    
    return name;
}

auto pBrowserWindow::createPreview() -> GtkWidget* {
	auto& state = browserWindow.state;
	GtkWidget* grid = gtk_grid_new();
	
	if (state.contentView.id) {
        listView = new ListView;
        listView->setHeaderText({""});
        listView->setHeaderVisible( false );
        listView->setBackgroundColor( state.contentView.backgroundColor );
        listView->setForegroundColor( state.contentView.foregroundColor );
        listView->onActivate = [this]() {
            if (browserWindow.state.contentView.onDblClick) {
                browserWindow.state.contentView.onDblClick( selectedPath, contentViewSelection() );
            }
        };
        
        if (!state.contentView.font.empty())
            listView->setFont( state.contentView.font );
        
		listView->setGeometry({0, 0, 200, 150});

		gtk_grid_attach(GTK_GRID(grid), listView->p.gtkWidget, 0, 0, 1, 1);
    }
	
	return grid;
}

auto pBrowserWindow::directory() -> std::string {
    std::string name = "";
	auto& state = browserWindow.state;

    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        !state.title.empty() ? state.title.c_str() : "Select Directory",
        state.window ? GTK_WINDOW(state.window->p.widget) : (GtkWindow*)nullptr,
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        g_dgettext("gtk30", "_Cancel"), GTK_RESPONSE_CANCEL,
        g_dgettext("gtk30", "_Open"), GTK_RESPONSE_ACCEPT,
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

auto pBrowserWindow::contentViewSelection() -> unsigned {
        
    return contentSelection;
}

auto pBrowserWindow::close() -> void {
	if (dialog)
		gtk_window_close( (GtkWindow*)dialog );
}

auto pBrowserWindow::visible() -> bool {
	
	if (dialog)
		return gtk_window_is_active(GTK_WINDOW(dialog));
	
	return false;
}


auto pBrowserWindow::setForeground() -> void {
	if (dialog)
		gtk_window_present(GTK_WINDOW(dialog));
}

pBrowserWindow::~pBrowserWindow() {
	if (listView)
		delete listView;
	
	if (dialog)
		gtk_widget_destroy(dialog);
	
	dialog = nullptr;
}