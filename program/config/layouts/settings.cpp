
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
	append(pause, {~0u, 0u}, 3);
    append(autostartDragnDrop, {~0u, 0u}, 3);
    append(saveSettingsOnExit, {~0u, 0u}, 3);
    append(openFullscreen, {~0u, 0u}, 3);
    append(alternateSoftwarePreview, {~0u, 0u}, 3);
    append(questionMediaWrite, {~0u, 0u});
    setFont(GUIKIT::Font::system("bold"));
}

PreviewLayout::PreviewLayout() {
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
    
    previewBox.setHeaderText( { "" } );
    previewBox.setHeaderVisible( false );
    
    append( control, {~0u, 0u}, 3 );    
    append( tooltips, {0u, 0u} );    
}

PreviewLayout::Control::Control() {
    GUIKIT::Label test;
    test.setText("600 px");

    append(fontSize,{0u, 0u}, 5);
    append(fontSizeCombo,{0u, 0u}, 5);
    append(dialogFontSize,{0u, 0u}, 5);
    append(dialogFontSizeCombo,{0u, 0u}, 5);
    append(dialogPreviewWidth,{0u, 0u}, 5);
    append(dialogPreviewWidthValue,{test.minimumSize().width + 3, 0u}, 5);
    append(dialogPreviewWidthSlider,{~0u, 0u});

    dialogPreviewWidthSlider.setLength(501);

    setAlignment(0.5);    
}

SettingsLayout::SettingsLayout() {
    setMargin(10);
    
    upperLayout.append(lang, {~0u, ~0u}, 10);
    upperLayout.append(switches, {~0u, 0u});
    append(upperLayout, {~0u, 0u}, 10);  
    append(previewLayout, {~0u, 0u}, 10);
    append(about, {~0u, 0u});    

    switches.fullscreenStatusbar.setChecked( settings->get<bool>("statusbar_fullscreen", false) );
    switches.fullscreenStatusbar.onToggle = [&]() {
		if (view->fullScreen()) {
			view->setStatusVisible( switches.fullscreenStatusbar.checked() );
            view->updateViewport();
		}
        settings->set<bool>("statusbar_fullscreen", switches.fullscreenStatusbar.checked());
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
    
    switches.openFullscreen.setChecked(settings->get<bool>("open_fullscreen", false));
    switches.openFullscreen.onToggle = [&]() {
        settings->set<bool>("open_fullscreen", switches.openFullscreen.checked());
    };
    
    switches.alternateSoftwarePreview.setChecked(settings->get<bool>("alternate_software_preview", false));
    switches.alternateSoftwarePreview.onToggle = [&]() {
        settings->set<bool>("alternate_software_preview", switches.alternateSoftwarePreview.checked());
    };

    switches.questionMediaWrite.setChecked(settings->get<bool>("question_media_write", true));
    switches.questionMediaWrite.onToggle = [this]() {
        settings->set<bool>("question_media_write", switches.questionMediaWrite.checked());
    };

    setLang();
    
    lang.listView.onChange = [&]() {
        changeLang();
    };
    
    for(unsigned i = 6; i <= 14; i++) {
        previewLayout.control.fontSizeCombo.append(std::to_string(i), i);
        previewLayout.control.dialogFontSizeCombo.append(std::to_string(i), i);
    }
    
    previewLayout.control.fontSizeCombo.onChange = [this]() {
        
        settings->set<unsigned>("software_preview_fontsize", previewLayout.control.fontSizeCombo.userData());
        
        for( auto mediaView : mediaViews )
            mediaView->updateListingFont( previewLayout.control.fontSizeCombo.userData() );
    };
    
    previewLayout.control.fontSizeCombo.setSelection( settings->get<unsigned>("software_preview_fontsize", 12, {6, 14}) - 6 );
    
    
    previewLayout.control.dialogFontSizeCombo.onChange = [this]() {
        
        settings->set<unsigned>("dialog_software_preview_fontsize", previewLayout.control.dialogFontSizeCombo.userData());
        
        previewTimer.setEnabled(true);
    };
    
    previewLayout.control.dialogFontSizeCombo.setSelection( settings->get<unsigned>("dialog_software_preview_fontsize", 11, {6, 14}) - 6 );
    
    previewLayout.control.dialogPreviewWidthSlider.onChange = [this]() {
        
        unsigned pos = previewLayout.control.dialogPreviewWidthSlider.position();
        
        previewLayout.control.dialogPreviewWidthValue.setText( std::to_string( pos + 200 ) + " px" );                
        
        settings->set<unsigned>("dialog_software_preview_width", pos + 200 );
        
        previewTimer.setEnabled(true);
    };
    
    previewLayout.control.dialogPreviewWidthSlider.setPosition( settings->get<unsigned>("dialog_software_preview_width", 445, {200, 700}) - 200 );
    
    previewLayout.control.dialogPreviewWidthValue.setText( std::to_string( previewLayout.control.dialogPreviewWidthSlider.position() + 200 ) + " px" );
        
    previewLayout.tooltips.onToggle = [this]() {
        bool state = previewLayout.tooltips.checked();
        
        settings->set<bool>("software_preview_tooltips", state );
        
        for( auto mediaView : mediaViews )
            mediaView->updateListings();
        
        previewLayout.previewBox.reset();
        
        previewTimer.setEnabled(true);
    };
    
    previewLayout.tooltips.setChecked( settings->get<bool>("software_preview_tooltips", true ) );
    
    previewLayout.previewBox.setBackgroundColor( 0xaaaaaa );
    
    previewTimer.setInterval( 100 );
    
    previewTimer.onFinished = [this]() {
        previewTimer.setEnabled(false);
        
        setPreviewContent();
        
        unsigned newWidth = settings->get<unsigned>("dialog_software_preview_width", 445, {200, 700});

        if (previewLayout.has(previewLayout.previewBox))
            previewLayout.update( previewLayout.previewBox, {newWidth, 60u} );
        else {
            previewLayout.update( previewLayout.tooltips, 10 );
            previewLayout.append( previewLayout.previewBox, {newWidth, 60u} );
        }

        synchronizeLayout();
    };    
}

auto SettingsLayout::removePreview() -> void {
    
    if (previewLayout.remove( previewLayout.previewBox )) { 
        previewLayout.update( previewLayout.tooltips, 0 );
        synchronizeLayout();
    }
}

auto SettingsLayout::setPreviewContent() -> void {
    
    bool useCustomFont = false;
    
    for (auto& mediaView : mediaViews) {
        if (mediaView->useCustomFont) {
            useCustomFont = true;
            break;
        }
    }

    auto fontSize = settings->get<unsigned>("dialog_software_preview_fontsize", 11, {6, 14});
    
    if (useCustomFont)
        previewLayout.previewBox.setFont("C64 Pro, " + std::to_string(fontSize), true);  
    else
        previewLayout.previewBox.setFont( GUIKIT::Font::system(fontSize) );          
    
    if (previewLayout.previewBox.rowCount())
        return;
    
    bool useTooltips = settings->get<bool>("software_preview_tooltips", true );
    
    std::vector<uint8_t> line = {0x30, 0x20, 0x20, 0x20, 0x20, 0x22, 0x20, 0x44, 0x45, 0x4e, 0x49, 0x53, 0x45, 0x20, 0x20, 0x44, 0x45, 0x4e, 0x49, 0x53, 0x45, 0x20, 0x22, 0x20, 0x50, 0x52, 0x47, 0x3c};
    std::vector<uint8_t> tooltipLine = { 0x4c, 0x4f, 0x41, 0x44, 0x20, 0x22, 0x44, 0x45, 0x4e, 0x49, 0x53, 0x45, 0x22, 0x2c, 0x38, 0x2c, 0x31 };
    
    if(useCustomFont) {
        line = {0x30, 0x20, 0x20, 0x20, 0x20, 0x22, 0x20, 4, 5, 0xe, 9, 0x13, 5, 0x20, 0x20, 4, 5, 0xe, 9, 0x13, 5, 0x20, 0x22, 0x20, 0x10, 0x12, 7, 0x3c};
        tooltipLine = { 0x0c, 0x0f, 0x01, 0x04, 0x20, 0x22, 0x4, 0x5, 0xe, 0x9, 0x13, 0x5, 0x22, 0x2c, 0x38, 0x2c, 0x31 };
    }
    
    std::vector<uint8_t> utf8;
    
    for (auto& code : line) {

        unsigned useCode = code;
        if (useCustomFont)
            useCode |= 0xee << 8;

        GUIKIT::Utf8::encode(useCode, utf8);
    }
        
    std::string out = std::string((const char*) utf8.data(), utf8.size());
    std::string outTooltip  = "";
    
    if (useTooltips) {
        utf8.clear();

        for (auto& code : tooltipLine) {

            unsigned useCode = code;
            if (useCustomFont)
                useCode |= 0xee << 8;

            GUIKIT::Utf8::encode(useCode, utf8);
        }

        outTooltip = std::string((const char*) utf8.data(), utf8.size());
    }
    
    for (unsigned i = 0; i < 8; i++) {
        
        previewLayout.previewBox.append( {out} );        
        if (useTooltips)
            previewLayout.previewBox.setRowTooltip(i, outTooltip );
    }
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
	
	for( auto mediaView : mediaViews )
		mediaView->translate();	
	
	for( auto emuConfigView : emuConfigViews )
		emuConfigView->translate();	
	
	configView->inputLayout->loadInputList();	
	for( auto emuConfigView : emuConfigViews )
		emuConfigView->inputLayout->loadDeviceList();
	
	for( auto mediaView : mediaViews )
		mediaView->synchronizeLayout();	
	
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
	switches.pause.setText(trans->get("pause_focus_loss"));
    switches.autostartDragnDrop.setText(trans->get("autostart_dragndrop"));
    switches.saveSettingsOnExit.setText(trans->get("save_changes_on_exit"));
    switches.openFullscreen.setText(trans->get("open_fullscreen"));
    switches.alternateSoftwarePreview.setText(trans->get("alternate software preview"));
    switches.questionMediaWrite.setText(trans->get("confirm writes"));

    about.left.license.setText( trans->get("license", {}, true) + " " + LICENSE );
    about.left.author.setText( trans->get("author", {}, true) + " " + AUTHOR );
	about.left.version.setText( trans->get("Version", {}, true) + " " + VERSION );
    about.setText( trans->get("about", {{"%app%", APP_NAME}}) );
	
    auto link = trans->get("go_to_website");
    
	about.right.icons8.setText("Icons8: " + link);
	about.right.icons8.setUri("http://www.icons8.com", link);
	about.right.icons8.setTooltip("http://www.icons8.com");
    
    previewLayout.setText( trans->get("Software Preview") );
    previewLayout.control.fontSize.setText( trans->get("Font Size", {}, true) );
    previewLayout.control.dialogFontSize.setText( trans->get("Dialog Font Size", {}, true) );
    previewLayout.control.dialogPreviewWidth.setText( trans->get("Dialog Preview Width", {}, true) );
    previewLayout.tooltips.setText( trans->get("Show Tooltips") );
}

