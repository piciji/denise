
MemoryPatternLayout::FirstLine::FirstLine() {
    append( valueLabel, {0u, 0u}, 10 );
    append( valueStepper, {0u, 0u}, 10 );
    append( invertValueEveryLabel, {0u, 0u}, 10 );
    append( invertValueEveryCombo, {0u, 0u} );
    
    valueStepper.setRange(0, 0xff);
    
    invertValueEveryCombo.append( "0 bytes", 0 );
    invertValueEveryCombo.append( "1 byte", 1 );       
    
    unsigned i = 2;
    while(i < (64 * 1024)) {
        invertValueEveryCombo.append( std::to_string(i) + " bytes", i );
        i <<= 1;
    }
    
    setAlignment(0.5);
}

MemoryPatternLayout::SecondLine::SecondLine() {
    append( valueLabel, {0u, 0u}, 10 );
    append( valueStepper, {0u, 0u}, 10 );
    append( invertValueEveryLabel, {0u, 0u}, 10 );
    append( invertValueEveryCombo, {0u, 0u} );

    valueStepper.setRange(0, 0xff);

    invertValueEveryCombo.append( "0 bytes", 0 );
    invertValueEveryCombo.append( "1 byte", 1 );

    unsigned i = 2;
    while(i < (64 * 1024)) {
        invertValueEveryCombo.append( std::to_string(i) + " bytes", i );
        i <<= 1;
    }

    setAlignment(0.5);
}

MemoryPatternLayout::ThirdLine::ThirdLine() {
    
    append( lengthRandomLabel, {0u, 0u}, 10 );
    append( lengthRandomCombo, {0u, 0u}, 10 );
    append( repeatRandomEveryLabel, {0u, 0u}, 10 );
    append( repeatRandomEveryCombo, {0u, 0u} );

    lengthRandomCombo.append("0 bytes", 0);
    lengthRandomCombo.append("1 byte", 1);
    repeatRandomEveryCombo.append("0 bytes", 0);
    repeatRandomEveryCombo.append("1 byte", 1);
    
    unsigned i = 2;
    while (i < (64 * 1024)) {
        lengthRandomCombo.append(std::to_string(i) + " bytes", i);
        repeatRandomEveryCombo.append(std::to_string(i) + " bytes", i);

        i <<= 1;
    }
    
    setAlignment(0.5);
}

MemoryPatternLayout::FourthLine::FourthLine() {
    
    append( randomChanceLabel, {0u, 0u}, 10 );
    append( randomChanceStepper, {0u, 0u}, 10 );
    append( offsetLabel, {0u, 0u}, 10 );
    append( offsetCombo, {0u, 0u} );
    
    randomChanceStepper.setRange(0, 1000);

    offsetCombo.append( "0 bytes", 0 );
    offsetCombo.append( "1 byte", 1 );

    unsigned i = 2;
    while(i < (64 * 1024)) {
        offsetCombo.append( std::to_string(i) + " bytes", i );
        i <<= 1;
    }
    
    setAlignment(0.5);
}

MemoryPatternLayout::FifthLine::FifthLine() {

    append( preConfigured1, {0u, 0u}, 10 );
    append( preConfigured2, {0u, 0u}, 10 );
    append( preConfigured3, {0u, 0u} );

    setAlignment(0.5);
}

MemoryPatternLayout::MemoryPatternLayout(TabWindow* tabWindow) {
    
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
	GUIKIT::Label test;
	test.setFont( GUIKIT::Font::system("", true) );
	test.setText( "0000: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 " );
	auto size = test.minimumSize();
    
    append( firstLine, {0u, 0u}, 10 );
    append( secondLine, {0u, 0u}, 10 );
    append( thirdLine, {0u, 0u}, 10 );
    append( fourthLine, {0u, 0u}, 10 );
    append( preview, {size.width + tabWindow->getScrollbarWidth(), size.height * 17}, 10 );
    append( fifthLine, {0u, 0u} );
    
    preview.setFont( GUIKIT::Font::system("", true) );
    preview.setForegroundColor( 0x5a5e63 );
    preview.setEditable(false);
}

SettingsLayout::Control::Control() {    
    append( load, {0u, 0u}, 10 );
    append( save, {0u, 0u}, 10 );
    append( remove, {0u, 0u}, 10 );
    append( edit, {~0u, 0u}, 10 );
    append( create, {0u, 0u} );
    
    load.setEnabled(false);
    save.setEnabled(false);
    
    setAlignment(0.5);
}

SettingsLayout::Active::Active() {
    append(activeLabel,{0u, 0u}, 10);
    append(fileLabel,{~0u, 0u});
    append(undockButton,{0u, 0u}, 10);
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
    append(startWithLastConfigCheckbox,{~0u, 0u}, 5);
    append(listView, {~0u, ~0u}, 5);
}

ConfigurationsFolderLayout::ConfigurationsFolderLayout() {
    append( label, {0u, 0u}, 10 );
    append( pathEdit, {~0u, 0u}, 10 );
    append( standard, {0u, 0u}, 10 );
    append( select, {0u, 0u} );
    
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

StateFastLayout::Options::Options() {
    append(autoLabel,{0u, 0u}, 10);
    append(autoIdentOff,{0u, 0u}, 10);
    append(autoIdentOn,{0u, 0u}, 10);
    append(autoIdentCutFollowUp,{0u, 0u});

    GUIKIT::RadioBox::setGroup(autoIdentOff, autoIdentOn, autoIdentCutFollowUp);
    setAlignment(0.5);
}

StateFastLayout::StateFastLayout() {
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
    listView.setHeaderVisible();
	listView.setHeaderText( { "", "", "" } );
    
    append(top,{~0u, 0u}, 5);
    append(options,{0u, 0u}, 5);
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
    if(dynamic_cast<LIBC64::Interface*>(emulator))
        memoryPattern = new MemoryPatternLayout(tabWindow);
    
    moduleList.setHeaderText( { "" } );
    moduleList.setHeaderVisible( false );     
    moduleList.append( {"settings"} );
    moduleList.append( {"states"} );
    if (memoryPattern)
        moduleList.append( {"memory"} );
    
    settingsImage.loadPng((uint8_t*)Icons::settings, sizeof(Icons::settings));
    scriptImage.loadPng((uint8_t*)Icons::script, sizeof(Icons::script));
    if (memoryPattern)
        memImage.loadPng((uint8_t*)Icons::memory, sizeof(Icons::memory));
    
    moduleList.setImage(0, 0, settingsImage);
    moduleList.setImage(1, 0, scriptImage);
    if (memoryPattern)
        moduleList.setImage(2, 0, memImage);
    
    moduleList.setSelection(0);
    moduleFrame.append( moduleList, { GUIKIT::Font::scale(140), GUIKIT::Font::scale(100)} );
    moduleFrame.setPadding(10);
    moduleFrame.setFont( GUIKIT::Font::system("bold") );
    
    moduleList.onChange = [this]() {

        if (!moduleList.selected())
            return;

        auto _sel = moduleList.selection();

        moduleSwitch.setSelection( _sel );
        if (memoryPattern && _sel == 2) {
            GUIKIT::HorizontalLayout::alignChildrenVertically( {&memoryPattern->firstLine, &memoryPattern->secondLine, &memoryPattern->thirdLine} );
            memoryPattern->updateLayout();
        }
    };
        
    append( moduleFrame, {0u, 0u}, 10 );
        
    settingsFrame.append( settings, {~0u, ~0u}, 5 );
    settingsFrame.append( settingsFolder, {~0u, 0u} );
    
    statesFrame.append( stateFast, {~0u, ~0u}, 5 );
    statesFrame.append( stateDirect, {~0u, 0u}, 5 );    
    statesFrame.append( stateFolder, {~0u, 0u} );

    append( moduleSwitch, {~0u, ~0u} );
    moduleSwitch.setLayout( 0, settingsFrame, {~0u, ~0u} );
    moduleSwitch.setLayout( 1, statesFrame, {~0u, ~0u} );
    if(memoryPattern)
        moduleSwitch.setLayout( 2, *memoryPattern, {~0u, ~0u} );
    
    if(memoryPattern) {
		
		memoryPattern->preview.onChange = [this]() {
         //   mes->warning( "on change" );
		};
		
		memoryPattern->preview.onFocus = [this]() {
		//	mes->warning( "on focus" );
		};

        memoryPattern->firstLine.valueStepper.onChange = [this]() {
			
            _settings->set<unsigned>("memory_value", (unsigned)(memoryPattern->firstLine.valueStepper.getValue()));

            this->updateMemoryPreview();
        };
        
        memoryPattern->firstLine.invertValueEveryCombo.onChange = [this]() {

            _settings->set<unsigned>("memory_invert_every", (unsigned)(memoryPattern->firstLine.invertValueEveryCombo.userData()));

            this->updateMemoryPreview();
        };

        memoryPattern->secondLine.valueStepper.onChange = [this]() {

            _settings->set<unsigned>("memory_second_value", (unsigned)(memoryPattern->secondLine.valueStepper.getValue()));

            this->updateMemoryPreview();
        };

        memoryPattern->secondLine.invertValueEveryCombo.onChange = [this]() {

            _settings->set<unsigned>("memory_second_invert_every", (unsigned)(memoryPattern->secondLine.invertValueEveryCombo.userData()));

            this->updateMemoryPreview();
        };
        
        memoryPattern->thirdLine.lengthRandomCombo.onChange = [this]() {

            _settings->set<unsigned>("memory_random_pattern", (unsigned)(memoryPattern->thirdLine.lengthRandomCombo.userData()));

            this->updateMemoryPreview();
        };

        memoryPattern->thirdLine.repeatRandomEveryCombo.onChange = [this]() {

            _settings->set<unsigned>("memory_random_repeat", (unsigned) (memoryPattern->thirdLine.repeatRandomEveryCombo.userData()));

            this->updateMemoryPreview();
        };

        memoryPattern->fourthLine.randomChanceStepper.onChange = [this]() {

            _settings->set<unsigned>("random_chance", (unsigned) (memoryPattern->fourthLine.randomChanceStepper.getValue()));

            this->updateMemoryPreview();
        };

        memoryPattern->fourthLine.offsetCombo.onChange = [this]() {

            _settings->set<unsigned>("memory_offset", (unsigned) (memoryPattern->fourthLine.offsetCombo.userData()));

            this->updateMemoryPreview();
        };

        memoryPattern->fifthLine.preConfigured1.onActivate = [this]() {
            memoryPattern->firstLine.valueStepper.setValue(0);
            memoryPattern->firstLine.invertValueEveryCombo.setSelectionByUserId(64);
            memoryPattern->secondLine.valueStepper.setValue(0);
            memoryPattern->secondLine.invertValueEveryCombo.setSelectionByUserId(0);
            memoryPattern->thirdLine.lengthRandomCombo.setSelectionByUserId(0);
            memoryPattern->thirdLine.repeatRandomEveryCombo.setSelectionByUserId(0);
            memoryPattern->fourthLine.randomChanceStepper.setValue(0);
            memoryPattern->fourthLine.offsetCombo.setSelectionByUserId(0);

            _settings->set<unsigned>("memory_value", 0);
            _settings->set<unsigned>("memory_invert_every", 64);
            _settings->set<unsigned>("memory_second_value", 0);
            _settings->set<unsigned>("memory_second_invert_every", 0);
            _settings->set<unsigned>("memory_random_pattern", 0);
            _settings->set<unsigned>("memory_random_repeat", 0);
            _settings->set<unsigned>("random_chance", 0);
            _settings->set<unsigned>("memory_offset", 0);

            this->updateMemoryPreview();
        };

        memoryPattern->fifthLine.preConfigured2.onActivate = [this]() {
            memoryPattern->firstLine.valueStepper.setValue(256);
            memoryPattern->firstLine.invertValueEveryCombo.setSelectionByUserId(64);
            memoryPattern->secondLine.valueStepper.setValue(0);
            memoryPattern->secondLine.invertValueEveryCombo.setSelectionByUserId(0);
            memoryPattern->thirdLine.lengthRandomCombo.setSelectionByUserId(1);
            memoryPattern->thirdLine.repeatRandomEveryCombo.setSelectionByUserId(256);
            memoryPattern->fourthLine.randomChanceStepper.setValue(0);
            memoryPattern->fourthLine.offsetCombo.setSelectionByUserId(0);

            _settings->set<unsigned>("memory_value", 255);
            _settings->set<unsigned>("memory_invert_every", 64);
            _settings->set<unsigned>("memory_second_value", 0);
            _settings->set<unsigned>("memory_second_invert_every", 0);
            _settings->set<unsigned>("memory_random_pattern", 1);
            _settings->set<unsigned>("memory_random_repeat", 256);
            _settings->set<unsigned>("random_chance", 0);
            _settings->set<unsigned>("memory_offset", 0);

            this->updateMemoryPreview();
        };

        memoryPattern->fifthLine.preConfigured3.onActivate = [this]() {
            memoryPattern->firstLine.valueStepper.setValue(0);
            memoryPattern->firstLine.invertValueEveryCombo.setSelectionByUserId(4);
            memoryPattern->secondLine.valueStepper.setValue(255);
            memoryPattern->secondLine.invertValueEveryCombo.setSelectionByUserId(16384);
            memoryPattern->thirdLine.lengthRandomCombo.setSelectionByUserId(0);
            memoryPattern->thirdLine.repeatRandomEveryCombo.setSelectionByUserId(0);
            memoryPattern->fourthLine.randomChanceStepper.setValue(1);
            memoryPattern->fourthLine.offsetCombo.setSelectionByUserId(2);

            _settings->set<unsigned>("memory_value", 0);
            _settings->set<unsigned>("memory_invert_every", 4);
            _settings->set<unsigned>("memory_second_value", 255);
            _settings->set<unsigned>("memory_second_invert_every", 16384);
            _settings->set<unsigned>("memory_random_pattern", 0);
            _settings->set<unsigned>("memory_random_repeat", 0);
            _settings->set<unsigned>("random_chance", 1);
            _settings->set<unsigned>("memory_offset", 2);

            this->updateMemoryPreview();
        };
    }
    settings.listView.onChange = [this]() {
        
        if (!settings.control.load.enabled()) {
            settings.control.load.setEnabled();
            settings.control.save.setEnabled();
        }
    };

    settings.startWithLastConfigCheckbox.onToggle = [this](bool checked) {
        globalSettings->set<bool>( this->emulator->ident + "_load_last_settings", checked );
    };

    settings.startWithLastConfigCheckbox.setChecked( globalSettings->get<bool>( this->emulator->ident + "_load_last_settings", false ) );

    settings.listView.onActivate = [this]() {
        
        settings.control.load.onActivate();
    };

    bool undockMode = globalSettings->get<bool>("undock", false);

    if (undockMode || GUIKIT::Application::isGtk())
        settings.active.remove( settings.active.undockButton );

    settings.active.undockButton.onActivate = [this]() {

        if (mes->question( trans->get("undock settings") ) ) {
            program->undockSettings();

            settings.active.remove( settings.active.undockButton );
        }
    };

    settings.active.standardButton.onActivate = [this]() {

        if (!cmd->hasCustomConfig(emulator) && ("" == globalSettings->get<std::string>( emulator->ident + "_custom_settings", "")))
            return;

        emuThread->lock();

        if (!this->load(program->settingsFileFromEmuFolder(emulator->ident + "_"), false)) {
            if (!this->load(program->settingsFile(emulator->ident + "_"))) {
                emuThread->unlock();
                return;
            }
        }

        globalSettings->set<std::string>(emulator->ident + "_custom_settings", "");
        cmd->removeCustomConfig(emulator);
        settings.active.fileLabel.setText( trans->get("default") );

        emuThread->unlock();
    };
    
    settings.control.load.onActivate = [this]() {
        auto& _list = settings.listView;
        
        if (!_list.selected())
            return;
        
        unsigned selection = _list.selection();

        std::string fileName = _list.text(selection, 0);

        std::string path = program->getCustomSettingsFolder(emulator) + fileName;

        emuThread->lock();
        if (this->load( path )) {
            globalSettings->set<std::string>(emulator->ident + "_custom_settings", fileName);
            cmd->removeCustomConfig(emulator);
            settings.active.fileLabel.setText( fileName );
        }
        emuThread->unlock();
    };
    
    settings.control.save.onActivate = [this]() {
        auto& _list = settings.listView;
        std::string fileName;
        
        if (!_list.selected()) {
            if (cmd->hasCustomConfig(emulator))
                return;

            fileName = globalSettings->get<std::string>(emulator->ident + "_custom_settings", "");
            if (fileName == "")
                return;
            // remove absolute path (deprecated)
            fileName = GUIKIT::String::getFileName(fileName);

        } else {
            unsigned selection = _list.selection();

            fileName = _list.text( selection, 0 );
        }

        std::string path = program->getCustomSettingsFolder(emulator, true) + fileName;

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
            fileName = trans->get("alternate settings");
        
        std::string path = program->getCustomSettingsFolder(emulator, true) + fileName;
        
        GUIKIT::File file( path );
        
        if (file.exists()) {
            if (!mes->question( trans->get("file_exist_error", {{"%path%", path}}) ))
                return;
        }

        if (_settings->save( path)) {
            mes->information( trans->get("file_creation_success", {{"%path%", path}}) );
            globalSettings->set(emulator->ident + "_custom_settings", fileName);
            cmd->removeCustomConfig(emulator);
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

        std::string fileName = _list.text( selection, 0 );
                
        std::string path = program->getCustomSettingsFolder(emulator) + fileName;
        
        GUIKIT::File file( path );

        if (file.exists()) {
            if (!mes->question(trans->get("file deletion confirmation",{
                    {"%path%", path}
                })))
                return;
        }        
		
        if (!file.del())
            mes->error( trans->get("file deletion error", {{"%path%", path}}) );
        else {
			
			if (!cmd->hasCustomConfig(emulator) && (fileName == globalSettings->get<std::string>(emulator->ident + "_custom_settings", ""))) {
                globalSettings->set<std::string>(emulator->ident + "_custom_settings", "");

                emuThread->lock();

                if (!this->load(program->settingsFileFromEmuFolder(emulator->ident + "_"), false)) {
                    if (!this->load(program->settingsFile(emulator->ident + "_"))) {
                        emuThread->unlock();
                        return;
                    }
                }

                settings.active.fileLabel.setText(trans->get("default"));

                emuThread->unlock();
			}
			
            updateSettingsList();
		}
    };    
    
    settingsFolder.select.onActivate = [this]() {

        auto path = GUIKIT::BrowserWindow()
                .setTitle(trans->get("select settings folder"))
                .setWindow(*this->tabWindow)
                .directory();

        if (path.empty())
            return;
        
        if (!cmd->hasCustomConfig(emulator) && (globalSettings->get<std::string>(emulator->ident + "_custom_settings", "") != ""))
            settings.active.standardButton.onActivate();

        settingsFolder.pathEdit.setEnabled();
        settingsFolder.pathEdit.setText( path );
        
        globalSettings->set<std::string>( emulator->ident + "_settings_path", path );
        
        updateSettingsList();
    };

    settingsFolder.standard.onActivate = [this]() {
        if (!cmd->hasCustomConfig(emulator) && (globalSettings->get<std::string>(emulator->ident + "_custom_settings", "") != ""))
            settings.active.standardButton.onActivate();

        settingsFolder.pathEdit.setText( "" );

        globalSettings->set<std::string>( emulator->ident + "_settings_path", "" );

        settingsFolder.pathEdit.setText( trans->get("home folder") );

        settingsFolder.pathEdit.setEnabled(false);

        updateSettingsList();
    };

    std::string _settingsPath = globalSettings->get<std::string>( emulator->ident + "_settings_path", "" );

    settingsFolder.pathEdit.setText( _settingsPath == "" ? trans->get("home folder") : _settingsPath );
    settingsFolder.pathEdit.setEnabled(_settingsPath != "");

    if (!settings.startWithLastConfigCheckbox.checked() || cmd->hasCustomConfig(emulator))
        settings.active.fileLabel.setText(trans->get("default"));
    else
        settings.active.fileLabel.setText( GUIKIT::String::getFileName( globalSettings->get<std::string>(emulator->ident + "_custom_settings", trans->get("default"))));
    
    // states
    
    stateFast.top.hotkeys.onActivate = [this]() {
		this->tabWindow->show(EmuConfigView::TabWindow::Layout::Control);
        this->tabWindow->inputLayout->triggerHotkeyMode();
	};
	
	stateFast.top.edit.onChange = [this]() {
		_settings->set<std::string>( "save_ident", stateFast.top.edit.text());
		_settings->set<unsigned>( "save_slot", 0);
	};
    
    stateFast.options.autoIdentOff.onActivate = [this]() {
        _settings->set<unsigned>( "auto_save_mode", 0);
    };

    stateFast.options.autoIdentOn.onActivate = [this]() {
        _settings->set<unsigned>( "auto_save_mode", 1);
    };

    stateFast.options.autoIdentCutFollowUp.onActivate = [this]() {
        _settings->set<unsigned>( "auto_save_mode", 2);
    };
    
    stateFast.listView.onActivate = [this]() {
		auto selection = stateFast.listView.selection();
		auto pos = stateFast.listView.text(selection, 0);
		_settings->set<unsigned>( "save_slot", std::stoul(pos));   
        
        unsigned statePos = 0;
        std::string baseName = splitFile( stateFast.listView.text(selection, 1), statePos );
        stateFast.top.edit.setText( baseName );
        _settings->set<std::string>("save_ident", baseName);

        emuThread->lock();
        States::getInstance( emulator )->load( stateFast.listView.text(selection, 1), true );
        emuThread->unlock();
        view->setFocused(100);
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

        emuThread->lock();
        States::getInstance(emulator)->load(filePath);
        emuThread->unlock();
        view->setFocused(100);
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

        emuThread->lock();
        States::getInstance( emulator )->save( filePath );
        emuThread->unlock();
	};
    
    stateFolder.select.onActivate = [this]() {
        auto path = GUIKIT::BrowserWindow()
                .setTitle(trans->get("select_states_folder"))
                .setWindow(*this->tabWindow)
                .directory();

        if (!path.empty()) {
            _settings->set<std::string>( "states_folder", path);
            stateFolder.pathEdit.setText(path);
        }
    };

    stateFolder.standard.onActivate = [this]() {
        _settings->set<std::string>("states_folder", "");
        stateFolder.pathEdit.setText("");
    };
        
    loadSettings();
    
    updateSettingsList();
}

auto ConfigurationsLayout::updateSettingsList() -> void {    
    settings.listView.reset();
    
    auto infos = GUIKIT::File::getFolderList( program->getCustomSettingsFolder( emulator ) );
    
    std::vector<SettingLine> lines;

    for (auto& info : infos)
        lines.push_back({info.name, info.date});   

    std::sort(lines.begin(), lines.end());

    for (auto& line : lines)
        settings.listView.append( {line.fileName, line.date} );    
}

auto ConfigurationsLayout::load( std::string path, bool showError ) -> bool {

    GUIKIT::File file(path);

    if (!file.exists()) {
        if (showError)
            mes->error(trans->get("file_open_error",{
                {"%path%", path}
            }));
        return false;
    }

    std::string _audioFloppyFolder = _settings->get<std::string>("audio_floppy_folder", "");
    std::string _audioTapeFolder = _settings->get<std::string>("audio_tape_folder", "");

    if (activeEmulator)
        program->powerOff();

    if (!_settings->load(path)) {
        if (showError)
            mes->error(trans->get("file_open_error",{
                {"%path%", path}
            }));
        return false;
    }

    program->initEmulator(this->emulator);

    auto inputManager = InputManager::getManager(this->emulator);
    inputManager->resetMappings();
    inputManager->updateAnalogSensitivity();
    inputManager->updateAutofireFrequency();
    inputManager->updateMiscSettings();
    inputManager->bindHids();

    auto firmwareManager = FirmwareManager::getInstance(this->emulator);
    firmwareManager->reload();

    std::string audioFloppyFolder = _settings->get<std::string>("audio_floppy_folder", "");
    std::string audioTapeFolder = _settings->get<std::string>("audio_tape_folder", "");

    if (_audioFloppyFolder != audioFloppyFolder)
        audioManager->drive.unload(this->emulator, this->emulator->getDiskMediaGroup());

    if (_audioTapeFolder != audioTapeFolder)
        audioManager->drive.unload(this->emulator, this->emulator->getTapeMediaGroup());

    view->updateDeviceSelection(this->emulator);
    
    if(this->tabWindow->audioLayout) this->tabWindow->audioLayout->loadSettings();

    if(this->tabWindow->geometryLayout) this->tabWindow->geometryLayout->loadSettings();

    if(this->tabWindow->firmwareLayout) this->tabWindow->firmwareLayout->loadSettings();

    if(this->tabWindow->inputLayout) this->tabWindow->inputLayout->loadSettings();

    if(this->tabWindow->miscLayout) this->tabWindow->miscLayout->loadSettings();

    if(this->tabWindow->paletteLayout) this->tabWindow->paletteLayout->loadSettings();

    if(this->tabWindow->systemLayout) this->tabWindow->systemLayout->loadSettings();

    if(this->tabWindow->videoLayout)
        this->tabWindow->videoLayout->loadSettings();
    else if (videoDriver)
        VideoManager::getInstance( emulator )->reloadSettings(true);

    if(this->tabWindow->mediaLayout)
        this->tabWindow->mediaLayout->loadSettings();
    else
        fileloader->loadSettings(this->emulator);
    
    loadSettings();

    view->updateSpeedLabels();
    view->buildShader();

    program->power(this->emulator);

    autoloader->set(this->emulator, nullptr, false, 0);

    return true;
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
        
    stateFast.top.edit.setText( fileName );
}

auto ConfigurationsLayout::loadSettings() -> void {
    auto autoSaveMode = _settings->get<unsigned>( "auto_save_mode", 2);
    switch(autoSaveMode) {
        case 0: stateFast.options.autoIdentOff.setChecked(); break;
        case 1: stateFast.options.autoIdentOn.setChecked(); break;
        default:
        case 2: stateFast.options.autoIdentCutFollowUp.setChecked(); break;
    }

    stateFast.top.edit.setText( _settings->get<std::string>( "save_ident", "") );
    stateFolder.pathEdit.setText(_settings->get<std::string>("states_folder", ""));
    
    if(memoryPattern) {
        Emulator::Interface::MemoryPattern pattern;
        program->getMemoryPatternFromConfig(emulator, pattern);
        
        memoryPattern->firstLine.valueStepper.setValue( pattern.value );
        memoryPattern->firstLine.invertValueEveryCombo.setSelectionByUserId( pattern.invertEvery );
        memoryPattern->secondLine.valueStepper.setValue( pattern.secondValue );
        memoryPattern->secondLine.invertValueEveryCombo.setSelectionByUserId( pattern.secondInvertEvery );
        
        memoryPattern->thirdLine.lengthRandomCombo.setSelectionByUserId( pattern.randomPatternLength );
        memoryPattern->thirdLine.repeatRandomEveryCombo.setSelectionByUserId( pattern.repeatRandomPattern );
        
        memoryPattern->fourthLine.randomChanceStepper.setValue( pattern.randomChance );
        memoryPattern->fourthLine.offsetCombo.setSelectionByUserId( pattern.offset );
        
        updateMemoryPreview();
    }
}

auto ConfigurationsLayout::updateMemoryPreview() -> void {
    program->setMemoryPattern(emulator);

    unsigned size = emulator->getMemorySize();
    
    uint8_t* pattern = new uint8_t[ size ];

    emulator->getMemoryInitPattern( pattern );
    
    char hex[6];
    
    std::string out = "";
    
    unsigned addr = 0;
    
    uint8_t i;
    
    while(true) {
        
        sprintf( hex, "%04x", addr );
        
        out += (std::string)hex;
        out += ": ";

        for (i = 0; i < 16; i++, addr++) {

            sprintf(hex, "%02x", pattern[addr]);

            out += (std::string)hex;
            out += " ";
        }
        
        out += "\r\n";
        
        if (addr == size)
            break;
        
        if ((addr & 0xff) == 0)
            out += "\r\n";
    }
    
    delete[] pattern;
    
    memoryPattern->preview.setText( out );
}

auto ConfigurationsLayout::translate() -> void {
            
    settings.control.load.setText( trans->get("load") );
    settings.control.save.setText( trans->get("save") );
    settings.control.create.setText( trans->get("create") );
    settings.control.remove.setText( trans->get("remove") );
    
    settingsFolder.label.setText( trans->get("folder", {}, true) );
    settingsFolder.standard.setText( trans->get("home folder") );
    settingsFolder.select.setText( trans->get("select") );
    
    settings.setText( trans->get("settings") );
    settings.active.activeLabel.setText( trans->get("active setting", {}, true) );
    settings.active.undockButton.setText( trans->get("undock") );
    settings.active.standardButton.setText( trans->get("default") );
    settings.listView.setHeaderText({trans->get("file"), trans->get("date")});
    settings.startWithLastConfigCheckbox.setText( trans->get("Start with last loaded Settings") );
    
    stateFolder.label.setText( trans->get("folder", {}, true) );
    stateFolder.standard.setText( trans->get("default") );
    stateFolder.select.setText( trans->get("select") );
    
    stateFast.top.label.setText( trans->get("labelling", {}, true) );
    stateFast.top.find.setText( trans->get("find") );
	stateFast.top.hotkeys.setText( trans->get("hotkeys") );
    stateFast.listView.setHeaderText({"#", trans->get("file"), trans->get("date")});

    stateFast.options.autoLabel.setText( trans->getA("automatic savestate identifier", true) );
    stateFast.options.autoIdentOff.setText( trans->getA("off") );
    stateFast.options.autoIdentOn.setText( trans->get("on") );
    stateFast.options.autoIdentCutFollowUp.setText( trans->get("cut off follow-up disks") );
    
    stateDirect.load.setText( trans->get("load") );
    stateDirect.save.setText( trans->get("save") );
    
    stateFast.setText( trans->get("fast_save") );
    stateDirect.setText( trans->get("direct_save") );
    
    moduleList.setText( 0, 0, trans->get( "settings" ) ); 
    moduleList.setText( 1, 0, trans->get( "states" ) );
    if (memoryPattern)
        moduleList.setText( 2, 0, trans->get( "memory" ) );
    
    moduleFrame.setText( trans->get("selection") );
    
    if (cmd->hasCustomConfig(emulator) || (globalSettings->get<std::string>( emulator->ident + "_custom_settings", "" ) == ""))
        settings.active.fileLabel.setText( trans->get("default") );
    
    if(memoryPattern) {
        memoryPattern->setText( trans->get("memory reset initialisation") );

        memoryPattern->firstLine.valueLabel.setText( trans->getA( "value memory cell", true ) );
        memoryPattern->firstLine.invertValueEveryLabel.setText( trans->getA( "invert value every", true ) );

        memoryPattern->secondLine.valueLabel.setText( trans->getA( "second value memory cell", true ) );
        memoryPattern->secondLine.invertValueEveryLabel.setText( trans->getA( "invert second value every", true ) );

        memoryPattern->thirdLine.lengthRandomLabel.setText( trans->getA( "length random pattern", true ) );
        memoryPattern->thirdLine.repeatRandomEveryLabel.setText( trans->getA( "repeat random every", true ) );

        memoryPattern->fourthLine.randomChanceLabel.setText( trans->getA( "random chance", true ) );
        memoryPattern->fourthLine.offsetLabel.setText( trans->getA( "first byte offset", true ) );

        memoryPattern->fifthLine.preConfigured1.setText( trans->get( "pre-configured 1" ) );
        memoryPattern->fifthLine.preConfigured2.setText( trans->get( "pre-configured 2" ) );
        memoryPattern->fifthLine.preConfigured3.setText( trans->get( "pre-configured 3" ) );

        GUIKIT::HorizontalLayout::alignChildrenVertically( {&memoryPattern->firstLine, &memoryPattern->secondLine, &memoryPattern->thirdLine, &memoryPattern->fourthLine} );

        memoryPattern->updateLayout();
    }
}
