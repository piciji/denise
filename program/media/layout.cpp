
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

MediaGroupLayout::Block::Header::Header(Emulator::Interface::Media* media) {
    auto group = media->group;
    bool IPMode = group->isExpansion() && group->expansion->isRS232();

    deviceName.setFont(GUIKIT::Font::system("bold"));
    inUse.setFont(GUIKIT::Font::system("bold"));
    if (!media->secondary && group->selected)
        append(inUse, {0u, 0u}, 5);
    else
        append(deviceName, {0u, 0u}, 10);
        
    if (group->isWritable())
        append(writeprotect, {0u, 0u}, 10);

    if (!IPMode) {
        append(eject, {0u, 0u}, 10);
        append(fileName, {~0u, 0u});
    }
    setAlignment(0.5);
}

MediaGroupLayout::Block::Selector::Selector(Emulator::Interface::Media* media) {
    auto group = media->group;
    bool IPMode = group->isExpansion() && group->expansion->isRS232();

    append(edit, {~0u, 0u}, 10);

    if (media->pcbLayout && group->expansion && !media->secondary && (group->expansion->pcbs.size() > 0) ) {
        for (auto& pcb : group->expansion->pcbs) {
            combo.append( pcb.name, pcb.id );

            if (media->pcbLayout == &pcb)
                combo.setSelection( combo.rows() - 1 );
        }
        
        append(combo, {0u, 0u}, 10);      
    }
                  
    if (group->expansion && (group->expansion->jumpers.size() > 0) ) {
        append(jumperLabel, {0u, 0u}, 5 );
        
        for(auto& jumper : group->expansion->jumpers) {
            auto jumpChecker = new GUIKIT::CheckBox;
            jumpChecker->setText( jumper.name );

            append(*jumpChecker, {0u, 0u}, 5);

            jumpers.push_back( jumpChecker );
        }        
    }

    if (!IPMode)
        append(open, {0u, 0u});
    
    setAlignment(0.5);
    edit.setEditable( IPMode );
    edit.setDroppable();
}

MediaGroupLayout::Block::Block(Emulator::Interface::Media* media) : media(media), header(media), selector(media) {
    append(header, {~0u, 0u}, 2);
    append(selector, {~0u, 0u});
}

MediaGroupLayout::MediaGroupLayout( Emulator::Interface::MediaGroup* mediaGroup, MediaLayout* mediaLayout ) {
    this->mediaGroup = mediaGroup;
    this->mediaLayout = mediaLayout;
    
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
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
        append(fastFileSystem, {0u, 0u}, 5);
        append(highDensity, {0u, 0u}, 5);
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

HdCreatorLayout::Creator::Creator() {
    append(diskSizeName, {0u, 0u}, 10);
    append(diskSize, {40, 0u}, 10);
    append(diskLabelName, {0u, 0u}, 5);
    append(diskLabel, {~0u, 0u}, 10);
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

HdCreatorLayout::HdCreatorLayout() {
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

    mediaLayout->synchronizeLayout();
	
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
	
    // first check is usefull, when App is quited and globalSettings object deleted at this point
    if (!globalSettings || !globalSettings->get<bool>("software_preview_tooltips", true ))
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
		
    if (!globalSettings->get<bool>("software_preview_tooltips", true ))
        return;
        
	unsigned i = 0;
	for( auto listing : emuListings )
        listings.setRowTooltip(i++, listing.tooltip);
}

auto MediaGroupLayout::showOnlyConnectedDevices() -> bool {
    
    return mediaGroup->isDrive() && mediaGroup->media.size() > 1;
}

auto MediaGroupLayout::build() -> void {

    auto addBlock = [&](Emulator::Interface::Media* media) -> MediaGroupLayout::Block* {
        auto block = new Block( media );
        blocks.push_back(block);
        
        if ( !showOnlyConnectedDevices() )
            blockContainer.append(*block, {~0u, 0u}, 2);
            
        return block;
    };  

    std::vector<GUIKIT::RadioBox*> radioGroup;
    
    for (auto& media : mediaGroup->media) {
        auto block = addBlock(&media);
        block->layout = this;

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

    if ( dynamic_cast<LIBC64::Interface*>(mediaLayout->emulator)) {
		listings.setHeaderText( { "" } );
		listings.setHeaderVisible( false );
        listings.colorRowTooltips( true );

        unsigned _fontSize = globalSettings->get<unsigned>("software_preview_fontsize", 12, {6, 14});
        
        applyFont(_fontSize);
        
        if ( mediaGroup->isProgram( ) )
            append( inject, {0u, 0u}, 3 );
            
        if ( mediaGroup->isProgram( ) || mediaGroup->isDrive() )
            append( listings, {~0u, ~0u} );
	}
}

auto MediaGroupLayout::setJumperSettings(Emulator::Interface::Media* media) -> void {
            
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

    if (GUIKIT::Window::countCustomFonts())
        listings.setFont("C64 Pro, " + std::to_string(fontSize), true);
    else
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
