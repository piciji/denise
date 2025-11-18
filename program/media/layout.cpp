
DialogPreviewLayout::DialogPreviewLayout() {
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));

    previewBox.setHeaderText( { "" } );
    previewBox.setHeaderVisible( false );
    previewBox.colorRowTooltips( true );

    append( mode, {0u, 0u}, 10 );
    append( control, {0u, 0u}, 10 );
    append( dimension, {~0u, 0u}, 10 );
}

DialogPreviewLayout::Mode::Mode() {
    append(label,{0u, 0u}, 10);
    append(noPreviewRadio,{0u, 0u}, 10);
    append(dialogPreviewRadio,{0u, 0u}, 10);
    append(softwarePreviewRadio,{0u, 0u});
    GUIKIT::RadioBox::setGroup(noPreviewRadio, dialogPreviewRadio, softwarePreviewRadio);

    setAlignment(0.5);
}

DialogPreviewLayout::Control::Control() {
    append(fontSize,{0u, 0u}, 10);
    append(fontSizeCombo,{0u, 0u}, 20);
    append(option,{0u, 0u});

    for(unsigned i = 8; i <= 16; i++)
        fontSizeCombo.append(std::to_string(i), i);

    setAlignment(0.5);
}

DialogPreviewLayout::Control::Option::Option() {
    append(tooltips,{0u, 0u}, 2);
    append(commodoreHighlight,{0u, 0u});
}

DialogPreviewLayout::Dimension::Dimension() :
dialogWidth("px"),
dialogHeight("px")
{
    append(dialogWidth,{~0u, 0u}, 10);

    if (GUIKIT::Application::isCocoa())
        append(dialogHeight,{~0u, 0u});

    dialogWidth.updateValueWidth("600 px", 5);
    dialogHeight.updateValueWidth("600 px", 5);

    dialogWidth.slider.setLength(401);
    dialogHeight.slider.setLength(351);

    setAlignment(0.5);
}

PathsLayout::Block::Block(Emulator::Interface::MediaGroup* mediaGroup) {
    this->mediaGroup = mediaGroup;
        
    edit.setEditable(false);
    append(label, {90, 0u}, 10);
    append(edit, {~0u, 0u}, 10);
    append(empty, {0u, 0u}, 10);
    append(select, {0u, 0u});
    setAlignment(0.5);
    label.setFont(GUIKIT::Font::system("bold"));    
}

PathsLayout::PathsLayout() {            
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
}

MediaGroupLayout::Block::Header::Header(Emulator::Interface::Media* media, Emulator::Interface* emulator) {
    auto group = media->group;
    bool IPMode = group->isExpansion() && group->expansion->isRS232();
    bool staticField = (media->id == 0) && (media->group->id == LIBC64::Interface::MediaGroupIdExpansionSuperCpu) && dynamic_cast<LIBC64::Interface*>(emulator);

    deviceName.setFont(GUIKIT::Font::system("bold"));
    inUse.setFont(GUIKIT::Font::system("bold"));
    if (!media->secondary && group->selected)
        append(inUse, {0u, 0u}, 5);
    else
        append(deviceName, {0u, 0u}, 10);

    if (group->isDrive() && group->media.size() > 1) {
        list = new GUIKIT::Button;
        append(*list, {0u, 0u}, 10);
    }
        
    if (group->isWritable()) {
        if ( (media->id < 4) && dynamic_cast<LIBC64::Interface*>(emulator) && (media->group->id == LIBC64::Interface::MediaGroupIdExpansionFinalChessCard));
        else
            append(writeprotect, {0u, 0u}, 10);
    }

    if (!IPMode) {
        if (!staticField)
            append(eject, {0u, 0u}, 10);
        append(fileName, {~0u, 0u});
    }
    setAlignment(0.5);
}

MediaGroupLayout::Block::Selector::Selector(Emulator::Interface::Media* media, Emulator::Interface* emulator) {
    auto group = media->group;
    bool IPMode = group->isExpansion() && group->expansion->isRS232();
    bool staticField = (media->id == 0) && (media->group->id == LIBC64::Interface::MediaGroupIdExpansionSuperCpu) && dynamic_cast<LIBC64::Interface*>(emulator);

    if (IPMode || staticField) {
        edit = new GUIKIT::LineEdit;
        pathCombo = nullptr;
        append(*edit, { ~0u, 0u }, 10);
    } else {
        edit = nullptr;
        pathCombo = new GUIKIT::ComboButton(true);
        append(*pathCombo, { ~0u, 0u }, 10);
    }    

    if (media->pcbLayout && group->expansion && !media->secondary && (group->expansion->pcbs.size() > 0)) {
        for (auto& pcb : group->expansion->pcbs) {
            if (!group->isHardDisk() || (media->id == 0) || (pcb.id < 2))
                combo.append( pcb.name, pcb.id );

            if (media->pcbLayout == &pcb)
                combo.setSelection( combo.rows() - 1 );
        }
        
        append(combo, {0u, 0u}, 10);
    }
                  
    if (group->expansion && !media->secondary && (group->expansion->jumpers.size() > 0) ) {
      //  append(jumperLabel, {0u, 0u}, 5 );
        
        for(auto& jumper : group->expansion->jumpers) {
            auto jumpChecker = new GUIKIT::CheckBox;
            jumpChecker->setText( jumper.name );

            append(*jumpChecker, {0u, 0u}, 5);

            jumpers.push_back( jumpChecker );
        }        
    }

    if (!IPMode) {
        append(open, {0u, 0u});
        if (staticField)
            open.setEnabled(false);
    }
    
    setAlignment(0.5);

    if (edit) {
        edit->setEditable(IPMode);
        edit->setDroppable(false);
        if (staticField)
            edit->setEnabled(false);
    } else if (pathCombo)
        pathCombo->setDroppable();
}

MediaGroupLayout::Block::Block(Emulator::Interface::Media* media, Emulator::Interface* emulator) : media(media), header(media, emulator), selector(media, emulator) {
    append(header, {~0u, 0u}, 2);
    append(selector, {~0u, 0u});
    dirty = true;
}

MediaGroupLayout::MediaGroupLayout( Emulator::Interface::MediaGroup* mediaGroup, MediaLayout* mediaLayout ) {
    this->mediaGroup = mediaGroup;
    this->mediaLayout = mediaLayout;
    
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
}

DiskCreatorLayout::Options::Options() {
    append(fastFileSystem, {0u, 0u}, 2);
    append(highDensity, {0u, 0u}, 2);
    append(bootable, {0u, 0u});
}

DiskCreatorLayout::DiskCreatorLayout( Emulator::Interface* emulator, Emulator::Interface::MediaGroup* mediaGroup ) {

    unsigned formatId = 0;
    for ( auto& creatable : mediaGroup->creatable )
        format.append( creatable, formatId++ );    
        
    append(formatName, {0u, 0u}, 10);        
    
    if (format.rows() == 1)
        format.setEnabled(false);
    
    append(format, {0u, 0u}, 10);
    
    if (dynamic_cast<LIBAMI::Interface*>(emulator)) {
        append(options, {0u, 0u}, 10);
    }
    
    append(diskLabelName, {0u, 0u}, 5);
    append(diskLabel, {~0u, 0u}, 10);

    append(insertLabel, {0u, 0u}, 5);
    for( auto& media : mediaGroup->media ) {
        insertDevice.append( media.name, media.id );
    }
    insertDevice.append( "-", -1 );

    append(insertDevice, {0u, 0u}, 10);

    append(button, {0u, 0u});
    setFont(GUIKIT::Font::system("bold"));
    setPadding(10);
    setAlignment(0.5);       
}

TapeCreatorLayout::TapeCreatorLayout(Emulator::Interface::MediaGroup* mediaGroup) {

    append(insertLabel, {0u, 0u}, 5);
    for( auto& media : mediaGroup->media ) {
        insertDevice.append( media.name, media.id );
    }
    insertDevice.append( "-", -1 );
    append(insertDevice, {0u, 0u}, 10);
    append(button, {0u, 0u});
    setFont(GUIKIT::Font::system("bold"));
    setPadding(10);
    setAlignment(0.5);
}

MemoryCreatorLayout::MemoryCreatorLayout() {
	append(button, {0u, 0u});
	setFont(GUIKIT::Font::system("bold"));
    setPadding(10);
    setAlignment(0.5);
}

FlashCreatorLayout::FlashCreatorLayout() {
    append(format, {0u, 0u}, 10);
	append(button, {0u, 0u});
	setFont(GUIKIT::Font::system("bold"));
    setPadding(10);
    setAlignment(0.5);
}

HdCreatorLayout::Creator::Creator(Emulator::Interface::MediaGroup* mediaGroup) {
    GUIKIT::LineEdit test;
    test.setText("9999");
    
    unsigned formatId = 0;
    for (auto& creatable : mediaGroup->creatable)
        format.append(creatable, formatId++);

    append(formatName, { 0u, 0u }, 10);

    if (format.rows() == 1)
        format.setEnabled(false);

    append(format, { 0u, 0u }, 10);

    append(diskSizeLabel, {0u, 0u}, 10);
    append(diskSize, {test.minimumSize().width, 0u}, 10);
    append(button, {0u, 0u});

    setAlignment(0.5);
    diskSize.onChange = [this]() {
        if (diskSize.text().length() > 4)
            diskSize.setText( "" );
    };
}

HdCreatorLayout::Progress::Progress() {
    label.setFont(GUIKIT::Font::system("bold"));
    setAlignment(0.5);

    append(bar, {~0u, 0u}, 5);
    append(label, {40u, 0u} );
}

HdCreatorLayout::HdCreatorLayout(Emulator::Interface::MediaGroup* mediaGroup)
: creator(mediaGroup) {
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));

    append(creator, {~0u, 0u}, 10);
    append(progress, {~0u, 0u});
}

auto MediaGroupLayout::updateVisibility( unsigned count, bool init ) -> void {
    
    if (!showOnlyConnectedDevices())
        return;
    
    bool listingInVisibleBlock = false;
    
    if (!count)
        count = 1;

    auto block = blocks[0];
    block->header.remove(*(block->header.list));
    if (count > 1)
        block->header.insert(*(block->header.list), block->header.deviceName, {0u, 0u}, 10);
    
    for(auto block : blocks)
        blockContainer.remove(*block);
    
    for(auto block : blocks) {
        
        if (count) {
            blockContainer.append(*block,{~0u, 0u}, 2);
            
            if (!listingInVisibleBlock)
                listingInVisibleBlock = block == selectedBlock;
            
            count--;
        } else {
            if (!init)
                block->header.eject.onActivate();
        }
    }

    mediaLayout->moduleSwitch.synchronizeLayout();
	
    if (!listingInVisibleBlock)
        updateListing( blocks[0] );
}

auto MediaGroupLayout::getBlock(Emulator::Interface::Media* media) -> Block* {
    for( auto block : blocks ) {
        
        if (block->media == media)
            return block;        
    } 
    
    return nullptr;
}

auto MediaGroupLayout::updateListing( MediaGroupLayout::Block* block ) -> void {

    selectedBlock = block;
    
    fillListing( block->listings );            
}

auto MediaGroupLayout::fillListing( std::vector<Emulator::Interface::Listing>& emuListings ) -> void {

    std::vector<std::vector<std::string>> rows;
    
    for( auto listing : mediaLayout->convertListing( emuListings, false ) )
        rows.push_back({listing});
        
    listings.append(rows);

    if (!mediaLayout->settings->get<bool>("software_preview_tooltips", true ))
        return;
        
	unsigned i = 0;
	for( auto listing : mediaLayout->convertListing( emuListings, true ) )
        listings.setRowTooltip(i++, listing);

}

auto MediaGroupLayout::fillListing( std::vector<GUIKIT::BrowserWindow::Listing>& emuListings ) -> void {

    std::vector<std::vector<std::string>> rows;
    
    for( auto listing : emuListings ) 
        rows.push_back({listing.entry});
        
    listings.append(rows);
		
    if (!mediaLayout->settings->get<bool>("software_preview_tooltips", true ))
        return;
        
	unsigned i = 0;
	for( auto listing : emuListings )
        listings.setRowTooltip(i++, listing.tooltip);
}

auto MediaGroupLayout::showOnlyConnectedDevices() -> bool {
    
    return mediaGroup->isDrive() && mediaGroup->media.size() > 1;
}

auto MediaGroupLayout::build(unsigned previewFontSize) -> void {

    bool isC64 = dynamic_cast<LIBC64::Interface*>(mediaLayout->emulator);

    auto addBlock = [&, isC64](Emulator::Interface::Media* media) -> MediaGroupLayout::Block* {
        auto block = new Block( media, mediaLayout->emulator );
        blocks.push_back(block);
        
        if ( !showOnlyConnectedDevices() ) {
            unsigned spacing = 2;
            if (isC64 && (media->group->id == LIBC64::Interface::MediaGroupIdExpansionFinalChessCard)
                && (media->id & 1))
                spacing += 20;

            blockContainer.append(*block, {~0u, 0u}, spacing);
        }
            
        return block;
    };  

    std::vector<GUIKIT::RadioBox*> radioGroup;
    
    for (auto& media : mediaGroup->media) {
        auto block = addBlock(&media);
        block->layout = this;
        if (block->header.list) {
            block->header.list->setImage(&mediaLayout->searchImage);
        }

        auto& header = block->header;
        
        if (!media.secondary && mediaGroup->selected)
            radioGroup.push_back( &header.inUse );           
                
        if (mediaGroup->expansion)            
            setJumperSettings( &media );        
    }
    
    if (radioGroup.size()) {
        GUIKIT::RadioBox::setGroup( radioGroup );
        for (auto block : blocks) {
            if (mediaGroup->selected == block->media) {
                block->header.inUse.setChecked();
                selectedBlock = block;
            }
        }
    }
    
    if (!selectedBlock)
        selectedBlock = blocks[0];
    
    append(blockContainer, {~0u, 0u}, 2);

    listings.setHeaderText( { "" } );
    listings.setHeaderVisible( false );
    listings.colorRowTooltips( true );
    applyFont(previewFontSize);

    if ( mediaGroup->isProgram( ) || mediaGroup->isTape() )
        append( inject, {0u, 0u}, 3 );

    if ( mediaGroup->isProgram( ) || mediaGroup->isDrive() )
        append( listings, {~0u, ~0u} );
}

auto MediaGroupLayout::setJumperSettings(Emulator::Interface::Media* media) -> void {
    if (media->secondary)
        return;

    auto block = getBlock( media );
    
    auto& selector = block->selector;
    
    for (auto& jumper : mediaGroup->expansion->jumpers) {
        
        auto jumperBox = selector.jumpers[jumper.id];

        std::string saveIdent = media->name + "_jumper_" + jumper.name;

        bool state = mediaLayout->settings->get<bool>(_underscore(saveIdent), jumper.defaultState);

        jumperBox->setChecked(state);
    }
}

auto MediaGroupLayout::applyFont(unsigned fontSize) -> void {

    auto customFont = GUIKIT::Window::getCustomFont(mediaLayout->emulator);

    if (customFont) {
        listings.setFont(customFont->name + ", " + std::to_string(fontSize + customFont->sizeAdjust));

        if (dynamic_cast<LIBC64::Interface*>(mediaLayout->emulator) || GUIKIT::Application::isWinApi())
            listings.setSpacing( 0 );
    } else
        listings.setFont(GUIKIT::Font::system(fontSize));
}

auto MediaGroupLayout::loadSettings() -> void {
        
    for (auto& media : mediaGroup->media) {

        auto block = getBlock( &media );

        block->listings = {};

        listings.reset();

        if (mediaGroup->expansion) {
            for (auto& pcb : mediaGroup->expansion->pcbs) {
                if (media.pcbLayout && (media.pcbLayout == &pcb) )
                    block->selector.combo.setSelectionByUserId( pcb.id );                
            }

            setJumperSettings( &media );
        }
        
        if (mediaGroup->selected && (mediaGroup->selected == &media)) {
             block->header.inUse.setChecked();
             selectedBlock = block;
        }       
    }
}

auto DialogPreviewLayout::updateWidgets(GUIKIT::Settings* settings, Emulator::Interface* emulator) -> void {
    auto prevMode = settings->get<unsigned>("dialog_preview_mode", dynamic_cast<LIBC64::Interface*>(emulator) ? Fileloader::PREV_DIALOG : Fileloader::PREV_OFF, {0,2});
    auto fontSize = settings->get<unsigned>("dialog_preview_fontsize", 11, {8, 16});
    auto tooltips = settings->get<bool>("software_preview_tooltips", true);
    auto commodoreHi = settings->get<bool>("software_preview_commodore_hi", true);
    auto dialogWidth = settings->get<unsigned>("dialog_preview_width", 450, {200, 600});
    auto dialogHeight = settings->get<unsigned>("dialog_preview_height", 200, {50, 400});

    switch(prevMode) {
        case 0: mode.noPreviewRadio.setChecked(); break;
        default:
        case 1: mode.dialogPreviewRadio.setChecked(); break;
        case 2: mode.softwarePreviewRadio.setChecked(); break;
    }

    control.fontSizeCombo.setSelectionByUserId((int)fontSize);
    control.option.tooltips.setChecked(tooltips);
    control.option.commodoreHighlight.setChecked(commodoreHi);
    dimension.dialogWidth.slider.setPosition( dialogWidth - 200 );
    dimension.dialogHeight.slider.setPosition( dialogHeight - 50 );
    dimension.dialogWidth.value.setText( std::to_string( dialogWidth ) + " px" );
    dimension.dialogHeight.value.setText( std::to_string( dialogHeight ) + " px" );

    if (has(previewBox))
        remove( previewBox );
}

auto DialogPreviewLayout::updatePreviewContent(GUIKIT::Settings* settings, Emulator::Interface* emulator) -> void {
    std::string out;
    std::string outTooltip  = "";

    auto customFont = GUIKIT::Window::getCustomFont(emulator );

    auto fontSize = settings->get<unsigned>("dialog_preview_fontsize", 11, {8, 16});

    if (customFont) {
        previewBox.setFont(customFont->name + ", " + std::to_string(fontSize + customFont->sizeAdjust));
        previewBox.setSpacing( 0 );
    } else
        previewBox.setFont( GUIKIT::Font::system(fontSize) );

    if (!previewBox.rowCount()) {
        auto useTooltips = settings->get<bool>("software_preview_tooltips", true);

        if (dynamic_cast<LIBC64::Interface*>(emulator)) {
            std::vector<uint8_t> line = {0x30, 0x20, 0x20, 0x20, 0x20, 0x22, 0x20, 0x44, 0x45, 0x4e, 0x49, 0x53, 0x45, 0x20, 0x20, 0x44, 0x45, 0x4e, 0x49, 0x53, 0x45, 0x20, 0x22, 0x20, 0x50, 0x52, 0x47, 0x3c};
            std::vector<uint8_t> tooltipLine = { 0x4c, 0x4f, 0x41, 0x44, 0x20, 0x22, 0x44, 0x45, 0x4e, 0x49, 0x53, 0x45, 0x22, 0x2c, 0x38, 0x2c, 0x31 };

            if(customFont) {
                line = {0x30, 0x20, 0x20, 0x20, 0x20, 0x22, 0x20, 4, 5, 0xe, 9, 0x13, 5, 0x20, 0x20, 4, 5, 0xe, 9, 0x13, 5, 0x20, 0x22, 0x20, 0x10, 0x12, 7, 0x3c};
                tooltipLine = { 0x0c, 0x0f, 0x01, 0x04, 0x20, 0x22, 0x4, 0x5, 0xe, 0x9, 0x13, 0x5, 0x22, 0x2c, 0x38, 0x2c, 0x31 };
            }

            std::vector<uint8_t> utf8;

            for (auto& code : line) {

                unsigned useCode = code;
                if (customFont)
                    useCode |= customFont->modifier;

                GUIKIT::Utf8::encode(useCode, utf8);
            }

            out = std::string((const char*) utf8.data(), utf8.size());

            if (useTooltips) {
                utf8.clear();

                for (auto& code : tooltipLine) {

                    unsigned useCode = code;
                    if (customFont)
                        useCode |= customFont->modifier;

                    GUIKIT::Utf8::encode(useCode, utf8);
                }

                outTooltip = std::string((const char*) utf8.data(), utf8.size());
            }
        } else {
            out = "Amiga Disk Amiga Disk Amiga Disk Amiga Disk Amiga Disk";
            outTooltip = "s/startup-sequence";
        }

        auto videoManager = VideoManager::getInstance(emulator);
        unsigned foregroundColor = videoManager->getForegroundColor();
        unsigned backgroundColor = videoManager->getBackgroundColor();
        previewBox.setForegroundColor( foregroundColor );
        previewBox.setBackgroundColor( backgroundColor );
        if (settings->get<bool>("software_preview_commodore_hi", true ))
            previewBox.setSelectionColor( backgroundColor, foregroundColor );
        else
            previewBox.resetSelectionColor();

        if (dynamic_cast<LIBAMI::Interface*>(emulator)) {
            previewBox.setRowForegroundColor( 0, backgroundColor );
            previewBox.setRowBackgroundColor( 0, foregroundColor );
        } else {
            previewBox.resetRowForegroundColor( 0 );
            previewBox.resetRowBackgroundColor( 0 );
        }
        
        for (unsigned i = 0; i < 8; i++) {
            previewBox.append( {out} );
            if (useTooltips)
                previewBox.setRowTooltip(i, outTooltip );
        }
    }

    unsigned newWidth = settings->get<unsigned>("dialog_preview_width", 450, {200, 600});
    unsigned newHeight = 80;
    if (GUIKIT::Application::isCocoa())
        newHeight = settings->get<unsigned>("dialog_preview_height", 200, {50, 400});

    if (!has(previewBox))
        append( previewBox, {0u, 0u} );

    update( previewBox, {newWidth, newHeight} );

    synchronizeLayout();
}
