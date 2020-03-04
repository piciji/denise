
#include "media.h"
#include "../cmd/cmd.h"
#include "../view/view.h"
#include "../view/message.h"
#include "../tools/filepool.h"
#include "../tools/filesetting.h"
#include "../config/archiveViewer.h"
#include "../states/states.h"

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
	
	if (emulator->ident == "C64" && !cmd->debug) {
        GUIKIT::CustomFont* font = new GUIKIT::CustomFont;
        font->name = "C64 Pro Mono";
        font->data = (uint8_t*)Fonts::c64ProMono;
        font->size = sizeof(Fonts::c64ProMono);	
        font->filePath = program->fontFolder() + "/C64_Pro_Mono-STYLE.ttf";
        useCustomFont = MediaWindow::addCustomFont( font );
        ((LIBC64::Interface*) emulator)->convertPetsciiToScreencode( useCustomFont );
    }
    
    GUIKIT::Geometry defaultGeometry = {100, 100, 850, 540};
    
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
    
    tabView.setMargin(10);
	tabView.setPadding(10);

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
            tabView.appendHeader("", hdImage);
            
        } else if (mediaGroup.isDisk()) {
            tabView.appendHeader("", diskImage);
            
        } else if (mediaGroup.isTape()) {
            tabView.appendHeader("", tapeImage);
            
        } else if (mediaGroup.isExpansion()) {
            tabView.appendHeader("", expansionImage);
           
		} else if (mediaGroup.isMemory()) {
            tabView.appendHeader("", memoryImage);
            
        } else 
            continue;
        
        MediaGroupLayout* mediaGroupLayout = new MediaGroupLayout( &mediaGroup, this );
        
        mediaGroupLayouts.push_back( mediaGroupLayout );
        
        mediaGroupLayout->build( );
        
        if (mediaGroupLayout->showOnlyConnectedDevices()) {
            
            unsigned counter = settings->get( ident(mediaGroup.name + "_count"), 1);
            
            mediaGroupLayout->updateVisibility( counter, true );
        }        
        
        tabs.push_back( getMediaGroupTransIdent(&mediaGroup) );
                
        tabView.setLayout(i++, *mediaGroupLayout, {~0u, ~0u});   
		
		bindSelectorAction( mediaGroupLayout );
    }
    
    tabView.appendHeader("", addImage); 
    tabs.push_back("create");    
    prepareCreator();
    tabView.setLayout(i++, creatorLayout, {~0u, 0u});  
    
    tabView.appendHeader("", pathImage); 
    tabs.push_back("paths");
    preparePaths();        
    tabView.setLayout(i++, pathsLayout, {~0u, 0u});
    
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
                    .setWindow(*this)
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
            
            block->selector.openW.onActivate = [this, block]() {                
                block->openWritable = true;
                block->selector.open.onActivate();
                block->openWritable = false;
            };
            
			block->selector.open.onActivate = [this, block, mediaGroup, setting, layout]() {                

                auto suffix = mediaGroup->suffix;
                GUIKIT::Vector::combine(suffix, GUIKIT::File::suppportedCompressionExtensions());    
                
				std::string filePath = GUIKIT::BrowserWindow()
					.setWindow(*this)
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
                
				if (!file->isSizeValid(MAX_MEDIUM_SIZE))
                    return program->errorMediumSize( file, message );				
                
                if ( block->openWritable && file->isArchived())
                    message->warning(trans->get("archive_wp_tooltip"));
                
				auto& items = file->scanArchive();

				archiveViewer->onCallback = [this, file, block, layout](GUIKIT::File::Item* item) {

                    if (!item || (item->info.size == 0) )
                        return program->errorOpen( file, item, message );                     
                                        
                    insertImage( layout, block, file, item );
				};
				archiveViewer->setView(items);
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
                
				
				if ( showC64Listing( layout, block ) ) {
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
                
                settings->set<unsigned>(ident(layout->mediaGroup->name + "_selected"), block->media->id);
            };
            
            block->selector.combo.onChange = [this, layout, block]() {
                
                int userData = block->selector.combo.userData();
                
                for( auto& pcb : layout->mediaGroup->getExpansion()->pcbs ) {
                    
                    if (pcb.id == userData) {
                        
                        block->media->pcbLayout = &pcb; 
                        
                        settings->set<unsigned>(ident(block->media->name + "_pcb"), pcb.id);
                        
                        break;
                    }
                }                                
            };
		}

        if (block->media->expansion) {
            for (auto& jumper : block->media->expansion->jumpers) {

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

		if ( showC64Listing( layout, block ) ) { //preload last listing
			GUIKIT::File* file = filePool->get( setting->path );
			uint8_t* data;

            if (program->loadImageDataWhenOk(file, setting->id, mediaGroup, data)) {
				filePool->assign(ident(block->media->name + "store"), file);
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
            Emulator::Interface::Media* mediaForSaveIdent = nullptr;
            
            if (layout->mediaGroup->isMemory()) {
                for (auto& media : layout->mediaGroup->media) {
                    emulator->selectListing(&media, layout->listings.selection() ); 
                    
                    if (!media.expansion) 
                        mediaForSaveIdent = &media;                    
                }                

            } else {
                emulator->selectListing( layout->selectedBlock->media, selection );
                mediaForSaveIdent = layout->selectedBlock->media;
            }
            
            if (mediaForSaveIdent) {
                auto setting = FileSetting::getInstance(ident(mediaForSaveIdent->name));
                if (setting)
					EmuConfigView::TabWindow::getView(emulator)->statesLayout->updateSaveIdent(setting->file);
            }
            
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

auto MediaWindow::createImage( Emulator::Interface::MediaGroup* mediaGroup ) -> void {

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
        
    } else if (mediaGroup->isMemory()) {
        data = emulator->getLoadedMemory( size );
        
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

auto MediaWindow::preparePaths() -> void {
    
    for (auto& mediaGroup : emulator->mediaGroups) {
        
        auto settingFolderIdent = ident( mediaGroup.name + "_folder" );
        
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
    block->header.writeprotect.setChecked( setting->writeProtect );
    block->header.writeprotect.setEnabled( setting->wpEnabled );
}

auto MediaWindow::updateListing( Emulator::Interface::Media* media ) -> void {    
    
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
                      
                block->selector.jumperLabel.setText( trans->get("jumper", {}, true) );
                
                for(auto& jumper : mediaGroup->getExpansion()->jumpers) {

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

auto MediaWindow::getMediaGroupTransIdent( Emulator::Interface::MediaGroup* mediaGroup ) -> std::string {
    auto ident = mediaGroup->name;
    
    if (mediaGroup->isDrive() || (mediaGroup->isExpansion() && mediaGroup->getExpansion()->isGame()) )
        ident += "s";
    
    return ident;
}

auto MediaWindow::showC64Listing( MediaGroupLayout* layout, MediaGroupLayout::Block* block ) -> bool {
    
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

auto MediaWindow::insertImage(Emulator::Interface::Media* media, GUIKIT::File* file, GUIKIT::File::Item* item) -> void {
    
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

auto MediaWindow::insertImage( MediaGroupLayout* layout, MediaGroupLayout::Block* block, GUIKIT::File* file, GUIKIT::File::Item* item ) -> void {
   
    if (!layout)
        return;

    if (!block)
        block = layout->blocks[0];                              
       
    auto media = block->media;
    auto mediaGroup = layout->mediaGroup;
    auto setting = FileSetting::getInstance( ident(media->name) );

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
        filePool->assign(ident(media->name), file);
    } else {        
        if (mediaGroup->getExpansion()->pcbs.size()) {
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

    filePool->assign(ident(media->name + "store"), file);    
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
    
    if (mediaGroup->isDrive())
        EmuConfigView::TabWindow::getView(emulator)->statesLayout->updateSaveIdent( setting->file );
}

auto MediaWindow::eject( Emulator::Interface::MediaGroup* mediaGroup ) -> void {
    
    auto layout = getMediaGroupLayout( mediaGroup );
    
    for( auto block : layout->blocks)
        block->header.eject.onActivate();     
}

auto MediaWindow::getMediaGroupLayout( Emulator::Interface::MediaGroup* mediaGroup ) -> MediaGroupLayout* {
    
    for (auto layout : mediaGroupLayouts) {
        
        if (layout->mediaGroup == mediaGroup)
            return layout;
    }
    
    return nullptr;
}

auto MediaWindow::showMediaGroupLayout( Emulator::Interface::MediaGroup* mediaGroup ) -> void {
    
    unsigned i = 0;
    
    for(auto layout : mediaGroupLayouts) {
        
        if (layout->mediaGroup == mediaGroup) {
            tabView.setSelection( i );
            
            break;
        }
        
        i++;
    }
}

auto MediaWindow::colorListing( unsigned color, bool foreground ) -> void {
    for(auto layout : mediaGroupLayouts) {
        if (foreground)
            layout->listings.setForegroundColor( color );
        else
            layout->listings.setBackgroundColor( color );
    }
}

auto MediaWindow::drop( std::string filePath, MediaGroupLayout::Block* block ) -> void {    
    
    MediaGroupLayout* layout;
    Emulator::Interface::MediaGroup* mediaGroup;
    
    if (!block) {
        layout = mediaGroupLayouts[ tabView.selection() ];
        
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

    archiveViewer->onCallback = [this, file, block, layout](GUIKIT::File::Item* item) {

        if (!item || (item->info.size == 0) )
            return program->errorOpen( file, item, message );        

        insertImage( layout, block, file, item );        
    };

    archiveViewer->setView(items);
}

auto MediaWindow::updateVisibility( Emulator::Interface::MediaGroup* mediaGroup, unsigned count) -> void {
    
    auto layout = getMediaGroupLayout( mediaGroup );
    
    if (!layout)
        return;
    
    layout->updateVisibility( count );
}

auto MediaWindow::disableWriteProtection(Emulator::Interface::Media* media) -> void {
    
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

auto MediaWindow::changeWriteProtection(Emulator::Interface::Media* media, bool state) -> void {
    
    auto layout = getMediaGroupLayout(media->group);
    
    if (!layout)
        return;
    
    for(auto block : layout->blocks) {
        
        if (block->media == media) {
                
            block->header.writeprotect.setEnabled();
            block->header.writeprotect.setChecked(state);
             
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

        for (auto& jumper : media->group->getExpansion()->jumpers) {

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

}
