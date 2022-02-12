
FirmwareContainer::Block::Top::Top() {
    append(fileLabelTitle, {0u, 0u}, 5);
    append(fileLabel, {~0u, 0u});
    setAlignment(0.5);
    fileLabelTitle.setFont(GUIKIT::Font::system("bold"));
}

FirmwareContainer::Block::Bottom::Bottom() {
    edit.setEditable(false);
    edit.setDroppable(false);
    open.setEnabled(false);
    eject.setEnabled(false);

    append(edit, {~0u,0u}, 5);
    append(open, {0u,0u}, 5);
    append(eject, {0u,0u});
        
    setAlignment(0.5);
}

FirmwareContainer::Block::Block() {
    append(top, {~0u,0u}, 2);
    append(bottom, {~0u,0u});
}

FirmwareContainer::FirmwareContainer() {
    setPadding( 10 );
    setFont(GUIKIT::Font::system("bold"));
}

FirmwareLayout::FirmwareLayout(TabWindow* tabWindow) {
    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;    
    this->manager = FirmwareManager::getInstance(this->emulator);

    for (unsigned i = 0; i <= manager->maxSets; i++) {

        auto radioBox = new GUIKIT::RadioBox;

        selectorBoxes.push_back(radioBox);

        radioBox->onActivate = [i, this]() {

            _settings->set<unsigned>("use_firmware", i);
            updateVisibility();

            emuThread->lock();
            hotSwap(i);

            if (i == 0) {
                if (dynamic_cast<LIBC64::Interface*>(this->emulator)) {
                    if (!this->tabWindow->systemLayout)
                        this->tabWindow->prepareLayout( EmuConfigView::TabWindow::Layout::System );

                    auto blockSpeeder = this->tabWindow->systemLayout->driveModelLayout.getBlock(
                            LIBC64::Interface::ModelId::ModelIdDriveFastLoader);
                    if (blockSpeeder && blockSpeeder->combo.selection())
                        blockSpeeder->combo.activate(0);
                }
            }
            emuThread->unlock();
        };

        customSelectorLayout.append(*radioBox, {0u, 0u}, i == manager->maxSets ? 0 : 10);
    }

    for (auto& firmware : emulator->firmwares) {
        auto block = new FirmwareContainer::Block;
        block->typeId = firmware.id;
        block->parent = &containerLayout;
        containerLayout.blocks.push_back(block);
        containerLayout.append(*block,{~0u, 0u}, &emulator->firmwares.back() == &firmware ? 0 : 5);

        block->top.fileLabelTitle.setText(trans->get(firmware.name,{}, true));

        block->bottom.eject.onActivate = [this, block]() {
            auto storeLevel = _settings->get<unsigned>("use_firmware", 0, {0, manager->maxSets});
            if (storeLevel == 0)
                return;
            auto& firmware = emulator->firmwares[block->typeId];
            auto fSetting = manager->getSetting( &firmware, storeLevel );

            emuThread->lock();
            block->bottom.edit.setText("");
            block->top.fileLabel.setText("");
            fSetting->init();
            this->manager->addImage(&firmware, storeLevel, nullptr, 0);
            selectedBlock = block;
            hotSwap( storeLevel );
            emuThread->unlock();
        };

        block->bottom.edit.onFocus = [this, block]() {
            selectedBlock = block;
        };

        block->bottom.open.onActivate = [this, block]() {
            auto storeLevel = _settings->get<unsigned>("use_firmware", 0, {0, manager->maxSets});
            if (storeLevel == 0)
                return;
            auto& firmware = emulator->firmwares[block->typeId];
            auto fSetting = manager->getSetting( &firmware, storeLevel );

            std::string filePath = GUIKIT::BrowserWindow()
                .setWindow(*this->tabWindow)
                .setTitle(trans->get("select_firmware_image",{
                    {"%type%", firmware.name}
                }))
                .setFilters({trans->get("firmware_image") + " (*)"})
                .setPath(_settings->get<std::string>("firmware_path", ""))
                .open();

            assign(filePath, block, fSetting, storeLevel);
        };

        block->bottom.edit.onDrop = [this, block](std::vector<std::string> files) {
            auto storeLevel = _settings->get<unsigned>("use_firmware", 0, {0, manager->maxSets});
            if (storeLevel == 0)
                return;
            auto& firmware = emulator->firmwares[block->typeId];
            auto fSetting = manager->getSetting( &firmware, storeLevel );

            assign( files[0], block, fSetting, storeLevel );
        };
    }
        
    GUIKIT::RadioBox::setGroup( selectorBoxes );
    append(customSelectorLayout, {~0u, 0u}, 10);
    append(containerLayout, {~0u, ~0u});
    
    loadSettings(  );
        
    setMargin( 10 );    
}

auto FirmwareLayout::hotSwap( unsigned storeLevel ) -> void {
    if (emulator == activeEmulator) {

        auto firmware = emulator->getCharRom();

        if (firmware) {
            auto missigFirmware = this->manager->swapIn( firmware, storeLevel );

            program->showOpenError( missigFirmware );              
        }
    }
}

auto FirmwareLayout::updateVisibility() -> void {
	
	auto storeLevel = _settings->get<unsigned>( "use_firmware", 0, {0, manager->maxSets} );
    
	if (selectorBoxes.size() >= storeLevel)
        selectedBlock = storeLevel == 0 ? nullptr : containerLayout.blocks[0];

    for (auto block : containerLayout.blocks) {
        auto& firmware = emulator->firmwares[block->typeId];
        auto fSetting = manager->getSetting( &firmware, storeLevel );

        block->top.fileLabel.setText( fSetting->file );
        block->bottom.edit.setText( fSetting->path );

        block->bottom.open.setEnabled(storeLevel != 0);
        block->bottom.eject.setEnabled(storeLevel != 0);
        block->bottom.edit.setDroppable(storeLevel != 0);
    }
}

auto FirmwareLayout::assign(std::string path, FirmwareContainer::Block* block, FileSetting* fSetting, unsigned storeLevel ) -> void {
    
    if (path.empty())
        return;	
                    
    GUIKIT::File* file = filePool->get( path );
    if (!file)
        return;
    
    file->setReadOnly();
    // remember path
    _settings->set<std::string>("firmware_path", file->getPath());

    if (!file->isSizeValid(MAX_MEDIUM_SIZE))
        return program->errorMediumSize( file, mes );  
    
    auto& items = file->scanArchive();

    archiveViewer->onCallback = [this, file, block, fSetting, storeLevel](GUIKIT::File::Item* item) {
        auto& firmware = emulator->firmwares[block->typeId];

        if (!item || (item->info.size == 0))
            return program->errorOpen(file, item, mes);

        if (item->info.size > MAX_FIRMWARE_SIZE)
            return program->errorFirmwareSize(item, mes);          

        block->top.fileLabel.setText(item->info.name);
        block->bottom.edit.setText(file->getFile());
        
        fSetting->setPath(file->getFile());
        fSetting->setFile(item->info.name);
        fSetting->setId(item->id);
				
		uint8_t* data = file->archiveData(item->id);
		unsigned size = file->archiveDataSize( item->id );

		uint8_t* copy = new uint8_t[size];
		std::memcpy(copy, data, size);

        emuThread->lock();
		this->manager->addImage( &firmware, storeLevel, copy, size );
		
        selectedBlock = block;
        
        filePool->unloadOrphaned();
        
        hotSwap( storeLevel );
        emuThread->unlock();
    };

    archiveViewer->setView(items);
}

auto FirmwareLayout::translate() -> void {

    unsigned storeLevel = 0;

    for( auto block : containerLayout.blocks ) {
        block->bottom.open.setText( trans->get("open") );
        block->bottom.eject.setText( trans->get("eject") );
    }

    for (auto box : selectorBoxes) {
        if (storeLevel == 0) {
            selectorBoxes[storeLevel++]->setText( trans->get("default") );
            continue;
        }

        std::string label = "Config " + std::to_string(storeLevel);

        selectorBoxes[storeLevel]->setText( trans->get( label ) );

        storeLevel++;
    }

    containerLayout.setText( trans->get("files") );
}

auto FirmwareLayout::drop( std::string path ) -> void {

    for( auto block : containerLayout.blocks ) {
        if ( block == selectedBlock ) {
            auto& firmware = emulator->firmwares[block->typeId];
            auto storeLevel = _settings->get<unsigned>("use_firmware", 0, {0, manager->maxSets});
            assign( path, block, manager->getSetting( &firmware, storeLevel ), storeLevel );
            break;
        }
    }
}

auto FirmwareLayout::loadSettings() -> void {
    
    auto firmwareInUse = _settings->get<unsigned>( "use_firmware", 0, {0, manager->maxSets} );

//    for (unsigned i = 0; i <= manager->maxSets; i++) {
//        for (auto& firmware : emulator->firmwares) {
//            auto fSetting = manager->getSetting( &firmware, i );
//            if ( (i != 0) && !init) {
//                fSetting->update();
//            }
//        }
//    }

//    if (!init) {
//        manager->clear();
//    }
    
    if (selectorBoxes.size() > firmwareInUse)
        selectorBoxes[firmwareInUse]->setChecked();    
    
    updateVisibility();
}