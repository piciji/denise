
SettingsLayout::Control::Control() {    
    append( load, {0u, 0u}, 10 );
    append( save, {0u, 0u}, 10 );
    append( remove, {0u, 0u}, 10 );
    append( edit, {~0u, 0u}, 10 );
    append( create, {0u, 0u} );    
    
    setAlignment(0.5);
}

SettingsLayout::Active::Active() {
    append(activeLabel,{0u, 0u}, 10);
    append(fileLabel,{~0u, 0u});
    append(standardButton,{0u, 0u});

    fileLabel.setFont(GUIKIT::Font::system("bold"));    

    setAlignment(0.5);
}

SettingsLayout::SettingsLayout() {
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
    listView.setHeaderVisible();
	listView.setHeaderText( { "", "" } );
    
    append(control, {~0u, 0u}, 5);
    append(active, {~0u, 0u}, 5);
    append(listView, {~0u, ~0u});
}

ConfigurationsFolderLayout::ConfigurationsFolderLayout() {
    append( label, {0u, 0u}, 10 );
    append( pathEdit, {~0u, 0u}, 10 );
    append( emptyButton, {0u, 0u}, 10 );
    append( selectButton, {0u, 0u} );
    
    pathEdit.setEditable( false );
    
    label.setFont(GUIKIT::Font::system("bold"));
    setAlignment(0.5);
}

StateFastLayout::Top::Top() {
    append(label,{0u, 0u}, 10);
    append(edit,{~0u, 0u}, 10);
    append(find,{0u, 0u}, 10);
	append(hotkeys,{0u, 0u});
    setAlignment(0.5);
}

StateFastLayout::StateFastLayout() {
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
    listView.setHeaderVisible();
	listView.setHeaderText( { "", "", "" } );
    
    append(top,{~0u, 0u}, 5);
    append(autoSaveIdent,{~0u, 0u}, 5);
    append(listView,{~0u, ~0u});
}

StateDirectLayout::StateDirectLayout() {
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
    
    append(load,{0u, 0u}, 20);
    append(save,{0u, 0u});
    setAlignment(0.5);
}

ConfigurationsLayout::ConfigurationsLayout(TabWindow* tabWindow) {

    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;
    
    setMargin(10);    
    
    moduleList.setHeaderText( { "" } );
    moduleList.setHeaderVisible( false );     
    moduleList.append( {"settings"} );
    moduleList.append( {"states"} );
    
    settingsImage.loadPng((uint8_t*)Icons::settings, sizeof(Icons::settings));
    scriptImage.loadPng((uint8_t*)Icons::script, sizeof(Icons::script));
    
    moduleList.setImage(0, 0, settingsImage);
    moduleList.setImage(1, 0, scriptImage);
    
    moduleList.setSelection(0);
    moduleFrame.append( moduleList, { GUIKIT::Font::scale(130), GUIKIT::Font::scale(100)} );
    moduleFrame.setPadding(10);
    moduleFrame.setFont( GUIKIT::Font::system("bold") );
    
    moduleList.onChange = [this]() {

        if (!moduleList.selected())
            return;

        moduleSwitch.setSelection( moduleList.selection() );
    };
        
    append( moduleFrame, {0u, 0u}, 10 );
        
    settingsFrame.append( settings, {~0u, ~0u}, 5 );
    settingsFrame.append( settingsFolder, {~0u, 0u} );
    
    statesFrame.append( stateFast, {~0u, ~0u}, 5 );
    statesFrame.append( stateDirect, {~0u, 0u}, 5 );    
    statesFrame.append( stateFolder, {~0u, 0u} );
    
    moduleSwitch.setLayout( 0, settingsFrame, {~0u, ~0u} );
    moduleSwitch.setLayout( 1, statesFrame, {~0u, ~0u} );
    
    append( moduleSwitch, {~0u, ~0u} );
    
    settings.listView.onActivate = [this]() {
        
        settings.control.load.onActivate();
    };
    
    settings.active.standardButton.onActivate = [this]() {
        
        saveCurrentSettings();
        
        std::string path = program->settingsFile( this->emulator->ident + "_" );
        
        if (this->load( path )) {
            _settings->set<std::string>("custom_settings", "", false);
            settings.active.fileLabel.setText( trans->get("default") );
        }
    };
    
    settings.control.load.onActivate = [this]() {
        auto& _list = settings.listView;
        
        if (!_list.selected())
            return;
        
        unsigned selection = _list.selection();

        std::string fileName = _list.text(selection, 0);

        std::string path = getSettingsFolder() + fileName;
        
        saveCurrentSettings();
        
        if (this->load( path )) {
            _settings->set<std::string>("custom_settings", path, false);
            settings.active.fileLabel.setText( fileName );
        }
    };
    
    settings.control.save.onActivate = [this]() {
        auto& _list = settings.listView;
        
        if (!_list.selected())
            return;
        
        unsigned selection = _list.selection();
        
        std::string fileName = _list.text( selection, 0 );

        std::string path = getSettingsFolder() + fileName;

        GUIKIT::File file(path);

        if (file.exists()) {
            if (!mes->question(trans->get("file_exist_error",{
                    {"%path%", path}})))
                return;
        }

        if (!_settings->save(path))
            mes->error(trans->get("file_creation_error",{
                {"%path%", path}}));
    };
    
    settings.control.create.onActivate = [this]() {
        
        auto fileName = settings.control.edit.text();  
        
        if (fileName == "")
            fileName = "alternate settings";
        
        std::string path = getSettingsFolder( true ) + fileName;
        
        GUIKIT::File file( path );
        
        if (file.exists()) {
            if (!mes->question( trans->get("file_exist_error", {{"%path%", path}}) ))
                return;
        }
            
        saveCurrentSettings();
        
        if (_settings->save( path)) {
            mes->information( trans->get("file_creation_success", {{"%path%", path}}) );
            _settings->set("custom_settings", path, false);
            settings.active.fileLabel.setText( fileName );
            updateSettingsList();
            
        } else
            mes->error( trans->get("file_creation_error", {{"%path%", path}}) );
    };
    
    settings.control.remove.onActivate = [this]() {
        auto& _list = settings.listView;
        
        if (!_list.selected())
            return;
        
        unsigned selection = _list.selection();
                
        std::string path = getSettingsFolder() + _list.text( selection, 0 );
        
        GUIKIT::File file( path );

        if (file.exists()) {
            if (!mes->question(trans->get("file deletion confirmation",{
                    {"%path%", path}
                })))
                return;
        }
        
        if (!file.del())
            mes->error( trans->get("file deletion error", {{"%path%", path}}) );
        else
            updateSettingsList();
    };    
    
    settingsFolder.selectButton.onActivate = [this]() {

        auto path = GUIKIT::BrowserWindow()
                .setTitle(trans->get("settings path"))
                .setWindow(*this->tabWindow)
                .directory();

        if (path.empty())
            return;
        
        if (_settings->get<std::string>("custom_settings", "") != "")
            settings.active.standardButton.onActivate();
        
        settingsFolder.pathEdit.setText( path );
        
        globalSettings->set<std::string>( emulator->ident + "_settings_path", path );
        
        updateSettingsList();
    };
    
    settingsFolder.emptyButton.onActivate = [this]() {
        
        if (_settings->get<std::string>("custom_settings", "") != "")
            settings.active.standardButton.onActivate();
                
        settingsFolder.pathEdit.setText( "" );                
        
        globalSettings->set<std::string>( emulator->ident + "_settings_path", "" );
        
        updateSettingsList();
    };
    
    settingsFolder.pathEdit.setText( globalSettings->get<std::string>( emulator->ident + "_settings_path", "" ) );
    
    settings.active.fileLabel.setText( trans->get("default") );
    
    // states
    
    stateFast.top.hotkeys.onActivate = [this]() {		
		auto emuConfigView = EmuConfigView::TabWindow::getView( this->emulator );
		emuConfigView->show(EmuConfigView::TabWindow::Layout::Control);
		emuConfigView->inputLayout->triggerHotkeyMode();
	};
	
	stateFast.top.edit.onChange = [this]() {
		_settings->set<std::string>( "save_ident", stateFast.top.edit.text());
		_settings->set<unsigned>( "save_slot", 0);
	};
    
    stateFast.autoSaveIdent.onToggle = [this]() {
        
        _settings->set<bool>( "auto_save_ident", stateFast.autoSaveIdent.checked());
    };
    
    stateFast.listView.onActivate = [this]() {
		auto selection = stateFast.listView.selection();
		auto pos = stateFast.listView.text(selection, 0);
		_settings->set<unsigned>( "save_slot", std::stoul(pos));   
        
        unsigned statePos = 0;
        std::string baseName = splitFile( stateFast.listView.text(selection, 1), statePos );
        stateFast.top.edit.setText( baseName );
        _settings->set<std::string>("save_ident", baseName);
        
        States::getInstance( emulator )->load( stateFast.listView.text(selection, 1), true );
        view->setFocused(300);
	};	
		
	stateFast.top.find.onActivate = [this]() {
		stateFast.listView.reset();
		
		auto fileName = stateFast.top.edit.text();
		if (fileName.empty()) {
			fileName = "savestate";
		}
		                
		auto infos = GUIKIT::File::getFolderList( States::getInstance( emulator )->statesFolder(), fileName );
				
		std::vector<StateLine> lines;
		
		for(auto& info : infos) {
            if (GUIKIT::String::endsWith(info.name, ".images"))
                continue;
            
            unsigned statePos = 0;
            splitFile( info.name, statePos );
            
			lines.push_back( {statePos, info.name, info.date} );
		}
		
		std::sort(lines.begin(), lines.end());
		
		for(auto& line : lines ) {
			stateFast.listView.append({ std::to_string(line.pos), line.fileName, line.date });
		}
	};
    
    stateDirect.load.onActivate = [this]() {
		std::string filePath = GUIKIT::BrowserWindow()
            .setWindow(*this->tabWindow)
            .setTitle(trans->get("select_savestate"))
            .setPath(_settings->get<std::string>( "save_direct_folder", "" ))
            .setFilters( { trans->get("state") + " (*.sav)", trans->get("all_files")} )
            .open();
		
		if (filePath.empty()) return;
            
        _settings->set<std::string>( "save_direct_folder", GUIKIT::File::getPath( filePath ) ); 
		
        States::getInstance(emulator)->load(filePath);
        view->setFocused(300);
	};
	
	stateDirect.save.onActivate = [this]() {
        
        if (activeEmulator != emulator)
            return mes->error( trans->get("no emulation active") );
        
		std::string filePath = GUIKIT::BrowserWindow()
            .setWindow(*this->tabWindow)
            .setTitle(trans->get("select_savestate"))
            .setPath(_settings->get<std::string>("states_folder", ""))
            .setFilters( { trans->get("state") + " (*.sav)", trans->get("all_files")} )
            .save();
		
		if (filePath.empty()) return;
            
        if ( !GUIKIT::String::foundSubStr( filePath, "." ))
            filePath += ".sav";
        
        _settings->set<std::string>( "save_direct_folder",  GUIKIT::File::getPath( filePath ) );            
            
        States::getInstance( emulator )->save( filePath );                
	};
    
    stateFolder.selectButton.onActivate = [this]() {
        auto path = GUIKIT::BrowserWindow()
                .setTitle(trans->get("select_states_folder"))
                .setWindow(*this->tabWindow)
                .directory();

        if (!path.empty()) {
            _settings->set<std::string>( "states_folder", path);
            stateFolder.pathEdit.setText(path);
        }
    };

    stateFolder.emptyButton.onActivate = [this]() {
        _settings->set<std::string>("states_folder", "");
        stateFolder.pathEdit.setText("");
    };
        
    loadSettings();
    
    updateSettingsList();
}

auto ConfigurationsLayout::updateSettingsList() -> void {    
    settings.listView.reset();
    
    auto infos = GUIKIT::File::getFolderList( getSettingsFolder() );
    
    std::vector<SettingLine> lines;

    for (auto& info : infos)
        lines.push_back({info.name, info.date});   

    std::sort(lines.begin(), lines.end());

    for (auto& line : lines)
        settings.listView.append( {line.fileName, line.date} );    
}            
            
auto ConfigurationsLayout::getSettingsFolder( bool createFolder ) -> std::string {
    
    auto path = globalSettings->get<std::string>( emulator->ident + "_settings_path", "");

    if (path.empty()) {
        std::string _emuIdent = emulator->ident;
        path = program->appFolder() + "/settings/" + GUIKIT::String::toLowerCase(_emuIdent);
        std::string basePath = GUIKIT::System::getUserDataFolder( );
        
        if (createFolder)
            GUIKIT::File::createDir( path, basePath );
        
        path = basePath + path;
    }
    
    return GUIKIT::File::beautifyPath(path);
}

auto ConfigurationsLayout::load( std::string path ) -> bool {

    GUIKIT::File file(path);

    if (!file.exists()) {
        mes->error(trans->get("file_open_error",{
            {"%path%", path}
        }));
        return false;
    }

    if (activeEmulator)
        view->poweroff.onActivate();

    if (!_settings->load(path)) {
        mes->error(trans->get("file_open_error",{
            {"%path%", path}
        }));
        return false;
    }

    program->initEmulator(this->emulator);

    auto inputManager = InputManager::getManager(this->emulator);
    inputManager->resetMappings();
    inputManager->updateAnalogSensitivity();
    inputManager->bindHids();

    view->updateDeviceSelection(this->emulator);

    this->tabWindow->audioLayout->loadSettings();

    this->tabWindow->borderLayout->loadSettings();

    this->tabWindow->firmwareLayout->loadSettings();

    this->tabWindow->inputLayout->loadSettings();

    this->tabWindow->miscLayout->loadSettings();

    this->tabWindow->paletteLayout->loadSettings();

    this->tabWindow->systemLayout->loadSettings();

    this->tabWindow->videoLayout->loadSettings();
    
    loadSettings();

    MediaView::MediaWindow::getView(this->emulator)->loadSettings();
    
    return true;
}

auto ConfigurationsLayout::saveCurrentSettings() -> void {
    
    if (!globalSettings->get<bool>("save_settings_on_exit", true))
        return;

    std::string path = _settings->get<std::string>("custom_settings", "");
    if (path == "")
        path = program->settingsFile( emulator->ident + "_" );
    
    _settings->save( path );
}

auto ConfigurationsLayout::splitFile( std::string file, unsigned& pos ) -> std::string {
    
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

auto ConfigurationsLayout::updateSaveIdent( std::string fileName ) -> void {
    
    std::size_t end = fileName.find_last_of(".");
    if (end != std::string::npos)
        fileName = fileName.erase(end);
    
    // for wav record
    _settings->set<std::string>( "record_ident", fileName, false);
    
    if (!_settings->get<bool>( "auto_save_ident", true))
        return;
        
    stateFast.top.edit.setText( fileName );
    stateFast.top.edit.onChange();
}

auto ConfigurationsLayout::loadSettings() -> void {
    
    stateFast.autoSaveIdent.setChecked( _settings->get<bool>( "auto_save_ident", true) );
    stateFast.top.edit.setText( _settings->get<std::string>( "save_ident", "") );
    stateFolder.pathEdit.setText(_settings->get<std::string>("states_folder", ""));
}

auto ConfigurationsLayout::translate() -> void {
            
    settings.control.load.setText( trans->get("load") );
    settings.control.save.setText( trans->get("save") );
    settings.control.create.setText( trans->get("create") );
    settings.control.remove.setText( trans->get("remove") );
    
    settingsFolder.label.setText( trans->get("folder", {}, true) );
    settingsFolder.emptyButton.setText( trans->get("remove") );
    settingsFolder.selectButton.setText( trans->get("select") );
    
    settings.setText( trans->get("settings") );
    settings.active.activeLabel.setText( trans->get("active setting", {}, true) );
    settings.active.standardButton.setText( trans->get("default") );
    settings.listView.setHeaderText({trans->get("file"), trans->get("date")});
    
    stateFolder.label.setText( trans->get("folder", {}, true) );
    stateFolder.emptyButton.setText( trans->get("remove") );
    stateFolder.selectButton.setText( trans->get("select") );
    
    stateFast.top.label.setText( trans->get("labelling", {}, true) );
    stateFast.top.find.setText( trans->get("find") );
	stateFast.top.hotkeys.setText( trans->get("hotkeys") );
    stateFast.listView.setHeaderText({"#", trans->get("file"), trans->get("date")});
    stateFast.autoSaveIdent.setText( trans->get("auto_savestate_identifier") );
    
    stateDirect.load.setText( trans->get("load") );
    stateDirect.save.setText( trans->get("save") );
    
    stateFast.setText( trans->get("fast_save") );
    stateDirect.setText( trans->get("direct_save") );
    
    moduleList.setText( 0, 0, trans->get( "settings" ) ); 
    moduleList.setText( 1, 0, trans->get( "states" ) );
    
    moduleFrame.setText( trans->get("selection") );
    
    if (_settings->get<std::string>( "custom_settings", "" ) == "")                    
        settings.active.fileLabel.setText( trans->get("default") );
}