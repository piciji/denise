
#include "media.h"
#include "../cmd/cmd.h"
#include "../view/view.h"
#include "../view/message.h"
#include "../view/status.h"
#include "../tools/filepool.h"
#include "../tools/filesetting.h"
#include "../config/archiveViewer.h"
#include "../states/states.h"
#include "autoloader.h"
#include "fileloader.h"
#include "../firmware/manager.h"
#include "../../data/resource.h"
#include "../../data/icons.h"
#include "../thread/emuThread.h"

#include <thread>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace MediaView {

#include "layout.cpp"
#include "swapper.cpp"

MediaLayout::FontSizeLayout::FontSizeLayout() {
    append(label, {0u, 0u}, 10);
    append(combo, {0u, 0u});

    setAlignment(0.5);
}

MediaLayout::MediaLayout(EmuConfigView::TabWindow* tabWindow) {
    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;
    this->message = tabWindow->message;     
    this->settings = program->getSettings( emulator );
}

auto MediaLayout::setMediaView() -> void {
       
    GUIKIT::TreeViewItem* diskItem = nullptr;
    
    if (expansionParent && expansionParent->selected())
        return;    
    
    for(auto& nav : navElements) {
                
        if (nav.mediaGroupLayout) {
            if (nav.tvi->selected())
                return;

            if (nav.mediaGroupLayout->mediaGroup->isDisk())
                diskItem = nav.tvi;
        }        
    }

    if (diskItem) {
        diskItem->setSelected();
        mediaTree.onChange();
    }
}

auto MediaLayout::setDiskSwapperView() -> void {
    
    for(auto& nav : navElements) {

        if ( nav.altLayout && (nav.altLayout == swapperLayout) ) {
            if (!nav.tvi->selected()) {
                nav.tvi->setSelected();
                mediaTree.onChange();
            }

            return;
        }      
    }
}

auto MediaLayout::updateSwitchLayout() -> void {
    auto item = mediaTree.selected();

    if (!item)
        return;

    unsigned navPos = (unsigned)item->userData();
		
    moduleSwitch.setSelection( navPos );
}

auto MediaLayout::updateOptionsVisibility() -> void {

    auto item = mediaTree.selected();
    bool _enabledExpansion = false;
    bool _enabledTraps = false;

    if (item) {
        unsigned navPos = (unsigned)item->userData();

        if (navPos < navElements.size()) {
            auto& navElement = navElements[navPos];

            if (navElement.mediaGroupLayout && navElement.mediaGroupLayout->mediaGroup->isDisk()) {
                _enabledTraps = true;
                useTraps.setChecked( settings->get<bool>("use_disk_traps", false) );
            } else if (navElement.mediaGroupLayout && navElement.mediaGroupLayout->mediaGroup->isTape()) {
                _enabledTraps = true;
                useTraps.setChecked( settings->get<bool>("use_tape_traps", false) );
            } else if (navElement.mediaGroupLayout && navElement.mediaGroupLayout->mediaGroup->isExpansion())
                _enabledExpansion = true;
        }
    }

    if (useTraps.enabled() != _enabledTraps) {
        useTraps.setEnabled( _enabledTraps );
    }

    if (bootCart.enabled() != _enabledExpansion) {
        bootCart.setEnabled( _enabledExpansion );
        deactivateCart.setEnabled( _enabledExpansion );
    }
}

auto MediaLayout::build() -> void {

    setMargin(10);

    diskImage.loadPng((uint8_t*) Icons::disk, sizeof (Icons::disk));
    hdImage.loadPng((uint8_t*) Icons::drive, sizeof (Icons::drive));
	tapeImage.loadPng((uint8_t*) Icons::tape, sizeof (Icons::tape));
	expansionImage.loadPng((uint8_t*) Icons::memory, sizeof (Icons::memory));
	memoryImage.loadPng((uint8_t*) Icons::memory, sizeof (Icons::memory));
    addImage.loadPng((uint8_t*) Icons::add, sizeof (Icons::add));
	pathImage.loadPng((uint8_t*) Icons::folderOpen, sizeof (Icons::folderOpen));       
    swapperImage.loadPng((uint8_t*) Icons::swapper, sizeof (Icons::swapper));
    
    imgFolderOpen.loadPng((uint8_t*)Icons::folderOpen, sizeof(Icons::folderOpen) );
    imgFolderClosed.loadPng((uint8_t*)Icons::folderClosed, sizeof(Icons::folderClosed) );
    imgDocument.loadPng((uint8_t*)Icons::document, sizeof(Icons::document) );
    settingsImage.loadPng((uint8_t*)Icons::settings, sizeof(Icons::settings) );

    openImg.loadPng((uint8_t*)Icons::open, sizeof(Icons::open) );
    ejectImg.loadPng((uint8_t*)Icons::eject, sizeof(Icons::eject) );

    GUIKIT::TreeViewItem* tvi;
    unsigned previewFontSize = settings->get<unsigned>("software_preview_fontsize", 12, {8, 16});
    
    for( auto& mediaGroup : emulator->mediaGroups ) {            
        tvi = new GUIKIT::TreeViewItem;
                
        if ( mediaGroup.isHardDisk() ) {
            tvi->setText( "hd" );
            tvi->setImage( hdImage );            
            
        } else if (mediaGroup.isDisk()) {
            tvi->setText( "disk" );
            tvi->setImage( diskImage );
            
        } else if (mediaGroup.isTape()) {
            tvi->setText( "tape" );
            tvi->setImage( tapeImage ); 
            
        } else if (mediaGroup.isExpansion()) {
            if (!expansionParent) {
                expansionParent = new GUIKIT::TreeViewItem;                
                expansionParent->setText( "expansion" );
                expansionParent->setImage( imgFolderClosed );
                expansionParent->setImageExpanded( imgFolderOpen );
				expansionParent->setExpanded();
                expansionParent->setUserData( (uintptr_t)(navElements.size()) );
                mediaTree.append( *expansionParent );
            }
            
            tvi->setText( "expansion" );
            tvi->setImage( memoryImage );
            
            expansionParent->append( *tvi );
           
		} else if (mediaGroup.isProgram()) {
            tvi->setText( "memory" );
            tvi->setImage( memoryImage );
            
        } else {
            delete tvi;
            continue;
        }                                
        
        if (!mediaGroup.isExpansion())
            mediaTree.append(*tvi);
        
        MediaGroupLayout* mediaGroupLayout = new MediaGroupLayout( &mediaGroup, this );                
        
        tvi->setUserData( (uintptr_t)(navElements.size()) );
        
        navElements.push_back( { tvi, mediaGroupLayout, nullptr } );
        
        mediaGroupLayout->build(previewFontSize);
        
        if (mediaGroupLayout->showOnlyConnectedDevices()) {

            auto modelId = emulator->getModelIdOfEnabledDrives(&mediaGroup);

            unsigned counter = emulator->getModelValue( modelId );

            mediaGroupLayout->updateVisibility( counter, true );
        }        
                
        moduleSwitch.setLayout( navElements.size() - 1, *mediaGroupLayout, {~0u, ~0u} );
        
		bindSelectorAction( mediaGroupLayout );
    }

    moduleFrame.append( mediaTree, { GUIKIT::Font::scale(170), ~0u}, 10 );
    if (dynamic_cast<LIBC64::Interface*>(emulator)) {
        moduleFrame.append(useTraps, {0u, 0u}, 10);
    }

    for(unsigned i = 8; i <= 16; i++) {
        fontSizeLayout.combo.append(std::to_string(i), i);
    }
    fontSizeLayout.combo.setSelectionByUserId((int)previewFontSize);
    dialogPreviewLayout.updateWidgets(settings, emulator);

    moduleFrame.append(fontSizeLayout, {0u, 0u}, 10);
    if (emulator->expansions.size() > 1) {
        moduleFrame.append(bootCart, {0u, 0u}, 10);
        moduleFrame.append(deactivateCart, {0u, 0u});
    }
    moduleFrame.setPadding(10);
    moduleFrame.setFont(GUIKIT::Font::system("bold"));

    append( moduleFrame, {0u, ~0u}, 10 );
    append( moduleSwitch, {~0u, ~0u} );
    
    mediaTree.onChange = [this]() {

        if (fileloader->fileDialogPtr && fileloader->fileDialogPtr->visible()) {
            fileloader->resetPreview(emulator);
        }
        
        updateSwitchLayout();
        updateOptionsVisibility();
    };
		
    tvi = new GUIKIT::TreeViewItem;
    tvi->setText( "swapper" );
    tvi->setImage( swapperImage );    
    mediaTree.append(*tvi);
    swapperLayout = new SwapperLayout(this);
    moduleSwitch.setLayout( navElements.size(), *swapperLayout, {~0u, ~0u} );
    tvi->setUserData( (uintptr_t)(navElements.size() ) );
    navElements.push_back( { tvi, nullptr, (Layout*)swapperLayout } );

    tvi = new GUIKIT::TreeViewItem;
    tvi->setText( "create" );
    tvi->setImage( addImage );    
    mediaTree.append(*tvi);    
    prepareCreator();
    moduleSwitch.setLayout( navElements.size(), creatorLayout, {~0u, ~0u} );
    tvi->setUserData( (uintptr_t)(navElements.size() ) );
    navElements.push_back( { tvi, nullptr, (Layout*)&creatorLayout } );
    
    tvi = new GUIKIT::TreeViewItem;
    tvi->setText( "paths" );
    tvi->setImage( imgDocument );    
    mediaTree.append(*tvi);    
    preparePaths();
    moduleSwitch.setLayout( navElements.size(), pathsLayout, {~0u, ~0u} );    
    tvi->setUserData( (uintptr_t)(navElements.size() ) );
    navElements.push_back( { tvi, nullptr, (Layout*)&pathsLayout } );

    tvi = new GUIKIT::TreeViewItem;
    tvi->setText( "dialog preview" );
    tvi->setImage( settingsImage );
    mediaTree.append(*tvi);
    moduleSwitch.setLayout( navElements.size(), dialogPreviewLayout, {~0u, ~0u} );
    tvi->setUserData( (uintptr_t)(navElements.size() ) );
    navElements.push_back( { tvi, nullptr, (Layout*)&dialogPreviewLayout } );

    auto videoManager = VideoManager::getInstance(emulator);
    colorListing(videoManager->getForegroundColor(), videoManager->getBackgroundColor());

    navElements[0].tvi->setSelected();
}

auto MediaLayout::bindSelectorAction(MediaGroupLayout* layout) -> void {
	
    auto mediaGroup = layout->mediaGroup;
        
	for (auto block : layout->blocks) {

		auto fSetting = FileSetting::getInstance(emulator, _underscore(block->media->name) );

        bool IPMode = mediaGroup->isExpansion() && mediaGroup->expansion->isRS232();

        if (IPMode && fSetting->path.empty())
            fSetting->setPath("127.0.0.1:25232");
					
		if (mediaGroup->isHardDisk()) {

			block->selector.open.onActivate = [this, block, mediaGroup, fSetting]() {
				
				std::string filePath = GUIKIT::BrowserWindow()
                    .setTitle(trans->get("select_" + mediaGroup->name + "_image"))
                    .setPath( fileloader->preselectPath( settings, mediaGroup->name ) )
                    .setFilters({ GUIKIT::BrowserWindow::transformFilter(trans->get(mediaGroup->name + "_image"), mediaGroup->suffix), trans->get("all_files")})
                    .open();

				if (filePath.empty())
					return;

				block->header.eject.onActivate();
				GUIKIT::File testFile(filePath);
                
				savePath( mediaGroup->name, testFile.getPath() );

                if (!testFile.exists() || !testFile.isSizeValid(MAX_HARDDISK_SIZE)) {
					message->error(trans->get("file_size_error",{
						{"%path%", filePath},
						{"%size%", GUIKIT::File::SizeFormated(MAX_HARDDISK_SIZE)}
					}));
				} else if (testFile.isArchived()) {
					message->error(trans->get("archive_none"));
				} else if (!testFile.open(GUIKIT::File::Mode::Update)) {
                    program->errorOpen(&testFile, message);
				} else {
					fSetting->setPath(filePath);
					block->selector.edit.setText(filePath);
				}
				testFile.unload();
			};

			block->header.eject.onActivate = [fSetting, block]() {
				fSetting->setPath("");
				block->selector.edit.setText("");
			};

			block->selector.edit.setText(fSetting->path);

		} else {
            
			block->selector.open.onActivate = [this, block]() {
                
                fileloader->load( this->emulator, block->media );
			};

			block->header.eject.onActivate = [this, block]() {

                bool locked = emuThread->lock();
			    fileloader->eject( emulator, block->media );

                if (locked)
                    // nested lock when changing drive count ... so don't unlock here
                    emuThread->unlock();
			};

			block->header.writeprotect.onToggle = [this, block, fSetting, mediaGroup](bool checked) {

                emuThread->lock();
				emulator->writeProtect(block->media, checked);
                emuThread->unlock();
                // wp is shared between main image, save states and disk swapper.
                // i.e. if save state changes it, it's valid for main image too (to keep it simple)
				fSetting->setWriteProtect(checked);
			};
			
			updateMediaBlock(block, fSetting);
            
            block->selector.edit.onFocus = [this, layout, block]() {
                fileloader->resetPreview( this->emulator, true);
                
                layout->selectedBlock = block;
                
                if (layout->mediaGroup->selected && !block->media->secondary) {
                    layout->mediaGroup->selected = block->media;
                    settings->set<unsigned>( _underscore(layout->mediaGroup->name) + "_selected", block->media->id);
                    block->header.inUse.setChecked();
                }
                
                if ( showListing( layout ) )
                    layout->updateListing( block );                
            };

            block->selector.edit.onChange = [block, fSetting]() {

                auto group = block->media->group;

                if (group->isExpansion() && group->expansion->isRS232()) {

                    fSetting->setPath( block->selector.edit.text() );
                }
            };
            
            block->selector.edit.onDrop = [this, layout, block]( std::vector<std::string> files ) {
                
                drop( files[0], block );
            };
            
            block->header.inUse.onActivate = [this, layout, block]() {
                fileloader->resetPreview( this->emulator, true);
                
                layout->mediaGroup->selected = block->media;
                
                layout->selectedBlock = block;                                
                
                settings->set<unsigned>( _underscore(layout->mediaGroup->name) + "_selected", block->media->id);
                
                if ( showListing( layout ) )
                    layout->updateListing( block );                
            };
            
            block->selector.combo.onChange = [this, layout, block]() {
                
                int userData = block->selector.combo.userData();
                
                for( auto& pcb : layout->mediaGroup->expansion->pcbs ) {
                    
                    if (pcb.id == userData) {
                        
                        block->media->pcbLayout = &pcb; 
                        
                        settings->set<unsigned>( _underscore(block->media->name) + "_pcb", pcb.id);
                        
                        if (activeEmulator) {
                            
                            if (block->media->group->name == "EasyFlash³") {
                            
                                if (!layout->hint) {
                                    layout->hint = new GUIKIT::MultilineEdit;                                                                

                                    layout->hint->setForegroundColor( ERROR_COLOR );
									
									layout->hint->setEditable( false );

                                    layout->append( *(layout->hint), {~0u, ~0u}, 0 );

                                    layout->synchronizeLayout();
                                }

                                if (block->media->pcbLayout->name == "Slot 0")
                                    layout->hint->setText( trans->get("ef3 switch to single slot") );
                                else
                                    layout->hint->setText( trans->get("ef3 switch to multi slot") );
                            }
                        }                                                                                                
                        
                        break;
                    }
                }                                
            };

		    block->selector.open.setImage(&openImg);
		    block->header.eject.setImage(&ejectImg);
		}

        if (mediaGroup->expansion && !block->media->secondary) {
            for (auto& jumper : mediaGroup->expansion->jumpers) {

                unsigned jumperId = jumper.id;

                auto jumperBox = block->selector.jumpers[jumperId];

                std::string saveIdent = _underscore( block->media->name + "_jumper_" + jumper.name );

                jumperBox->onToggle = [this, jumperBox, saveIdent, block, jumperId](bool checked) {

                    this->settings->set<bool>( saveIdent, checked);

                    emuThread->lock();
                    this->emulator->setExpansionJumper(block->media, jumperId, checked);
                    emuThread->unlock();
                };
            }
        }

		if ( showListing( layout ) ) { //preload last listing
			GUIKIT::File* file = filePool->get( fSetting->path );
			uint8_t* data = nullptr;

            if (program->loadImageDataWhenOk(file, fSetting->id, mediaGroup, data)) {

                if (!mediaGroup->isProgram())
                    filePool->assign( _ident(emulator, block->media->name + "store"), file);

                if (mediaGroup->selected || (activeEmulator != emulator)) {
                    block->media->guid = uintptr_t(file);
                    emulator->insertMedium(block->media, data, file->archiveDataSize(fSetting->id));
                }

                block->listings = emulator->getListing( block->media );

                if (mediaGroup->selected ) {
                    if (mediaGroup->selected == block->media)
                        layout->updateListing( block );			
                    
                } else if (block->media->id == 0)
                    layout->updateListing( block );				
			}
		}
	}
    
    if ( showListing( layout ) ) {

        layout->listings.onActivate = [this, layout]( ) {
            
            auto selection = layout->listings.selection( );

            fileloader->insertCurrentPreview( layout->mediaGroup );
            emuThread->lock();

            auto media = layout->selectedBlock->media;

            fileloader->autoload(emulator, media, selection, media->group->isDrive() && useTraps.checked());
            emuThread->unlock();
        };

        layout->inject.onActivate = [this, layout]() {

            if (activeEmulator != emulator)
                return;
            
            unsigned selection = layout->listings.selection( );
            
            if (!layout->listings.selected() && layout->listings.rowCount() > 0)
                selection = 0;
            
            auto media = layout->selectedBlock->media;

            emuThread->lock();
            if ( emulator->selectListing( media, selection ) ) {
                if (media->group->isTape())
                    statusHandler->setMessage( trans->get( "tape spooled" ) );
                else
                    statusHandler->setMessage( trans->get( "program_injected" ) );
                view->setFocused( 100 );
            }
            emuThread->unlock();
        };
    }

    useTraps.onToggle = [this](bool checked) {
        auto layout = this->getActiveLayout();

        if (layout && layout->mediaGroup->isDisk())
            settings->set<bool>("use_disk_traps", checked);
        else if (layout && layout->mediaGroup->isTape())
            settings->set<bool>("use_tape_traps", checked);
    };

    useTraps.setChecked( settings->get<bool>("use_disk_traps", false) );

    bootCart.onActivate = [this]() {

        auto mediaGroupLayout = getActiveLayout();

        if (!mediaGroupLayout || !mediaGroupLayout->mediaGroup->isExpansion())
            return;
        
		this->settings->set<unsigned>("expansion", mediaGroupLayout->mediaGroup->expansion->id);

        if (tabWindow->systemLayout)
            tabWindow->systemLayout->setExpansion( mediaGroupLayout->mediaGroup->expansion );

        emuThread->lock();
        program->power( emulator );
        emuThread->unlock();
        
        view->setFocused( 100 );
    };
    
    deactivateCart.onActivate = [this]() {        
        
		this->settings->set<unsigned>("expansion", 0);

        if (tabWindow->systemLayout)
            tabWindow->systemLayout->setExpansion( nullptr );

        emuThread->lock();
        program->power( emulator );
        emuThread->unlock();
        
        view->setFocused( 100 );
    };

    fontSizeLayout.combo.onChange = [this]() {
        this->settings->set<unsigned>("software_preview_fontsize", fontSizeLayout.combo.userData());
        updateListingFont(fontSizeLayout.combo.userData());
    };

    dialogPreviewLayout.mode.noPreviewRadio.onActivate = [this]() {
        this->settings->set<unsigned>("dialog_preview_mode", Fileloader::PREV_OFF);
    };

    dialogPreviewLayout.mode.dialogPreviewRadio.onActivate = [this]() {
        this->settings->set<unsigned>("dialog_preview_mode", Fileloader::PREV_DIALOG);
    };

    dialogPreviewLayout.mode.softwarePreviewRadio.onActivate = [this]() {
        this->settings->set<unsigned>("dialog_preview_mode", Fileloader::PREV_SOFTWARE);
    };
    dialogPreviewLayout.control.fontSizeCombo.onChange = [this]() {
        this->settings->set<unsigned>("dialog_preview_fontsize", dialogPreviewLayout.control.fontSizeCombo.userData());
        dialogPreviewLayout.updatePreviewContent(settings, emulator);
    };
    dialogPreviewLayout.control.option.tooltips.onToggle = [this](bool checked) {
        this->settings->set<bool>("software_preview_tooltips", checked );

        updateListings();
        dialogPreviewLayout.previewBox.reset();
        dialogPreviewLayout.updatePreviewContent(settings, emulator);
    };
    dialogPreviewLayout.control.option.commodoreHighlight.onToggle = [this](bool checked) {
        this->settings->set<bool>("software_preview_commodore_hi", checked );
        selectionColorListing();
        dialogPreviewLayout.previewBox.reset();
        dialogPreviewLayout.updatePreviewContent(settings, emulator);
    };
    dialogPreviewLayout.dimension.dialogWidth.slider.onChange = [this](unsigned position) {

        dialogPreviewLayout.dimension.dialogWidth.value.setText( std::to_string( position + 200 ) + " px" );

        this->settings->set<unsigned>("dialog_preview_width", position + 200 );

        dialogPreviewLayout.updatePreviewContent(settings, emulator);
    };

    dialogPreviewLayout.dimension.dialogHeight.slider.onChange = [this](unsigned position) {

        dialogPreviewLayout.dimension.dialogHeight.value.setText( std::to_string( position + 50 ) + " px" );

        this->settings->set<unsigned>("dialog_preview_height", position + 50 );

        dialogPreviewLayout.updatePreviewContent(settings, emulator);
    };
}

auto MediaLayout::createImage( Emulator::Interface::MediaGroup* mediaGroup ) -> void {

	std::string ident = mediaGroup->name;
	std::string suffix = mediaGroup->creatable[0];
    unsigned id = 0;
	
    if (mediaGroup->isExpansion()) {
        id = flashCreatorLayout->format.userData();
        
        mediaGroup = &emulator->mediaGroups[ id & 0xff ];
        id >>= 8;
        
        if (id < mediaGroup->creatable.size())
            suffix = mediaGroup->creatable[id];

		ident = mediaGroup->expansion->creationIdents[id];
    }
    
    std::string title = ident + "_image";    
    GUIKIT::File file;
    GUIKIT::File* filePtr;
    std::string filePath;
    uint8_t* data = nullptr;
    unsigned size = 0;
    int insertId = -1;

    if (mediaGroup->isHardDisk()) {

        try {
            size = std::stoi( hdCreatorLayout->creator.diskSize.text() );
            if (size > 4095)
                throw "";
            size = size * 1024u * 1024u;
        } catch (...) {
            message->error(trans->get("invalid_input"));
            return;
        }        
        
    } else if (mediaGroup->isDisk()) {
        suffix = diskCreatorLayout->format.text();
        
        unsigned typeId = diskCreatorLayout->format.userData();
        bool hd = diskCreatorLayout->options.highDensity.checked();
        bool bootable = diskCreatorLayout->options.bootable.checked();
        bool useFFS = diskCreatorLayout->options.fastFileSystem.checked();
        std::string diskName = diskCreatorLayout->diskLabel.text();
        insertId = diskCreatorLayout->insertDevice.userData();
        
        Emulator::Interface::Data _data = emulator->createDiskImage( typeId, diskName, hd, useFFS, bootable );

        data = _data.ptr;
        size = _data.size;

    } else if (mediaGroup->isTape()) {
        insertId = tapeCreatorLayout->insertDevice.userData();
        data = emulator->createTapeImage( size );
        
    } else if (mediaGroup->isProgram()) {
        data = emulator->getLoadedProgram( size );
        
    } else if (mediaGroup->isExpansion()) {
        data = emulator->createExpansionImage( mediaGroup, size, id );
    }
    
    if (!size)
        goto Done; //internal error
    
    filePath = GUIKIT::BrowserWindow()
        .setWindow(*this->tabWindow)
        .setTitle(trans->get( "blank_" + title ))
        .setPath( fileloader->preselectPath( settings, ident ))
        .setFilters({GUIKIT::BrowserWindow::transformFilter( trans->get( title ), {suffix}), trans->get("all_files")})
        .save();

    if (filePath.empty())
        goto Done;

    if ( !GUIKIT::String::foundSubStr( filePath, "." ))
        filePath += "." + suffix;

    filePtr = filePool->get( filePath, insertId >= 0 );
    if (filePtr)
        filePtr->forceDataChange();
        
    file.setFile( filePath );

    if (GUIKIT::Application::isGtk()) {
        if (file.exists() && !message->question(trans->get("file_exist_error", {
                {"%path%", filePath}})))
            goto Done;
    }

    if ( !file.open(GUIKIT::File::Mode::Write) ) {
        message->error(trans->get("file_creation_error",{
            {"%path%", filePath}
        }));
        
        goto Done;
    }

    savePath( ident, file.getPath() );

    if (data) {
        if (!file.write( data, size )) {
            message->error(trans->get("file_creation_error",{
                {"%path%", filePath}
            }));

            goto Done;
        }

        message->information(trans->get("file_creation_success",{
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

    if (insertId >= 0) {
        auto media = emulator->getMedia( *mediaGroup, insertId );
        if (media) {
            auto items = filePtr->scanArchive();
            emuThread->lock();
            insertImage(media, filePtr, &items[0]);
            emuThread->unlock();
        }
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

            diskCreatorLayout = new DiskCreatorLayout(emulator, &mediaGroup );

            diskCreatorLayout->button.onActivate = [this, group]() {
                createImage( group );                
            };

            creatorLayout.append(*diskCreatorLayout, {~0u, 0u}, 5);

        } else if (mediaGroup.isTape()) {

            tapeCreatorLayout = new TapeCreatorLayout( &mediaGroup);

            tapeCreatorLayout->button.onActivate = [this, group]() {
                createImage( group );                
            };

            creatorLayout.append(*tapeCreatorLayout, {~0u, 0u}, 5);
			
        } else if (mediaGroup.isProgram()) {
			
			memoryCreatorLayout = new MemoryCreatorLayout;
			
			memoryCreatorLayout->button.onActivate = [this, group]() {
                createImage( group );
			};
			
			creatorLayout.append(*memoryCreatorLayout, {~0u, 0u}, 5);
            
		} else if (mediaGroup.isExpansion() && (mediaGroup.expansion->isFlash() || mediaGroup.expansion->isEprom() || mediaGroup.expansion->isBattery()) ) {
            
            if (!flashCreatorLayout) {
                flashCreatorLayout = new FlashCreatorLayout;

                flashCreatorLayout->button.onActivate = [this, group]() {
                    createImage(group);
                };

                creatorLayout.append(*flashCreatorLayout, {~0u, 0u}, 5);
            }

            unsigned i = 0;
            for( auto& creationIdent : mediaGroup.expansion->creationIdents )                                
                flashCreatorLayout->format.append( creationIdent, (i++ << 8) | mediaGroup.id );
		}
    }
}

auto MediaLayout::preparePath(Emulator::Interface::MediaGroup& mediaGroup) -> void {
    if (mediaGroup.isExpansion() && mediaGroup.expansion->isRS232())
        return;

    auto settingFolderIdent = _underscore(mediaGroup.name) + "_folder";

    auto block = new PathsLayout::Block( &mediaGroup );

    pathsLayout.blocks.push_back( block );
    pathsLayout.append( *block,{~0u, 0u}, 5 );

    std::string title = "select_" + mediaGroup.name + "_folder";

    if (mediaGroup.isExpansion())
        title = "select_cartridge_folder";

    block->select.onActivate = [this, block, title, settingFolderIdent]() {
        auto path = GUIKIT::BrowserWindow()
            .setTitle(trans->get(title))
            .setPath( settings->get<std::string>(settingFolderIdent, "") )
            .setWindow(*this->tabWindow)
            .directory();

        if (!path.empty()) {
            settings->set<std::string>(settingFolderIdent, path);
            block->edit.setText(path);
        }
    };

    block->empty.onActivate = [this, block, settingFolderIdent]() {
        settings->set<std::string>(settingFolderIdent, "");
        block->edit.setText("");
    };

    block->edit.setText( settings->get<std::string>(settingFolderIdent, "") );

    block->select.setImage(&openImg);
    block->empty.setImage(&ejectImg);
}

auto MediaLayout::preparePaths() -> void {
    for (auto& mediaGroup : emulator->mediaGroups)
        preparePath(mediaGroup);

    if (dynamic_cast<LIBAMI::Interface*>(emulator)) {
        auto saveImage = new Emulator::Interface::MediaGroup;
        saveImage->id = emulator->mediaGroups.size();
        saveImage->name = "disksave";
        saveImage->suffix.push_back("sav");
        saveImage->type = Emulator::Interface::MediaGroup::Type::Disk;
        preparePath(*saveImage);
    }
}

auto MediaLayout::updateMediaBlock(MediaGroupLayout::Block* block, FileSetting* fSetting) -> void {

    bool IPMode = block->media->group->isExpansion() && block->media->group->expansion->isRS232();

    block->selector.edit.setText( fSetting->path );

    if (!IPMode) {
        block->header.fileName.setText(fSetting->file);
        block->header.writeprotect.setChecked( fSetting->writeProtect );
    }
}

auto MediaLayout::updateListing( Emulator::Interface::Media* media ) -> void {    
    
    auto mediaGroupLayout = getMediaGroupLayout( media->group );
    
    if (!mediaGroupLayout)
        return;
                
    if ( !showListing( mediaGroupLayout ) )
        return;

    for( auto block : mediaGroupLayout->blocks ) {

        if ( block->media == media ) {
            block->listings = emulator->getListing( media );

            if ( mediaGroupLayout->selectedBlock->media == media )
                mediaGroupLayout->updateListing( block );

            return;
        }
    }
}

auto MediaLayout::savePath( std::string& groupName, std::string path ) -> void {
	
	auto baseFolderIdent = _underscore(groupName) + "_folder";
	
	settings->set<std::string>(baseFolderIdent + "_auto", path);
}

auto MediaLayout::translate() -> void {

    pathsLayout.setText( trans->get("paths") );
    moduleFrame.setText( trans->get("selection") );   
    bootCart.setText( trans->get("boot cartridge") );
    useTraps.setText( trans->get("VDT Autostart") );
    useTraps.setTooltip( trans->get("VDT Autostart tooltip") );
    deactivateCart.setText( trans->get("deactivate cartridge") );

    fontSizeLayout.label.setText( trans->getA("Font Size") );

    dialogPreviewLayout.setText( trans->getA("Dialog Preview") );
    dialogPreviewLayout.mode.label.setText( trans->getA("preview", true) );
    dialogPreviewLayout.mode.noPreviewRadio.setText( trans->getA("off") );
    dialogPreviewLayout.mode.dialogPreviewRadio.setText( trans->getA("Dialog Preview") );
    dialogPreviewLayout.mode.softwarePreviewRadio.setText( trans->getA("Software Preview") );
    dialogPreviewLayout.control.fontSize.setText( trans->get("Font Size", {}, true) );
    dialogPreviewLayout.control.option.tooltips.setText( trans->get("Show Tooltips") );
    dialogPreviewLayout.control.option.commodoreHighlight.setText( trans->get("Commodore Highlight Color" ) );
    dialogPreviewLayout.dimension.dialogWidth.name.setText( trans->get("Width", {}, true) );
    dialogPreviewLayout.dimension.dialogHeight.name.setText( trans->get("Height", {}, true) );

    if (expansionParent)
        expansionParent->setText( trans->get("cartridges") );
    
    for(auto& nav : navElements) {
        if (nav.mediaGroupLayout) {
            nav.tvi->setText( trans->get( getMediaGroupTransIdent( nav.mediaGroupLayout->mediaGroup ) ) );
            
        } else if ( dynamic_cast<SwapperLayout*>(nav.altLayout))
            nav.tvi->setText( trans->get( emulator->getTapeMediaGroup() ? "swapper" : "disk swapper" ) );
        else if ( dynamic_cast<PathsLayout*>(nav.altLayout))
            nav.tvi->setText( trans->get( "paths" ) );
        else if ( nav.altLayout == &creatorLayout )
            nav.tvi->setText( trans->get( "create" ) );
        else if ( nav.altLayout == &dialogPreviewLayout )
            nav.tvi->setText( trans->get( "Dialog Preview" ) );

        if (!nav.mediaGroupLayout)
            continue;
        
        auto mediaGroup = nav.mediaGroupLayout->mediaGroup;
        
        nav.mediaGroupLayout->setText( trans->get( mediaGroup->name + "_insert") );
        if (mediaGroup->isProgram())
            nav.mediaGroupLayout->inject.setText( trans->get("program_inject") );
        else if (mediaGroup->isTape())
            nav.mediaGroupLayout->inject.setText( trans->get("tape spool") );

        for ( auto block : nav.mediaGroupLayout->blocks ) {
            block->header.writeprotect.setText(trans->get("write_protected"));
            block->header.eject.setTooltip(trans->get("eject"));
            block->header.deviceName.setText( trans->get( block->media->name, {}, true ) );
            block->header.inUse.setText( trans->get( block->media->name, {}, true ) );

            if (mediaGroup->isProgram())
                block->selector.open.setTooltip(trans->get("c64_list_tip"));
            
            if (mediaGroup->isExpansion() && !block->media->secondary) {
                unsigned id = 0;
                for( auto& pcb : mediaGroup->expansion->pcbs ) {
                    block->selector.combo.setText(id++, trans->get( pcb.name ));
                }
                      
                block->selector.jumperLabel.setText( trans->get("jumper", {}, true) );
                
                for(auto& jumper : mediaGroup->expansion->jumpers) {

                    auto jumperBox = block->selector.jumpers[jumper.id];

                    jumperBox->setText( trans->get( jumper.name ) );
                }        
            }
        }  
    }
        
    if (diskCreatorLayout) {        
        diskCreatorLayout->setText( trans->get("disc_creator") );

        diskCreatorLayout->formatName.setText(trans->get("format",{}, true));
        diskCreatorLayout->options.fastFileSystem.setText(trans->get("ffs"));
        diskCreatorLayout->options.highDensity.setText(trans->get("high_density"));
        diskCreatorLayout->options.bootable.setText(trans->get("bootable"));
        diskCreatorLayout->diskLabelName.setText(trans->get("disk label",{}, true));
        diskCreatorLayout->insertLabel.setText(trans->get("insert",{}, true));
        diskCreatorLayout->button.setText(trans->get("create"));
    }
    
    if (hdCreatorLayout) {
        hdCreatorLayout->setText( trans->get("hd_creator") );
        
        hdCreatorLayout->creator.diskSizeName.setText( trans->get("size_in_mb", {}, true) );
        hdCreatorLayout->creator.diskLabelName.setText( trans->get("disk label", {}, true) );
        hdCreatorLayout->creator.button.setText( trans->get("create") );
    }
            
    if (tapeCreatorLayout) {
        tapeCreatorLayout->setText( trans->get("tape_creator") );
        tapeCreatorLayout->insertLabel.setText(trans->get("insert",{}, true));
        tapeCreatorLayout->button.setText(trans->get("create"));
    }
	
	if (memoryCreatorLayout) {
		memoryCreatorLayout->setText( trans->get("program_creator") );		
        memoryCreatorLayout->button.setText(trans->get("create")); 
	}
    
    if (flashCreatorLayout) {
		flashCreatorLayout->setText( trans->get("flash_creator") );
        flashCreatorLayout->button.setText(trans->get("create"));         
    }

	unsigned neededWidth = 90;
	
    for(auto block : pathsLayout.blocks) {
        				
        block->label.setText( trans->get( getMediaGroupTransIdent(block->mediaGroup) ) );
      //  block->empty.setText( trans->get("remove") );
        //block->select.setText( trans->get("select") );
		
		neededWidth = std::max(neededWidth, block->label.minimumSize().width );
    }
	
	for(auto block : pathsLayout.blocks)	
		block->update( block->label, { neededWidth, 0u }, 10 );	
        
    swapperLayout->translate();
}

auto MediaLayout::getMediaGroupTransIdent( Emulator::Interface::MediaGroup* mediaGroup ) -> std::string {
    auto ident = mediaGroup->name;
    
    if (mediaGroup->isDrive() || mediaGroup->isProgram())
        ident += "s";
    
    return ident;
}

auto MediaLayout::showListing( MediaGroupLayout* layout ) -> bool {

    auto mediaGroup = layout->mediaGroup;
    
    if ( mediaGroup->isDrive() || mediaGroup->isProgram())
        return true;
        
    return false;
}

auto MediaLayout::ejectImage( Emulator::Interface::Media* media) -> void {
    auto layout = getMediaGroupLayout( media->group );

    if (!layout)
        return;

    for( auto block : layout->blocks ) {

        if (block->media == media) {
            ejectImage( block );
            break;
        }
    }
}

auto MediaLayout::ejectImage( MediaGroupLayout::Block* block ) -> void {
    auto layout = block->layout;
    auto media = block->media;

    auto fSetting = FileSetting::getInstance( emulator, _underscore(media->name ) );

    if ( showListing( layout ) ) {
        block->listings.clear();

        if (layout->selectedBlock->media == media)
            layout->listings.reset();
    }

    updateMediaBlock(block, fSetting);
}

auto MediaLayout::insertImage(Emulator::Interface::Media* media, GUIKIT::File* file, GUIKIT::File::Item* item, int options) -> void {
    
    auto layout = getMediaGroupLayout( media->group );
    
    if (!layout)
        return;

    for( auto block : layout->blocks ) {
        
        if (block->media == media) {
            insertImage( block, file, item, options );
            break;
        }
    }        
}

auto MediaLayout::insertImage( MediaGroupLayout::Block* block, GUIKIT::File* file, GUIKIT::File::Item* item, int options) -> void {
    bool fromState = options & 1;
    bool dontUpdateSelected = options & 2;
    bool asWP = options & 4;

    if (!block)
        return;
    
    fileloader->resetPreview(this->emulator, true);
    auto layout = block->layout;
       
    auto media = block->media;
    auto mediaGroup = layout->mediaGroup;
    auto fSetting = FileSetting::getInstance( emulator, _underscore(media->name ) );

    unsigned size = file->archiveDataSize(item->id);

    // unarchived tape files are writable which results in unpredictable filesizes
    // so they are loaded in chunks by an emulator callback
    auto data = mediaGroup->isTape() && !file->isArchived() ? nullptr
        : file->archiveData(item->id);

    if (!mediaGroup->isExpansion() || media->secondary) {
        emulator->ejectMedium(media);
        fileloader->updateFileSetting(fSetting, file, item, asWP);
        media->guid = uintptr_t(file);

        emulator->insertMedium(media, data, size);
        emulator->writeProtect(media, fSetting->writeProtect);
        if (!mediaGroup->isProgram())
            filePool->assign( _ident(emulator, media->name), file);
    } else {        
        if (media->pcbLayout && mediaGroup->expansion->pcbs.size()) {
            block->selector.combo.setSelection(0);
            block->selector.combo.onChange();        
        }
        fileloader->updateFileSetting(fSetting, file, item, asWP);
    }

    if (showListing(layout)) {
        block->listings.clear();
        block->listings = emulator->getListing(media);
        layout->selectedBlock = block;
        layout->updateListing(block);
    }

    if (!fromState && activeEmulator && mediaGroup->isTape())
        view->updateTapeIcons();
    
    if (!dontUpdateSelected && mediaGroup->selected && !media->secondary && !block->header.inUse.checked() ) {
        block->header.inUse.setChecked();
        layout->selectedBlock = block;
        layout->mediaGroup->selected = block->media;
        settings->set<unsigned>( _underscore(layout->mediaGroup->name) + "_selected", block->media->id);
    }

    if (!mediaGroup->isProgram())
        filePool->assign( _ident(emulator, media->name + "store"), file);

    filePool->unloadOrphaned();

    if (!mediaGroup->isExpansion())
        States::getInstance(emulator)->updateImage(fSetting, media);
    else
        States::getInstance(emulator)->forcePowerNextLoad = true;

    updateMediaBlock(block, fSetting);
    
    if (!fromState && fSetting && mediaGroup->isDrive())
        program->updateSaveIdent( emulator, fSetting );
}

auto MediaLayout::getMediaGroupLayout( Emulator::Interface::MediaGroup* mediaGroup ) -> MediaGroupLayout* {
    
    for (auto& nav : navElements) {
        
        if (nav.mediaGroupLayout && (nav.mediaGroupLayout->mediaGroup == mediaGroup))
            return nav.mediaGroupLayout;
    }
    
    return nullptr;
}

auto MediaLayout::showMediaGroupLayout( Emulator::Interface::MediaGroup* mediaGroup ) -> void {
    
    for (auto& nav : navElements) {
        if (nav.mediaGroupLayout && (nav.mediaGroupLayout->mediaGroup == mediaGroup)) {
            nav.tvi->setSelected();
            updateSwitchLayout();
            updateOptionsVisibility();
            break;
        }
    }
}

auto MediaLayout::getActiveLayout() -> MediaGroupLayout* {

    auto itemSelected = mediaTree.selected();
    
    if (itemSelected) {
        unsigned navPos = (unsigned)itemSelected->userData();
        if (navPos < navElements.size())
            return navElements[navPos].mediaGroupLayout;            
    }
    
    return nullptr;
}

auto MediaLayout::colorListing( unsigned foregroundColor, unsigned backgroundColor ) -> void {

    for (auto& nav : navElements) {
        if (nav.mediaGroupLayout && showListing( nav.mediaGroupLayout ) ) {
            nav.mediaGroupLayout->listings.setForegroundColor( foregroundColor );
            nav.mediaGroupLayout->listings.setBackgroundColor( backgroundColor );

            if (settings->get<bool>("software_preview_commodore_hi", true ))
                nav.mediaGroupLayout->listings.setSelectionColor( backgroundColor, foregroundColor );
            else
                nav.mediaGroupLayout->listings.resetSelectionColor();

            if (dynamic_cast<LIBAMI::Interface*>(emulator))
                nav.mediaGroupLayout->listings.setFirstRowColor( backgroundColor, foregroundColor );
            else
                nav.mediaGroupLayout->listings.resetFirstRowColor();
        }
    }
}

auto MediaLayout::selectionColorListing( ) -> void {
    bool comHi = settings->get<bool>("software_preview_commodore_hi", true );

    for (auto& nav : navElements) {
        if (nav.mediaGroupLayout && showListing( nav.mediaGroupLayout ) ) {
            if (comHi)
                nav.mediaGroupLayout->listings.setSelectionColor(
                    nav.mediaGroupLayout->listings.backgroundColor(), nav.mediaGroupLayout->listings.foregroundColor() );
            else
                nav.mediaGroupLayout->listings.resetSelectionColor();
        }
    }
}

auto MediaLayout::fillListing(Emulator::Interface::Media* media, std::vector<GUIKIT::BrowserWindow::Listing>& listings, bool markPreview) -> void {

    auto layout = getMediaGroupLayout( media->group );

    if (layout)
        layout->fillListing( listings );

    if (!markPreview)
        return;

    auto block = layout->getBlock( media );

    if (block) {
        block->header.fileName.setText(trans->get("Preview"));
        block->header.fileName.setFont(GUIKIT::Font::system("bold"));
        block->header.fileName.setForegroundColor(ERROR_COLOR);
    }
}

auto MediaLayout::drop( std::string filePath, MediaGroupLayout::Block* block ) -> void {    
    
    MediaGroupLayout* layout = nullptr;
    Emulator::Interface::MediaGroup* mediaGroup;
    
    if (!block) {        
        layout = getActiveLayout();
        
        if (!layout)
            return;
                
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

    if (!file->exists() || !file->isSizeValid(MAX_MEDIUM_SIZE))
        return program->errorMediumSize( file, message );

    auto& items = file->scanArchive();

    archiveViewer->onCallback = [this, file, block](GUIKIT::File::Item* item) {

        if (!item || (item->info.size == 0) )
            return program->errorOpen( file, item, message );        

        emuThread->lock();
        insertImage( block, file, item );
        emuThread->unlock();
    };

    archiveViewer->setView(items);
}

auto MediaLayout::updateVisibility( Emulator::Interface::MediaGroup* mediaGroup, unsigned count) -> void {
    
    auto layout = getMediaGroupLayout( mediaGroup );
    
    if (!layout)
        return;
    
    layout->updateVisibility( count );
}

auto MediaLayout::updateWriteProtection( Emulator::Interface::Media* media, bool state ) -> void {
    auto layout = getMediaGroupLayout(media->group);
    
    if (!layout)
        return;

    auto block = layout->getBlock(media);

    if (block && (state != block->header.writeprotect.checked()))
        block->header.writeprotect.setChecked( state );
}

auto MediaLayout::updateJumper(Emulator::Interface::Media* media) -> void {
    if (media->secondary)
        return;

    auto layout = getMediaGroupLayout(media->group);
    
    if (!layout)
        return;
    
    for(auto block : layout->blocks) {
        
        if (media && (block->media != media))
            continue;        

        for (auto& jumper : media->group->expansion->jumpers) {

            auto jumperBox = block->selector.jumpers[jumper.id];
            
            bool state = emulator->getExpansionJumper( media, jumper.id );

            if (state != jumperBox->checked()) {
                jumperBox->setChecked(state);
            }
        }  
        
        if (media)        
            break;
    }    
}

auto MediaLayout::resetPreview( bool light ) -> void {
    
    for (auto& nav : navElements) {
    
        if (!nav.mediaGroupLayout)
            continue;
        
        if (!showListing( nav.mediaGroupLayout ))
            continue;
        
        if (!light)
            nav.mediaGroupLayout->updateListing( nav.mediaGroupLayout->selectedBlock ); 
        
        for (auto block : nav.mediaGroupLayout->blocks) {
            
            if (block->header.fileName.overrideForegroundColor()) {
                auto fSetting = FileSetting::getInstance( emulator, _underscore(block->media->name) );
                block->header.fileName.setText( fSetting ? fSetting->file : "" );
                block->header.fileName.setFont( GUIKIT::Font::system() );
                block->header.fileName.resetForegroundColor();                
            }
        }
    }
}

auto MediaLayout::convertListing( std::vector<Emulator::Interface::Listing>& emuListings, bool loadCommand ) -> std::vector<std::string> {

    std::vector<std::string> list;
    auto customFont = GUIKIT::Window::getCustomFont(emulator);
    
    for (auto& listing : emuListings) {

        std::vector<uint8_t> utf8;

        for (auto& code : (loadCommand ? listing.loadCommand : listing.line) ) {

			unsigned useCode = code;
            if (customFont)
                useCode |= customFont->modifier;
			
            GUIKIT::Utf8::encode(useCode, utf8);
        }

        list.push_back( std::string((const char*) utf8.data(), utf8.size()) );
    }  
    
    return list;
}

auto MediaLayout::updateListingFont( unsigned fontSize ) -> void {

    for(auto& nav : navElements) {
        if (nav.mediaGroupLayout)
            nav.mediaGroupLayout->applyFont( fontSize );
    }
}

auto MediaLayout::updateListings( ) -> void {

    for(auto& nav : navElements) {
        if (nav.mediaGroupLayout && showListing( nav.mediaGroupLayout ) )
            nav.mediaGroupLayout->updateListing( nav.mediaGroupLayout->selectedBlock );
    }
}

auto MediaLayout::loadSettings() -> void {

    auto selectedLayout = this->getActiveLayout();

    if (selectedLayout && selectedLayout->mediaGroup->isDisk())
        useTraps.setChecked( settings->get<bool>("use_disk_traps", false) );
    else if (selectedLayout && selectedLayout->mediaGroup->isTape())
        useTraps.setChecked( settings->get<bool>("use_tape_traps", false) );

    for(auto& nav : navElements) {
        
        if (!nav.mediaGroupLayout)
            continue;
        
        auto layout = nav.mediaGroupLayout;
        
        layout->loadSettings();
        
        auto mediaGroup = layout->mediaGroup;
        
        auto pathBlock = pathsLayout.getBlock( mediaGroup );
        
        auto settingFolderIdent = _underscore(mediaGroup->name) + "_folder";

        if (pathBlock)
            pathBlock->edit.setText( settings->get<std::string>(settingFolderIdent, "") );
                
        if (mediaGroup->isDisk())
            layout->updateVisibility(emulator->getModelValue( emulator->getModelIdOfEnabledDrives(mediaGroup) ), true );
        
        for (auto block : layout->blocks) {
            
            auto fSetting = FileSetting::getInstance(emulator, _underscore(block->media->name) );
            fSetting->update();
            
            updateMediaBlock(block, fSetting);

            if ( showListing( layout ) ) {
                
                GUIKIT::File* file = filePool->get(fSetting->path);
                uint8_t* data = nullptr;
               
                if (program->loadImageDataWhenOk(file, fSetting->id, mediaGroup, data)) {
                    block->media->guid = uintptr_t(file);
                    if (!mediaGroup->isProgram())
                        filePool->assign(_ident(emulator, block->media->name + "store"), file);
                    emulator->insertMedium(block->media, data, file->archiveDataSize(fSetting->id));
                    block->listings = emulator->getListing(block->media);

                    if (mediaGroup->selected) {
                        if (mediaGroup->selected == block->media)
                            layout->updateListing(block);

                    } else if (block->media->id == 0)
                        layout->updateListing(block);
                }
            }
        }
    }

    int previewFontSizeCur = fontSizeLayout.combo.userData();
    int previewFontSize = settings->get<unsigned>("software_preview_fontsize", 12, {8, 16});

    if (previewFontSize != previewFontSizeCur) {
        fontSizeLayout.combo.setSelectionByUserId(previewFontSize);
        updateListingFont(previewFontSize);
    }
    dialogPreviewLayout.updateWidgets(settings, emulator);
    selectionColorListing();
    updateListings();

    swapperLayout->loadSettings();
}

auto PathsLayout::getBlock(Emulator::Interface::MediaGroup* mediaGroup) -> PathsLayout::Block* {
    
    for(auto block : blocks) {
        if (block->mediaGroup == mediaGroup)
            return block;
    }
    
    return nullptr;
}

}

