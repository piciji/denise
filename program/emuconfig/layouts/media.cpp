
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
}

MediaGroupLayout::Block::Header::Header() {
    deviceName.setFont(GUIKIT::Font::system("bold"));
    inUse.setFont(GUIKIT::Font::system("bold"));
    append(inUse, {0u, 0u}, 5);
    append(deviceName, {0u, 0u}, 10);
    append(writeprotect, {0u, 0u}, 10);
    append(eject, {0u, 0u}, 10);
    append(fileName, {~0u, 0u});
    setAlignment(0.5);
}

MediaGroupLayout::Block::Selector::Selector() {    
    append(edit, {~0u, 0u}, 10);
    append(combo, {0u, 0u}, 10);
    append(open, {0u, 0u});
    append(spacer, {0u, 0u}, 5);
    append(openW, {0u, 0u});
    setAlignment(0.5);
    edit.setEditable(false);
    edit.setDroppable();
}

MediaGroupLayout::Block::Block() {
    append(header, {~0u, 0u}, 2);
    append(selector, {~0u, 0u});
}

MediaGroupLayout::MediaGroupLayout( Emulator::Interface::MediaGroup* mediaGroup, TabWindow* tabWindow ) {
    this->mediaGroup = mediaGroup;
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

CartCreatorLayout::CartCreatorLayout() {
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

auto MediaLayout::bindSelectorAction(MediaGroupLayout* layout) -> void {
	
    auto mediaGroup = layout->mediaGroup;
        
	for (auto block : layout->blocks) {

		auto setting = FileSetting::getInstance(tabWindow->ident(block->media->name));
					
		if (mediaGroup->isHardDisk()) {

			block->selector.open.onActivate = [this, block, mediaGroup, setting]() {
				
				std::string filePath = GUIKIT::BrowserWindow()
                    .setWindow(*tabWindow)
                    .setTitle(trans->get("select_" + mediaGroup->name + "_image"))
                    .setPath( preselectPath( mediaGroup->name ) )
                    .setFilters({ GUIKIT::BrowserWindow::transformFilter(trans->get(mediaGroup->name + "_image"), mediaGroup->suffix), trans->get("all_files")})
                    .open();

				if (filePath.empty())
					return;

				block->header.eject.onActivate();
				GUIKIT::File testFile(filePath);
                
				savePath( mediaGroup->name, testFile.getPath() );

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
            
			block->selector.open.onActivate = [this, block, mediaGroup, setting, layout]() {                

                auto suffix = mediaGroup->suffix;
                GUIKIT::Vector::combine(suffix, GUIKIT::File::suppportedCompressionExtensions());    
                
				std::string filePath = GUIKIT::BrowserWindow()
					.setWindow(*tabWindow)
					.setTitle(trans->get("select_" + mediaGroup->name + "_image"))
					.setPath( preselectPath( mediaGroup->name ) )
					.setFilters({ GUIKIT::BrowserWindow::transformFilter(trans->get(mediaGroup->name + "_image"), suffix ),
						trans->get("all_files")})
					.open();

				if (filePath.empty())
					return;

				GUIKIT::File* file = filePool->get(filePath);
				if (!file)
					return;

				savePath( mediaGroup->name, file->getPath() );
                
				if (file->getSize() > MAX_ARCHIVE_SIZE)
                    return program->errorArchiveSize( file, mes );				
                
                if ( block->openWritable && file->isArchived())
                    mes->warning(trans->get("archive_wp_tooltip"));
                
				auto& items = file->scanArchive();

				archiveViewer->onCallback = [this, file, block, layout](GUIKIT::File::Item* item) {

                    if (!item || (item->info.size == 0) )
                        return program->errorOpen( file, item, mes );                     
                    
                    if (!layout->mediaGroup->isTape() && (item->info.size > MAX_MEDIUM_SIZE))
                        return program->errorMediumSize( item, mes );                    
                    
                    insertImage( layout, block, file, item );
				};
				archiveViewer->setView(items);
			};

			block->header.eject.onActivate = [this, mediaGroup, block, setting, layout]() {
                
                auto media = block->media;
                
				if ( !mediaGroup->isExpansion() ) {
					emulator->ejectMedium(media);					
					filePool->assign(tabWindow->ident(media->name), nullptr);
                    States::getInstance( emulator )->updateImage( nullptr, media );
				} else
                    States::getInstance(emulator)->forcePowerNextLoad = true;
                
                // an expansion can't be removed while emulation is running.
                // but we have to cut the file link or EasyFlash could
                // write back data, even when user has removed file from UI.
                media->guid = (uintptr_t)nullptr;
                
				
				if ( showC64Listing( layout, block ) ) {
                    block->listings.clear();
                    
                    if (layout->selectedBlock->media == media)
                        layout->listings.reset();
                }                
				
				if (mediaGroup->isTape())
					view->updateTapeIcons();
				
				filePool->assign(tabWindow->ident(media->name + "store"), nullptr);
                filePool->unloadOrphaned();

				setting->init();
				updateMediaBlock(block, setting);
			};

			block->header.writeprotect.onToggle = [this, block, setting, mediaGroup]() {
				
				bool state = block->header.writeprotect.checked();

				if (!state && !setting->wpEnabled) {
                    block->header.writeprotect.setChecked();
                    block->header.writeprotect.setEnabled(false);
                    return;
				}

				emulator->writeProtect(block->media, state);
				setting->setWriteProtect(state);
                States::getInstance( emulator )->updateImage( setting, block->media );
			};
			
			updateMediaBlock(block, setting);
            
            block->selector.edit.onFocus = [this, layout, block]() {
                
                layout->selectedBlock = block;
                if ( showC64Listing( layout, block ) ) {
                    layout->fillListing( block );
                }
            };
            
            block->selector.edit.onDrop = [this, layout, block]( std::vector<std::string> files ) {
                
                drop( files[0], block );
            };
            
            block->header.inUse.onActivate = [this, layout, block]() {
                
                layout->mediaGroup->selected = block->media;
                
                settings->set<unsigned>(tabWindow->ident(layout->mediaGroup->name + "_selected"), block->media->id);
            };
            
            block->selector.combo.onChange = [this, layout, block]() {
                
                int userData = block->selector.combo.userData();
                
                for( auto& pcb : layout->mediaGroup->getExpansion()->pcbs ) {
                    
                    if (pcb.id == userData) {
                        
                        block->media->pcbLayout = &pcb; 
                        
                        settings->set<unsigned>(tabWindow->ident(block->media->name + "_pcb"), pcb.id);
                        
                        break;
                    }
                }                                
            };
		}

		if ( showC64Listing( layout, block ) ) { //preload last listing
			GUIKIT::File* file = filePool->get( setting->path );
			uint8_t* data;

            if (program->loadImageDataWhenOk(file, setting->id, mediaGroup, data)) {
				filePool->assign(tabWindow->ident(block->media->name + "store"), file);
                emulator->insertMedium(block->media, data, file->archiveDataSize( setting->id ));
                block->listings = emulator->getListing( block->media );
                if (block->media->id == 0)
                    layout->fillListing( block );				
			}
		}
	}
    
    if ( showC64Listing( layout ) ) {

        layout->listings.onActivate = [this, layout]( ) {
            auto selection = layout->listings.selection( );
            program->power( emulator );
            
            if (layout->mediaGroup->isMemory()) {
                for (auto& media : layout->mediaGroup->media)
                    emulator->selectListing(&media, layout->listings.selection() );                                        
            } else
                emulator->selectListing( layout->selectedBlock->media, selection );
            
            view->setFocused(300);
        };

        layout->inject.onActivate = [this, layout]() {

            if (!layout->listings.selected())
                return;
            
            bool injected = true;
            
            for (auto& media : layout->mediaGroup->media) {                
                if ( !emulator->selectListing( &media, layout->listings.selection( ) ) )
                    injected = false;
            }
            
            if ( injected ) {
                status->addMessage( trans->get( "memory_injected" ) );
                view->setFocused( 300 );
            }	
        };
    }
}

auto MediaLayout::createImage( Emulator::Interface::MediaGroup* mediaGroup ) -> void {

    if (mediaGroup->isExpansion()) {
        if (mediaGroup->getExpansion()->isFlash()) {
            unsigned groupId = flashCreatorLayout->format.userData();
            mediaGroup = &emulator->mediaGroups[groupId];
        }
    }
    
    std::string title = mediaGroup->name + "_image";
    std::string suffix = mediaGroup->creatable[0];
    GUIKIT::File file;
    GUIKIT::File* filePtr;
    std::string filePath;
    uint8_t* data = nullptr;
    unsigned size = 0;

    if (mediaGroup->isHardDisk()) {

        try {
            size = std::stoi( hdCreatorLayout->creator.diskSize.text() );
            if (size > 4095)
                throw "";
            size = size * 1024u * 1024u;
        } catch (...) {
            mes->error(trans->get("invalid_input"));
            return;
        }        
        
    } else if (mediaGroup->isDisk()) {
        suffix = diskCreatorLayout->format.text();
        
        unsigned typeId = diskCreatorLayout->format.userData();
        bool hd = diskCreatorLayout->highDensity.checked();
        
        data = emulator->createDiskImage( typeId, hd,
            diskCreatorLayout->diskLabel.text(),
            diskCreatorLayout->fastFileSystem.checked()
        );
        
        size = emulator->getDiskImageSize(typeId, hd);
        
    } else if (mediaGroup->isTape()) {
        data = emulator->createTapeImage( size );
        
    } else if (mediaGroup->isMemory()) {
        data = emulator->getLoadedMemory( size );
        
    } else if (mediaGroup->isExpansion()) {
        data = emulator->createExpansionImage( mediaGroup, size );
    }
    
    if (!size)
        goto Done; //internal error
    
    filePath = GUIKIT::BrowserWindow()
        .setWindow(*tabWindow)
        .setTitle(trans->get( "blank_" + title ))
        .setPath(preselectPath( mediaGroup->name ))
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

    savePath( mediaGroup->name, file.getPath() );

    if (data) {
        if (!file.write( data, size )) {
            mes->error(trans->get("file_creation_error",{
                {"%path%", filePath}
            }));

            goto Done;
        }

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
            emulator->createHardDisk(onCreate, size, hdCreatorLayout->creator.diskLabel.text() );
            hdCreatorLayout->creator.button.setEnabled();
        });
        t1.detach();
    }
            
    Done:
        if (data)
            delete[] data;                   
}

auto MediaLayout::prepareCreator() -> void {

    for (auto& mediaGroup : emulator->mediaGroups) {
		Emulator::Interface::MediaGroup* group = &mediaGroup;
        
        if (mediaGroup.isHardDisk()) {

            hdCreatorLayout = new HdCreatorLayout;

            hdCreatorLayout->creator.button.onActivate = [this, group]() {
                createImage( group );
            };

            creatorLayout.append(*hdCreatorLayout, {~0u, 0u}, 5);

        } else if (mediaGroup.isDisk()) {

            diskCreatorLayout = new DiskCreatorLayout(emulator, mediaGroup.creatable );

            diskCreatorLayout->button.onActivate = [this, group]() {
                createImage( group );                
            };

            creatorLayout.append(*diskCreatorLayout, {~0u, 0u}, 5);

        } else if (mediaGroup.isTape()) {

            tapeCreatorLayout = new TapeCreatorLayout;

            tapeCreatorLayout->button.onActivate = [this, group]() {
                createImage( group );                
            };

            creatorLayout.append(*tapeCreatorLayout, {~0u, 0u}, 5);
			
        } else if (mediaGroup.isMemory()) {
			
			memoryCreatorLayout = new MemoryCreatorLayout;
			
			memoryCreatorLayout->button.onActivate = [this, group]() {
                createImage( group );
			};
			
			creatorLayout.append(*memoryCreatorLayout, {~0u, 0u}, 5);
            
		} else if (mediaGroup.isExpansion() && mediaGroup.getExpansion()->isFlash() ) {
			
            if (!flashCreatorLayout) {            
                flashCreatorLayout = new CartCreatorLayout;

                flashCreatorLayout->button.onActivate = [this, group]() {
                    createImage( group );
                };

                creatorLayout.append(*flashCreatorLayout, {~0u, 0u}, 5);
            }
            
            flashCreatorLayout->format.append( mediaGroup.name, mediaGroup.id );  
		}
    }
}

auto MediaLayout::preparePaths() -> void {
    
    for (auto& mediaGroup : emulator->mediaGroups) {
        
        auto settingFolderIdent = tabWindow->ident( mediaGroup.name + "_folder" );
        
        auto block = new PathsLayout::Block( &mediaGroup );
        
        pathsLayout.blocks.push_back( block );                
        pathsLayout.append( *block,{~0u, 0u}, &mediaGroup != &emulator->mediaGroups.back() ? 5 : 0 );
        
        std::string title = "select_" + mediaGroup.name + "_folder";
        
        if (mediaGroup.isExpansion())
            title = "select_module_folder";        

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

MediaLayout::MediaLayout(TabWindow* tabWindow) {

    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;
    
    setPadding(10);
    setMargin(10);

    diskImage.loadPng((uint8_t*) Icons::disk, sizeof (Icons::disk));
    hdImage.loadPng((uint8_t*) Icons::drive, sizeof (Icons::drive));
	tapeImage.loadPng((uint8_t*) Icons::tape, sizeof (Icons::tape));
	expansionImage.loadPng((uint8_t*) Icons::memory, sizeof (Icons::memory));
	memoryImage.loadPng((uint8_t*) Icons::memory, sizeof (Icons::memory));
    addImage.loadPng((uint8_t*) Icons::add, sizeof (Icons::add));
	pathImage.loadPng((uint8_t*) Icons::folderOpen, sizeof (Icons::folderOpen));
    
    unsigned i = 0;
    
    for( auto& mediaGroup : emulator->mediaGroups ) {            
        
        if ( mediaGroup.isHardDisk() ) {
            appendHeader("", hdImage);
            
        } else if (mediaGroup.isDisk()) {
            appendHeader("", diskImage);
            
        } else if (mediaGroup.isTape()) {
            appendHeader("", tapeImage);
            
        } else if (mediaGroup.isExpansion()) {
            appendHeader("", expansionImage);
           
		} else if (mediaGroup.isMemory()) {
            appendHeader("", memoryImage);
            
        } else 
            continue;
        
        MediaGroupLayout* mediaGroupLayout = new MediaGroupLayout( &mediaGroup, tabWindow );
        
        mediaGroupLayouts.push_back( mediaGroupLayout );
        
        mediaGroupLayout->build( );
        
        if (mediaGroupLayout->showOnlyConnectedDevices()) {
            
            unsigned counter = settings->get( tabWindow->ident(mediaGroup.name + "_count"), 1);
            
            mediaGroupLayout->updateVisibility( counter, true );
        }        
        
        tabs.push_back( getMediaGroupTransIdent(&mediaGroup) );
                
        setLayout(i++, *mediaGroupLayout, {~0u, ~0u});   
		
		bindSelectorAction( mediaGroupLayout );
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

auto MediaLayout::updateMediaBlock(MediaGroupLayout::Block* block, FileSetting* setting) -> void {

    block->selector.edit.setText( setting->path );
    block->header.fileName.setText( setting->file );
    block->header.writeprotect.setChecked( setting->writeProtect );
    block->header.writeprotect.setEnabled( setting->wpEnabled );
}

auto MediaLayout::updateListing( Emulator::Interface::Media* media ) -> void {    
    
    auto mediaGroupLayout = getMediaGroupLayout( media->group );
    
    if (!mediaGroupLayout)
        return;
                
    if ( !showC64Listing( mediaGroupLayout, mediaGroupLayout->getBlock( media ) ) )
        return;

    for( auto block : mediaGroupLayout->blocks ) {

        if ( block->media == media ) {

            block->listings = emulator->getListing( media );

            if ( mediaGroupLayout->selectedBlock->media == media )
                mediaGroupLayout->fillListing( block );

            return;
        }
    }
}

auto MediaLayout::preselectPath( std::string& groupName ) -> std::string {
	
	auto baseFolderIdent = tabWindow->ident( groupName + "_folder" );

	auto path = settings->get<std::string>( baseFolderIdent, "" );
	
	if ( path == "" )
		path = settings->get<std::string>( baseFolderIdent + "_auto", "" );
	
	return path;
}

auto MediaLayout::savePath( std::string& groupName, std::string path ) -> void {
	
	auto baseFolderIdent = tabWindow->ident( groupName + "_folder" );
	
	settings->set<std::string>(baseFolderIdent + "_auto", path);
}

auto MediaLayout::translate() -> void {
    
    unsigned i = 0;
    for(auto& tab : tabs)
        setHeader(i++, trans->get(tab));

    for( auto mediaGroupLayout : mediaGroupLayouts ) {
        
        auto mediaGroup = mediaGroupLayout->mediaGroup;
        
        mediaGroupLayout->setText( trans->get( mediaGroup->name + "_insert") );
        mediaGroupLayout->inject.setText( trans->get("memory_inject") );

        for ( auto block : mediaGroupLayout->blocks ) {
            block->header.writeprotect.setText(trans->get("write_protected"));
            block->header.eject.setText(trans->get("eject"));
            block->header.deviceName.setText( trans->get( block->media->name, {}, true ) );            
            block->header.inUse.setText( trans->get( block->media->name, {}, true ) );            
            
            if (mediaGroup->isWritable()) {
                block->selector.open.setText("...");
                block->selector.openW.setText(trans->get("open_w"));                
            } else {
                block->selector.open.setText(trans->get("open") );                 
            }
                
            if (mediaGroup->isMemory()) {                
                block->selector.open.setTooltip(trans->get("c64_list_tip"));
            }
            
            if (mediaGroup->isExpansion()) {
                unsigned id = 0;
                for( auto& pcb : mediaGroup->getExpansion()->pcbs ) {
                    block->selector.combo.setText(id++, trans->get( pcb.name ));
                }
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
    
    if (flashCreatorLayout) {
		flashCreatorLayout->setText( trans->get("flash_creator") );		
        flashCreatorLayout->button.setText(trans->get("create"));         
    }
    
    for(auto block : pathsLayout.blocks) {
        
        block->label.setText( trans->get( getMediaGroupTransIdent(block->mediaGroup) ) );
        block->empty.setText( trans->get("remove") );
        block->select.setText( trans->get("select") );
    }
}

auto MediaLayout::getMediaGroupTransIdent( Emulator::Interface::MediaGroup* mediaGroup ) -> std::string {
    auto ident = mediaGroup->name;
    
    if (mediaGroup->isDrive() || (mediaGroup->isExpansion() && mediaGroup->getExpansion()->isGame()) )
        ident += "s";
    
    return ident;
}

auto MediaLayout::showC64Listing( MediaGroupLayout* layout, MediaGroupLayout::Block* block ) -> bool {
    
    if ( !dynamic_cast<LIBC64::Interface*>(emulator) )
        return false;
    
    auto mediaGroup = layout->mediaGroup;
    
    if ( mediaGroup->isDisk() )
        return true;
    
    if ( mediaGroup->isMemory() )
        if (!block || (layout->blocks[0] == block) )
            return true;
    
    return false;
}

auto MediaLayout::insertImage(Emulator::Interface::Media* media, GUIKIT::File* file, GUIKIT::File::Item* item) -> void {
    
    auto layout = getMediaGroupLayout( media->group );
    
    if (!layout)
        return;

    for( auto block : layout->blocks ) {
        
        if (block->media == media) {
            insertImage( layout, block, file, item );
            break;
        }
    }        
}

auto MediaLayout::insertImage( MediaGroupLayout* layout, MediaGroupLayout::Block* block, GUIKIT::File* file, GUIKIT::File::Item* item ) -> void {
   
    if (!layout)
        return;

    if (!block)
        block = layout->blocks[0];                              
       
    auto media = block->media;
    auto mediaGroup = layout->mediaGroup;
    auto setting = FileSetting::getInstance( tabWindow->ident(media->name) );

    unsigned size = file->archiveDataSize(item->id);

    // unarchived tape files are writable which results in unpredictable filesizes
    // so they are loaded in chunks by an emulator callback
    auto data = mediaGroup->isTape() && !file->isArchived() ? nullptr
        : file->archiveData(item->id);

    bool openWritable = !file->isArchived() && block->openWritable;

    if (!mediaGroup->isExpansion()) {
        emulator->ejectMedium(media);
        
        media->guid = uintptr_t(file);
        emulator->insertMedium(media, data, size);
        emulator->writeProtect(media, !openWritable);
        filePool->assign(tabWindow->ident(media->name), file);
    } else {
        
        if (block->selector.combo.visible()) {
            block->selector.combo.setSelection(0);
            block->selector.combo.onChange();
        }
    }

    if (showC64Listing(layout, block)) {
        block->listings.clear();
        block->listings = emulator->getListing(media);        
        block->selector.edit.setFocused();
        layout->fillListing(block);
    }

    if (mediaGroup->isTape())
        view->updateTapeIcons();
    
    if (mediaGroup->selected && !block->header.inUse.checked() ) {
        block->header.inUse.setChecked();
        block->header.inUse.onActivate();
    }

    filePool->assign(tabWindow->ident(media->name + "store"), file);    
    filePool->unloadOrphaned();

    setting->setPath(file->getFile());
    setting->setFile(item->info.name);
    setting->setId(item->id);
    setting->setWriteProtect(!openWritable);
    setting->setWpEnabled(!file->isArchived());

    if (!mediaGroup->isExpansion())
        States::getInstance(emulator)->updateImage(setting, media);
    else
        States::getInstance(emulator)->forcePowerNextLoad = true;

    updateMediaBlock(block, setting);  
}

auto MediaLayout::eject( Emulator::Interface::MediaGroup* mediaGroup ) -> void {
    
    auto layout = getMediaGroupLayout( mediaGroup );
    
    for( auto block : layout->blocks)
        block->header.eject.onActivate();     
}

auto MediaLayout::getMediaGroupLayout( Emulator::Interface::MediaGroup* mediaGroup ) -> MediaGroupLayout* {
    
    for (auto layout : mediaGroupLayouts) {
        
        if (layout->mediaGroup == mediaGroup)
            return layout;
    }
    
    return nullptr;
}

auto MediaLayout::showMediaGroupLayout( Emulator::Interface::MediaGroup* mediaGroup ) -> void {
    
    unsigned i = 0;
    
    for(auto layout : mediaGroupLayouts) {
        
        if (layout->mediaGroup == mediaGroup) {
            setSelection( i );
            
            break;
        }
        
        i++;
    }
}

auto MediaLayout::colorListing( unsigned color, bool foreground ) -> void {
    for(auto layout : mediaGroupLayouts) {
        if (foreground)
            layout->listings.setForegroundColor( color );
        else
            layout->listings.setBackgroundColor( color );
    }
}

auto MediaLayout::drop( std::string filePath, MediaGroupLayout::Block* block ) -> void {    
    
    MediaGroupLayout* layout;
    Emulator::Interface::MediaGroup* mediaGroup;
    
    if (!block) {
        layout = mediaGroupLayouts[ selection() ];
        
        mediaGroup = layout->mediaGroup; 
        
        block = layout->selectedBlock;
        
    } else {
        mediaGroup = block->media->group;
        
        layout = getMediaGroupLayout( mediaGroup );
    }
    
    if (mediaGroup->isHardDisk())
        return;

    GUIKIT::File* file = filePool->get(filePath);
    if (!file)
        return;

    if (file->getSize() > MAX_ARCHIVE_SIZE)     
        return program->errorArchiveSize( file, mes );    

    auto& items = file->scanArchive();

    archiveViewer->onCallback = [this, file, block, layout](GUIKIT::File::Item* item) {

        if (!item || (item->info.size == 0) )
            return program->errorOpen( file, item, mes );        

        if (!layout->mediaGroup->isTape() && !layout->mediaGroup->isMemory() && (item->info.size > MAX_MEDIUM_SIZE))
            return program->errorMediumSize( item, mes );            

        insertImage( layout, block, file, item );        
    };

    archiveViewer->setView(items);
}

auto MediaLayout::updateVisibility( Emulator::Interface::MediaGroup* mediaGroup, unsigned count) -> void {
    
    auto layout = getMediaGroupLayout( mediaGroup );
    
    if (!layout)
        return;
    
    layout->updateVisibility( count );
}

auto MediaLayout::disableWriteProtection(Emulator::Interface::Media* media) -> void {
    
    auto layout = getMediaGroupLayout(media->group);
    
    if (!layout)
        return;
    
    for(auto block : layout->blocks) {
        
        if (block->media == media) {
                        
            if (block->header.writeprotect.checked()) {                
                block->header.writeprotect.setChecked(false);
                block->header.writeprotect.onToggle();
            }
                               
            break;
        }
    }
}

// MediaGroupLayout
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
            blockContainer.append(*block,{~0u, 0u}, 4);      
            
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

auto MediaGroupLayout::getBlock(Emulator::Interface::Media* media) -> Block* {
    for( auto block : blocks ) {
        
        if (block->media == media)
            return block;        
    } 
    
    return nullptr;
}

auto MediaGroupLayout::fillListing( MediaGroupLayout::Block* block ) -> void {

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

auto MediaGroupLayout::showOnlyConnectedDevices() -> bool {
    
    return mediaGroup->isDrive() && mediaGroup->media.size() > 1;
}

auto MediaGroupLayout::build() -> void {

    auto addBlock = [&](Emulator::Interface::Media* media) -> MediaGroupLayout::Block* {
        auto block = new Block;
        block->media = media;
        block->openWritable = false;
        blocks.push_back(block);
        
        if ( !showOnlyConnectedDevices() )
            blockContainer.append(*block, {~0u, 0u}, 4);
            
        return block;
    };  

    std::vector<GUIKIT::RadioBox*> radioGroup;
    
    for (auto& media : mediaGroup->media) {
        auto block = addBlock(&media);

        auto& header = block->header;
        auto& selector = block->selector;
        
        if (!mediaGroup->isWritable()) {
            header.remove( header.writeprotect );
            selector.remove( selector.spacer );
            selector.remove( selector.openW );
        }
        
        if (!mediaGroup->selected)
            header.remove( header.inUse );
        else {
            header.remove( header.deviceName );
            radioGroup.push_back( &header.inUse );                   
        }
        
        if (!mediaGroup->isExpansion() || (mediaGroup->getExpansion()->pcbs.size() == 0) )
            selector.remove( selector.combo );
        else {
            for (auto& pcb : mediaGroup->getExpansion()->pcbs) {
                selector.combo.append( pcb.name, pcb.id );

                if (media.pcbLayout && (media.pcbLayout == &pcb) )
                    selector.combo.setSelection( selector.combo.rows() - 1 );
            }
        }        
    }
    
    if (radioGroup.size()) {
        GUIKIT::RadioBox::setGroup( radioGroup );
        for (auto block : blocks) {
            if (mediaGroup->selected == block->media)
                block->header.inUse.setChecked();
        }
    }
    
    selectedBlock = blocks[0];
    
    append(blockContainer, {~0u, 0u}, 2);

    if ( dynamic_cast<LIBC64::Interface*>(tabWindow->emulator)) {
		listings.setHeaderText( { "" } );
		listings.setHeaderVisible( false );

        if (tabWindow->useCustomFont)
            listings.setFont("C64 Pro Mono, 12");           
        
        if ( mediaGroup->isMemory( ) )
            append( inject, {0u, 0u}, 3 );
            
        if ( mediaGroup->isMemory( ) || mediaGroup->isDisk() )
            append( listings, {~0u, ~0u} );
	}
}
