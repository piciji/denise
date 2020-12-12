
#include "media.h"
#include "../cmd/cmd.h"
#include "../view/view.h"
#include "../view/message.h"
#include "../tools/filepool.h"
#include "../tools/filesetting.h"
#include "../config/archiveViewer.h"
#include "../states/states.h"
#include "../../data/resource.h"

#include <thread>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace MediaView {
	
namespace Icons {
    #include "../../data/img/icons.data"
}

namespace Fonts {
	#include "../../data/fonts/fonts.data"
}

#include "layout.cpp"
#include "swapper.cpp"

MediaLayout::MediaLayout(EmuConfigView::TabWindow* tabWindow) {
    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;
    this->message = tabWindow->message;     
    this->settings = program->getSettings( emulator );
}

auto MediaLayout::show() -> void {		
       
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

auto MediaLayout::showDiskSwapper() -> void {
    
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

auto MediaLayout::updateExpansionBootButtonVisibility() -> void {
    
    auto item = mediaTree.selected();
    bool _enabled = false;
    
    if (item) {
        unsigned navPos = (unsigned)item->userData();
        
        if (navPos < navElements.size()) {
            auto& navElement = navElements[navPos];

            if (navElement.mediaGroupLayout && navElement.mediaGroupLayout->mediaGroup->isExpansion())
                _enabled = true;          
        }
    }
    
    if (bootCart.enabled() != _enabled) {
        bootCart.setEnabled( _enabled );
        deactivateCart.setEnabled( _enabled );
    }
}

auto MediaLayout::build() -> void {
    
    setMargin(10);
    ftimer.setInterval(150);
	
	ftimer.onFinished = [this]() {
		ftimer.setEnabled(false);
        
        if (fileDialogPtr && fileDialogPtr->visible())
			fileDialogPtr->setForeground();        
	};
	
    alternateFileDialog = globalSettings->getOrInit("alternate_software_preview", false);    
    
	if (emulator->ident == "C64" && !cmd->debug) {
        GUIKIT::CustomFont* font = new GUIKIT::CustomFont;
        font->name = "C64 Pro";
        font->data = (uint8_t*)Fonts::c64Pro;
        font->size = sizeof(Fonts::c64Pro);	
        font->filePath = program->fontFolder() + "/C64_Pro-STYLE121.ttf";
        useCustomFont = tabWindow->addCustomFont( font );
        ((LIBC64::Interface*) emulator)->convertPetsciiToScreencode( useCustomFont );
    }    			 

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
        
    GUIKIT::TreeViewItem* tvi;
    
    for( auto& mediaGroup : emulator->mediaGroups ) {            
        tvi = new GUIKIT::TreeViewItem;
                
        if ( mediaGroup.isHardDisk() ) {
            tvi->setText( "hd" );
            tvi->setImage( hdImage );            
            
        } else if (mediaGroup.isDisk()) {
            tvi->setText( "disk" );
            tvi->setImage( diskImage );            
			tvi->setSelected();
            
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
        
        mediaGroupLayout->build( );
        
        if (mediaGroupLayout->showOnlyConnectedDevices()) {
            
            unsigned counter = settings->get( _underscore(mediaGroup.name) + "_count", 1);
            
            mediaGroupLayout->updateVisibility( counter, true );
        }        
                
        moduleSwitch.setLayout( navElements.size() - 1, *mediaGroupLayout, {~0u, ~0u} );        
        
		bindSelectorAction( mediaGroupLayout );
    }
    
    moduleFrame.append( mediaTree, { GUIKIT::Font::scale(165), GUIKIT::Font::scale(270)}, 10 );
    moduleFrame.append( bootCart, {0u, 0u}, 10 );
    moduleFrame.append( deactivateCart, {0u, 0u} );
    moduleFrame.setPadding(10);
    moduleFrame.setFont(GUIKIT::Font::system("bold"));
    
    append( moduleFrame, {0u, 0u}, 10 );    
    append( moduleSwitch, {~0u, ~0u} );        
    
    mediaTree.onChange = [this]() {

        if (fileDialogPtr && fileDialogPtr->visible()) {
			//message->warning("dialog tab change");
			resetPreview();
        }
        
        updateSwitchLayout();
        updateExpansionBootButtonVisibility();
    };
		
    tvi = new GUIKIT::TreeViewItem;
    tvi->setText( "disk_swapper" );
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
	
    translate();
}

auto MediaLayout::bindSelectorAction(MediaGroupLayout* layout) -> void {
	
    auto mediaGroup = layout->mediaGroup;
        
	for (auto block : layout->blocks) {

		auto fSetting = FileSetting::getInstance(emulator, _underscore(block->media->name) );
					
		if (mediaGroup->isHardDisk()) {

			block->selector.open.onActivate = [this, block, mediaGroup, fSetting]() {
				
				std::string filePath = GUIKIT::BrowserWindow()
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
					message->error(trans->get("file_size_error",{
						{"%path%", filePath},
						{"%size%", GUIKIT::File::SizeFormated(MAX_HARDDISK_SIZE)}
					}));
				} else if (testFile.isArchived()) {
					message->error(trans->get("archive_none"));
				} else if (!testFile.open(GUIKIT::File::Mode::Update)) {
					message->error(trans->get("file_open_error",{
						{"%path%", filePath}
					}));
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
            
			block->selector.open.onActivate = [this, block, mediaGroup, layout]() {                
                
                auto media = block->media;
                
                auto suffix = mediaGroup->suffix;
                GUIKIT::Vector::combine(suffix, GUIKIT::File::suppportedCompressionExtensions());    
                
                if (fileDialogPtr) {
                    fileDialogPtr->close();
                    delete fileDialogPtr;
                    resetPreview();
                }
                
                fileDialogPtr = new GUIKIT::BrowserWindow;
                
                fileDialogPtr->setDefaultButtonText( trans->get("insert") );
                
                fileDialogPtr->setTemplateId( IDD_FILE_TEMPLATE );
                
                fileDialogPtr->resizeTemplate( true, -6 );
                
                fileDialogPtr->setTitle(trans->get("select_" + mediaGroup->name + "_image"));
                
                fileDialogPtr->setPath( preselectPath( mediaGroup->name ) );
                
                fileDialogPtr->setFilters({ GUIKIT::BrowserWindow::transformFilter(trans->get(mediaGroup->name + "_image"), suffix ),
						trans->get("all_files")});
                                                
                fileDialogPtr->setOnChangeCallback( [this, block](std::string file) {

                    return this->previewFile(file, block);
                } );
                
                fileDialogPtr->addCustomButton( trans->get("auto start"), [this, block, mediaGroup, layout](std::string filePath, unsigned selection) {
                    
                    if (filePath.empty())
                        return false;
                    
                    return insertFile(block, filePath, true, selection);
                }, IDC_BUTTON );    

				if (!*alternateFileDialog) {
					fileDialogPtr->addContentView(IDC_LIST, [this, block](std::string filePath, unsigned selection) {
						if (filePath.empty())
							return false;

						return insertFile(block, filePath, true, selection);
					});
									                    
                    applyPreviewFont( globalSettings->get<unsigned>("dialog_software_preview_fontsize", 11, {6, 14}) );
                    fileDialogPtr->setContentViewWidth( globalSettings->get<unsigned>("dialog_software_preview_width", 450, {200, 600}) );
                    fileDialogPtr->setContentViewHeight( globalSettings->get<unsigned>("dialog_software_preview_height", 200, {100, 600}) );

                    for (auto& nav : navElements) {
                        if (nav.mediaGroupLayout && nav.mediaGroupLayout->mediaGroup->isDisk()) {
                            fileDialogPtr->setContentViewBackground(nav.mediaGroupLayout->listings.backgroundColor());
                            fileDialogPtr->setContentViewForeground(nav.mediaGroupLayout->listings.foregroundColor());
                            fileDialogPtr->setContentViewColorTooltips(true);
                            break;
                        }
                    }
                }
				
                fileDialogPtr->setCallbacks( [this, block](std::string filePath, unsigned selection) {
                    
                    insertFile(block, filePath);
                }, [this]() {
                    resetPreview();
                } );
                
				fileDialogPtr->setWindow( *view ).setNonModal();
				
                std::string filePath = fileDialogPtr->open();
                
                if (fileDialogPtr && fileDialogPtr->detached()) {
					//message->warning("detached software");
                    return;
				}
                
				if (filePath.empty()) {
                    //message->warning("cancel software");
                    if (GUIKIT::Application::loop)
                        resetPreview();
					return;
                }
                
                insertFile(block, filePath);
			};

			block->header.eject.onActivate = [this, mediaGroup, block, fSetting, layout]() {
                
                auto media = block->media;
                
				if ( !mediaGroup->isExpansion() ) {
					emulator->ejectMedium(media);					
					filePool->assign( _ident(emulator, media->name), nullptr);
                    States::getInstance( emulator )->updateImage( nullptr, media );
				} else
                    States::getInstance(emulator)->forcePowerNextLoad = true;
                
                // an expansion can't be removed while emulation is running.
                // but we have to cut the file link or EasyFlash could
                // write back data, even when user has removed file from UI.
                media->guid = (uintptr_t)nullptr;
                
				
				if ( showC64Listing( layout ) ) {
                    block->listings.clear();
                    
                    if (layout->selectedBlock->media == media)
                        layout->listings.reset();
                }                
				
				if (mediaGroup->isTape())
					view->updateTapeIcons();
                
				filePool->assign( _ident(emulator, media->name + "store"), nullptr);
                filePool->unloadOrphaned();

				fSetting->init();
				updateMediaBlock(block, fSetting);
			};

			block->header.writeprotect.onToggle = [this, block, fSetting, mediaGroup]() {
				
				bool state = block->header.writeprotect.checked();
                
				emulator->writeProtect(block->media, state);
                // wp is shared between main image, save states and disk swapper.
                // i.e. if save state changes it, it's valid for main image too (to keep it simple)
				fSetting->setWriteProtect(state);
			};
			
			updateMediaBlock(block, fSetting);
            
            block->selector.edit.onFocus = [this, layout, block]() {
                resetPreview(true);
                
                layout->selectedBlock = block;
                
                if (layout->mediaGroup->selected && !block->media->alternate) {
                    layout->mediaGroup->selected = block->media;
                    settings->set<unsigned>( _underscore(layout->mediaGroup->name) + "_selected", block->media->id);
                    block->header.inUse.setChecked();
                }
                
                if ( showC64Listing( layout ) )
                    layout->updateListing( block );                
            };
            
            block->selector.edit.onDrop = [this, layout, block]( std::vector<std::string> files ) {
                
                drop( files[0], block );
            };
            
            block->header.inUse.onActivate = [this, layout, block]() {
                resetPreview(true);
                
                layout->mediaGroup->selected = block->media;
                
                layout->selectedBlock = block;                                
                
                settings->set<unsigned>( _underscore(layout->mediaGroup->name) + "_selected", block->media->id);
                
                if ( showC64Listing( layout ) )
                    layout->updateListing( block );                
            };
            
            block->selector.combo.onChange = [this, layout, block]() {
                
                int userData = block->selector.combo.userData();
                
                for( auto& pcb : layout->mediaGroup->expansion->pcbs ) {
                    
                    if (pcb.id == userData) {
                        
                        block->media->pcbLayout = &pcb; 
                        
                        settings->set<unsigned>( _underscore(block->media->name) + "_pcb", pcb.id);
                        
                        break;
                    }
                }                                
            };
		}

        if (mediaGroup->expansion) {
            for (auto& jumper : mediaGroup->expansion->jumpers) {

                unsigned jumperId = jumper.id;

                auto jumperBox = block->selector.jumpers[jumperId];

                std::string saveIdent = _underscore( block->media->name + "_jumper_" + jumper.name );
                
                jumperBox->onToggle = [this, jumperBox, saveIdent, block, jumperId]() {

                    bool state = jumperBox->checked();

                    this->settings->set<bool>( saveIdent, state);

                    this->emulator->setExpansionJumper(block->media, jumperId, state);
                };
            }
        }

		if ( showC64Listing( layout ) ) { //preload last listing
			GUIKIT::File* file = filePool->get( fSetting->path );
			uint8_t* data;

            if (program->loadImageDataWhenOk(file, fSetting->id, mediaGroup, data)) {
				filePool->assign( _ident(emulator, block->media->name + "store"), file);
                emulator->insertMedium(block->media, data, file->archiveDataSize( fSetting->id ));
                block->listings = emulator->getListing( block->media );
                
                if (mediaGroup->selected ) {
                    if (mediaGroup->selected == block->media)
                        layout->updateListing( block );			
                    
                } else if (block->media->id == 0)
                    layout->updateListing( block );				
			}
		}
	}
    
    if ( showC64Listing( layout ) ) {

        layout->listings.onActivate = [this, layout]( ) {
            
            auto selection = layout->listings.selection( );
            
            if (fileDialogPtr && fileDialogPtr->visible()) {
                //message->warning("dialog autostart");
                fileDialogPtr->close();
                delete fileDialogPtr;
                fileDialogPtr = nullptr;

                if (lastPreview.block && lastPreview.block->layout == layout)
                    insertFile(lastPreview.block, lastPreview.filePath);                                
            }
                       
            auto media = layout->selectedBlock->media;
            
            program->power( emulator );
            
            program->removeBootableExpansion();

            emulator->selectListing( media, selection );
       
            auto fSetting = FileSetting::getInstance(emulator, _underscore(media->name) );
            if (fSetting)
                EmuConfigView::TabWindow::getView(emulator)->configurationsLayout->updateSaveIdent(fSetting->file);            
            
            view->setFocused(300);
        };

        layout->inject.onActivate = [this, layout]() {

            if (activeEmulator != emulator)
                return;
            
            unsigned selection = layout->listings.selection( );
            
            if (!layout->listings.selected() && layout->listings.rowCount() > 0)
                selection = 0;
            
            auto media = layout->selectedBlock->media;
                                    
            if ( emulator->selectListing( media, selection ) ) {
                status->addMessage( trans->get( "program_injected" ) );
                view->setFocused( 300 );                
            }
        };
    }
    
    bootCart.onActivate = [this]() {
        
        auto selectedItem = mediaTree.selected();
        
        if (!selectedItem)
            return;
        
        auto navPos = (unsigned)selectedItem->userData();
        
        if (navPos >= navElements.size())
            return;
        
        auto& navElement = navElements[navPos];
        
        if (!navElement.mediaGroupLayout || !navElement.mediaGroupLayout->mediaGroup->isExpansion())
            return;
        
        tabWindow->systemLayout->setExpansion( navElement.mediaGroupLayout->mediaGroup->expansion );
        
        program->power( emulator );
        
        view->setFocused( 300 );                
    };
    
    deactivateCart.onActivate = [this]() {        
        
        tabWindow->systemLayout->setExpansion( nullptr );
        
        program->power( emulator );
        
        view->setFocused( 300 );                
    };
}

auto MediaLayout::createImage( Emulator::Interface::MediaGroup* mediaGroup ) -> void {

    if (mediaGroup->isExpansion()) {
        if (mediaGroup->expansion->isFlash()) {
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
            message->error(trans->get("invalid_input"));
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
        
    } else if (mediaGroup->isProgram()) {
        data = emulator->getLoadedProgram( size );
        
    } else if (mediaGroup->isExpansion()) {
        data = emulator->createExpansionImage( mediaGroup, size );
    }
    
    if (!size)
        goto Done; //internal error
    
    filePath = GUIKIT::BrowserWindow()
        .setWindow(*this->tabWindow)
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
    
    if ( file.exists() && !message->question(trans->get("file_exist_error", {
        {"%path%", filePath } })))
        goto Done;                

    if ( !file.open(GUIKIT::File::Mode::Write) ) {
        message->error(trans->get("file_creation_error",{
            {"%path%", filePath}
        }));
        
        goto Done;
    }

    savePath( mediaGroup->name, file.getPath() );

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
			
        } else if (mediaGroup.isProgram()) {
			
			memoryCreatorLayout = new MemoryCreatorLayout;
			
			memoryCreatorLayout->button.onActivate = [this, group]() {
                createImage( group );
			};
			
			creatorLayout.append(*memoryCreatorLayout, {~0u, 0u}, 5);
            
		} else if (mediaGroup.isExpansion() && mediaGroup.expansion->isFlash() ) {
			
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
        
        auto settingFolderIdent = _underscore(mediaGroup.name) + "_folder";
        
        auto block = new PathsLayout::Block( &mediaGroup );
        
        pathsLayout.blocks.push_back( block );                
        pathsLayout.append( *block,{~0u, 0u}, &mediaGroup != &emulator->mediaGroups.back() ? 5 : 0 );
        
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
    }
}

auto MediaLayout::updateMediaBlock(MediaGroupLayout::Block* block, FileSetting* fSetting) -> void {

    block->selector.edit.setText( fSetting->path );
    block->header.fileName.setText( fSetting->file );
    
    updateWriteProtection( block->media, fSetting->writeProtect );
}

auto MediaLayout::updateListing( Emulator::Interface::Media* media ) -> void {    
    
    auto mediaGroupLayout = getMediaGroupLayout( media->group );
    
    if (!mediaGroupLayout)
        return;
                
    if ( !showC64Listing( mediaGroupLayout ) )
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

auto MediaLayout::preselectPath( std::string& groupName ) -> std::string {
	
	auto baseFolderIdent = _underscore(groupName) + "_folder";    

	auto path = settings->get<std::string>( baseFolderIdent, "" );
	
	if ( path == "" )
		path = settings->get<std::string>( baseFolderIdent + "_auto", "" );
	
	return path;
}

auto MediaLayout::savePath( std::string& groupName, std::string path ) -> void {
	
	auto baseFolderIdent = _underscore(groupName) + "_folder";
	
	settings->set<std::string>(baseFolderIdent + "_auto", path);
}

auto MediaLayout::translate() -> void {

    pathsLayout.setText( trans->get("paths") );
    moduleFrame.setText( trans->get("selection") );   
    bootCart.setText( trans->get("boot cartridge") );
    deactivateCart.setText( trans->get("deactivate cartridge") );
    if (expansionParent)
        expansionParent->setText( trans->get("cartridges") );
    
    for(auto& nav : navElements) {
        if (nav.mediaGroupLayout) {
            nav.tvi->setText( trans->get( getMediaGroupTransIdent( nav.mediaGroupLayout->mediaGroup ) ) );
            
        } else if ( dynamic_cast<SwapperLayout*>(nav.altLayout))
            nav.tvi->setText( trans->get( "disk_swapper" ) );
        else if ( dynamic_cast<PathsLayout*>(nav.altLayout))
            nav.tvi->setText( trans->get( "paths" ) );
        else if ( nav.altLayout == &creatorLayout )
            nav.tvi->setText( trans->get( "create" ) );
        
        if (!nav.mediaGroupLayout)
            continue;
        
        auto mediaGroup = nav.mediaGroupLayout->mediaGroup;
        
        nav.mediaGroupLayout->setText( trans->get( mediaGroup->name + "_insert") );
        nav.mediaGroupLayout->inject.setText( trans->get("program_inject") );
        
        for ( auto block : nav.mediaGroupLayout->blocks ) {
            block->header.writeprotect.setText(trans->get("write_protected"));
            block->header.eject.setText(trans->get("eject"));
            block->header.deviceName.setText( trans->get( block->media->name, {}, true ) );            
            block->header.inUse.setText( trans->get( block->media->name, {}, true ) );            
            
            block->selector.open.setText("...");
                
            if (mediaGroup->isProgram()) {                
                block->selector.open.setTooltip(trans->get("c64_list_tip"));
            }
            
            if (mediaGroup->isExpansion()) {
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
        block->empty.setText( trans->get("remove") );
        block->select.setText( trans->get("select") );
		
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

auto MediaLayout::showC64Listing( MediaGroupLayout* layout ) -> bool {
    
    if ( !dynamic_cast<LIBC64::Interface*>(emulator) )
        return false;
    
    auto mediaGroup = layout->mediaGroup;
    
    if ( mediaGroup->isDisk() || mediaGroup->isProgram())
        return true;
        
    return false;
}

auto MediaLayout::insertImage(Emulator::Interface::Media* media, GUIKIT::File* file, GUIKIT::File::Item* item) -> void {
    
    auto layout = getMediaGroupLayout( media->group );
    
    if (!layout)
        return;

    for( auto block : layout->blocks ) {
        
        if (block->media == media) {
            insertImage( block, file, item );
            break;
        }
    }        
}

auto MediaLayout::insertImage( MediaGroupLayout::Block* block, GUIKIT::File* file, GUIKIT::File::Item* item ) -> void {
   
    if (!block)
        return;
    
    resetPreview(true);
    auto layout = block->layout;
       
    auto media = block->media;
    auto mediaGroup = layout->mediaGroup;
    auto fSetting = FileSetting::getInstance( emulator, _underscore(media->name ) );

    unsigned size = file->archiveDataSize(item->id);

    // unarchived tape files are writable which results in unpredictable filesizes
    // so they are loaded in chunks by an emulator callback
    auto data = mediaGroup->isTape() && !file->isArchived() ? nullptr
        : file->archiveData(item->id);

    if (!mediaGroup->isExpansion()) {
        emulator->ejectMedium(media);
        
        media->guid = uintptr_t(file);
        emulator->insertMedium(media, data, size);
        emulator->writeProtect(media, false);
        filePool->assign( _ident(emulator, media->name), file);
    } else {        
        if (mediaGroup->expansion->pcbs.size()) {
            block->selector.combo.setSelection(0);
            block->selector.combo.onChange();        
        }
    }

    if (showC64Listing(layout)) {
        block->listings.clear();
        block->listings = emulator->getListing(media);        
        block->selector.edit.setFocused();
        layout->updateListing(block);
    }

    if (mediaGroup->isTape())
        view->updateTapeIcons();
    
    if (mediaGroup->selected && !media->alternate && !block->header.inUse.checked() ) {
        block->header.inUse.setChecked();
        block->header.inUse.onActivate();
    }

    filePool->assign( _ident(emulator, media->name + "store"), file);    
    filePool->unloadOrphaned();

    fSetting->setPath(file->getFile());
    fSetting->setFile(item->info.name);
    fSetting->setId(item->id);
    fSetting->setWriteProtect(false);

    if (!mediaGroup->isExpansion())
        States::getInstance(emulator)->updateImage(fSetting, media);
    else
        States::getInstance(emulator)->forcePowerNextLoad = true;

    updateMediaBlock(block, fSetting);  
    
    if (mediaGroup->isDrive())
        EmuConfigView::TabWindow::getView(emulator)->configurationsLayout->updateSaveIdent( fSetting->file );
}

auto MediaLayout::eject( Emulator::Interface::MediaGroup* mediaGroup, bool alternateOnly ) -> void {
    
    auto layout = getMediaGroupLayout( mediaGroup );
    
    for( auto block : layout->blocks) {
        
        if (!alternateOnly || block->media->alternate)
            block->header.eject.onActivate();
    }
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
            updateExpansionBootButtonVisibility();
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

auto MediaLayout::colorListing( unsigned color, bool foreground ) -> void {	
    
    for (auto& nav : navElements) {
        if (nav.mediaGroupLayout && showC64Listing( nav.mediaGroupLayout ) ) {
            if (foreground)
                nav.mediaGroupLayout->listings.setForegroundColor( color );
            else
                nav.mediaGroupLayout->listings.setBackgroundColor( color );            
        }
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

    if (!file->isSizeValid(MAX_MEDIUM_SIZE))  
        return program->errorMediumSize( file, message );    

    auto& items = file->scanArchive();

    archiveViewer->onCallback = [this, file, block](GUIKIT::File::Item* item) {

        if (!item || (item->info.size == 0) )
            return program->errorOpen( file, item, message );        

        insertImage( block, file, item );        
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
            
    bool enabled = false;
    
    auto file = (GUIKIT::File*)media->guid;
    
    if (!file)
        enabled = true;    
    else if (file->isArchived() || file->isReadOnly())
        state = true;
    else
        enabled = true;
        
    auto layout = getMediaGroupLayout(media->group);
    
    if (!layout)
        return;
    
    for(auto block : layout->blocks) {
        
        if (block->media == media) {
                        
            auto fSetting = FileSetting::getInstance( emulator, _underscore(media->name) );
            
            if (state != block->header.writeprotect.checked())            
                block->header.writeprotect.setChecked( state );                            
            
            if (enabled != block->header.writeprotect.enabled())               
                block->header.writeprotect.setEnabled( enabled );                            
            
            fSetting->setWriteProtect( state );
                               
            break;
        }
    }
}

auto MediaLayout::updateJumper(Emulator::Interface::Media* media) -> void {
    
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
                std::string saveIdent = block->media->name + "_jumper_" + jumper.name;

                settings->set<bool>( _underscore(saveIdent), state);

                jumperBox->setChecked(state);
            }
        }  
        
        if (media)        
            break;
    }    
}

auto MediaLayout::previewFile( std::string filePath, MediaGroupLayout::Block* block ) -> std::vector<GUIKIT::BrowserWindow::Listing> {
    
    MediaGroupLayout* layout = nullptr;
    
    if (block) {
        layout = block->layout;

        if ( !showC64Listing(layout) ) {
            lastPreview.block = nullptr;
            return {};
        }
    }
    
    lastPreview.filePath = filePath;    
    
    std::vector<Emulator::Interface::Listing> listings;       
    std::vector<GUIKIT::BrowserWindow::Listing> convertedListings;
    GUIKIT::File file;
    Emulator::Interface::MediaGroup* group = nullptr;
    std::string fileName;
    std::string extension = "";
    std::size_t end;
    uint8_t* data;
    
    file.setFile( filePath );
    file.setReadOnly();
    auto items = file.scanArchive();         
    
    if (items.size() > 1)
        goto clearList;   
    
    data = file.archiveData( 0 );
    
    if (!data)
        goto clearList;
        
    fileName = items[0].info.name;    
    end = fileName.find_last_of(".");
    
    if (end != std::string::npos)
        extension = fileName.substr(end + 1);
    
    GUIKIT::String::toLowerCase( extension );            
    
    for(auto& mediaGroup : emulator->mediaGroups) {
        
        if (block && (block->media->group != &mediaGroup) )
            continue;
        
        if (mediaGroup.isDisk()) {
            if (extension == "d64" || extension == "g64") {
                listings = emulator->getDiskPreview(data, file.archiveDataSize(0), block ? block->media : nullptr);
                group = &mediaGroup;
                break;
            }            
        }
        
        if (mediaGroup.isProgram()) {
            if (extension == "t64" || extension == "prg" || extension == "p00") {
                listings = emulator->getProgramPreview(data, file.archiveDataSize(0));        
                group = &mediaGroup;
                break;
            }
        }
    }
    
    if (!group)
        goto clearList;   
    
    layout = getMediaGroupLayout( group );
    convertedListings = convertListing( listings );        
    
    if (!block)
        block = layout->getBlock( group->selected ? group->selected : &group->media[0] );
    
    if (lastPreview.block && (lastPreview.block != block))
        resetPreview();
    
    layout->fillListing( convertedListings );
    
    if (!lastPreview.block) {
        block->header.fileName.setText( trans->get("Preview") );
        block->header.fileName.setFont( GUIKIT::Font::system("bold") );
        block->header.fileName.setForegroundColor( 0xff4500 );
    }
    
    lastPreview.block = block;
       
    showMediaGroupLayout( group );
	
    if (*alternateFileDialog) {
        tabWindow->setLayout( EmuConfigView::TabWindow::Layout::Media );
        
        if ( !tabWindow->visible() ) {
            tabWindow->setVisible();
            tabWindow->setFocused();
            ftimer.setEnabled();
        } else if ( tabWindow->minimized() ) {
            tabWindow->restore();            
            ftimer.setEnabled();
        } else if ( GUIKIT::Application::isWinApi() && !tabWindow->focused() && view->fullScreen() ) {
            tabWindow->setForeground();
            if (fileDialogPtr && fileDialogPtr->visible())
                fileDialogPtr->setForeground();
        } else
            tabWindow->setFocused();
    }
    
    return convertedListings;
    
    clearList:
        if (lastPreview.block)
            resetPreview();
    
    return {};
}

auto MediaLayout::resetPreview( bool light ) -> void {
    
    for (auto& nav : navElements) {
    
        if (!nav.mediaGroupLayout)
            continue;
        
        if (!showC64Listing( nav.mediaGroupLayout ))
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
    
    lastPreview.block = nullptr;
}

auto MediaLayout::insertFile( MediaGroupLayout::Block* block, std::string filePath, bool autoLoad, unsigned selection ) -> bool {
    
    GUIKIT::File* file = filePool->get(filePath);
    if (!file)
        return false;
                    
    savePath( block->media->group->name, file->getPath() );

    if (!file->isSizeValid(MAX_MEDIUM_SIZE))
        return program->errorMediumSize( file, message ), true;				

    auto& items = file->scanArchive();

    archiveViewer->onCallback = [this, file, block, autoLoad, selection](GUIKIT::File::Item* item) {

        if (!item || (item->info.size == 0) )
            return program->errorOpen( file, item, message );                     

        insertImage( block, file, item );
        
        if (autoLoad) {
            auto mediaGroup = block->media->group;
            auto emuConfigView = EmuConfigView::TabWindow::getView(this->emulator);
            
            if (mediaGroup->isDrive())				
				emuConfigView->systemLayout->activateDrive(mediaGroup, 1 );			
                        
            if (mediaGroup->isExpansion())
                emuConfigView->systemLayout->setExpansion( mediaGroup->expansion );
            
            program->power( emulator );
            
            if (!mediaGroup->isExpansion())
                program->removeBootableExpansion();           

            if (mediaGroup->selected)
                emulator->selectListing(mediaGroup->selected, selection);
            else
                emulator->selectListing(block->media, selection);

            EmuConfigView::TabWindow::getView(emulator)->configurationsLayout->updateSaveIdent( file->getFileName() );
            
            if (mediaGroup->isTape())
                view->updateTapeIcons(Emulator::Interface::TapeMode::Play);

            view->setFocused(300);
        }
    };
    archiveViewer->setView(items);
    
    return true;
}

#define HideMouseIfWasBefore if (mIsAcquiredBefore && !inputDriver->mIsAcquired() && view->fullScreen() && fileDialogPtr && fileDialogPtr->detached()) inputDriver->mAcquire();

auto MediaLayout::anyLoad( bool mIsAcquiredBefore ) -> void {
    
    if (fileDialogPtr) {
        fileDialogPtr->close();
        resetPreview();
        delete fileDialogPtr;
    }

    fileDialogPtr = new GUIKIT::BrowserWindow;
    
    fileDialogPtr->setTemplateId( IDD_FILE_TEMPLATE );
    
	if (!*alternateFileDialog) {
		fileDialogPtr->addContentView( IDC_LIST, [this, mIsAcquiredBefore](std::string filePath, unsigned selection) {
			if (filePath.empty())
				return false;

			settings->set<std::string>("anyload_path", GUIKIT::File::getPath( filePath ) );

			view->autoloadInit( {filePath}, false, View::AutoLoad::AutoStart, selection );
			view->autoloadFiles();

			resetPreview();

            HideMouseIfWasBefore
            
			return true;
		} );

        applyPreviewFont( globalSettings->get<unsigned>("dialog_software_preview_fontsize", 11, {6, 14}) );
        fileDialogPtr->setContentViewWidth( globalSettings->get<unsigned>("dialog_software_preview_width", 450, {200, 600}) );
        fileDialogPtr->setContentViewHeight( globalSettings->get<unsigned>("dialog_software_preview_height", 200, {100, 600}) );
        
        for(auto& nav : navElements) {
            if (nav.mediaGroupLayout && nav.mediaGroupLayout->mediaGroup->isDisk()) {
                fileDialogPtr->setContentViewBackground( nav.mediaGroupLayout->listings.backgroundColor() );
                fileDialogPtr->setContentViewForeground( nav.mediaGroupLayout->listings.foregroundColor() );
                fileDialogPtr->setContentViewColorTooltips(true);
                break;
            }
        }
        
    }
	
    fileDialogPtr->setTitle(trans->get("select image"));

    fileDialogPtr->setPath( settings->get<std::string>( "anyload_path", "") );

    fileDialogPtr->setFilters({trans->get("all_files")});

    fileDialogPtr->setOnChangeCallback( [this](std::string file) {
        
        auto listings = this->previewFile(file);
        
        if (listings.size())
            settings->set<std::string>("anyload_path", GUIKIT::File::getPath( file ) );
        
        return listings;
    } );
    fileDialogPtr->addCustomButton( trans->get("insert"), [this, mIsAcquiredBefore](std::string filePath, unsigned selection) {

        if (filePath.empty())
            return false;

        settings->set<std::string>("anyload_path", GUIKIT::File::getPath( filePath ) );
        
        view->autoloadInit( {filePath}, false, View::AutoLoad::Open );
        view->autoloadFiles();
        
        resetPreview();
        
        HideMouseIfWasBefore
        
        return true;
    }, IDC_BUTTON );       
    
    fileDialogPtr->setCallbacks( [this, mIsAcquiredBefore](std::string filePath, unsigned selection) {
        settings->set<std::string>("anyload_path", GUIKIT::File::getPath( filePath ) );
        
        view->autoloadInit( {filePath}, false, View::AutoLoad::AutoStart, selection );
        view->autoloadFiles();
        
        resetPreview();
        
        HideMouseIfWasBefore
        
    }, [this, mIsAcquiredBefore]() {
        resetPreview();
        
        HideMouseIfWasBefore
    } );
    
    fileDialogPtr->resizeTemplate( true, -6 );
    
    fileDialogPtr->setDefaultButtonText( trans->get("auto start") );
    
    fileDialogPtr->setWindow( *view ).setNonModal();

    std::string filePath = fileDialogPtr->open();

    if (fileDialogPtr && fileDialogPtr->detached()) {
        // cocoa/gtk doesn't block for modeless dialog
        // it handles OK state in callback
		//message->warning("detached multi");
        return;
	}        
    
    if ( !filePath.empty() ) {
        settings->set<std::string>("anyload_path", GUIKIT::File::getPath( filePath ) );

        view->autoloadInit( {filePath}, false, View::AutoLoad::AutoStart, fileDialogPtr ? fileDialogPtr->getContentViewSelection() : 0 );
        view->autoloadFiles();
    } //else
        //message->warning("cancel multi");
    
    resetPreview();
    if (mIsAcquiredBefore && view->fullScreen())
        inputDriver->mAcquire();
}

auto MediaLayout::convertListing( std::vector<Emulator::Interface::Listing>& emuListings, bool loadCommand ) -> std::vector<std::string> {

    std::vector<std::string> list;
    
    for (auto& listing : emuListings) {

        std::vector<uint8_t> utf8;

        for (auto& code : (loadCommand ? listing.loadCommand : listing.line) ) {

			unsigned useCode = code;
            if (useCustomFont)
                useCode |= 0xee << 8;
			
            GUIKIT::Utf8::encode(useCode, utf8);
        }

        list.push_back( std::string((const char*) utf8.data(), utf8.size()) );
    }  
    
    return list;
}

auto MediaLayout::convertListing( std::vector<Emulator::Interface::Listing>& emuListings ) -> std::vector<GUIKIT::BrowserWindow::Listing> {

    std::vector<GUIKIT::BrowserWindow::Listing> list;
    
    bool useTooltips = globalSettings->get<bool>("software_preview_tooltips", true );
    
    for (auto& listing : emuListings) {

		GUIKIT::BrowserWindow::Listing browserListing;
		
        std::vector<uint8_t> utf8;

        for (auto& code : listing.line ) {

			unsigned useCode = code;
            if (useCustomFont)
                useCode |= 0xee << 8;
			
            GUIKIT::Utf8::encode(useCode, utf8);
        }

        browserListing.entry = std::string((const char*) utf8.data(), utf8.size());
		
        if (useTooltips) {                    
            utf8.clear();

            for (auto& code : listing.loadCommand ) {

                unsigned useCode = code;
                if (useCustomFont)
                    useCode |= 0xee << 8;

                GUIKIT::Utf8::encode(useCode, utf8);
            }

            browserListing.tooltip = std::string((const char*) utf8.data(), utf8.size());		
        }
        
		list.push_back( browserListing );
    }  
    
    return list;
}

auto MediaLayout::updateListingFont( unsigned fontSize ) -> void {
    
    if ( !dynamic_cast<LIBC64::Interface*>(emulator)) 
        return;
    
    for(auto& nav : navElements) {
        if (nav.mediaGroupLayout)
            nav.mediaGroupLayout->applyFont( fontSize );
    }
}

auto MediaLayout::updateListings( ) -> void {
    
    if ( !dynamic_cast<LIBC64::Interface*>(emulator)) 
        return;
    
    for(auto& nav : navElements) {
        if (nav.mediaGroupLayout && showC64Listing( nav.mediaGroupLayout ) )
            nav.mediaGroupLayout->updateListing( nav.mediaGroupLayout->selectedBlock );
    }
}

auto MediaLayout::applyPreviewFont(unsigned fontSize) -> void {

    if (useCustomFont)
        fileDialogPtr->setContentViewFont("C64 Pro, " + std::to_string(fontSize), true);
    else
        fileDialogPtr->setContentViewFont(GUIKIT::Font::system(fontSize));     
}

auto MediaLayout::loadSettings() -> void {
    
    for(auto& nav : navElements) {
        
        if (!nav.mediaGroupLayout)
            continue;
        
        auto layout = nav.mediaGroupLayout;
        
        layout->loadSettings();
        
        auto mediaGroup = layout->mediaGroup;
        
        auto pathBlock = pathsLayout.getBlock( mediaGroup );
        
        auto settingFolderIdent = _underscore(mediaGroup->name) + "_folder";
        
        pathBlock->edit.setText( settings->get<std::string>(settingFolderIdent, "") );
                
        if (mediaGroup->isDisk())
            layout->updateVisibility(settings->get<unsigned>( _underscore(mediaGroup->name) + "_count", mediaGroup->defaultUsage() ), true );
        
        for (auto block : layout->blocks) {
            
            auto fSetting = FileSetting::getInstance(emulator, _underscore(block->media->name) );
            fSetting->update();
            
            updateMediaBlock(block, fSetting);

            if ( showC64Listing( layout ) ) {
                
                GUIKIT::File* file = filePool->get(fSetting->path);
                uint8_t* data;
               
                if (program->loadImageDataWhenOk(file, fSetting->id, mediaGroup, data)) {
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
