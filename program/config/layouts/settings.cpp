
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
    append(autostartDragnDrop, {~0u, 0u}, 3);
    append(saveSettingsOnExit, {~0u, 0u}, 3);
    append(openFullscreen, {~0u, 0u}, 3);
    append(alternateSoftwarePreview, {~0u, 0u}, 3);
    append(questionMediaWrite, {~0u, 0u}, 3);
    append(threadedEmu, {~0u, 0u});
    setFont(GUIKIT::Font::system("bold"));
    threadedEmu.setForegroundColor( 0xff4500 );
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

PreviewLayout::PreviewLayout() {
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
    
    previewBox.setHeaderText( { "" } );
    previewBox.setHeaderVisible( false );
    
    append( top, {0u, 0u}, 5 );    
    append( bottom, {~0u, 0u} );    
}

PreviewLayout::Top::Top() {
    append(fontSize,{0u, 0u}, 5);
    append(fontSizeCombo,{0u, 0u}, 5);
    append(dialogFontSize,{0u, 0u}, 5);
    append(dialogFontSizeCombo,{0u, 0u}, 20);
    append(option,{0u, 0u});

    setAlignment(0.5);    
}

PreviewLayout::Top::Option::Option() {
    append(tooltips,{0u, 0u}, 2);
    append(commodoreHighlight,{0u, 0u});
}

PreviewLayout::Bottom::Bottom() :
dialogWidth("px"),
dialogHeight("px")
{
    append(dialog,{0u, 0u}, 5);
    append(dialogWidth,{~0u, 0u}, 10);
    
    if (GUIKIT::Application::isCocoa())
        append(dialogHeight,{~0u, 0u});
        
    dialogWidth.updateValueWidth("600 px", 5);
    dialogHeight.updateValueWidth("600 px", 5);
    
    dialogWidth.slider.setLength(401);
    dialogWidth.slider.setLength(401);
	dialogHeight.slider.setLength(501);
    
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
    append(previewLayout, {~0u, 0u}, 10);
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

                if (!atLeastOneIsChecked && altCore) {
                    altCore->checkBox->setChecked();
                    globalSettings->set<bool>("core_" + altCore->emulator->ident, true);
                }
            }
            view->updateEmuUsage();
        };

        bool useCore = globalSettings->get<bool>("core_" + emulator->ident, true);
        checkBox->setChecked(useCore);
    }

	switches.autostartDragnDrop.setChecked(globalSettings->get<bool>("autostart_dragndrop", false));
    switches.autostartDragnDrop.onToggle = [&](bool checked) {
        globalSettings->set<bool>("autostart_dragndrop", checked);
    };
    
    switches.saveSettingsOnExit.setChecked(globalSettings->get<bool>("save_settings_on_exit", true));
    switches.saveSettingsOnExit.onToggle = [&](bool checked) {
        globalSettings->set<bool>("save_settings_on_exit", checked);
    };
    
	switches.pause.setChecked(globalSettings->get<bool>("pause_focus_loss", false));
    switches.pause.onToggle = [&](bool checked) {
        globalSettings->set<bool>("pause_focus_loss", checked);
    };
    
    switches.openFullscreen.setChecked(globalSettings->get<bool>("open_fullscreen", false));
    switches.openFullscreen.onToggle = [&](bool checked) {
        globalSettings->set<bool>("open_fullscreen", checked);
    };
    
    switches.alternateSoftwarePreview.setChecked(globalSettings->get<bool>("alternate_software_preview", false));
    switches.alternateSoftwarePreview.onToggle = [&](bool checked) {
        globalSettings->set<bool>("alternate_software_preview", checked);
    };

    switches.questionMediaWrite.setChecked(globalSettings->get<bool>("question_media_write", true));
    switches.questionMediaWrite.onToggle = [](bool checked) {
        globalSettings->set<bool>("question_media_write", checked);
    };

    switches.threadedEmu.setChecked(globalSettings->get<bool>("threaded_emu", false));
    switches.threadedEmu.onToggle = [](bool checked) {
        globalSettings->set<bool>("threaded_emu", checked);
        configView->videoLayout->updateDriverPropsVisibility();
        program->hintExclusiveFullscreen();
        program->initUserInterface();
    };

    setLang();
    
    lang.listView.onChange = [&]() {
        emuThread->lock();
        changeLang();
        emuThread->unlock();
    };
    
    for(unsigned i = 6; i <= 14; i++) {
        previewLayout.top.fontSizeCombo.append(std::to_string(i), i);
        previewLayout.top.dialogFontSizeCombo.append(std::to_string(i), i);
    }
    
    previewLayout.top.fontSizeCombo.onChange = [this]() {
        
        globalSettings->set<unsigned>("software_preview_fontsize", previewLayout.top.fontSizeCombo.userData());
        
        for( auto emuView : emuConfigViews ) {
            if (emuView->mediaLayout)
                emuView->mediaLayout->updateListingFont(previewLayout.top.fontSizeCombo.userData());
        }
    };
    
    previewLayout.top.fontSizeCombo.setSelection( globalSettings->get<unsigned>("software_preview_fontsize", 12, {6, 14}) - 6 );
    
    
    previewLayout.top.dialogFontSizeCombo.onChange = [this]() {
        
        globalSettings->set<unsigned>("dialog_software_preview_fontsize", previewLayout.top.dialogFontSizeCombo.userData());
        
        previewTimer.setEnabled(true);
    };
    
    previewLayout.top.dialogFontSizeCombo.setSelection( globalSettings->get<unsigned>("dialog_software_preview_fontsize", 11, {6, 14}) - 6 );
    
    previewLayout.bottom.dialogWidth.slider.onChange = [this](unsigned position) {

        previewLayout.bottom.dialogWidth.value.setText( std::to_string( position + 200 ) + " px" );
        
        globalSettings->set<unsigned>("dialog_software_preview_width", position + 200 );
        
        previewTimer.setEnabled(true);
    };
    
    previewLayout.bottom.dialogWidth.slider.setPosition( globalSettings->get<unsigned>("dialog_software_preview_width", 450, {200, 600}) - 200 );
    
    previewLayout.bottom.dialogWidth.value.setText( std::to_string( previewLayout.bottom.dialogWidth.slider.position() + 200 ) + " px" );            
    
    
    previewLayout.bottom.dialogHeight.slider.onChange = [this](unsigned position) {

        previewLayout.bottom.dialogHeight.value.setText( std::to_string( position + 100 ) + " px" );
        
        globalSettings->set<unsigned>("dialog_software_preview_height", position + 100 );
    };
    
    previewLayout.bottom.dialogHeight.slider.setPosition( globalSettings->get<unsigned>("dialog_software_preview_height", 200, {100, 600}) - 100 );
    
    previewLayout.bottom.dialogHeight.value.setText( std::to_string( previewLayout.bottom.dialogHeight.slider.position() + 100 ) + " px" );
         
    previewLayout.top.option.tooltips.onToggle = [this](bool checked) {
        globalSettings->set<bool>("software_preview_tooltips", checked );
        
        for( auto emuView : emuConfigViews ) {
            if(emuView->mediaLayout)
                emuView->mediaLayout->updateListings();
        }
        
        previewLayout.previewBox.reset();
        
        previewTimer.setEnabled(true);
    };
    
    previewLayout.top.option.tooltips.setChecked( globalSettings->get<bool>("software_preview_tooltips", true ) );

    previewLayout.top.option.commodoreHighlight.onToggle = [](bool checked) {
        globalSettings->set<bool>("software_preview_commodore_hi", checked );

        for( auto emuView : emuConfigViews ) {
            if(emuView->mediaLayout)
                emuView->mediaLayout->selectionColorListing();
        }
    };

    previewLayout.top.option.commodoreHighlight.setChecked( globalSettings->get<bool>("software_preview_commodore_hi", true ) );
    
    previewLayout.previewBox.setBackgroundColor( 0xaaaaaa );
    
    previewTimer.setInterval( 100 );
    
    previewTimer.onFinished = [this]() {
        previewTimer.setEnabled(false);
        
        setPreviewContent();
        
        unsigned newWidth = globalSettings->get<unsigned>("dialog_software_preview_width", 450, {200, 600});

        if (previewLayout.has(previewLayout.previewBox))
            previewLayout.update( previewLayout.previewBox, {newWidth, 60u} );
        else {
            previewLayout.update( previewLayout.bottom, 10 );
            previewLayout.append( previewLayout.previewBox, {newWidth, 60u} );
        }

        synchronizeLayout();
    };    
}

auto SettingsLayout::removePreview() -> void {
    
    if (previewLayout.remove( previewLayout.previewBox )) { 
        previewLayout.update( previewLayout.bottom, 0 );
        synchronizeLayout();
    }
}

auto SettingsLayout::setPreviewContent() -> void {
    
    bool useCustomFont = GUIKIT::Window::countCustomFonts() > 0;

    auto fontSize = globalSettings->get<unsigned>("dialog_software_preview_fontsize", 11, {6, 14});
    
    if (useCustomFont)
        previewLayout.previewBox.setFont("C64 Pro, " + std::to_string(fontSize), true);  
    else
        previewLayout.previewBox.setFont( GUIKIT::Font::system(fontSize) );          
    
    if (previewLayout.previewBox.rowCount())
        return;
    
    bool useTooltips = globalSettings->get<bool>("software_preview_tooltips", true );
    
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
        configView->inputLayout->loadInputList();
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

auto SettingsLayout::translate() -> void {
    lang.setText( trans->get("language") );
    
    for(unsigned i = 0; i < lang.listView.rowCount(); i++) {

        std::string _displayString = langIdents[i];
        GUIKIT::String::replace(_displayString, ".txt", "");
        lang.listView.setText( i, 0, trans->get( _displayString ) );
    }

    switches.setText( trans->get("settings") );

	switches.pause.setText(trans->get("pause_focus_loss"));
    switches.autostartDragnDrop.setText(trans->get("autostart_dragndrop"));
    switches.saveSettingsOnExit.setText(trans->get("save_changes_on_exit"));
    switches.saveSettingsOnExit.setTooltip(trans->get("save changes on exit tooltip"));
    switches.openFullscreen.setText(trans->get("open_fullscreen"));
    switches.alternateSoftwarePreview.setText(trans->get("alternate software preview"));
    switches.questionMediaWrite.setText(trans->get("confirm writes"));
    switches.threadedEmu.setText(trans->get("Threaded Emulation"));
    switches.threadedEmu.setTooltip(trans->get("Threaded Emulation tooltip"));

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
    
    previewLayout.setText( trans->get("Software Preview") );
    previewLayout.top.fontSize.setText( trans->get("Font Size", {}, true) );
    previewLayout.top.dialogFontSize.setText( trans->get("Dialog Font Size", {}, true) );    
    previewLayout.top.option.tooltips.setText( trans->get("Show Tooltips") );
    previewLayout.top.option.commodoreHighlight.setText( trans->get("Commodore Highlight Color" ) );
    
    previewLayout.bottom.dialog.setText( trans->get("Dialog Preview")  );
    previewLayout.bottom.dialogWidth.name.setText( trans->get("Width", {}, true) );
    previewLayout.bottom.dialogHeight.name.setText( trans->get("Height", {}, true) );

    emuSelection.setText( trans->get("Core Selection") );
}

