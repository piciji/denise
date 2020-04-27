
StatePathLayout::StatePathLayout() {
            
    edit.setEditable(false);
    append(label, {0u, 0u}, 10);
    append(edit, {~0u, 0u}, 10);
    append(empty, {0u, 0u}, 10);
    append(select, {0u, 0u});
    setAlignment(0.5);
    label.setFont(GUIKIT::Font::system("bold"));
}

FastSaveLayout::Top::Top() {
    append(label,{0u, 0u}, 10);
    append(edit,{~0u, 0u}, 10);
    append(find,{0u, 0u}, 10);
	append(hotkeys,{0u, 0u});
    setAlignment(0.5);
}

FastSaveLayout::FastSaveLayout() {
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
    listView.setHeaderVisible();
	listView.setHeaderText( { "", "", "" } );
    
    append(top,{~0u, 0u}, 5);
    append(autoSaveIdent,{~0u, 0u}, 5);
    append(listView,{~0u, ~0u});
}

DirectSaveLayout::DirectSaveLayout() {
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
    
    append(load,{0u, 0u}, 20);
    append(save,{0u, 0u});
    setAlignment(0.5);
	save.setEnabled(false);
}

StatesLayout::StatesLayout(TabWindow* tabWindow) {
    
    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;

    setMargin(10);
    
    append(fastSave,{~0u, ~0u}, 10);
    append(directSave,{~0u, 0u}, 10);
    append(statePath,{~0u, 0u});
	
	fastSave.top.hotkeys.onActivate = [this]() {		
		auto emuConfigView = EmuConfigView::TabWindow::getView( this->emulator );
		emuConfigView->show(EmuConfigView::TabWindow::Layout::Control);
		emuConfigView->inputLayout->triggerHotkeyMode();
	};
	
	fastSave.top.edit.onChange = [this]() {
		settings->set<std::string>( this->tabWindow->ident("save_ident"), fastSave.top.edit.text());
		settings->set<unsigned>( this->tabWindow->ident("save_slot"), 0);
	};
    
    fastSave.autoSaveIdent.onToggle = [this]() {
        
        settings->set<bool>( this->tabWindow->ident("auto_save_ident"), fastSave.autoSaveIdent.checked());
    };
    
    fastSave.autoSaveIdent.setChecked( settings->get<bool>( this->tabWindow->ident("auto_save_ident"), true) );
    
    settings->set<bool>( this->tabWindow->ident("auto_save_ident"), fastSave.autoSaveIdent.checked());
	
	directSave.load.onActivate = [this]() {
		std::string filePath = GUIKIT::BrowserWindow()
            .setWindow(*this->tabWindow)
            .setTitle(trans->get("select_savestate"))
            .setPath(settings->get<std::string>( this->tabWindow->ident( "save_direct_folder" ), "" ))
            .setFilters( { trans->get("state") + " (*.sav)", trans->get("all_files")} )
            .open();
		
		if (filePath.empty()) return;
            
        settings->set<std::string>( this->tabWindow->ident( "save_direct_folder" ), GUIKIT::File::getPath( filePath ) ); 
		
        States::getInstance(emulator)->load(filePath);
        view->setFocused(300);
	};
	
	directSave.save.onActivate = [this]() {
		std::string filePath = GUIKIT::BrowserWindow()
            .setWindow(*this->tabWindow)
            .setTitle(trans->get("select_savestate"))
            .setPath(settings->get<std::string>(this->tabWindow->ident("states_folder"), ""))
            .setFilters( { trans->get("state") + " (*.sav)", trans->get("all_files")} )
            .save();
		
		if (filePath.empty()) return;
            
        if ( !GUIKIT::String::foundSubStr( filePath, "." ))
            filePath += ".sav";
        
        settings->set<std::string>( this->tabWindow->ident( "save_direct_folder" ),  GUIKIT::File::getPath( filePath ) );            
            
        States::getInstance( emulator )->save( filePath );                
	};
	
	fastSave.listView.onActivate = [this]() {
		auto selection = fastSave.listView.selection();
		auto pos = fastSave.listView.text(selection, 0);
		settings->set<unsigned>( this->tabWindow->ident("save_slot"), std::stoul(pos));   
        
        unsigned statePos = 0;
        std::string baseName = splitFile( fastSave.listView.text(selection, 1), statePos );
        fastSave.top.edit.setText( baseName );
        settings->set<std::string>( this->tabWindow->ident("save_ident"), baseName);
        
        States::getInstance( emulator )->load( fastSave.listView.text(selection, 1), true );
        view->setFocused(300);
	};	
		
	fastSave.top.find.onActivate = [this]() {
		fastSave.listView.reset();
		
		auto fileName = fastSave.top.edit.text();
		if (fileName.empty()) {
			fileName = "savestate";
		}
		                
		auto infos = GUIKIT::File::getFolderList( States::getInstance( emulator )->statesFolder(), fileName );
				
		std::vector<Line> lines;
		
		for(auto& info : infos) {
            if (GUIKIT::String::endsWith(info.name, ".images"))
                continue;
            
            unsigned statePos = 0;
            splitFile( info.name, statePos );
            
			lines.push_back( {statePos, info.name, info.date} );
		}
		
		std::sort(lines.begin(), lines.end());
		
		for(auto& line : lines ) {
			fastSave.listView.append({ std::to_string(line.pos), line.fileName, line.date });
		}
	};
	
	fastSave.top.edit.setText( settings->get<std::string>( tabWindow->ident("save_ident"), "") );

    statePath.select.onActivate = [this]() {
        auto path = GUIKIT::BrowserWindow()
                .setTitle(trans->get("select_states_folder"))
                .setWindow(*this->tabWindow)
                .directory();

        if (!path.empty()) {
            settings->set<std::string>( this->tabWindow->ident("states_folder"), path);
            statePath.edit.setText(path);
        }
    };

    statePath.empty.onActivate = [this]() {
        settings->set<std::string>(this->tabWindow->ident("states_folder"), "");
        statePath.edit.setText("");
    };

    statePath.edit.setText(settings->get<std::string>(tabWindow->ident("states_folder"), ""));
}

auto StatesLayout::splitFile( std::string file, unsigned& pos ) -> std::string {
    
    auto parts = GUIKIT::String::split( file, '_' );
    if (parts.size() < 2)
        return file;
    
    auto ident = parts[ parts.size() - 1 ];
    
    try {
        pos = std::stoi( ident );
    } catch(...) { 
        pos = 0;
    }
    
    std::size_t end = file.find_last_of("_");
    if (end == std::string::npos)
        return file;
    
    file = file.erase(end);

    return file;
}

auto StatesLayout::translate() -> void {

    fastSave.top.label.setText( trans->get("labelling", {}, true) );
    fastSave.top.find.setText( trans->get("find") );
	fastSave.top.hotkeys.setText( trans->get("hotkeys") );
    fastSave.listView.setHeaderText({"#", trans->get("file"), trans->get("date")});
    fastSave.autoSaveIdent.setText( trans->get("auto_savestate_identifier") );
    
    directSave.load.setText( trans->get("load") );
    directSave.save.setText( trans->get("save") );
    
    fastSave.setText( trans->get("fast_save") );
    directSave.setText( trans->get("direct_save") );    
    
    statePath.label.setText( trans->get("folder", {}, true) );
    statePath.empty.setText( trans->get("remove") );
    statePath.select.setText( trans->get("select") );        
}

auto StatesLayout::updateSaveIdent( std::string fileName ) -> void {
    
    if (!settings->get<bool>( this->tabWindow->ident("auto_save_ident"), true))
        return;
    
    std::size_t end = fileName.find_last_of(".");
    if (end != std::string::npos)
        fileName = fileName.erase(end);
    
    fastSave.top.edit.setText( fileName );
    fastSave.top.edit.onChange();
}
