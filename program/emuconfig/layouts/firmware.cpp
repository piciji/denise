
FirmwareContainer::Block::Top::Top() {
    append(fileLabelTitle, {0u, 0u}, 5);
    append(fileLabel, {~0u, 0u});
    setAlignment(0.5);
    fileLabelTitle.setFont(GUIKIT::Font::system("bold"));
}

FirmwareContainer::Block::Bottom::Bottom(bool useSwap) {
    edit.setEditable(false);
    edit.setDroppable(true);
    append(edit, {~0u,0u}, 5);
    append(open, {0u,0u}, 5);
    append(eject, {0u,0u}, useSwap ? 5 : 0);
	if (useSwap)
		append(swapIn, {0u,0u});
		
    setAlignment(0.5);
}

FirmwareContainer::Block::Block(bool useSwap) : bottom(useSwap) {        
    append(top, {~0u,0u}, 2);
    append(bottom, {~0u,0u});
}

FirmwareLayout::FirmwareLayout(TabWindow* tabWindow) {
    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;    
    this->manager = FirmwareManager::getInstance(this->emulator);
    
    auto firmwareInUse = settings->get<unsigned>( this->tabWindow->ident("use_firmware"), 0, {0, manager->maxSets} );
    
    append(customSelectorLayout, {~0u, 0u}, 10);   
    
	auto defaultBox = new GUIKIT::RadioBox;
	
    selectorBoxes.push_back( defaultBox );
    
    defaultBox->onActivate = [this]() {
        // default firmware
        settings->set<unsigned>( this->tabWindow->ident("use_firmware"), 0 );
		updateVisibility();
    };

    customSelectorLayout.append( *defaultBox, {0u, 0u}, 10 );        
    
    for (unsigned i = 1; i <= manager->maxSets; i++) {
        
        auto radioBox = new GUIKIT::RadioBox;
        
        selectorBoxes.push_back( radioBox );
        
        radioBox->onActivate = [i, this]() {
            
            settings->set<unsigned>( this->tabWindow->ident("use_firmware"), i );
			updateVisibility();
        };

        customSelectorLayout.append( *radioBox, {0u, 0u}, i == manager->maxSets ? 0 : 10 );        
        
        // container for custom roms
        auto container = new FirmwareContainer();
        container->storeLevel = i;
        container->setPadding(10);     
        container->setFont(GUIKIT::Font::system("bold"));

        for (auto& firmware : emulator->firmwares) {
            auto block = new FirmwareContainer::Block;
            block->typeId = firmware.id;            
            block->parent = container;
            container->blocks.push_back(block);
            container->append(*block,{~0u, 0u}, &emulator->firmwares.back() == &firmware ? 0 : 5);

            auto setting = manager->getSetting( &firmware, i );
            block->top.fileLabelTitle.setText(trans->get(firmware.name,{}, true));
            block->top.fileLabel.setText(setting->file);
            block->bottom.edit.setText(setting->path);

            block->bottom.eject.onActivate = [this, block, container, setting]() {
                auto& firmware = emulator->firmwares[block->typeId];
                block->bottom.edit.setText("");
                block->top.fileLabel.setText("");
                setting->init();
                this->manager->addImage(&firmware, container->storeLevel, nullptr, 0);
                selectedBlock = block;
            };

            block->bottom.edit.onFocus = [this, block]() {
                selectedBlock = block;
            };

            block->bottom.open.onActivate = [this, block, setting]() {
                auto& firmware = emulator->firmwares[block->typeId];

                std::string filePath = GUIKIT::BrowserWindow()
                    .setWindow(*this->tabWindow)
                    .setTitle(trans->get("select_firmware_image",{
                        {"%type%", firmware.name}
                    }))
                    .setFilters({trans->get("firmware_image") + " (*)"})
                    .setPath(settings->get<std::string>(this->tabWindow->ident("firmware_path"), ""))
                    .open();

                assign(filePath, block, setting);
            };
            
            block->bottom.edit.onDrop = [this, block, setting](std::vector<std::string> files) {
                assign( files[0], block, setting );
            };
        }
        
        containers.push_back(container);
    }    
	
	// add 4 char rom swapper
	if ( dynamic_cast<LIBC64::Interface*>(emulator) ) {
		
		auto& firmware = emulator->firmwares[2];
		
		auto container = new FirmwareContainer();
        container->setPadding(10);     
        container->setFont(GUIKIT::Font::system("bold"));
		
		for(unsigned i = 0; i < 4; i++) {
			auto block = new FirmwareContainer::Block(true);
            block->typeId = firmware.id;            
			block->position = i;
            block->parent = container;
            container->blocks.push_back(block);
            container->append(*block,{~0u, 0u}, i == 3 ? 0 : 5);

			auto setting = manager->getSetting( &firmware, manager->swapStartLevel + i );
            block->top.fileLabelTitle.setText(trans->get(firmware.name,{}, true));
            block->top.fileLabel.setText(setting->file);
            block->bottom.edit.setText(setting->path);

            block->bottom.eject.onActivate = [this, block,  setting]() {
                auto& firmware = emulator->firmwares[block->typeId];
                block->bottom.edit.setText("");
                block->top.fileLabel.setText("");
                setting->init();
				this->manager->addImage(&firmware, this->manager->swapStartLevel + block->position, nullptr, 0);
                selectedBlock = block;
            };

            block->bottom.edit.onFocus = [this, block]() {
                selectedBlock = block;
            };

            block->bottom.open.onActivate = [this, block, setting]() {
                auto& firmware = emulator->firmwares[block->typeId];

                std::string filePath = GUIKIT::BrowserWindow()
                    .setWindow(*this->tabWindow)
                    .setTitle(trans->get("select_firmware_image",{
                        {"%type%", firmware.name}
                    }))
                    .setFilters({trans->get("firmware_image") + " (*)"})
                    .setPath(settings->get<std::string>(this->tabWindow->ident("firmware_path"), ""))
                    .open();

                assign(filePath, block, setting);
            };
			
			block->bottom.swapIn.onActivate = [this, block, setting]() {
				auto& firmware = emulator->firmwares[block->typeId];
				
				auto missigFirmware = this->manager->swapIn( &firmware, this->manager->swapStartLevel + block->position );
				
				program->showOpenError( missigFirmware );
			};
            
            block->bottom.edit.onDrop = [this, block, setting](std::vector<std::string> files) {
                assign( files[0], block, setting );
            };
		}
		
		customSelectorLayout.append( spacer, {~0u, 0u} );  
		customSelectorLayout.append( hotSwapButton, {0u, 0u} );		
		
		hotSwapButton.onToggle = [this]() {
			
			if (!hotSwapButton.checked()) {
				updateVisibility();
				return;
			}
			
			if (selectedBlock)	
				remove(*selectedBlock->parent);	
			
			append(*containers.back(), {~0u, 0u});
			
			selectedBlock = containers.back()->blocks[0];
			
			this->tabWindow->synchronizeLayout();
		};
		
		containers.push_back(container);
	}	
	
   
    GUIKIT::RadioBox::setGroup( selectorBoxes ); 
    
    if (selectorBoxes.size() > firmwareInUse) {
        selectorBoxes[firmwareInUse]->setChecked();
	}
	
	updateVisibility();
        
    setMargin( 10 );    
}

auto FirmwareLayout::updateVisibility() -> void {
	
	hotSwapButton.setChecked(false);
	
	auto firmwareInUse = settings->get<unsigned>( this->tabWindow->ident("use_firmware"), 0, {0, manager->maxSets} );
	
	if (selectedBlock)	
		remove(*selectedBlock->parent);	
	
	if (firmwareInUse == 0) {
		selectedBlock = nullptr;
		return;
	}
	
	if (selectorBoxes.size() >= firmwareInUse) {
		append(*containers[firmwareInUse-1], {~0u, 0u});
		 
		selectedBlock = containers[firmwareInUse-1]->blocks[0];
	}
	
	tabWindow->synchronizeLayout();
}

auto FirmwareLayout::assign(std::string path, FirmwareContainer::Block* block, FileSetting* setting ) -> void {
    
    if (path.empty())
        return;	
                    
    GUIKIT::File* file = filePool->get( path );
    if (!file)
        return;
    // remember path
    settings->set<std::string>(this->tabWindow->ident("firmware_path"), file->getPath());

    if (!file->isSizeValid(MAX_MEDIUM_SIZE))
        return program->errorMediumSize( file, mes );  
    
    auto& items = file->scanArchive();

    archiveViewer->onCallback = [this, file, block, setting](GUIKIT::File::Item* item) {
        auto& firmware = emulator->firmwares[block->typeId];

        if (!item || (item->info.size == 0))
            return program->errorOpen(file, item, mes);

        if (item->info.size > MAX_FIRMWARE_SIZE)
            return program->errorFirmwareSize(item, mes);          

        block->top.fileLabel.setText(item->info.name);
        block->bottom.edit.setText(file->getFile());
        
        setting->setPath(file->getFile());
        setting->setFile(item->info.name);
        setting->setId(item->id);
        
		bool swapBlock = block->parent == containers.back();
				
		uint8_t* data = file->archiveData(item->id);
		unsigned size = file->archiveDataSize( item->id );

		uint8_t* copy = new uint8_t[size];
		std::memcpy(copy, data, size);

		this->manager->addImage( &firmware,
			swapBlock ? (this->manager->swapStartLevel + block->position) : block->parent->storeLevel, copy, size );
		
        selectedBlock = block;
        
        filePool->unloadOrphaned();
    };

    archiveViewer->setView(items);
}

auto FirmwareLayout::translate() -> void {
    
    unsigned i = 0;
	
	selectorBoxes[i++]->setText( trans->get("default") );
    
    for (auto container : containers ) {        
        for( auto block : container->blocks ) {        
            block->bottom.open.setText( trans->get("open") );
            block->bottom.eject.setText( trans->get("eject") );
			block->bottom.swapIn.setText( trans->get("swap") );
        }   
        
		if (containers.back() == container) {
			container->setText( trans->get( "Config" ) );
			
		} else {
		
			std::string label = "Config " + std::to_string(i);
        
		    container->setText( trans->get( label ) );
		
			selectorBoxes[i]->setText( trans->get( label ) );
		}
		i++;
    }    
	
	hotSwapButton.setText( trans->get( "swap_char" ) );
}

auto FirmwareLayout::drop( std::string path ) -> void {
    
    for (auto container : containers ) {     
		
        for( auto block : container->blocks ) {
            if ( block == selectedBlock ) {            
                auto& firmware = emulator->firmwares[block->typeId];    
				
				bool swapBlock = containers.back() == container;
				
				
				assign( path, block, manager->getSetting( &firmware,
					swapBlock ? (manager->swapStartLevel + block->position) : container->storeLevel ) );
								
                break;
            }
        }
    }
}
