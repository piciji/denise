
FirmwareContainer::Block::Top::Top() {
    append(fileLabelTitle, {0u, 0u}, 5);
    append(fileLabel, {~0u, 0u});
    setAlignment(0.5);
    fileLabelTitle.setFont(GUIKIT::Font::system("bold"));
}

FirmwareContainer::Block::Bottom::Bottom( bool readOnly ) {
    edit.setEditable(false);
    
    if (readOnly) {
        edit.setDroppable(false);
        open.setEnabled(false);
        eject.setEnabled(false);
    } else {
        edit.setDroppable(true);
    }
    
    append(edit, {~0u,0u}, 5);
    append(open, {0u,0u}, 5);
    append(eject, {0u,0u});
        
    setAlignment(0.5);
}

FirmwareContainer::Block::Block( bool readOnly ) : bottom( readOnly ) {        
    append(top, {~0u,0u}, 2);
    append(bottom, {~0u,0u});
}

FirmwareLayout::FirmwareLayout(TabWindow* tabWindow) {
    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;    
    this->manager = FirmwareManager::getInstance(this->emulator);
    
    append(customSelectorLayout, {~0u, 0u}, 10);
    append(boxLayout, {~0u, ~0u});
    boxLayout.setPadding( 10 );
    boxLayout.setFont(GUIKIT::Font::system("bold"));  
    boxLayout.setText( trans->get("default") );
    boxLayout.append(switchLayout, {~0u, ~0u});
    
    for (unsigned i = 0; i <= manager->maxSets; i++) {
        
        auto radioBox = new GUIKIT::RadioBox;
        
        selectorBoxes.push_back( radioBox );
        
        radioBox->onActivate = [i, this]() {
            
            _settings->set<unsigned>( "use_firmware", i );
			updateVisibility();
            
            hotSwap(i);

            if (i == 0) {
                if (dynamic_cast<LIBC64::Interface*>(this->emulator)) {
                    auto blockSpeeder = this->tabWindow->systemLayout->driveModelLayout.getBlock( LIBC64::Interface::ModelId::ModelIdDriveFastLoader );
                    if (blockSpeeder && blockSpeeder->combo.selection())
                        blockSpeeder->combo.activate(0);
                }
            }
        };

        customSelectorLayout.append( *radioBox, {0u, 0u}, i == manager->maxSets ? 0 : 10 );        
        GUIKIT::Layout* myContainer;     
        
        if (i == 0) {
            auto container = new FirmwareContainer();
            container->storeLevel = 0;
            for (auto& firmware : emulator->firmwares) {
                auto block = new FirmwareContainer::Block( true );                
                
                block->typeId = firmware.id;
                block->parent = container;
                container->blocks.push_back(block);
                container->append(*block,{~0u, 0u}, &emulator->firmwares.back() == &firmware ? 0 : 5);

                block->top.fileLabelTitle.setText(trans->get(firmware.name,{}, true));
            }
            
            myContainer = container;
            
        } else {
            auto container = new FirmwareContainer();
            container->storeLevel = i;           
            
            for (auto& firmware : emulator->firmwares) {
                auto block = new FirmwareContainer::Block;
                block->typeId = firmware.id;            
                block->parent = container;
                container->blocks.push_back(block);
                container->append(*block,{~0u, 0u}, &emulator->firmwares.back() == &firmware ? 0 : 5);

                auto fSetting = manager->getSetting( &firmware, i );
                block->top.fileLabelTitle.setText(trans->get(firmware.name,{}, true));

                block->bottom.eject.onActivate = [this, block, container, fSetting]() {
                    auto& firmware = emulator->firmwares[block->typeId];
                    block->bottom.edit.setText("");
                    block->top.fileLabel.setText("");
                    fSetting->init();
                    this->manager->addImage(&firmware, container->storeLevel, nullptr, 0);
                    selectedBlock = block;
                    hotSwap( block->parent->storeLevel );
                };

                block->bottom.edit.onFocus = [this, block]() {
                    selectedBlock = block;
                };

                block->bottom.open.onActivate = [this, block, fSetting]() {
                    auto& firmware = emulator->firmwares[block->typeId];

                    std::string filePath = GUIKIT::BrowserWindow()
                        .setWindow(*this->tabWindow)
                        .setTitle(trans->get("select_firmware_image",{
                            {"%type%", firmware.name}
                        }))
                        .setFilters({trans->get("firmware_image") + " (*)"})
                        .setPath(_settings->get<std::string>("firmware_path", ""))
                        .open();

                    assign(filePath, block, fSetting);
                };

                block->bottom.edit.onDrop = [this, block, fSetting](std::vector<std::string> files) {
                    assign( files[0], block, fSetting );
                };
            }
            
            myContainer = container;
        }
        
        containers.push_back(myContainer);
        switchLayout.setLayout(i, *myContainer, {~0u, ~0u});
    }    
		  
    GUIKIT::RadioBox::setGroup( selectorBoxes ); 
    
    loadSettings( true );
        
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
	
	auto firmwareInUse = _settings->get<unsigned>( "use_firmware", 0, {0, manager->maxSets} );
    
	if (selectorBoxes.size() >= firmwareInUse) {
        
        switchLayout.setSelection( firmwareInUse );
        selectedBlock = firmwareInUse == 0 ? nullptr : ((FirmwareContainer*)containers[firmwareInUse])->blocks[0];
	}
}

auto FirmwareLayout::assign(std::string path, FirmwareContainer::Block* block, FileSetting* fSetting ) -> void {
    
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

    archiveViewer->onCallback = [this, file, block, fSetting](GUIKIT::File::Item* item) {
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

		this->manager->addImage( &firmware, block->parent->storeLevel, copy, size );       
		
        selectedBlock = block;
        
        filePool->unloadOrphaned();
        
        hotSwap( block->parent->storeLevel );
    };

    archiveViewer->setView(items);
}

auto FirmwareLayout::translate() -> void {
    
    unsigned i = 0;
	    
    for (auto container : containers ) {                
        
        auto fContainer = (FirmwareContainer*)container;
        
        for( auto block : fContainer->blocks ) {        
            block->bottom.open.setText( trans->get("open") );
            block->bottom.eject.setText( trans->get("eject") );
        }   
        	
        if (i == 0) {
            selectorBoxes[i++]->setText( trans->get("default") );            
            continue;
        }
        
        std::string label = "Config " + std::to_string(i);

        selectorBoxes[i]->setText( trans->get( label ) );
		
		i++;
    }    	
    
    boxLayout.setText( trans->get("files") );
}

auto FirmwareLayout::drop( std::string path ) -> void {
    
    for (auto container : containers ) {     
		
        if (!dynamic_cast<FirmwareContainer*>(container))
            continue;
        
        auto fContainer = (FirmwareContainer*)container;
        
        for( auto block : fContainer->blocks ) {
            if ( block == selectedBlock ) {            
                auto& firmware = emulator->firmwares[block->typeId];    
								
				assign( path, block, manager->getSetting( &firmware, fContainer->storeLevel ) );
								
                break;
            }
        }
    }
}

auto FirmwareLayout::loadSettings(bool init) -> void {
    
    auto firmwareInUse = _settings->get<unsigned>( "use_firmware", 0, {0, manager->maxSets} );

    for (unsigned i = 0; i <= manager->maxSets; i++) {
        unsigned j = 0;
        for (auto& firmware : emulator->firmwares) {
            auto block = ((FirmwareContainer*)containers[i])->blocks[j++];
            
       //     if (i == 0) {
         //       block->top.fileLabel.setText( firmware.name );
           //     block->bottom.edit.setText( program->dataFolder() + firmware.name );
            //} else {
                auto fSetting = manager->getSetting( &firmware, i );
                if ( (i != 0) && !init) {
                    fSetting->update();
                }

                block->top.fileLabel.setText( fSetting->file );
                block->bottom.edit.setText( fSetting->path );
            //}
        }
    }

    if (!init) {
        manager->clear();
    }
    
    if (selectorBoxes.size() > firmwareInUse)
        selectorBoxes[firmwareInUse]->setChecked();    
    
    updateVisibility();
}