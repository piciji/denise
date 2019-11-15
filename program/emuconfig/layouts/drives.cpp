
PathsLayout::Block::Block(Emulator::Interface::DriveGroup* driveGroup) {
    this->driveGroup = driveGroup;
        
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
}

DriveGroupLayout::Block::Header::Header() {
    deviceName.setFont(GUIKIT::Font::system("bold"));
    append(deviceName, {0u, 0u}, 10);
    append(writeprotect, {0u, 0u}, 10);
    append(eject, {0u, 0u}, 10);
    append(fileName, {~0u, 0u});
    setAlignment(0.5);
}

DriveGroupLayout::Block::Selector::Selector() {
    append(edit, {~0u, 0u}, 10);
    append(open, {0u, 0u}, 5);
    append(openW, {0u, 0u});
    setAlignment(0.5);
    edit.setEditable(false);
    edit.setDroppable();
}

DriveGroupLayout::Block::Block() {
    append(header, {~0u, 0u}, 2);
    append(selector, {~0u, 0u});
}

DriveGroupLayout::DriveGroupLayout( Emulator::Interface::DriveGroup* driveGroup, TabWindow* tabWindow ) {
    this->driveGroup = driveGroup;
    this->tabWindow = tabWindow;
    
    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
}

DiskCreatorLayout::DiskCreatorLayout( Emulator::Interface* emulator, std::vector<std::string> creatables ) {

    unsigned formatId = 0;
    for ( auto creatable : creatables )        
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

    append(button, {0u, 0u});
    setFont(GUIKIT::Font::system("bold"));
    setPadding(10);
    setAlignment(0.5);       
}

TapeCreatorLayout::TapeCreatorLayout() {
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

auto DrivesLayout::bindSelectorAction(DriveGroupLayout* layout) -> void {
	
    auto driveGroup = layout->driveGroup;
        
	for (auto block : layout->blocks) {

		auto setting = FileSetting::getInstance(tabWindow->ident(block->drive->name));
					
		if (driveGroup->isHardDrive()) {

			block->selector.open.onActivate = [this, block, driveGroup, setting]() {
				
				std::string filePath = GUIKIT::BrowserWindow()
                    .setWindow(*tabWindow)
                    .setTitle(trans->get("select_" + driveGroup->name + "_image"))
                    .setPath( preselectPath( driveGroup->name ) )
                    .setFilters({ GUIKIT::BrowserWindow::transformFilter(trans->get(driveGroup->name + "_image"), driveGroup->suffix), trans->get("all_files")})
                    .open();

				if (filePath.empty())
					return;

				block->header.eject.onActivate();
				GUIKIT::File testFile(filePath);
                
				savePath( driveGroup->name, testFile.getPath() );

                if (!testFile.isSizeValid(MAX_HARDDISK_SIZE)) {
					mes->error(trans->get("file_size_error",{
						{"%path%", filePath},
						{"%size%", GUIKIT::File::SizeFormated(MAX_HARDDISK_SIZE)}
					}));
				} else if (testFile.isArchived()) {
					mes->error(trans->get("archive_none"));
				} else if (!testFile.open(GUIKIT::File::Mode::Update)) {
					mes->error(trans->get("file_open_error",{
						{"%path%", filePath}
					}));
				} else {
					setting->setPath(filePath);
					block->selector.edit.setText(filePath);
				}
				testFile.unload();
			};

			block->header.eject.onActivate = [setting, block]() {
				setting->setPath("");
				block->selector.edit.setText("");
			};

			block->selector.edit.setText(setting->path);

		} else {
            
            block->selector.openW.onActivate = [this, block]() {                
                block->openWritable = true;
                block->selector.open.onActivate();
                block->openWritable = false;
            };
            
			block->selector.open.onActivate = [this, block, driveGroup, setting, layout]() {                

                auto suffix = driveGroup->suffix;
                GUIKIT::Vector::combine(suffix, GUIKIT::File::suppportedCompressionExtensions());    
                
				std::string filePath = GUIKIT::BrowserWindow()
					.setWindow(*tabWindow)
					.setTitle(trans->get("select_" + driveGroup->name + "_image"))
					.setPath( preselectPath( driveGroup->name ) )
					.setFilters({ GUIKIT::BrowserWindow::transformFilter(trans->get(driveGroup->name + "_image"), suffix ),
						trans->get("all_files")})
					.open();

				if (filePath.empty())
					return;

				GUIKIT::File* file = filePool->get(filePath);
				if (!file)
					return;

				savePath( driveGroup->name, file->getPath() );
                
				if (file->getSize() > MAX_ARCHIVE_SIZE)
                    return program->errorArchiveSize( file, mes );				
                
                if ( block->openWritable && file->isArchived())
                    mes->warning(trans->get("archive_wp_tooltip"));
                
				auto& items = file->scanArchive();

				archiveViewer->onCallback = [this, file, block, driveGroup](GUIKIT::File::Item* item) {

                    if (!item || (item->info.size == 0) )
                        return program->errorOpen( file, item, mes );                     
                    
                    if (!driveGroup->isTapeDrive() && (item->info.size > MAX_MEDIUM_SIZE))
                        return program->errorMediumSize( item, mes );                    
                    
                    insertImage({file, item, driveGroup, block});                                       
				};
				archiveViewer->setView(items);
			};

			block->header.eject.onActivate = [this, driveGroup, block, setting, layout]() {
                
                auto drive = block->drive;
                
				if ( !driveGroup->isModuleSlot() ) {
					emulator->ejectMedium(driveGroup->type, drive->id);
					drive->guid = (uintptr_t)nullptr;
					filePool->assign(tabWindow->ident(drive->name), nullptr);
                    States::getInstance( emulator )->updateImage( nullptr, drive );
				}		
				
				if ( showC64Listing( layout ) )
                    block->listings.clear();
                
                if (layout->selectedBlock->drive == drive)
                    layout->listings.reset();
				
				if (driveGroup->isTapeDrive())
					view->updateTapeIcons();
				
				filePool->assign(tabWindow->ident(drive->name + "store"), nullptr);
                filePool->unloadOrphaned();

				setting->init();
				updateDriveBlock(block, setting);
			};

			block->header.writeprotect.onToggle = [this, block, setting, driveGroup]() {
				
				bool state = block->header.writeprotect.checked();

				if (!state && !setting->wpEnabled) {
                    block->header.writeprotect.setChecked();
                    block->header.writeprotect.setEnabled(false);
                    return;
				}

				emulator->writeProtect(driveGroup->type, block->drive->id, state);
				setting->setWriteProtect(state);
                States::getInstance( emulator )->updateImage( setting, block->drive );
			};
			
			updateDriveBlock(block, setting);
            
            block->selector.edit.onFocus = [this, layout, block]() {
                layout->selectedBlock = block;
                if ( showC64Listing( layout ) ) {
                    layout->fillListing( block );
                }
            };
            
            block->selector.edit.onDrop = [this, layout, block]( std::vector<std::string> files ) {
                
                drop( files[0], block );
            };
		}

		if ( showC64Listing( layout ) ) { //preload last listing
			GUIKIT::File* file = filePool->get( setting->path );
			uint8_t* data;

            if (program->loadImageDataWhenOk(file, setting->id, driveGroup, data)) {
				filePool->assign(tabWindow->ident(block->drive->name + "store"), file);
                emulator->insertMedium(driveGroup->type, block->drive->id, data, file->archiveDataSize( setting->id ));
                block->listings = emulator->getListing( driveGroup->type, block->drive->id );
                if (block->drive->id == 0)
                    layout->fillListing( block );				
			}
		}
	}
    
    if ( showC64Listing( layout ) ) {

        layout->listings.onActivate = [this, layout, driveGroup]( ) {
            
            auto selection = layout->listings.selection( );
            program->power( emulator );
            emulator->selectListing( driveGroup->type, layout->selectedBlock->drive->id, selection );
            view->setFocused(300);
        };

        layout->inject.onActivate = [this, layout, driveGroup]() {

            if (!layout->listings.selected())
                return;

            if ( emulator->selectListing( driveGroup->type, layout->selectedBlock->drive->id, layout->listings.selection( ) ) ) {
                status->addMessage( trans->get( "memory_injected" ) );
                view->setFocused( 300 );
            }	
        };
    }
}

auto DrivesLayout::createImage( unsigned groupId ) -> void {

    auto& driveGroup = emulator->driveGroups[ groupId ];
    std::string title = driveGroup.name + "_image";
    std::string suffix = driveGroup.suffix[0];
    GUIKIT::File file;
    GUIKIT::File* filePtr;
    std::string filePath;
    uint8_t* data = nullptr;
    unsigned size = 0;

    if (driveGroup.isHardDrive()) {

        try {
            size = std::stoi( hdCreatorLayout->creator.diskSize.text() );
            if (size > 4095)
                throw "";
            size = size * 1024u * 1024u;
        } catch (...) {
            mes->error(trans->get("invalid_input"));
            return;
        }        
        
    } else if (driveGroup.isDiskDrive()) {
        suffix = diskCreatorLayout->format.text();
        
        unsigned typeId = diskCreatorLayout->format.userData();
        bool hd = diskCreatorLayout->highDensity.checked();
        
        data = emulator->createDiskImage( typeId, hd,
            diskCreatorLayout->diskLabel.text(),
            diskCreatorLayout->fastFileSystem.checked()
        );
        
        size = emulator->getDiskImageSize(typeId, hd);
        
    } else if (driveGroup.isTapeDrive()) {
        data = emulator->createTapeImage( size );
        
    } else if (driveGroup.isMemory()) {
        data = emulator->getLoadedMemory( size );
    }        
    
    if (!size)
        goto Done; //internal error
    
    filePath = GUIKIT::BrowserWindow()
        .setWindow(*tabWindow)
        .setTitle(trans->get( "blank_" + title ))
        .setPath(preselectPath( driveGroup.name ))
        .setFilters({GUIKIT::BrowserWindow::transformFilter( trans->get( title ), {suffix}), trans->get("all_files")})
        .save();

    if (filePath.empty())
        goto Done;

    if ( !GUIKIT::String::foundSubStr( filePath, "." ))
        filePath += "." + suffix;

    filePtr = filePool->get( filePath, false );    
    if (filePtr)
        filePtr->forceDataChange();
        
    file.setFile( filePath );
    
    if ( file.exists() && !mes->question(trans->get("file_exist_error", {
        {"%path%", filePath } })))
        goto Done;                

    if ( !file.open(GUIKIT::File::Mode::Write) ) {
        mes->error(trans->get("file_creation_error",{
            {"%path%", filePath}
        }));
        
        goto Done;
    }

    savePath( driveGroup.name, file.getPath() );

    if (data) {
        file.write( data, size );

        mes->information(trans->get("file_creation_success",{
            {"%path%", filePath}
        }));
        
    } else {
        // hd creation
        file.unload();        
        
        std::thread t1([this, size, filePath] {
            GUIKIT::File file(filePath);
            file.open(GUIKIT::File::Mode::Write);

            std::function<void (uint8_t* buffer, unsigned length, unsigned offset) > onCreate;

            onCreate = [this, size, &file](uint8_t* buffer, unsigned length, unsigned offset) {

                file.write(buffer, length, offset);

                unsigned posPercent = (double(offset + length) * 100.0) / (double) size;

                hdCreatorLayout->progress.bar.setPosition(posPercent);
                hdCreatorLayout->progress.label.setText(std::to_string(posPercent) + " %");
            };

            hdCreatorLayout->creator.button.setEnabled(false);
            emulator->createHardDrive(onCreate, size, hdCreatorLayout->creator.diskLabel.text() );
            hdCreatorLayout->creator.button.setEnabled();
        });
        t1.detach();
    }
            
    Done:
        if (data)
            delete[] data;                   
}

auto DrivesLayout::prepareCreator() -> void {

    for (auto& driveGroup : emulator->driveGroups) {
		auto groupId = driveGroup.id;
		
        if (driveGroup.isHardDrive()) {

            hdCreatorLayout = new HdCreatorLayout;

            hdCreatorLayout->creator.button.onActivate = [this, groupId]() {
                createImage( groupId );
            };

            creatorLayout.append(*hdCreatorLayout, {~0u, 0u}, 5);

        } else if (driveGroup.isDiskDrive()) {

            diskCreatorLayout = new DiskCreatorLayout(emulator, driveGroup.creatable );

            diskCreatorLayout->button.onActivate = [this, groupId]() {
                createImage( groupId );                
            };

            creatorLayout.append(*diskCreatorLayout, {~0u, 0u}, 5);

        } else if (driveGroup.isTapeDrive()) {

            tapeCreatorLayout = new TapeCreatorLayout;

            tapeCreatorLayout->button.onActivate = [this, groupId]() {
                createImage( groupId );                
            };

            creatorLayout.append(*tapeCreatorLayout, {~0u, 0u}, 5);
			
        } else if (driveGroup.isMemory()) {
			
			memoryCreatorLayout = new MemoryCreatorLayout;
			
			memoryCreatorLayout->button.onActivate = [this, groupId]() {
                createImage( groupId );
			};
			
			creatorLayout.append(*memoryCreatorLayout, {~0u, 0u}, 5);
		}
    }
}

auto DrivesLayout::preparePaths() -> void {
    
    for (auto& driveGroup : emulator->driveGroups) {
        
        auto settingFolderIdent = tabWindow->ident( driveGroup.name + "_folder" );
        
        auto block = new PathsLayout::Block( &driveGroup );
        
        pathsLayout.blocks.push_back( block );                
        pathsLayout.append( *block,{~0u, 0u}, &driveGroup != &emulator->driveGroups.back() ? 5 : 0 );
        
        std::string title = "select_" + driveGroup.name + "_folder";

        block->select.onActivate = [this, block, title, settingFolderIdent]() {
            auto path = GUIKIT::BrowserWindow()
                .setTitle(trans->get(title))
				.setPath( settings->get<std::string>(settingFolderIdent, "") )
                .setWindow(*tabWindow)
                .directory();

            if (!path.empty()) {
                settings->set<std::string>(settingFolderIdent, path);
                block->edit.setText(path);
            }
        };

        block->empty.onActivate = [block, settingFolderIdent]() {
            settings->set<std::string>(settingFolderIdent, "");
            block->edit.setText("");
        };

        block->edit.setText( settings->get<std::string>(settingFolderIdent, "") );
    }
}

DrivesLayout::DrivesLayout(TabWindow* tabWindow) {

    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;
    
    setPadding(10);
    setMargin(10);

    diskImage.loadPng((uint8_t*) Icons::disk, sizeof (Icons::disk));
    hdImage.loadPng((uint8_t*) Icons::drive, sizeof (Icons::drive));
	tapeImage.loadPng((uint8_t*) Icons::tape, sizeof (Icons::tape));
	moduleImage.loadPng((uint8_t*) Icons::memory, sizeof (Icons::memory));
	memoryImage.loadPng((uint8_t*) Icons::memory, sizeof (Icons::memory));
    addImage.loadPng((uint8_t*) Icons::add, sizeof (Icons::add));
	pathImage.loadPng((uint8_t*) Icons::folderOpen, sizeof (Icons::folderOpen));
    
    unsigned i = 0;
    
    for( auto& driveGroup : emulator->driveGroups ) {            
        
        if ( driveGroup.isHardDrive() ) {
            appendHeader("", hdImage);
            
        } else if (driveGroup.isDiskDrive()) {
            appendHeader("", diskImage);
            
        } else if (driveGroup.isTapeDrive()) {
            appendHeader("", tapeImage);
            
        } else if (driveGroup.isModuleSlot()) {
            appendHeader("", moduleImage);
           
		} else if (driveGroup.isMemory()) {
            appendHeader("", memoryImage);
                        
        } else 
            continue;
        
        DriveGroupLayout* driveGroupLayout = new DriveGroupLayout( &driveGroup, tabWindow );
        
        driveGroupLayouts.push_back( driveGroupLayout );
        
        driveGroupLayout->build( );
        
        unsigned counter = settings->get( tabWindow->ident(driveGroup.name + "_count"), driveGroup.defaultUsage());
        
        driveGroupLayout->updateVisibility( counter, true );
        
        tabs.push_back( driveGroup.name + "_drives");
                
        setLayout(i++, *driveGroupLayout, {~0u, ~0u});   
		
		bindSelectorAction( driveGroupLayout );
    }
    
    appendHeader("", addImage); 
    tabs.push_back("create");    
    prepareCreator();
    setLayout(i++, creatorLayout, {~0u, 0u});  
    
    appendHeader("", pathImage); 
    tabs.push_back("paths");
    preparePaths();        
    setLayout(i++, pathsLayout, {~0u, 0u});
    
    setSelection(0);
}

auto DrivesLayout::updateDriveBlock(DriveGroupLayout::Block* block, FileSetting* setting) -> void {

    block->selector.edit.setText( setting->path );
    block->header.fileName.setText( setting->file );
    block->header.writeprotect.setChecked( setting->writeProtect );
    block->header.writeprotect.setEnabled( setting->wpEnabled );
}

auto DrivesLayout::updateListing( Emulator::Interface::Drive* drive ) -> void {    
    
    auto driveGroupLayout = getDriveGroupLayout( drive->group );
    
    if (!driveGroupLayout)
        return;
                
    if ( !showC64Listing( driveGroupLayout ) )
        return;

    for( auto block : driveGroupLayout->blocks ) {

        if ( block->drive == drive ) {

            block->listings = emulator->getListing( drive->group->type, drive->id );

            if ( driveGroupLayout->selectedBlock->drive == drive )
                driveGroupLayout->fillListing( block );

            return;
        }
    }
}

auto DrivesLayout::preselectPath( std::string& groupName ) -> std::string {
	
	auto baseFolderIdent = tabWindow->ident( groupName + "_folder" );

	auto path = settings->get<std::string>( baseFolderIdent, "" );
	
	if ( path == "" )
		path = settings->get<std::string>( baseFolderIdent + "_auto", "" );
	
	return path;
}

auto DrivesLayout::savePath( std::string& groupName, std::string path ) -> void {
	
	auto baseFolderIdent = tabWindow->ident( groupName + "_folder" );
	
	settings->set<std::string>(baseFolderIdent + "_auto", path);
}

auto DrivesLayout::translate() -> void {
    
    unsigned i = 0;
    for(auto& tab : tabs)
        setHeader(i++, trans->get(tab));

    for( auto driveGroupLayout : driveGroupLayouts ) {
        
        auto driveGroup = driveGroupLayout->driveGroup;
        
        driveGroupLayout->setText( trans->get( driveGroup->name + "_selector") );
        driveGroupLayout->inject.setText( trans->get("memory_inject") );

        for ( auto& block : driveGroupLayout->blocks ) {
            block->header.writeprotect.setText(trans->get("write_protected"));
            block->header.eject.setText(trans->get("eject"));
            block->selector.open.setText("...");
            block->selector.openW.setText(trans->get("open_w"));
            
            if (driveGroup->isHardDrive()) {
                block->selector.open.setText(trans->get("open"));    
                
            } else if (driveGroup->isModuleSlot()) {
                block->header.deviceName.setText( trans->get("module") );
                block->selector.open.setText(trans->get("open") );                 
                
            } else if (driveGroup->isMemory()) {
                block->header.deviceName.setText(trans->get("memory"));
                block->selector.open.setText(trans->get("open"));
                block->selector.open.setTooltip(trans->get("c64_list_tip"));
            }
        }        
    }
    
    if (diskCreatorLayout) {        
        diskCreatorLayout->setText( trans->get("disc_creator") );

        diskCreatorLayout->formatName.setText(trans->get("format",{}, true));
        diskCreatorLayout->fastFileSystem.setText(trans->get("ffs"));
        diskCreatorLayout->highDensity.setText(trans->get("high_density"));
        diskCreatorLayout->diskLabelName.setText(trans->get("Name",{}, true));
        diskCreatorLayout->button.setText(trans->get("create"));
    }
    
    if (hdCreatorLayout) {
        hdCreatorLayout->setText( trans->get("hd_creator") );
        
        hdCreatorLayout->creator.diskSizeName.setText( trans->get("size_in_mb", {}, true) );
        hdCreatorLayout->creator.diskLabelName.setText( trans->get("Name", {}, true) );
        hdCreatorLayout->creator.button.setText( trans->get("create") );        
    }
            
    if (tapeCreatorLayout) {
        tapeCreatorLayout->setText( trans->get("tape_creator") );                
        tapeCreatorLayout->button.setText(trans->get("create"));        
    }
	
	if (memoryCreatorLayout) {
		memoryCreatorLayout->setText( trans->get("memory_creator") );		
        memoryCreatorLayout->button.setText(trans->get("create")); 
	}
    
    for(auto block : pathsLayout.blocks) {
        
        block->label.setText( trans->get( block->driveGroup->name + "s" ) );
        block->empty.setText( trans->get("remove") );
        block->select.setText( trans->get("select") );
    }
}

auto DrivesLayout::showC64Listing( DriveGroupLayout* layout ) -> bool {
    
    if ( !dynamic_cast<LIBC64::Interface*>(emulator) )
        return false;
    
    auto driveGroup = layout->driveGroup;
    
    if ( driveGroup->isMemory() || driveGroup->isDiskDrive() )
        return true;
    
    return false;
}

auto DrivesLayout::insertImage( ImageInsertHelper iih ) -> void {
    
    auto item = iih.item;
    auto file = iih.file;
    auto block = iih.block;
    auto driveGroup = iih.driveGroup;   
    
    auto layout = getDriveGroupLayout( driveGroup );
   
    if (!layout)
        return;

    if (!block)
        block = layout->blocks[0];                              
       
    auto drive = block->drive;
    auto setting = FileSetting::getInstance( tabWindow->ident(drive->name) );

    unsigned size = file->archiveDataSize(item->id);

    // unarchived tape files are writable which results in unpredictable filesizes
    // so they are loaded in chunks by an emulator callback
    auto data = driveGroup->isTapeDrive() && !file->isArchived() ? nullptr
        : file->archiveData(item->id);

    bool openWritable = !file->isArchived() && block->openWritable;

    if (!driveGroup->isModuleSlot()) {
        emulator->ejectMedium(driveGroup->type, drive->id);
        
        drive->guid = uintptr_t(file);
        emulator->insertMedium(driveGroup->type, drive->id, data, size);
        emulator->writeProtect(driveGroup->type, drive->id, !openWritable);
        filePool->assign(tabWindow->ident(drive->name), file);
    }

    if (showC64Listing(layout)) {
        block->listings.clear();
        block->listings = emulator->getListing(driveGroup->type, drive->id);        
        block->selector.edit.setFocused();
        layout->fillListing(block);
    }

    if (driveGroup->isTapeDrive())
        view->updateTapeIcons();

    filePool->assign(tabWindow->ident(drive->name + "store"), file);    
    filePool->unloadOrphaned();

    setting->setPath(file->getFile());
    setting->setFile(item->info.name);
    setting->setId(item->id);
    setting->setWriteProtect(!openWritable);
    setting->setWpEnabled(!file->isArchived());

    if (!driveGroup->isModuleSlot())
        States::getInstance(emulator)->updateImage(setting, drive);

    updateDriveBlock(block, setting);  
}

auto DrivesLayout::eject( Emulator::Interface::DriveGroup* driveGroup ) -> void {
    
    auto layout = getDriveGroupLayout( driveGroup );
    
    for( auto block : layout->blocks)
        block->header.eject.onActivate();     
}

auto DrivesLayout::getDriveGroupLayout( Emulator::Interface::DriveGroup* driveGroup ) -> DriveGroupLayout* {
    
    for (auto layout : driveGroupLayouts) {
        
        if (layout->driveGroup == driveGroup)
            return layout;
    }
    
    return nullptr;
}

auto DrivesLayout::showDriveGroupLayout( Emulator::Interface::DriveGroup* driveGroup ) -> void {
    
    unsigned i = 0;
    
    for(auto layout : driveGroupLayouts) {
        
        if (layout->driveGroup == driveGroup) {
            setSelection( i );
            
            break;
        }
        
        i++;
    }
}

auto DrivesLayout::colorListing( unsigned color, bool foreground ) -> void {
    for(auto layout : driveGroupLayouts) {
        if (foreground)
            layout->listings.setForegroundColor( color );
        else
            layout->listings.setBackgroundColor( color );
    }
}

auto DrivesLayout::drop( std::string filePath, DriveGroupLayout::Block* block ) -> void {    
    
    DriveGroupLayout* layout;
    Emulator::Interface::DriveGroup* driveGroup;
    
    if (!block) {
        layout = driveGroupLayouts[ selection() ];
        
        driveGroup = layout->driveGroup; 
        
        block = layout->selectedBlock;
        
    } else {
        driveGroup = block->drive->group;
        
        layout = getDriveGroupLayout( driveGroup );
    }
    
    if (driveGroup->isHardDrive())
        return;

    GUIKIT::File* file = filePool->get(filePath);
    if (!file)
        return;

    if (file->getSize() > MAX_ARCHIVE_SIZE)     
        return program->errorArchiveSize( file, mes );    

    auto& items = file->scanArchive();

    archiveViewer->onCallback = [this, file, block, driveGroup](GUIKIT::File::Item* item) {

        if (!item || (item->info.size == 0) )
            return program->errorOpen( file, item, mes );        

        if (!driveGroup->isTapeDrive() && (item->info.size > MAX_MEDIUM_SIZE))
            return program->errorMediumSize( item, mes );            

        insertImage({file, item, driveGroup, block});
    };

    archiveViewer->setView(items);
}

auto DrivesLayout::updateVisibility( Emulator::Interface::DriveGroup* driveGroup, unsigned count) -> void {
    
    auto layout = getDriveGroupLayout( driveGroup );
    
    if (!layout)
        return;
    
    layout->updateVisibility( count );
}

// DriveGroupLayout
auto DriveGroupLayout::updateVisibility( unsigned count, bool init ) -> void {
    
    bool listingInVisibleBlock = false;
    
    if (!count)
        count = 1;
    
    for(auto block : blocks) {     
        blockContainer.remove(*block);    
    }
    
    for(auto block : blocks) {  
        
        if (count) {
            block->setVisible(false);
            blockContainer.append(*block,{~0u, 0u}, 2);      

            if (!listingInVisibleBlock)
                listingInVisibleBlock = block == selectedBlock;     
            
            count--;
        } else {
            if (!init)
                block->header.eject.onActivate(); 
        }
    }
    
    if (!listingInVisibleBlock)
        fillListing( blocks[0] );	  
}

auto DriveGroupLayout::fillListing( DriveGroupLayout::Block* block ) -> void {

    selectedBlock = block;
            
	listings.reset( );

	for ( auto& listing : block->listings ) {

		std::vector<uint8_t> utf8;

		for ( auto& code : listing.line ) {

			unsigned useCode = code;
			if ( tabWindow->useCustomFont )
				useCode |= 0xee << 8;

			GUIKIT::Utf8::encode( useCode, utf8 );
		}

		std::string str = std::string( (const char*) utf8.data( ), utf8.size( ) );

		listings.append( { str } );
	}
}

auto DriveGroupLayout::build() -> void {

    auto addBlock = [&](std::string& name, Emulator::Interface::Drive* drive) {
        auto block = new Block;
        block->drive = drive;
        block->openWritable = false;
        blocks.push_back(block);
        block->header.deviceName.setText(name + ":");
    };  

    for (auto& drive : driveGroup->drives) {
        addBlock(drive.name, &drive);

        auto& header = blocks[blocks.size() - 1]->header;
        auto& selector = blocks[blocks.size() - 1]->selector;
        
        if (driveGroup->isHardDrive() || driveGroup->isModuleSlot() || driveGroup->isMemory()) {     
            header.remove( header.writeprotect );
            selector.remove( selector.openW );
        }
    }
    
    selectedBlock = blocks[0];
    
    append(blockContainer, {~0u, 0u}, 2);

    if ( dynamic_cast<LIBC64::Interface*>(tabWindow->emulator)) {
		listings.setHeaderText( { "" } );
		listings.setHeaderVisible( false );

        if (tabWindow->useCustomFont)
            listings.setFont("C64 Pro Mono, 12");           
        
        if ( driveGroup->isMemory( ) )
            append( inject, {0u, 0u}, 3 );
            
        if ( driveGroup->isMemory( ) || driveGroup->isDiskDrive() )
            append( listings, {~0u, ~0u} );
	}
}
