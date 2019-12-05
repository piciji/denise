
FirmwareContainer::Block::Top::Top() {
    append(fileLabelTitle, {0u, 0u}, 5);
    append(fileLabel, {~0u, 0u});
    setAlignment(0.5);
    fileLabelTitle.setFont(GUIKIT::Font::system(8, "bold"));
    fileLabel.setFont(GUIKIT::Font::system(8));
}

FirmwareContainer::Block::Bottom::Bottom() {
    edit.setEditable(false);
    edit.setDroppable(true);
    append(edit, {~0u,0u}, 5);
    append(open, {0u,0u}, 5);
    append(eject, {0u,0u});
    setAlignment(0.5);
}

FirmwareContainer::Block::Block() {        
    append(top, {~0u,0u}, 1);
    append(bottom, {~0u,0u}, 1);
}

FirmwareLayout::FirmwareLayout(TabWindow* tabWindow) {
    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;    
    this->manager = FirmwareManager::getInstance(this->emulator);
    
    auto firmwareInUse = settings->get<unsigned>( this->tabWindow->ident("use_firmware"), 0, {0, manager->maxSets} );
    
    append(customSelectorLayout, {~0u, 0u}, 5);   
    std::vector<GUIKIT::RadioBox*> selectorBoxes;   
    selectorBoxes.push_back( &defaultGroup );
    
    defaultGroup.onActivate = [this]() {
        // default firmware
        settings->set<unsigned>( this->tabWindow->ident("use_firmware"), 0 );
    };

    customSelectorLayout.append( defaultGroup, {0u, 0u}, 10 );        
    
    for (unsigned i = 1; i <= manager->maxSets; i++) {
        
        auto radioBox = new GUIKIT::RadioBox;
        
        selectorBoxes.push_back( radioBox );
        
        radioBox->onActivate = [i, this]() {
            
            settings->set<unsigned>( this->tabWindow->ident("use_firmware"), i );
        };

        customSelectorLayout.append( *radioBox, {0u, 0u}, 10 );        
        
        // container for custom roms
        auto container = new FirmwareContainer();
        container->storeLevel = i;
        container->setPadding(10);     
        container->setFont(GUIKIT::Font::system("bold"));
        container->selectedGroup = radioBox;

        for (auto& firmware : emulator->firmwares) {
            auto block = new FirmwareContainer::Block;
            block->typeId = firmware.id;            
            block->parent = container;
            container->blocks.push_back(block);
            container->append(*block,{~0u, 0u}, 0);

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

        append(*container,{~0u, 0u}, 3);
    }    
   
    GUIKIT::RadioBox::setGroup( selectorBoxes ); 
    
    if (selectorBoxes.size() > firmwareInUse)
        selectorBoxes[firmwareInUse]->setChecked();
        
    setMargin( 10 );    
}

auto FirmwareLayout::assign(std::string path, FirmwareContainer::Block* block, FileSetting* setting ) -> void {
    
    if (path.empty())
        return;	
                    
    GUIKIT::File* file = filePool->get( path );
    if (!file)
        return;
    // remember path
    settings->set<std::string>(this->tabWindow->ident("firmware_path"), file->getPath());

    if (file->getSize() > MAX_ARCHIVE_SIZE)
        return program->errorArchiveSize( file, mes );  
    
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
        
        uint8_t* data = file->archiveData(item->id);
        unsigned size = file->archiveDataSize( item->id );
        
        uint8_t* copy = new uint8_t[size];
        std::memcpy(copy, data, size);
        
        this->manager->addImage( &firmware, block->parent->storeLevel, copy, size );
        
        selectedBlock = block;
        
        filePool->unloadOrphaned();
    };

    archiveViewer->setView(items);
}

auto FirmwareLayout::translate() -> void {
    
    unsigned i = 1;
    
    for (auto container : containers ) {        
        for( auto block : container->blocks ) {        
            block->bottom.open.setText( trans->get("open") );
            block->bottom.eject.setText( trans->get("eject") );
        }   
        
        std::string label = "Config " + std::to_string(i++);
        
        container->setText( trans->get( label ) );
        
        container->selectedGroup->setText( trans->get( label ) );
    }   
    
    defaultGroup.setText( trans->get("default") );
}

auto FirmwareLayout::drop( std::string path ) -> void {
    
    for (auto container : containers ) {         
        for( auto block : container->blocks ) {
            if ( block == selectedBlock ) {            
                auto& firmware = emulator->firmwares[block->typeId];                     
                assign( path, block, manager->getSetting( &firmware, container->storeLevel ) );
                break;
            }
        }
    }
}
