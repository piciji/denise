
SettingsLayout::~SettingsLayout() {
    for(auto& image : images) delete image;
}

AboutLayout::AboutLayout() {
    setPadding(10);
    left.append(left.author, {0u, 0u}, 2);
    left.append(left.license, {0u, 0u}, 2);
	left.append(left.version, {0u, 0u});
	right.append(right.icons8, {0u, 0u}, 2);
    right.append(right.trackersWorld, {0u, 0u});
	
	append(left, {0u, 0u}, 10);
    append(denise, {0u, 0u});
	append( *new GUIKIT::Widget, {~0u, 0u});
	append(right, {0u, 0u});
	
    setFont(GUIKIT::Font::system("bold"));
}

LangLayout::LangLayout() {
    setPadding(10);
    append(listView, {~0u, ~0u});
    setFont(GUIKIT::Font::system("bold"));
}

SwitchesLayout::SwitchesLayout() {
    setPadding(10);
	append(pause, {~0u, 0u}, 3);
    append(saveSettingsOnExit, {~0u, 0u}, 3);
    append(openFullscreen, {~0u, 0u}, 3);
    append(questionMediaWrite, {~0u, 0u}, 3);
    append(threadedEmu, {~0u, 0u}, 3);

    if (!GUIKIT::Application::isCocoa()) {
        append(splashScreen, {~0u, 0u}, 3);
        append(singleInstance, {~0u, 0u});
    } else
        append(splashScreen, {~0u, 0u});

    setFont(GUIKIT::Font::system("bold"));
}

EmuSelectionLayout::EmuSelectionLayout() {
    for(auto emu : emulators) {
        auto box = new GUIKIT::CheckBox;
        box->setText( emu->ident );

        cores.push_back( {box, emu} );
        append(*box, {0u, 0u}, 10);
    }
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
    setAlignment(0.5);
}

SettingsLayout::SettingsLayout() {
    setMargin(10);

    denise.loadPng((uint8_t*)Logos::denise, sizeof(Logos::denise));
    about.denise.setImage( &denise );
    about.denise.setUri( "https://sourceforge.net/projects/deniseemu/" );
    about.denise.setTooltip( APP_NAME );
    
    upperLayout.append(lang, {~0u, ~0u}, 10);
    upperLayout.append(switches, {~0u, 0u});
    append(upperLayout, {~0u, 0u}, 10);
    append(emuSelection, {~0u, 0u}, 10);
    append(about, {~0u, 0u});    

    for(auto& core : emuSelection.cores) {
        auto checkBox = core.checkBox;
        auto emulator = core.emulator;

        core.checkBox->onToggle = [this, checkBox, emulator](bool checked) {
            globalSettings->set<bool>("core_" + emulator->ident, checked);

            if (!checked) {
                bool atLeastOneIsChecked = false;
                EmuSelectionLayout::Core* altCore = nullptr;

                for(auto& core : emuSelection.cores) {
                    if (!altCore && (core.checkBox != checkBox))
                        altCore = &core;

                    if (core.checkBox->checked())
                        atLeastOneIsChecked = true;
                }

                if (altCore) {
                    globalSettings->set("last_used_emu", altCore->emulator->ident);

                    if (!atLeastOneIsChecked) {
                        altCore->checkBox->setChecked();
                        globalSettings->set<bool>("core_" + altCore->emulator->ident, true);
                    }

                    for(auto& core : emuSelection.cores) {
                        if ( (core.emulator == activeEmulator) && !core.checkBox->checked()) {
                            emuThread->lock();
                            program->power(altCore->emulator);
                            emuThread->unlock();
                            break;
                        }
                    }
                }
            }
            view->updateEmuUsage();
        };

        bool useCore = globalSettings->get<bool>("core_" + emulator->ident, true);
        checkBox->setChecked(useCore);
    }

    switches.saveSettingsOnExit.setChecked(globalSettings->get<bool>("save_settings_on_exit", true));
    switches.saveSettingsOnExit.onToggle = [&](bool checked) {
        globalSettings->set<bool>("save_settings_on_exit", checked);
    };
    
	switches.pause.setChecked(globalSettings->get<bool>("pause_focus_loss", false));
    switches.pause.onToggle = [&](bool checked) {
        globalSettings->set<bool>("pause_focus_loss", checked);
        program->isPause &= ~2;
        if (checked)
            program->isPause |= 2;
    };
    
    switches.openFullscreen.setChecked(globalSettings->get<bool>("open_fullscreen", false));
    switches.openFullscreen.onToggle = [&](bool checked) {
        globalSettings->set<bool>("open_fullscreen", checked);
    };

    switches.questionMediaWrite.setChecked(globalSettings->get<bool>("question_media_write", true));
    switches.questionMediaWrite.onToggle = [](bool checked) {
        globalSettings->set<bool>("question_media_write", checked);
    };

    switches.threadedEmu.setChecked(globalSettings->get<bool>("threaded_emu", false));
    switches.threadedEmu.onToggle = [](bool checked) {
        globalSettings->set<bool>("threaded_emu", checked);
        configView->driversLayout->updateDriverPropsVisibility();
        program->hintExclusiveFullscreen();
        program->initUserInterface();
    };

    switches.splashScreen.setChecked(globalSettings->get<bool>("splash_screen", true));
    switches.splashScreen.onToggle = [](bool checked) {
        globalSettings->set<bool>("splash_screen", checked);
    };

    switches.singleInstance.setChecked(globalSettings->get<bool>("single_instance", false));
    switches.singleInstance.onToggle = [](bool checked) {
        globalSettings->set<bool>("single_instance", checked);
        if (checked)
            GUIKIT::Application::closeOtherInstances();
    };

    setLang();
    
    lang.listView.onChange = [&]() {
        emuThread->lock();
        changeLang();
        emuThread->unlock();
    };
}

auto SettingsLayout::changeLang() -> void {
    if (!lang.listView.selected())
        return;

    unsigned selection = lang.listView.selection();
    
    if (selection >= langIdents.size())
        return;
    
    std::string file = langIdents[selection];

    if (file.empty())
        return;

    if ( !trans->read( program->translationFolder() + file ) )
        trans->clear();

    globalSettings->set<std::string>("translation", file);

    if (archiveViewer)
	    archiveViewer->translate();

    view->translate();

    if (configView) {
        configView->translate();
        configView->synchronizeLayout();
    }
	
	for( auto emuView : emuConfigViews ) {
        emuView->translate();
        if(emuView->inputLayout)
            emuView->inputLayout->loadDeviceList();
        emuView->synchronizeLayout();
	}
}

auto SettingsLayout::setLang() -> void {
    bool foundDefaultLang = false;
    std::string selectedLang = globalSettings->get<std::string>("translation", program->getSystemLangFile());

    auto files = GUIKIT::File::getFolderList( program->translationFolder() );

    for (auto& file : files) {
        if (GUIKIT::String::foundSubStr(file.name, ".png"))
            continue;

        langIdents.push_back( file.name );        
        lang.listView.append( { file.name } );
        addLangImage(lang.listView.rowCount() - 1, file.name );

        if (DEFAULT_TRANS_FILE == file.name)
            foundDefaultLang = true;

        if (selectedLang == file.name) {
            lang.listView.setSelection( lang.listView.rowCount() - 1 );
        }
    }

    if (!foundDefaultLang) {
        lang.listView.append( {"english - system"} );
        langIdents.push_back( "english - system" );
        
        if (!lang.listView.selected())
            lang.listView.setSelection( lang.listView.rowCount() - 1 );
    }
}

auto SettingsLayout::addLangImage(unsigned selection, std::string file) -> void {
    auto parts = GUIKIT::String::split(file, '.');
    if (parts.size() == 0) return;

    GUIKIT::File png( program->translationFolder() + parts[0] + ".png" );
    if ( !png.open() ) return;
    uint8_t* data = png.read();
    if ( !data ) return;

    auto image = new GUIKIT::Image;
    if ( !image->loadPng(data, png.getSize()) ) {
        delete image;
        return;
    }

    images.push_back( image );

    lang.listView.setImage(selection, 0, *image);
}

auto SettingsLayout::activateCore(Emulator::Interface* emulator) -> void {
    for(auto& core : emuSelection.cores) {
        if (core.emulator == emulator) {
            core.checkBox->setChecked();
            break;
        }
    }
}

auto SettingsLayout::translate() -> void {
    lang.setText( trans->get("language") );
    
    for(unsigned i = 0; i < lang.listView.rowCount(); i++) {

        std::string _displayString = langIdents[i];
        GUIKIT::String::replace(_displayString, ".txt", "");
        lang.listView.setText( i, 0, trans->get( _displayString ) );
    }

    switches.setText( trans->get("settings") );

	switches.pause.setText(trans->get("pause_focus_loss"));
    switches.saveSettingsOnExit.setText(trans->get("save_changes_on_exit"));
    switches.saveSettingsOnExit.setTooltip(trans->get("save changes on exit tooltip"));
    switches.openFullscreen.setText(trans->get("open_fullscreen"));
    switches.questionMediaWrite.setText(trans->get("confirm writes"));
    switches.threadedEmu.setText(trans->get("Threaded Emulation"));
    switches.threadedEmu.setTooltip(trans->get("Threaded Emulation tooltip"));
    switches.splashScreen.setText(trans->get("Splash Screen"));
    switches.singleInstance.setText(trans->get("Single Instance"));

    about.left.license.setText( trans->get("license", {}, true) + " " + LICENSE );
    about.left.author.setText( trans->get("author", {}, true) + " " + AUTHOR );
	about.left.version.setText( trans->get("Version", {}, true) + " " + VERSION );
    about.setText( trans->get("about", {{"%app%", APP_NAME}}) );
	
    auto link = trans->get("go_to_website");
    
	about.right.icons8.setText("Icons8: " + link);
	about.right.icons8.setUri("https://icons8.com", link);
	about.right.icons8.setTooltip("https://icons8.com");

    about.right.trackersWorld.setText("Trackers-World.NET: " + link);
    about.right.trackersWorld.setUri("https://www.twdotnet.de/wp/2016/11/c64-floppy-sounds/", link);
    about.right.trackersWorld.setTooltip("Trackers-World.NET");

    emuSelection.setText( trans->get("Core Selection") );
}
