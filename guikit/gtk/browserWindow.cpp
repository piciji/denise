
pBrowserWindow::pBrowserWindow(BrowserWindow& browserWindow) : browserWindow(browserWindow) {}

auto pBrowserWindow::responseHandler(GtkDialog* dialog, gint responseId, gpointer data) -> void {
  
	pBrowserWindow* instance = (pBrowserWindow*)data;

	auto& state = instance->browserWindow.state;
	
	if (responseId == GTK_RESPONSE_CANCEL) {
		
		if (state.onCancelClick) 
			state.onCancelClick();
		
		instance->close();
	} else if (responseId == GTK_RESPONSE_ACCEPT) {
		
		if (state.onOkClick) 
			state.onOkClick( instance->selectedPath, instance->contentViewSelection() );
		
		instance->close();
	} else {		
		for(auto& button : state.buttons) {

			if (button.id == responseId) {

				if (button.onClick) {
					if ( button.onClick( instance->selectedPath, instance->contentViewSelection() ) )					
						instance->close();

				}
				break;
			}		        
		}
	}			
}

auto pBrowserWindow::selectionHandler(GtkFileChooser* chooser, gpointer data) -> void {
  
	pBrowserWindow* instance = (pBrowserWindow*)data;
	
	if (instance->listView)
		gtk_file_chooser_set_preview_widget_active(chooser, true);
	
	auto& state = instance->browserWindow.state;
	
	auto fileNamePtr = gtk_file_chooser_get_filename(chooser);
	
	if (!fileNamePtr)
		return;
	
	std::string path = (std::string)fileNamePtr;
	
	if (!path.empty() && path != instance->selectedPath) {
		if (instance->listView)
            instance->listView->reset();
				
        if (state.onSelectionChange) {
            auto listings = state.onSelectionChange(path);
			
			if (instance->listView) {
                for(auto& listing : listings) {
                    instance->listView->append({listing.entry});
                }
				unsigned i = 0;
                for(auto& listing : listings) {
                    instance->listView->setRowTooltip(i++, listing.tooltip);
                }
				
            }
		}
        instance->selectedPath = path;
    }  
}

auto pBrowserWindow::file(bool save) -> std::string {
    std::string name  = "";
	auto& state = browserWindow.state;

	const gchar* _cancel = g_dgettext("gtk30", "_Cancel");
	if (!state.textCancel.empty())
		_cancel = state.textCancel.c_str();
	
	const gchar* _ok = save ? g_dgettext("gtk30", "_Save") : g_dgettext("gtk30", "_Open");
	if (!state.textOk.empty())
		_ok = state.textOk.c_str();
	
    dialog = gtk_file_chooser_dialog_new(
        !state.title.empty() ? state.title.c_str() : (save ? "Save File" : "Open File"),
        state.window ? GTK_WINDOW(state.window->p.widget) : (GtkWindow*)nullptr,
        save ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN,
        0, (const gchar*)nullptr );
	
    if(!state.path.empty())
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), state.path.c_str());

	gtk_dialog_add_button( (GtkDialog*)dialog, _cancel, GTK_RESPONSE_CANCEL );	
	
	for(auto& button : state.buttons)		
		gtk_dialog_add_button( (GtkDialog*)dialog, button.text.c_str(), button.id );	
	
	gtk_dialog_add_button( (GtkDialog*)dialog, _ok, GTK_RESPONSE_ACCEPT );	
	
	if (state.buttons.size())
		g_signal_connect(dialog, "response", G_CALLBACK(pBrowserWindow::responseHandler), (gpointer)this);
	
	if (state.contentView.id)
		gtk_file_chooser_set_preview_widget(GTK_FILE_CHOOSER(dialog), createPreview());
	
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
	
	g_signal_connect(dialog, "selection-changed", G_CALLBACK(pBrowserWindow::selectionHandler), (gpointer)this);	
	
	if (state.modal) {		
		if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
			char* temp = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
			name = temp;
			g_free(temp);
		}   

		close();
	} else
		gtk_widget_show_all( GTK_WIDGET(dialog) );
	
    return name;	
}

auto pBrowserWindow::createPreview() -> GtkWidget* {
	auto& state = browserWindow.state;
	
	listView = new ListView;
	listView->setHeaderText({""});
	listView->setHeaderVisible( false );
	if (state.contentView.overrideBackgroundColor)
		listView->setBackgroundColor( state.contentView.backgroundColor );
	if (state.contentView.overrideForegroundColor)
		listView->setForegroundColor( state.contentView.foregroundColor );
	listView->colorRowTooltips( state.contentView.colorTooltips );
	listView->onActivate = [this]() {
		if (browserWindow.state.contentView.onDblClick) {
			if (browserWindow.state.contentView.onDblClick( selectedPath, contentViewSelection() )) {
				close();
			}
		}
	};

	if (!state.contentView.font.empty())
		listView->setFont( state.contentView.font );

	gtk_widget_set_size_request(listView->p.gtkWidget, 380, 0);	

	return listView->p.gtkWidget;
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
        
    return (listView && listView->selected()) ? listView->selection() : 0;
}

auto pBrowserWindow::close() -> void {
	if (dialog)
		//gtk_window_close( (GtkWindow*)dialog );
		gtk_widget_destroy(dialog);
	
	dialog = nullptr;
	
	// dialog destroys it
	listView = nullptr;
}

auto pBrowserWindow::visible() -> bool {
	
	if (dialog)
		return gtk_widget_is_visible(dialog);
	
	return false;
}

auto pBrowserWindow::detached() -> bool {
	return !browserWindow.state.modal;
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