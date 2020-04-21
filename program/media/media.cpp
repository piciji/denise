
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

std::vector<MediaView::MediaWindow*> mediaViews;

namespace MediaView {
	
namespace Icons {
    #include "../../data/img/icons.data"
}

namespace Fonts {
	#include "../../data/fonts/fonts.data"
}

#include "layout.cpp"

MediaWindow::MediaWindow(Emulator::Interface* emulator) {
    this->emulator = emulator;
    message = new Message(this);	
}

auto MediaWindow::getView( Emulator::Interface* emulator ) -> MediaWindow* {
	
	for (auto view : mediaViews) {
		if (view->emulator == emulator)
			return view;
	}
	return nullptr;
}

auto MediaWindow::showDelayed() -> void {
	inputDriver->mUnacquire();
	mtimer.setInterval(100);
	
	mtimer.onFinished = [this]() {
		mtimer.setEnabled(false);
		show();
	};
	mtimer.setEnabled();
}

auto MediaWindow::show() -> void {					
    setVisible();
	setFocused();
}

auto MediaWindow::ident( std::string name ) -> std::string {
	std::string _ident = emulator->ident;
    return GUIKIT::String::toLowerCase( _ident )+ "_" + GUIKIT::String::replace(name, " ", "_");
}

auto MediaWindow::build() -> void {
    winapi.disableBackgroundRedrawDuringResize();
    cocoa.keepMenuVisibilityOnDisplay();
    setDroppable();
    
    ftimer.setInterval(150);
	
	ftimer.onFinished = [this]() {
		ftimer.setEnabled(false);
        
        if (fileDialogPtr && fileDialogPtr->visible())
			fileDialogPtr->setForeground();        
	};
	
    alternateFileDialog = settings->getOrInit("alternate_software_preview", false);
    
	if (emulator->ident == "C64" && !cmd->debug) {
        GUIKIT::CustomFont* font = new GUIKIT::CustomFont;
        font->name = "C64 Pro Mono";
        font->data = (uint8_t*)Fonts::c64ProMono;
        font->size = sizeof(Fonts::c64ProMono);	
        font->filePath = program->fontFolder() + "/C64_Pro_Mono-STYLE121.ttf";
        useCustomFont = MediaWindow::addCustomFont( font );
        ((LIBC64::Interface*) emulator)->convertPetsciiToScreencode( useCustomFont );
    }
    
    GUIKIT::Geometry defaultGeometry = {100, 100, 900, 500};
    
    GUIKIT::Geometry geometry = {settings->get<int>(ident("screen_media_x"), defaultGeometry.x)
        ,settings->get<int>(ident("screen_media_y"), defaultGeometry.y)
        ,settings->get<unsigned>(ident("screen_media_width"), defaultGeometry.width)
        ,settings->get<unsigned>(ident("screen_media_height"), defaultGeometry.height)
    };
    
    setGeometry( geometry );
    
	if (isOffscreen())        
        setGeometry( defaultGeometry ); 
	
	append(tabView);
		
	onClose = [this]() {
        setVisible(false);
        view->setFocused();
    };

    onMove = [&]() {
        if (fullScreen()) return;
        GUIKIT::Geometry geometry = this->geometry();
        settings->set<int>( ident("screen_media_x"), geometry.x);
        settings->set<int>( ident("screen_media_y"), geometry.y);
    };

    onSize = [&]() {
        if (fullScreen()) return;
        GUIKIT::Geometry geometry = this->geometry();
        settings->set<unsigned>( ident("screen_media_width"), geometry.width);
        settings->set<unsigned>( ident("screen_media_height"), geometry.height);
    };
	
    onDrop = [this]( std::vector<std::string> files ) {
        
		this->drop( files[0] );
    };
    
    tabView.onChange = [this]() {
        if (fileDialogPtr && fileDialogPtr->visible()) {
			//message->warning("dialog tab change");
			resetPreview();
        }
    };
    
    tabView.setMargin(10);
	tabView.setPadding(10);

    diskImage.loadPng((uint8_t*) Icons::disk, sizeof (Icons::disk));
    hdImage.loadPng((uint8_t*) Icons::drive, sizeof (Icons::drive));
	tapeImage.loadPng((uint8_t*) Icons::tape, sizeof (Icons::tape));
	expansionImage.loadPng((uint8_t*) Icons::memory, sizeof (Icons::memory));
	memoryImage.loadPng((uint8_t*) Icons::memory, sizeof (Icons::memory));
    addImage.loadPng((uint8_t*) Icons::add, sizeof (Icons::add));
	pathImage.loadPng((uint8_t*) Icons::folderOpen, sizeof (Icons::folderOpen));       
    
    for( auto& mediaGroup : emulator->mediaGroups ) {            
        
        if ( mediaGroup.isHardDisk() ) {
            tabView.appendHeader("", hdImage);
            
        } else if (mediaGroup.isDisk()) {
            tabView.appendHeader("", diskImage);
            
        } else if (mediaGroup.isTape()) {
            tabView.appendHeader("", tapeImage);
            
        } else if (mediaGroup.isExpansion()) {
            cartList.append( {mediaGroup.name} );            
           
		} else if (mediaGroup.isProgram()) {
            tabView.appendHeader("", memoryImage);
            
        } else 
            continue;
        
        MediaGroupLayout* mediaGroupLayout = new MediaGroupLayout( &mediaGroup, this );        
        
        mediaGroupLayout->build( );
        
        if (mediaGroupLayout->showOnlyConnectedDevices()) {
            
            unsigned counter = settings->get( ident(mediaGroup.name + "_count"), 1);
            
            mediaGroupLayout->updateVisibility( counter, true );
        }        
        
        if (!mediaGroup.isExpansion()) {
            mediaGroupLayouts.push_back( mediaGroupLayout );
            
            tabs.push_back( getMediaGroupTransIdent(&mediaGroup) );
                
            tabView.setLayout(tabs.size() - 1, *mediaGroupLayout, {~0u, ~0u});   
		} else {
            cartLayouts.push_back( mediaGroupLayout );
            carts.setLayout( cartLayouts.size() - 1, *mediaGroupLayout, {~0u, 0u} );
        }
        
		bindSelectorAction( mediaGroupLayout );
    }             
        
    if (cartLayouts.size()) {
        cartList.setHeaderText( { "" } );
		cartList.setHeaderVisible( false );        
        cartSelectorFrame.append( cartList, {130u, 200u}, 10 );
        cartSelectorFrame.append( insertCart, {0u, 0u}, 10 );
        cartSelectorFrame.append( removeCart, {0u, 0u} );
        cartSelectorFrame.setPadding(10);
        cartSelectorFrame.setFont( GUIKIT::Font::system("bold") );
        
        tabView.appendHeader("", expansionImage); 
        tabs.push_back( "cartridges" );    
        tabView.setLayout(tabs.size() - 1, cartWrapper, {~0u, 0u});       
        cartWrapper.append( cartSelectorFrame, {0u, 0u}, 10 );
        cartWrapper.append( carts, {~0u, ~0u} );
                
        cartList.setSelection(0);
        
        cartList.onChange = [this]() {
            
            if (!cartList.selected())
                return;
            
            carts.setSelection( cartList.selection() );
        };
    }
    
    tabView.appendHeader("", addImage); 
    tabs.push_back("create");    
    prepareCreator();
    tabView.setLayout(tabs.size() - 1, creatorLayout, {~0u, 0u});  
    
    tabView.appendHeader("", pathImage); 
    tabs.push_back("paths");
    preparePaths();        
    tabView.setLayout(tabs.size() - 1, pathsLayout, {~0u, 0u});
    
    tabView.setSelection(0);	
	
    translate();
}

auto MediaWindow::bindSelectorAction(MediaGroupLayout* layout) -> void {
	
    auto mediaGroup = layout->mediaGroup;
        
	for (auto block : layout->blocks) {

		auto setting = FileSetting::getInstance(ident(block->media->name));
					
		if (mediaGroup->isHardDisk()) {

			block->selector.open.onActivate = [this, block, mediaGroup, setting]() {
				
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
            
			block->selector.open.onActivate = [this, block, mediaGroup, setting, layout]() {                
                
                auto media = block->media;
                
                auto suffix = mediaGroup->suffix;
                GUIKIT::Vector::combine(suffix, GUIKIT::File::suppportedCompressionExtensions());    
                
                if (fileDialogPtr) {
                    fileDialogPtr->close();
                    delete fileDialogPtr;
                    resetPreview();
                }
                
                fileDialogPtr = new GUIKIT::BrowserWindow;
                
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
									
                    fileDialogPtr->setContentViewFont(useCustomFont ? "C64 Pro Mono, 12" : "");
                    fileDialogPtr->setContentViewBackground(mediaGroupLayouts[0]->listings.backgroundColor());
                    fileDialogPtr->setContentViewForeground(mediaGroupLayouts[0]->listings.foregroundColor());
                    fileDialogPtr->setContentViewColorTooltips(true);
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
                    resetPreview();
					return;
                }
                
                insertFile(block, filePath);
			};

			block->header.eject.onActivate = [this, mediaGroup, block, setting, layout]() {
                
                auto media = block->media;
                
				if ( !mediaGroup->isExpansion() ) {
					emulator->ejectMedium(media);					
					filePool->assign(ident(media->name), nullptr);
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
				
				filePool->assign(ident(media->name + "store"), nullptr);
                filePool->unloadOrphaned();

				setting->init();
				updateMediaBlock(block, setting);
			};

			block->header.writeprotect.onToggle = [this, block, setting, mediaGroup]() {
				
				bool state = block->header.writeprotect.checked();
                
				emulator->writeProtect(block->media, state);
                // wp is shared between main image, save states and disk swapper.
                // i.e. if save state changes it, it's valid for main image too (to keep it simple)
				setting->setWriteProtect(state);
			};
			
			updateMediaBlock(block, setting);
            
            block->selector.edit.onFocus = [this, layout, block]() {
                resetPreview(true);
                
                layout->selectedBlock = block;
                
                if (layout->mediaGroup->selected && !block->media->memoryDump) {
                    layout->mediaGroup->selected = block->media;
                    settings->set<unsigned>(ident(layout->mediaGroup->name + "_selected"), block->media->id);
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
                
                settings->set<unsigned>(ident(layout->mediaGroup->name + "_selected"), block->media->id);
                
                if ( showC64Listing( layout ) )
                    layout->updateListing( block );                
            };
            
            block->selector.combo.onChange = [this, layout, block]() {
                
                int userData = block->selector.combo.userData();
                
                for( auto& pcb : layout->mediaGroup->expansion->pcbs ) {
                    
                    if (pcb.id == userData) {
                        
                        block->media->pcbLayout = &pcb; 
                        
                        settings->set<unsigned>(ident(block->media->name + "_pcb"), pcb.id);
                        
                        break;
                    }
                }                                
            };
		}

        if (mediaGroup->expansion) {
            for (auto& jumper : mediaGroup->expansion->jumpers) {

                unsigned jumperId = jumper.id;

                auto jumperBox = block->selector.jumpers[jumperId];

                std::string saveIdent = block->media->name + "_jumper_" + jumper.name;

                jumperBox->onToggle = [this, jumperBox, saveIdent, block, jumperId]() {

                    bool state = jumperBox->checked();

                    settings->set<bool>(this->ident(saveIdent), state);

                    this->emulator->setExpansionJumper(block->media, jumperId, state);
                };
            }
        }

		if ( showC64Listing( layout ) ) { //preload last listing
			GUIKIT::File* file = filePool->get( setting->path );
			uint8_t* data;

            if (program->loadImageDataWhenOk(file, setting->id, mediaGroup, data)) {
				filePool->assign(ident(block->media->name + "store"), file);
                emulator->insertMedium(block->media, data, file->archiveDataSize( setting->id ));
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
       
            auto setting = FileSetting::getInstance(ident(media->name));
            if (setting)
                EmuConfigView::TabWindow::getView(emulator)->statesLayout->updateSaveIdent(setting->file);            
            
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
    
    insertCart.onActivate = [this]() {
        
        unsigned selection = 0;
        
        if (cartList.selected())
            selection = cartList.selection();        
        
        if ( selection >= cartLayouts.size() )
            selection = 0;
        
        auto layout = cartLayouts[selection];
        
        EmuConfigView::TabWindow::getView(this->emulator)->systemLayout
            ->setExpansion( layout->mediaGroup->expansion );
        
        program->power( emulator );
    };
    
    removeCart.onActivate = [this]() {        
        
        EmuConfigView::TabWindow::getView(this->emulator)->systemLayout
            ->setExpansion( nullptr );
        
        program->power( emulator );
    };
}

auto MediaWindow::createImage( Emulator::Interface::MediaGroup* mediaGroup ) -> void {

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
        .setWindow(*this)
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

auto MediaWindow::prepareCreator() -> void {

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

auto MediaWindow::preparePaths() -> void {
    
    for (auto& mediaGroup : emulator->mediaGroups) {
        
        auto settingFolderIdent = ident( mediaGroup.name + "_folder" );
        
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
                .setWindow(*this)
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

auto MediaWindow::updateMediaBlock(MediaGroupLayout::Block* block, FileSetting* setting) -> void {

    block->selector.edit.setText( setting->path );
    block->header.fileName.setText( setting->file );
    
    updateWriteProtection( block->media, setting->writeProtect );
}

auto MediaWindow::updateListing( Emulator::Interface::Media* media ) -> void {    
    
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

auto MediaWindow::preselectPath( std::string& groupName ) -> std::string {
	
	auto baseFolderIdent = ident( groupName + "_folder" );

	auto path = settings->get<std::string>( baseFolderIdent, "" );
	
	if ( path == "" )
		path = settings->get<std::string>( baseFolderIdent + "_auto", "" );
	
	return path;
}

auto MediaWindow::savePath( std::string& groupName, std::string path ) -> void {
	
	auto baseFolderIdent = ident( groupName + "_folder" );
	
	settings->set<std::string>(baseFolderIdent + "_auto", path);
}

auto MediaWindow::translate() -> void {
    
    unsigned i = 0;
	
	setTitle( trans->get("software") + " - " + emulator->ident );
    for(auto& tab : tabs)
        tabView.setHeader(i++, trans->get(tab));
    
    i = 0;
    cartSelectorFrame.setText( trans->get("cartridges") );   
    insertCart.setText( trans->get("insert cartridge") );
    removeCart.setText( trans->get("remove cartridge") );

    for( auto mediaGroupLayout : GUIKIT::Vector::concat(mediaGroupLayouts, cartLayouts) ) {
        
        auto mediaGroup = mediaGroupLayout->mediaGroup;
        
        mediaGroupLayout->setText( trans->get( mediaGroup->name + "_insert") );
        mediaGroupLayout->inject.setText( trans->get("program_inject") );
        
        if (mediaGroup->isExpansion()) {
            cartList.setText( i++, 0, trans->get( mediaGroup->name ) ); 
        }

        for ( auto block : mediaGroupLayout->blocks ) {
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
    
    for(auto block : pathsLayout.blocks) {
        
        block->label.setText( trans->get( getMediaGroupTransIdent(block->mediaGroup) ) );
        block->empty.setText( trans->get("remove") );
        block->select.setText( trans->get("select") );
    }
}

auto MediaWindow::getMediaGroupTransIdent( Emulator::Interface::MediaGroup* mediaGroup ) -> std::string {
    auto ident = mediaGroup->name;
    
    if (mediaGroup->isDrive() || mediaGroup->isProgram())
        ident += "s";
    
    return ident;
}

auto MediaWindow::showC64Listing( MediaGroupLayout* layout ) -> bool {
    
    if ( !dynamic_cast<LIBC64::Interface*>(emulator) )
        return false;
    
    auto mediaGroup = layout->mediaGroup;
    
    if ( mediaGroup->isDisk() || mediaGroup->isProgram())
        return true;
        
    return false;
}

auto MediaWindow::insertImage(Emulator::Interface::Media* media, GUIKIT::File* file, GUIKIT::File::Item* item) -> void {
    
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

auto MediaWindow::insertImage( MediaGroupLayout::Block* block, GUIKIT::File* file, GUIKIT::File::Item* item ) -> void {
   
    if (!block)
        return;
    
    resetPreview(true);
    auto layout = block->layout;
       
    auto media = block->media;
    auto mediaGroup = layout->mediaGroup;
    auto setting = FileSetting::getInstance( ident(media->name) );

    unsigned size = file->archiveDataSize(item->id);

    // unarchived tape files are writable which results in unpredictable filesizes
    // so they are loaded in chunks by an emulator callback
    auto data = mediaGroup->isTape() && !file->isArchived() ? nullptr
        : file->archiveData(item->id);

    if (!mediaGroup->isExpansion()) {
        emulator->ejectMedium(media);
        
        media->guid = uintptr_t(file);
        emulator->insertMedium(media, data, size);
        emulator->writeProtect(media, true);
        filePool->assign(ident(media->name), file);
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
    
    if (mediaGroup->selected && !media->memoryDump && !block->header.inUse.checked() ) {
        block->header.inUse.setChecked();
        block->header.inUse.onActivate();
    }

    filePool->assign(ident(media->name + "store"), file);    
    filePool->unloadOrphaned();

    setting->setPath(file->getFile());
    setting->setFile(item->info.name);
    setting->setId(item->id);
    setting->setWriteProtect(false);

    if (!mediaGroup->isExpansion())
        States::getInstance(emulator)->updateImage(setting, media);
    else
        States::getInstance(emulator)->forcePowerNextLoad = true;

    updateMediaBlock(block, setting);  
    
    if (mediaGroup->isDrive())
        EmuConfigView::TabWindow::getView(emulator)->statesLayout->updateSaveIdent( setting->file );
}

auto MediaWindow::eject( Emulator::Interface::MediaGroup* mediaGroup ) -> void {
    
    auto layout = getMediaGroupLayout( mediaGroup );
    
    for( auto block : layout->blocks)
        block->header.eject.onActivate();     
}

auto MediaWindow::getMediaGroupLayout( Emulator::Interface::MediaGroup* mediaGroup ) -> MediaGroupLayout* {
    
    for (auto layout : GUIKIT::Vector::concat(mediaGroupLayouts, cartLayouts) ) {
        
        if (layout->mediaGroup == mediaGroup)
            return layout;
    }
    
    return nullptr;
}

auto MediaWindow::showMediaGroupLayout( Emulator::Interface::MediaGroup* mediaGroup ) -> void {
    
    unsigned i = 0;
    
    for(auto layout : mediaGroupLayouts) {
        
        if (layout->mediaGroup == mediaGroup) {
            
            if (tabView.selection() != i) {
                tabView.setSelection( i );
            }
            return;
        }
        
        i++;
    }
    
    unsigned j = 0;
    
    for(auto layout : cartLayouts) {
        
        if (layout->mediaGroup == mediaGroup) {
            tabView.setSelection( i );
            cartList.setSelection( j );
            cartList.onChange();
            return;
        }
        
        j++;
    }
}

auto MediaWindow::getActiveLayout() -> MediaGroupLayout* {

    unsigned i = 0;

    for (auto layout : mediaGroupLayouts) {
        
        if (tabView.selection() == i)
            return layout;
                
        i++;
    }
    
    i = 0;
    
    if (cartList.selected())
        for(auto layout : cartLayouts) {

            if(cartList.selection() == i)
                return layout;

            i++;
        }
    
    return nullptr;
}

auto MediaWindow::colorListing( unsigned color, bool foreground ) -> void {	
    for(auto layout : mediaGroupLayouts) {
        if (foreground)
            layout->listings.setForegroundColor( color );
        else
            layout->listings.setBackgroundColor( color );
    }    	
	
    if (!foreground) {
        auto layout = getActiveLayout();
        if (layout) {
            layout->listings.setVisible(false);
            layout->listings.setVisible();
        }
    }
        
}

auto MediaWindow::drop( std::string filePath, MediaGroupLayout::Block* block ) -> void {    
    
    MediaGroupLayout* layout = nullptr;
    Emulator::Interface::MediaGroup* mediaGroup;
    
    if (!block) {
        unsigned pos = tabView.selection();
        
        if (pos < mediaGroupLayouts.size()) {
            layout = mediaGroupLayouts[ pos ];    
            
        } else if (pos == mediaGroupLayouts.size()) {
            
             pos = cartList.selection();
             
             if (pos < cartLayouts.size()) {
                 layout = cartLayouts[ pos ];    
             }
        }
        
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

auto MediaWindow::updateVisibility( Emulator::Interface::MediaGroup* mediaGroup, unsigned count) -> void {
    
    auto layout = getMediaGroupLayout( mediaGroup );
    
    if (!layout)
        return;
    
    layout->updateVisibility( count );
}

auto MediaWindow::updateWriteProtection( Emulator::Interface::Media* media, bool state ) -> void {
            
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
                        
            auto setting = FileSetting::getInstance( this->ident( media->name ) );
            
            if (state != block->header.writeprotect.checked())            
                block->header.writeprotect.setChecked( state );                            
            
            if (enabled != block->header.writeprotect.enabled())               
                block->header.writeprotect.setEnabled( enabled );                            
            
            setting->setWriteProtect( state );
                               
            break;
        }
    }
}

auto MediaWindow::updateJumper(Emulator::Interface::Media* media) -> void {
    
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

                settings->set<bool>(this->ident(saveIdent), state);

                jumperBox->setChecked(state);
            }
        }  
        
        if (media)        
            break;
    }    
}

auto MediaWindow::previewFile( std::string filePath, MediaGroupLayout::Block* block ) -> std::vector<GUIKIT::BrowserWindow::Listing> {
    
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
    
    layout->fillListing( convertedListings );
    
    if (!block)
        block = layout->getBlock( group->selected ? group->selected : &group->media[0] );
    
    if (!lastPreview.block) {
        block->header.fileName.setText( trans->get("Preview") );
        block->header.fileName.setFont( GUIKIT::Font::system("bold") );
        block->header.fileName.setForegroundColor( 0xff4500 );
    }
    
    lastPreview.block = block;
       
    showMediaGroupLayout( group );
	
    if (*alternateFileDialog) {
        if ( !visible() ) {
            setVisible();
            ftimer.setEnabled();
        } else if ( minimized() ) {
            restore(); 
            ftimer.setEnabled();
        } else if ( GUIKIT::Application::isWinApi() && !focused() && view->fullScreen() ) {
            setForeground();
            if (fileDialogPtr && fileDialogPtr->visible())
                fileDialogPtr->setForeground();
        }
    }
    
    return convertedListings;
    
    clearList:
        if (lastPreview.block)
            resetPreview();
    
    return {};
}

auto MediaWindow::resetPreview( bool light ) -> void {
    
    for(auto layout : mediaGroupLayouts) {
        
        if (!showC64Listing(layout))
            continue;
        
        if (!light)
            layout->updateListing( layout->selectedBlock ); 
        
        for (auto block : layout->blocks) {
            
            if (block->header.fileName.overrideForegroundColor()) {
                auto setting = FileSetting::getInstance( ident(block->media->name) );
                block->header.fileName.setText( setting ? setting->file : "" );
                block->header.fileName.setFont( GUIKIT::Font::system() );
                block->header.fileName.resetForegroundColor();                
            }            
        }
    }   
    
    lastPreview.block = nullptr;
}

auto MediaWindow::insertFile( MediaGroupLayout::Block* block, std::string filePath, bool autoLoad, unsigned selection ) -> bool {
    
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

            EmuConfigView::TabWindow::getView(emulator)->statesLayout->updateSaveIdent( file->getFileName() );
            
            if (mediaGroup->isTape())
                view->updateTapeIcons(Emulator::Interface::TapeMode::Play);

            view->setFocused(300);
        }
    };
    archiveViewer->setView(items);
    
    return true;
}

#define HideMouseIfWasBefore if (mIsAcquiredBefore && !inputDriver->mIsAcquired() && view->fullScreen() && fileDialogPtr && fileDialogPtr->detached()) inputDriver->mAcquire();

auto MediaWindow::anyLoad( bool mIsAcquiredBefore ) -> void {
    
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

			settings->set<std::string>(ident("anyload_path"), GUIKIT::File::getPath( filePath ) );

			view->autoloadInit( {filePath}, false, View::AutoLoad::AutoStart, selection );
			view->autoloadFiles();

			resetPreview();

            HideMouseIfWasBefore
            
			return true;
		} );

        fileDialogPtr->setContentViewFont(useCustomFont ? "C64 Pro Mono, 12" : "");
        fileDialogPtr->setContentViewBackground(mediaGroupLayouts[0]->listings.backgroundColor());
        fileDialogPtr->setContentViewForeground(mediaGroupLayouts[0]->listings.foregroundColor());
        fileDialogPtr->setContentViewColorTooltips(true);
    }
	
    fileDialogPtr->setTitle(trans->get("select image"));

    fileDialogPtr->setPath( settings->get<std::string>( ident("anyload_path"), "") );

    fileDialogPtr->setFilters({trans->get("all_files")});

    fileDialogPtr->setOnChangeCallback( [this](std::string file) {
        
        auto listings = this->previewFile(file);
        
        if (listings.size())
            settings->set<std::string>(ident("anyload_path"), GUIKIT::File::getPath( file ) );
        
        return listings;
    } );
    fileDialogPtr->addCustomButton( trans->get("open"), [this, mIsAcquiredBefore](std::string filePath, unsigned selection) {

        if (filePath.empty())
            return false;

        settings->set<std::string>(ident("anyload_path"), GUIKIT::File::getPath( filePath ) );
        
        view->autoloadInit( {filePath}, false, View::AutoLoad::Open );
        view->autoloadFiles();
        
        resetPreview();
        
        HideMouseIfWasBefore
        
        return true;
    }, IDC_BUTTON );       
    
    fileDialogPtr->setCallbacks( [this, mIsAcquiredBefore](std::string filePath, unsigned selection) {
        settings->set<std::string>(ident("anyload_path"), GUIKIT::File::getPath( filePath ) );
        
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
        settings->set<std::string>(ident("anyload_path"), GUIKIT::File::getPath( filePath ) );

        view->autoloadInit( {filePath}, false, View::AutoLoad::AutoStart, fileDialogPtr ? fileDialogPtr->getContentViewSelection() : 0 );
        view->autoloadFiles();
    } //else
        //message->warning("cancel multi");
    
    resetPreview();
    if (mIsAcquiredBefore && view->fullScreen())
        inputDriver->mAcquire();
}

auto MediaWindow::convertListing( std::vector<Emulator::Interface::Listing>& emuListings, bool loadCommand ) -> std::vector<std::string> {

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

auto MediaWindow::convertListing( std::vector<Emulator::Interface::Listing>& emuListings ) -> std::vector<GUIKIT::BrowserWindow::Listing> {

    std::vector<GUIKIT::BrowserWindow::Listing> list;
    
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
		
		utf8.clear();

        for (auto& code : listing.loadCommand ) {

			unsigned useCode = code;
            if (useCustomFont)
                useCode |= 0xee << 8;
			
            GUIKIT::Utf8::encode(useCode, utf8);
        }
		
		browserListing.tooltip = std::string((const char*) utf8.data(), utf8.size());
		
		list.push_back( browserListing );
    }  
    
    return list;
}

}
