
pBrowserWindow::pBrowserWindow(BrowserWindow& browserWindow) : browserWindow(browserWindow) {}

auto pBrowserWindow::onToggleOrder(GtkToggleButton* toggleButton, BrowserWindow* self) -> void {
    g_signal_stop_emission_by_name(G_OBJECT(self->p.orderSelectedWidget), "clicked"); // prevent closing of dialog
    self->state.orderBySelected->checked = gtk_toggle_button_get_active(toggleButton);
    self->state.orderBySelected->onToggle(self->state.orderBySelected->checked);
}

auto pBrowserWindow::closeHandler(GtkDialog* dialog, GdkEvent* event, gpointer data) -> void {
    pBrowserWindow* instance = (pBrowserWindow*)data;

    auto& state = instance->browserWindow.state;

    if (state.onCancelClick)
        state.onCancelClick();

    instance->close();
}

auto pBrowserWindow::responseHandler(GtkDialog* dialog, gint responseId, gpointer data) -> void {
  
	pBrowserWindow* instance = (pBrowserWindow*)data;

	auto& state = instance->browserWindow.state;
	
	if (responseId == GTK_RESPONSE_CANCEL) {
		
		if (state.onCancelClick) 
			state.onCancelClick();
		
		instance->close();
	} else if (responseId == GTK_RESPONSE_ACCEPT) {
		
		if (state.onOkClick) {
            if (instance->multi && instance->sortedFiles.size())
                state.onOkClick(instance->sortedFiles, instance->contentViewSelection());
            else
                state.onOkClick({instance->selectedPath}, instance->contentViewSelection());
        }
		
		instance->close();
	} else {		
		for(auto& button : state.buttons) {

			if (button.id == responseId) {

				if (button.onClick) {
                    if (instance->multi && instance->sortedFiles.size()) {
                        if ( button.onClick( instance->sortedFiles, instance->contentViewSelection() ) )
                            instance->close();
                    } else {
                        if (button.onClick( {instance->selectedPath}, instance->contentViewSelection()))
                            instance->close();
                    }
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

    if (instance->multi) {
        std::vector<std::string> curSelectedFiles;
        GSList* fileList = gtk_file_chooser_get_filenames(chooser);
        for (GSList *iter = fileList; iter != nullptr; iter = g_slist_next(iter)) {
            std::string name = static_cast<char *>(iter->data);
            curSelectedFiles.push_back(name);
            g_free(iter->data);
        }
        g_slist_free(fileList);

        if (state.orderBySelected) {
            std::vector<std::string> resultFiles;

            for (auto &selectedFile: instance->sortedFiles) { // sorted selection before
                unsigned pos = 0;
                for (auto &curSelectedFile: curSelectedFiles) { // unsorted current selection
                    if (selectedFile == curSelectedFile) {
                        resultFiles.push_back(selectedFile);
                        GUIKIT::Vector::eraseVectorPos(curSelectedFiles, pos);
                        break;
                    }
                    pos++;
                }
            }

            for (auto &curSelectedFile: curSelectedFiles)
                resultFiles.push_back(curSelectedFile);

            instance->sortedFiles = resultFiles;
        } else
            instance->sortedFiles = curSelectedFiles;
    }

    bool informInsertion = instance->multi && (instance->sortedFiles.size() > 1);

    if (!path.empty() && ((path != instance->selectedPath) || informInsertion )) {
        if (state.onSelectionChange) {
            if (!instance->selectedPath.empty() && instance->listView)
                instance->listView->reset();

            if (informInsertion) {
                auto listings = state.onSelectionChange( "" );

            	if (instance->listView) {
                  //  listView->resetFirstRowColor();
            		int i = 0;
            		auto _s = pFont::size(instance->listView->p.pfont, " ");
            		int maxChars = state.contentView.width / _s.width;

            		for(auto& file : instance->sortedFiles) {
            			if (i >= listings.size())
            				break;

            			std::string ident = String::removeExtension(file, 2);
            			ident += "  " + listings[i++].entry + ": ";
            			if (ident.size() > maxChars)
            				ident = ident.substr( ident.size() - maxChars );

            			instance->listView->append({ident});
            		}
            	}
            } else {
                auto listings = state.onSelectionChange(path);

            	if (instance->listView) {
                    //if (state.contentView.overrideFirstRowColor)
                      //  listView->setFirstRowColor(state.contentView.firstRowForegroundColor, state.contentView.firstRowBackgroundColor);

            		for(auto& listing : listings) {
            			instance->listView->append({listing.entry});
            		}
            		unsigned i = 0;
            		for(auto& listing : listings) {
            			instance->listView->setRowTooltip(i++, listing.tooltip);
            		}
            	}
            }
		}
        instance->selectedPath = path;
    }
}

auto pBrowserWindow::setListings( std::vector<BrowserWindow::Listing>& listings ) -> void {
    if (listView) {
        for(auto& listing : listings) {
            listView->append({listing.entry});
        }
        unsigned i = 0;
        for(auto& listing : listings) {
            listView->setRowTooltip(i++, listing.tooltip);
        }
    }
}

auto pBrowserWindow::fileGeneric(bool save, bool multi) -> std::vector<std::string> {
    std::vector<std::string> out;
    std::string name  = "";
	auto& state = browserWindow.state;
    this->multi = multi;

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

    if (state.orderBySelected) {
        orderSelectedWidget = gtk_check_button_new_with_label( state.orderBySelected->text.c_str() );
        gtk_widget_show(orderSelectedWidget);
        gtk_dialog_add_action_widget((GtkDialog*)dialog, orderSelectedWidget, 100);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(orderSelectedWidget), state.orderBySelected->checked);
        g_signal_connect(G_OBJECT(orderSelectedWidget), "toggled", G_CALLBACK(pBrowserWindow::onToggleOrder), (gpointer)&browserWindow);
    }

	gtk_dialog_add_button( (GtkDialog*)dialog, _cancel, GTK_RESPONSE_CANCEL );

	for(auto& button : state.buttons)
		gtk_dialog_add_button( (GtkDialog*)dialog, button.text.c_str(), button.id );

    gtk_dialog_add_button( (GtkDialog*)dialog, _ok, GTK_RESPONSE_ACCEPT );

    g_signal_connect(G_OBJECT(dialog), "delete-event", G_CALLBACK(pBrowserWindow::closeHandler), (gpointer)this);

	if (state.buttons.size())
		g_signal_connect(dialog, "response", G_CALLBACK(pBrowserWindow::responseHandler), (gpointer)this);
	
	if (state.contentView.id) {
        gtk_file_chooser_set_preview_widget(GTK_FILE_CHOOSER(dialog), createPreview());
        gtk_file_chooser_set_use_preview_label(GTK_FILE_CHOOSER(dialog), false);
    }
	
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

    if (multi) {
        sortedFiles.clear();
        gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), true);
    }

    if (state.modal) {
        if (multi) {
            if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
                GSList *fileList = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));

                for (GSList *iter = fileList; iter != nullptr; iter = g_slist_next(iter)) {
                    name = static_cast<char *>(iter->data);
                    out.push_back(name);
                    g_free(iter->data);
                }

                g_slist_free(fileList);

                if (state.orderBySelected && state.orderBySelected->checked && sortedFiles.size()) {
                    std::vector<std::string> temp;

                    for (auto &sSortedFile: sortedFiles) {
                        for (auto &sFile: out) {
                            if (GUIKIT::String::findString(sFile, sSortedFile))
                                temp.push_back(sFile);
                        }
                    }
                    out = temp;
                }
            }

            close();
        } else {
            if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
                char* temp = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
                name = temp;
                g_free(temp);
            }
            out.push_back(name);
            close();
        }
    } else {
        out.push_back(name);
        gtk_widget_show_all(GTK_WIDGET(dialog));
    }

    if (state.hideOkButton) {
        // allow to add Ok button first and hide later, otherwise the double click action doesn't work
        GtkWidget* okButton = gtk_dialog_get_widget_for_response( GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT );
        if (okButton)
            gtk_widget_set_visible(okButton, false);
    }
	
    return out;
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
    if (state.contentView.overrideSelectionColor)
        listView->setSelectionColor( state.contentView.selectionForegroundColor, state.contentView.selectionBackgroundColor );
	if (state.contentView.overrideFirstRowColor)
		listView->setFirstRowColor( state.contentView.firstRowForegroundColor, state.contentView.firstRowBackgroundColor );

	listView->colorRowTooltips( state.contentView.colorTooltips );
	listView->onActivate = [this]() {
		if (browserWindow.state.contentView.onDblClick) {
			if (browserWindow.state.contentView.onDblClick( selectedPath, contentViewSelection() )) {
				close();
			}
		}
	};

	if (!state.contentView.font.empty())
		listView->setFont( state.contentView.font, state.contentView.specialFont );
		
	unsigned margin = 5;
	
	gtk_widget_set_size_request(listView->p.gtkWidget, state.contentView.width + margin, 0);	
	
	pSystem::addCssClass(listView->p.gtkWidget, "someMargin");
	pSystem::applyCss( listView->p.gtkWidget, ".someMargin { margin-right: " + std::to_string(margin) + "px;} " );

    if (!state.contentView.hint.empty()) {
        //listView->append({""});
        listView->append({state.contentView.hint});
        //listView->setRowTooltip(0, "");
        listView->setRowTooltip(0, state.contentView.hintTooltip);
    }

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