
SettingsLayout::~SettingsLayout() {
    for(auto& image : images) delete image;
}

AboutLayout::AboutLayout() {
    setPadding(10);
    left.append(left.author, {0u, 0u}, 2);
    left.append(left.license, {0u, 0u}, 2);
	left.append(left.version, {0u, 0u});
	right.append(right.icons8, {0u, 0u});
	
	append(left, {0u, 0u});
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
    append(fullscreenStatusbar, {~0u, 0u}, 3);
    append(aspectCorrect, {~0u, 0u}, 3);
	append(pause, {~0u, 0u}, 3);
    append(autostartDragnDrop, {~0u, 0u}, 3);
    append(saveSettingsOnExit, {~0u, 0u});
    setFont(GUIKIT::Font::system("bold"));
}

SettingsLayout::SettingsLayout() {
    setMargin(10);

    switches.synchronizeLayout();
    
    upperLayout.append(lang, {~0u, ~0u}, 10);
    upperLayout.append(switches, {~0u, 0u});
    append(upperLayout, {~0u, 0u}, 10);    
    append(about, {~0u, 0u});    

    switches.fullscreenStatusbar.setChecked( settings->get<bool>("statusbar_fullscreen", false) );
    switches.fullscreenStatusbar.onToggle = [&]() {
		if (view->fullScreen()) {
			view->setStatusVisible( switches.fullscreenStatusbar.checked() );
            view->updateViewport();
		}
        settings->set<bool>("statusbar_fullscreen", switches.fullscreenStatusbar.checked());
    };

    switches.aspectCorrect.setChecked(settings->get<bool>("aspect_correct", false));
    switches.aspectCorrect.onToggle = [&]() {
        settings->set<bool>("aspect_correct", switches.aspectCorrect.checked());
        view->updateViewport();
    };
	
	switches.autostartDragnDrop.setChecked(settings->get<bool>("autostart_dragndrop", false));
    switches.autostartDragnDrop.onToggle = [&]() {
        settings->set<bool>("autostart_dragndrop", switches.autostartDragnDrop.checked());
    };
    
    switches.saveSettingsOnExit.setChecked(settings->get<bool>("save_settings_on_exit", true));
    switches.saveSettingsOnExit.onToggle = [&]() {
        settings->set<bool>("save_settings_on_exit", switches.saveSettingsOnExit.checked());
		
		if (!switches.saveSettingsOnExit.checked())
			program->rememberNotToSaveSettings();
    };
    
	switches.pause.setChecked(settings->get<bool>("pause_focus_loss", false));
    switches.pause.onToggle = [&]() {
        settings->set<bool>("pause_focus_loss", switches.pause.checked());
    };
    

    setLang();
    
    lang.listView.onChange = [&]() {
        changeLang();
    };
}

auto SettingsLayout::changeLang() -> void {
    if (!lang.listView.selected()) return;

    unsigned selection = lang.listView.selection();
    std::string file = lang.listView.text(selection, 0);

    if (file.empty()) return;

    if ( !trans->read( program->translationFolder() + file ) )
        trans->clear();

    settings->set<std::string>("translation", file);

	archiveViewer->translate();
    view->translate();
    
    configView->translate();
	for( auto emuConfigView : emuConfigViews )
		emuConfigView->translate();	
	
	configView->inputLayout->loadInputList();	
	for( auto emuConfigView : emuConfigViews )
		emuConfigView->inputLayout->loadDeviceList();
	
    configView->synchronizeLayout();	
	for( auto emuConfigView : emuConfigViews )
		emuConfigView->synchronizeLayout();
}

auto SettingsLayout::setLang() -> void {
    bool foundDefaultLang = false;
    std::string selectedLang = settings->get<std::string>("translation", program->getSystemLangFile());

    auto files = GUIKIT::File::getFolderList( program->translationFolder() );

    for (auto& file : files) {
        if (GUIKIT::String::foundSubStr(file.name, ".png")) continue;

        lang.listView.append( { file.name } );
        addLangImage(lang.listView.rowCount() - 1, file.name );

        if (DEFAULT_TRANS_FILE == file.name) foundDefaultLang = true;

        if (selectedLang == file.name) {
            lang.listView.setSelection( lang.listView.rowCount() - 1 );
        }
    }

    if (!foundDefaultLang) {
        lang.listView.append( {"english - system"} );
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

auto SettingsLayout::translate() -> void {
    lang.setText( trans->get("language") );
    
    switches.setText( trans->get("settings") );

    switches.fullscreenStatusbar.setText( trans->get("statusbar_fullscreen") );
    switches.aspectCorrect.setText(trans->get("aspect_ratio"));
	switches.pause.setText(trans->get("pause_focus_loss"));
    switches.autostartDragnDrop.setText(trans->get("autostart_dragndrop"));
    switches.saveSettingsOnExit.setText(trans->get("save_changes_on_exit"));

    about.left.license.setText( trans->get("license", {}, true) + " " + LICENSE );
    about.left.author.setText( trans->get("author", {}, true) + " " + AUTHOR );
	about.left.version.setText( trans->get("Version", {}, true) + " " + VERSION );
    about.setText( trans->get("about", {{"%app%", APP_NAME}}) );
	
    auto link = trans->get("go_to_website");
    
	about.right.icons8.setText("Icons8: " + link);
	about.right.icons8.setUri("http://www.icons8.com", link);
	about.right.icons8.setTooltip("http://www.icons8.com");
}
