
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
#include "recentFiles.h"
#include "../emuconfig/layouts/system.h"
#include "../helper/fileHelper.h"
#include "../helper/miscHelper.h"

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
    this->settings = Program::getSettings( emulator );
}

auto MediaLayout::setMediaView() -> void {
       
    GUIKIT::TreeViewItem* diskItem = nullptr;
    
    if (expansionParent && expansionParent->selected())
        return;    
    
    for(auto& nav : navElements) {
                
        if (nav.mediaGroup) {
            if (nav.tvi->selected())
                return;

            if (nav.mediaGroup->isDisk())
                diskItem = nav.tvi;
        }        
    }

    if (diskItem) {
        diskItem->setSelected();
        mediaTree.onChange(nullptr);
    }
}

auto MediaLayout::setDiskSwapperView() -> void {
    
    for(auto& nav : navElements) {

        if ( nav.layout && (nav.layout == swapperLayout) ) {
            if (!nav.tvi->selected()) {
                nav.tvi->setSelected();
                mediaTree.onChange(nullptr);
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

    if (navPos < navElements.size()) {
        auto& navElement = navElements[navPos];

        if (navElement.mediaGroup) {
            if (!navElement.layout) {
                auto mediaGroupLayout = new MediaGroupLayout( navElement.mediaGroup, this );
                navElement.layout = (GUIKIT::Layout*)mediaGroupLayout;
                mediaGroupLayout->build(12);

                moduleSwitch.setLayout( navPos, *mediaGroupLayout, {~0u, ~0u} );
                bindSelectorAction( mediaGroupLayout );
                translate(navElement);
            } else {
                for (auto block : ((MediaGroupLayout*)navElement.layout)->blocks) {
                    if (block->dirty) {
                        auto fSetting = FileSetting::getInstance(emulator, _underscore(block->media->name));
                        updateMediaBlock(block, fSetting);
                    }
                }
            }
        }

        moduleSwitch.setSelection( navPos );
    }    
}

auto MediaLayout::updateOptionsVisibility() -> void {

    auto item = mediaTree.selected();
    bool _enabledExpansion = false;
    bool _enabledTraps = false;

    if (item) {
        unsigned navPos = (unsigned)item->userData();

        if (navPos < navElements.size()) {
            auto& navElement = navElements[navPos];

            if (navElement.mediaGroup && navElement.mediaGroup->isDisk()) {
                _enabledTraps = true;
                useTraps.setChecked( settings->get<bool>("use_disk_traps", false) );
            } else if (navElement.mediaGroup && navElement.mediaGroup->isTape()) {
                _enabledTraps = true;
                useTraps.setChecked( settings->get<bool>("use_tape_traps", false) );
            } else if (navElement.mediaGroup && navElement.mediaGroup->isExpansion())
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
    closeImage.loadPng((uint8_t*) Icons::close, sizeof (Icons::close));
	pathImage.loadPng((uint8_t*) Icons::folderOpen, sizeof (Icons::folderOpen));       
    swapperImage.loadPng((uint8_t*) Icons::swapper, sizeof (Icons::swapper));
    
    imgFolderOpen.loadPng((uint8_t*)Icons::folderOpen, sizeof(Icons::folderOpen) );
    imgFolderClosed.loadPng((uint8_t*)Icons::folderClosed, sizeof(Icons::folderClosed) );
    imgDocument.loadPng((uint8_t*)Icons::document, sizeof(Icons::document) );
    settingsImage.loadPng((uint8_t*)Icons::settings, sizeof(Icons::settings) );
    searchImage.loadPng((uint8_t*)Icons::search, sizeof(Icons::search) );
    
    openImage.loadPng((uint8_t*)Icons::open, sizeof(Icons::open) );
    ejectImg.loadPng((uint8_t*)Icons::eject, sizeof(Icons::eject) );

    binaryImage.loadPng((uint8_t*)Icons::binary, sizeof(Icons::binary));

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
            tvi->setImage( binaryImage );
            
        } else {
            delete tvi;
            continue;
        }                                
        
        if (!mediaGroup.isExpansion())
            mediaTree.append(*tvi);
        
        tvi->setUserData( (uintptr_t)(navElements.size()) );

        MediaGroupLayout* mediaGroupLayout = nullptr;
        if (!mediaGroup.isExpansion()) {
            mediaGroupLayout = new MediaGroupLayout( &mediaGroup, this );
            mediaGroupLayout->build(previewFontSize);
        }

        navElements.push_back( { tvi, &mediaGroup, (GUIKIT::Layout*)mediaGroupLayout } );        
        
        if (mediaGroupLayout && mediaGroupLayout->showOnlyConnectedDevices()) {

            auto modelId = emulator->getModelIdOfEnabledDrives(&mediaGroup);

            unsigned counter = emulator->getModelValue( modelId );

            mediaGroupLayout->updateVisibility( counter, true );
        }        
           
        if (!mediaGroup.isExpansion()) {
            moduleSwitch.setLayout( navElements.size() - 1, *mediaGroupLayout, {~0u, ~0u} );
		    bindSelectorAction( mediaGroupLayout );
        } else {
            static GUIKIT::FixedLayout placeHolderLayout;
            moduleSwitch.setLayout( navElements.size() - 1, placeHolderLayout, {~0u, ~0u}, false );
        }
    }

    moduleFrame.append( mediaTree, { GUIKIT::Font::scale(170), ~0u}, 10 );
    if (dynamic_cast<LIBC64::Interface*>(emulator)) {
        moduleFrame.append(useTraps, {0u, 0u}, 10);
    }

    for(unsigned i = 8; i <= 16; i++) {
        fontSizeLayout.combo.append(std::to_string(i), i);
    }
    fontSizeLayout.combo.setSelectionByUserData((int)previewFontSize);
    dialogPreviewLayout.updateWidgets(settings, emulator);

    moduleFrame.append(fontSizeLayout, {0u, 0u}, 10);
    if (emulator->expansions.size() > 1) {
        bootCart.setEnabled(false);
        deactivateCart.setEnabled(false);
        moduleFrame.append(bootCart, {0u, 0u}, 10);
        moduleFrame.append(deactivateCart, {0u, 0u});
    }
    moduleFrame.setPadding(10);
    moduleFrame.setFont(GUIKIT::Font::system("bold"));

    append( moduleFrame, {0u, ~0u}, 10 );
    append( moduleSwitch, {~0u, ~0u} );
    
    mediaTree.onChange = [this](GUIKIT::TreeViewItem* selectedBefore) {

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
    navElements.push_back( { tvi, nullptr, (GUIKIT::Layout*)swapperLayout } );

    tvi = new GUIKIT::TreeViewItem;
    tvi->setText( "file dialog preview" );
    tvi->setImage( settingsImage );
    mediaTree.append(*tvi);
    moduleSwitch.setLayout( navElements.size(), dialogPreviewLayout, {~0u, ~0u} );
    tvi->setUserData( (uintptr_t)(navElements.size() ) );
    navElements.push_back( { tvi, nullptr, (GUIKIT::Layout*)&dialogPreviewLayout } );

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

	    block->selector.add.onActivate = [this, block]() {
	        if (!creatorWindow) {
	            creatorWindow = new CreatorWindow(this->emulator);
	            creatorWindow->build( this );
	        }

	        creatorWindow->open(block->media);
	    };
            
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
        
        if (block->selector.edit) {
            block->selector.edit->onFocus = [this, layout, block]() {
                fileloader->resetPreview( this->emulator, true);
                
                layout->selectedBlock = block;
                
                if (layout->mediaGroup->selected && !block->media->parent) {
                    layout->mediaGroup->selected = block->media;
                    settings->set<unsigned>( _underscore(layout->mediaGroup->name) + "_selected", block->media->id);
                    block->header.inUse.setChecked();
                }
                
                // if ( showListing( layout ) )
                //     layout->updateListing( block );                
            };

            block->selector.edit->onChange = [block, fSetting]() {

                auto group = block->media->group;

                if (group->isExpansion() && group->expansion->isRS232()) {

                    fSetting->setPath( block->selector.edit->text() );
                }
            };

        } else if (block->selector.pathCombo) {

            block->selector.pathCombo->onDrop = [this, layout, block](std::vector<std::string> files) {

                drop(files[0], block);
            };

            block->selector.pathCombo->onChange = [this, layout, block]() {

                drop(GUIKIT::File::resolveRelativePath(block->selector.pathCombo->text()), block);
            };
        }

        if (block->header.list) {
            block->header.list->onActivate = [this, layout, block]() {
                if (layout->blockContainer.children.size() <= 1)
                    return;

                fileloader->resetPreview(this->emulator, true);

                layout->selectedBlock = block;

                if (showListing(layout))
                    layout->updateListing(block);
            };
        }

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

        block->selector.open.setImage(&openImage);
	    block->selector.add.setImage(&addImage);
        block->header.eject.setImage(&ejectImg);

        if (mediaGroup->expansion && !block->media->parent) {
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
            GUIKIT::File* file = filePool->get(GUIKIT::File::resolveRelativePath(fSetting->path));
			uint8_t* data = nullptr;

            if (FileHelper::loadImageDataWhenOk(file, fSetting->id, mediaGroup, data)) {

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
            emuThread->lock(true);

            auto media = layout->selectedBlock->media;

            fileloader->autoload(emulator, media, selection, media->group->isDrive() && useTraps.checked());
            emuThread->unlock();
        };

        layout->control.inject.onActivate = [this, layout]() {

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

        layout->control.save.onActivate = [this, layout]() {
            createImage( layout->selectedBlock->media );
        };
    }

    useTraps.onToggle = [this](bool checked) {
        auto layout = this->getActiveMediaGroupLayout();

        if (layout && layout->mediaGroup->isDisk())
            settings->set<bool>("use_disk_traps", checked);
        else if (layout && layout->mediaGroup->isTape())
            settings->set<bool>("use_tape_traps", checked);
    };

    useTraps.setChecked( settings->get<bool>("use_disk_traps", false) );

    bootCart.onActivate = [this]() {

        auto mediaGroupLayout = getActiveMediaGroupLayout();

        if (!mediaGroupLayout || !mediaGroupLayout->mediaGroup->isExpansion())
            return;
        
		this->settings->set<unsigned>("expansion", mediaGroupLayout->mediaGroup->expansion->id);

        if (tabWindow->systemLayout)
            tabWindow->systemLayout->setExpansion( mediaGroupLayout->mediaGroup->expansion );

        emuThread->lock(true);
        program->power( emulator );
        if (statusHandler && emulator->isExpansionUnsupported())
            statusHandler->setMessage(trans->getA("unsupported cartridge"), true);
        emuThread->unlock();
        
        view->setFocused( 100 );
    };
    
    deactivateCart.onActivate = [this]() {        
        
		this->settings->set<unsigned>("expansion", 0);

        if (tabWindow->systemLayout)
            tabWindow->systemLayout->setExpansion( nullptr );

        emuThread->lock(true);
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

auto CreatorWindow::HdCreatorLayout::reset() -> void {
    cancel = true;
    progress.bar.setPosition(0);
    progress.label.setText("0 %");
}

auto CreatorWindow::open(Emulator::Interface::Media* media) -> void {
    this->media = media;
    auto mediaGroup = media->group;
    unsigned _width = 500;
    unsigned _height = 120;

    if (mediaGroup->isDisk()) {
        if (!hasAppended(diskCreatorLayout)) {
            removeActiveLayout();
            append( diskCreatorLayout );
        }
    } else if (mediaGroup->isHardDisk()) {
        hdCreatorLayout.reset();

        if (!hasAppended(hdCreatorLayout)) {
            removeActiveLayout();
            append( hdCreatorLayout );
        }
    } else if (mediaGroup->isTape()) {
        if (!hasAppended(tapeCreatorLayout)) {
            removeActiveLayout();
            append( tapeCreatorLayout );
        }
        _width = 270;
    } else if (mediaGroup->isExpansion()) {
        if (!hasAppended(cartCreatorLayout)) {
            removeActiveLayout();
            append( cartCreatorLayout );
        }
        _width = 270;
    }

    MiscHelper::centerGeometry( this, {_width, _height}, mediaLayout->tabWindow->geometry() );

    setVisible();
    setFocused();
}

auto CreatorWindow::build( MediaLayout* mediaLayout ) -> void {
    this->mediaLayout = mediaLayout;

    diskCreatorLayout.create.setImage( &mediaLayout->addImage );
    diskCreatorLayout.close.setImage( &mediaLayout->closeImage );

    hdCreatorLayout.creator.create.setImage( &mediaLayout->addImage );
    hdCreatorLayout.creator.close.setImage( &mediaLayout->closeImage );

    tapeCreatorLayout.create.setImage( &mediaLayout->addImage );
    tapeCreatorLayout.close.setImage( &mediaLayout->closeImage );

    cartCreatorLayout.create.setImage( &mediaLayout->addImage );
    cartCreatorLayout.close.setImage( &mediaLayout->closeImage );

    diskCreatorLayout.create.onActivate = [this, mediaLayout]() {
        if (mediaLayout->insertCreatedImage( media ))
            setVisible(false);
        else
            setFocused();
    };

    hdCreatorLayout.creator.create.onActivate = [this, mediaLayout]() {
        if (mediaLayout->insertCreatedImage( media ))
            setVisible(false);
        else
            setFocused();
    };

    tapeCreatorLayout.create.onActivate = [this, mediaLayout]() {
        if (mediaLayout->insertCreatedImage( media ))
            setVisible(false);
        else
            setFocused();
    };

    cartCreatorLayout.create.onActivate = [this, mediaLayout]() {
        if (mediaLayout->insertCreatedImage( media ))
            setVisible(false);
        else
            setFocused();
    };

    diskCreatorLayout.close.onActivate = [this]() {
        setVisible(false);
    };

    hdCreatorLayout.creator.close.onActivate = [this]() {
        hdCreatorLayout.reset();
        setVisible(false);
    };

    tapeCreatorLayout.close.onActivate = [this]() {
        setVisible(false);
    };

    cartCreatorLayout.close.onActivate = [this]() {
        setVisible(false);
    };

    translate();
}

auto CreatorWindow::translate() -> void {
    diskCreatorLayout.setText( trans->get("disk_creator") );

    diskCreatorLayout.block.format.label.setText(trans->get("format",{}, true));
    diskCreatorLayout.block.diskLabel.label.setText(trans->get("disk label",{}, true));
    diskCreatorLayout.options.fastFileSystem.setText(trans->get("ffs"));
    diskCreatorLayout.options.highDensity.setText(trans->get("high_density"));
    diskCreatorLayout.options.bootable.setText(trans->get("bootable"));

    hdCreatorLayout.setText( trans->get("harddisk_creator") );

    hdCreatorLayout.creator.diskSizeLabel.setText( trans->get("size_in_mb", {}, true) );
    hdCreatorLayout.creator.formatName.setText(trans->getA("format", true));

    tapeCreatorLayout.setText( trans->get("tape_creator") );
    tapeCreatorLayout.create.setText(trans->get("create"));
    tapeCreatorLayout.close.setText(trans->get("close"));

    cartCreatorLayout.setText( trans->get("memory_creator") );
    cartCreatorLayout.create.setText(trans->get("create"));
    cartCreatorLayout.close.setText(trans->get("close"));

    GUIKIT::Layout::alignChildWidth({&diskCreatorLayout.block.format, &diskCreatorLayout.block.diskLabel});
}

auto MediaLayout::insertCreatedImage(Emulator::Interface::Media* media) -> bool {
    auto filePtr = createImage( media );

    if (filePtr) {
        auto items = filePtr->scanArchive();
        emuThread->lock();
        insertImage(media, filePtr, &items[0]);
        emuThread->unlock();
    }
    return filePtr != nullptr;
}

auto MediaLayout::createImage( Emulator::Interface::Media* media ) -> GUIKIT::File* {
    auto mediaGroup = media->group;
    std::string title = mediaGroup->name + "_image";
    std::string suffix = mediaGroup->suffix[0];
    std::string fn;
    std::vector<std::vector<std::string>> replacements;
    GUIKIT::File file;
    GUIKIT::File* filePtr = nullptr;
    std::string filePath;
    uint8_t* data = nullptr;
    unsigned size = 0;
	
    if (mediaGroup->isExpansion()) {
        title = "cart_image";

        switch (media->type) {
            case Emulator::Interface::Media::MemoryType::SRAM:
                suffix = "bin";
                replacements.push_back({"%cart%", "SRAM"});
                break;
            case Emulator::Interface::Media::MemoryType::EEPROM:
                suffix = "bin";
                replacements.push_back({"%cart%", "EEPROM"});
                break;
            case Emulator::Interface::Media::MemoryType::FLASH:
                suffix = "crt";
                replacements.push_back({"%cart%", "FLASH"});
                break;
            default:
                return nullptr;
        }
    }

    if (mediaGroup->isHardDisk()) {
        if (!creatorWindow)
            return nullptr;

        auto& hdCreator = creatorWindow->hdCreatorLayout.creator;

        try {
            size = std::stoi( hdCreator.diskSize.text() );
            if (size > 102400) // 100 GB
                throw "";
        } catch (...) {
            message->error(trans->getA("invalid_input"));
            return nullptr;
        }    
        
        suffix = hdCreator.format.text();
        bool vhd = hdCreator.format.userData() != 0;
        Emulator::Interface::Data _data = emulator->createHardDiskImage((uint64_t)size * 1024ull * 1024ull, vhd);
        if (_data.ptr) {
            data = _data.ptr;
            size = _data.size;
        }
        
    } else if (mediaGroup->isDisk()) {
        if (!creatorWindow)
            return nullptr;

        auto& diskCreator = creatorWindow->diskCreatorLayout;

        suffix = diskCreator.block.format.combo.text();
        unsigned typeId = diskCreator.block.format.combo.userData();
        bool hd = diskCreator.options.highDensity.checked();
        bool bootable = diskCreator.options.bootable.checked();
        bool useFFS = diskCreator.options.fastFileSystem.checked();
        std::string diskName = diskCreator.block.diskLabel.edit.text();
        
        Emulator::Interface::Data _data = emulator->createDiskImage( typeId, diskName, hd, useFFS, bootable );
        data = _data.ptr;
        size = _data.size;

    } else if (mediaGroup->isTape()) {
        data = emulator->createTapeImage( size );
        
    } else if (mediaGroup->isProgram()) {
        data = emulator->getLoadedProgram( size );
        
    } else if (mediaGroup->isExpansion()) {
        data = emulator->createExpansionImage( media, size );
    }
    
    if (!size)
        goto Done; //internal error
    
    filePath = GUIKIT::BrowserWindow()
        .setWindow(*this->tabWindow)
        .setTitle(trans->get( "blank_" + title, replacements ))
        .setPath( fileloader->preselectPath( settings, mediaGroup->name ))
        .setFilters({GUIKIT::BrowserWindow::transformFilter( trans->get( title, replacements ), {suffix}), trans->get("all_files")})
        .save();

    if (filePath.empty())
        goto Done;

    fn = GUIKIT::String::getFileName(filePath);
    if (GUIKIT::String::getExtension(fn, "").empty())
        filePath += "." + suffix;

    filePtr = filePool->get( filePath );
    if (filePtr)
        filePtr->forceDataChange();
        
    file.setFile( filePath );

    if (!GUIKIT::Application::isCocoa()) {
        if (file.exists() && !message->question(trans->get("file_exist_error", {
                {"%path%", filePath}}))) {
            filePtr = nullptr;
            goto Done;
        }
    }

    if ( !file.open(GUIKIT::File::Mode::Write) ) {
        message->error(trans->get("file_creation_error",{
            {"%path%", filePath}
        }));
        filePtr = nullptr;
        goto Done;
    }

    settings->set<std::string>(_underscoreEx(mediaGroup->name) + "_folder_auto", GUIKIT::File::buildRelativePath(file.getPath()));

    if (data) {
        if (!file.write( data, size )) {
            message->error(trans->get("file_creation_error",{
                {"%path%", filePath}
            }));
            filePtr = nullptr;
            goto Done;
        }

        message->information(trans->get("file_creation_success",{
            {"%path%", filePath}
        }));
        
    } else {
        // hd creation in chunks without any data
        auto* hdCreator = &creatorWindow->hdCreatorLayout;
        file.unload();
        filePtr = nullptr; // insert delayed

      //  hdCreator->progress.bar.setPosition(0);
      //  hdCreator->progress.label.setText("0 %");
        hdCreator->creator.create.setEnabled(false);
        hdCreator->cancel = false;
        
        std::thread t1([this, size, filePath, media, hdCreator] {
            GUIKIT::File file(filePath);
            file.open(GUIKIT::File::Mode::Write);
            bool error = false;

            uint64_t totalKb = (uint64_t)size * 1024ull;
            uint64_t bufLengthKB = (totalKb * 2ull) / 100ull;
            if (bufLengthKB > (1024ull * 1024ull))
                bufLengthKB = 1024ull * 1024ull;

            auto* buf = new uint8_t[bufLengthKB * 1024ull];

            for (uint64_t offset = 0; offset < totalKb; offset += bufLengthKB) {
                std::memset(buf, 0, bufLengthKB * 1024ull);

                if ((offset + bufLengthKB) > totalKb)
                    bufLengthKB = totalKb - offset;

                if (file.write(buf, bufLengthKB * 1024ull, offset * 1024ull) == 0) {
                    message->error(trans->get("file_creation_error", {
                        {"%path%", filePath}
                    }));
                    error = true;
                    break;
                }

                if (hdCreator->cancel)
                    break;

                unsigned posPercent = ((double)(offset + bufLengthKB) * 100.0 / (double)totalKb) + 0.5;

                hdCreator->progress.bar.setPositionThreaded(posPercent);
                hdCreator->progress.label.setTextThreaded(std::to_string(posPercent) + " %");
            }

            delete[] buf;

            hdCreator->creator.create.setEnabledThreaded();

            if (error || hdCreator->cancel) {
                file.reset();
                file.del();
                return;
            }

            if (emuThread->enabled) {
                emuThread->events |= EmuThread::EVT_INSERT_MEDIA;
                emuThread->insertMedia.emulator = emulator;
                emuThread->insertMedia.media = media;
                emuThread->insertMedia.file = filePool->get( filePath );
            }
        });
        t1.detach();
    }
            
    Done:
    delete[] data;
    return filePtr;
}

auto MediaLayout::updateMediaBlock(MediaGroupLayout::Block* block, FileSetting* fSetting) -> void {

    bool IPMode = block->media->group->isExpansion() && block->media->group->expansion->isRS232();

    if (block->selector.edit)
        block->selector.edit->setText( fSetting->path );

    auto& pathCombo = block->selector.pathCombo;

    if (pathCombo) {
        auto recentFile = fileloader->getRecentFile(emulator);
        auto& files = recentFile->list(block->media->group, block->media->parent != nullptr, fSetting->path);

        //pathCombo->reset();
        std::vector<GUIKIT::ComboButton::Entry> rows;

        for (auto& file : files)
           rows.push_back( {file, 0, ""} );

        pathCombo->appendMulti( rows );

        if (fSetting->path.empty())
            pathCombo->unselect();
        else
            pathCombo->setSelectionByText(fSetting->path);
    }

    if (!IPMode) {
        block->header.fileName.setText(fSetting->file);
        block->header.writeprotect.setChecked( fSetting->writeProtect );
    }

    block->dirty = false;
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

auto MediaLayout::translate(NavElement& nav) -> void {   
    bool isC64 = dynamic_cast<LIBC64::Interface*>(emulator);
    auto mediaGroup = nav.mediaGroup;
    auto mediaGroupLayout = dynamic_cast<MediaGroupLayout*>(nav.layout);

    auto transInsert = trans->getOrEmpty(mediaGroup->name + "_insert");
    if (transInsert.empty())
        transInsert = trans->get("cart_insert", {{"%cart%", mediaGroup->name}});

    mediaGroupLayout->setText( transInsert );

    if (mediaGroup->isProgram()) {
        mediaGroupLayout->control.inject.setText( trans->get("program_inject") );
        mediaGroupLayout->control.save.setText( trans->get("program_creator") );
    } else if (mediaGroup->isTape())
        mediaGroupLayout->control.inject.setText( trans->get("tape spool") );

    for ( auto block : mediaGroupLayout->blocks ) {
        block->header.writeprotect.setText(trans->get("write_protected"));
        block->header.eject.setTooltip(trans->get("eject"));
        block->header.deviceName.setText( trans->get( block->media->name, {}, true ) );
        block->header.inUse.setText( trans->get( block->media->name, {}, true ) );

        if ( isC64 && (block->media->group->id == LIBC64::Interface::MediaGroupIdExpansionFinalChessCard)) {
            if (block->media->id > 3)
                block->header.deviceName.setTooltip( trans->getA( "SRAM Battery tooltip" ) );
            else if (block->media->id < 4)
                block->selector.open.setTooltip( trans->getA( "Final Chesscard ROMS tooltip" ) );
        }
        
        if (mediaGroup->isExpansion() && !block->media->parent) {
            unsigned id = 0;
            for( auto& pcb : mediaGroup->expansion->pcbs ) {
                block->selector.combo.setText(id++, trans->get( pcb.name ));
            }
                    
          //  block->selector.jumperLabel.setText( trans->get("jumper", {}, true) );
            
            for(auto& jumper : mediaGroup->expansion->jumpers) {

                auto jumperBox = block->selector.jumpers[jumper.id];

                jumperBox->setText( trans->get( jumper.name ) );

                if ( isC64 && (block->media->group->id == LIBC64::Interface::MediaGroupIdExpansionSuperCpu)) {
                    if (jumper.id == 2)
                        jumperBox->setTooltip(trans->getA("SuperCPU DRAM Boost tooltip"));
                }
            }        
        }
    } 
}

auto MediaLayout::translate() -> void {

    moduleFrame.setText( trans->get("selection") );   
    bootCart.setText( trans->get("boot cartridge") );
    useTraps.setText( trans->get("VDT Autostart") );
    useTraps.setTooltip( trans->get("VDT Autostart tooltip") );
    deactivateCart.setText( trans->get("deactivate cartridge") );

    fontSizeLayout.label.setText( trans->getA("Font Size") );

    dialogPreviewLayout.setText( trans->getA("File Dialog Preview") );
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
        if (nav.mediaGroup)
            nav.tvi->setText( trans->get( getMediaGroupTransIdent( nav.mediaGroup ) ) );
        else if (nav.layout && dynamic_cast<SwapperLayout*>(nav.layout))
            nav.tvi->setText( trans->get( emulator->getTapeMediaGroup() ? "swapper" : "disk swapper" ) );
        else if ( nav.layout == &dialogPreviewLayout )
            nav.tvi->setText( trans->get( "File Dialog Preview" ) );

        if (nav.layout && nav.mediaGroup)
            translate(nav);
    }

    if (creatorWindow) {
        creatorWindow->translate();
        if (creatorWindow->visible())
            creatorWindow->synchronizeLayout();
    }
        
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
    
    if (!layout) // (Expansion) UI has not been built yet.
        return fileloader->insertImage(emulator, media, file, item, options);

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

    if (!block)
        return;
    
    fileloader->resetPreview(this->emulator, true);
    auto layout = block->layout;
       
    auto media = block->media;
    auto mediaGroup = layout->mediaGroup;
    auto fSetting = FileSetting::getInstance( emulator, _underscore(media->name ) );

    uint64_t size = file->archiveDataSize(item->id);

    // unarchived tape files are writable which results in unpredictable filesizes
    // so they are loaded in chunks by an emulator callback
    auto data = mediaGroup->isHardDisk() && !file->isArchived() ? nullptr
        : file->archiveData(item->id);

    bool updateGenericFileList = !media->parent;

    auto useExpansion = emulator->getExpansion();

    auto insertedExpansion = emulator == activeEmulator && useExpansion && useExpansion->mediaGroup == mediaGroup;

    if (!mediaGroup->isExpansion() || (insertedExpansion && (media->parent && (mediaGroup->selected == media->parent)))) {
        emulator->ejectMedium(media);
        fileloader->updateFileSetting(fSetting, file, item);
        media->guid = uintptr_t(file);

        emulator->insertMedium(media, data, size);
        emulator->writeProtect(media, fSetting->writeProtect);
        if (!mediaGroup->isProgram())
            filePool->assign( _ident(emulator, media->name), file);

        if (mediaGroup->isHardDisk() && media->pcbLayout && mediaGroup->expansion->pcbs.size()) {
            block->selector.combo.setSelection(0);
            block->selector.combo.onChange();
        }
    } else {    
        auto ext = GUIKIT::String::getExtension(file->getFile(), "bin");
        GUIKIT::String::toLowerCase(ext);
        if (ext != "crt")
            updateGenericFileList = false;

        if (media->pcbLayout && mediaGroup->expansion->pcbs.size()) {
            block->selector.combo.setSelection(0);
            block->selector.combo.onChange();
        }
        fileloader->updateFileSetting(fSetting, file, item);
    }

    auto recentFile = fileloader->getRecentFile(emulator);
    recentFile->add(mediaGroup, media->parent != nullptr, GUIKIT::File::buildRelativePath(file->getFile()), updateGenericFileList);
    if (view)
        view->updateRecentList(emulator);

    if (showListing(layout)) {
        block->listings.clear();
        block->listings = emulator->getListing(media);
        layout->selectedBlock = block;
        layout->updateListing(block);
    }

    if (view && !fromState && activeEmulator && mediaGroup->isTape())
        view->updateTapeIcons();
    
    if (!dontUpdateSelected && mediaGroup->selected && !media->parent && !block->header.inUse.checked() ) {
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
        FileHelper::updateSaveIdent( emulator, fSetting );
}

auto MediaLayout::getMediaGroupLayout( Emulator::Interface::MediaGroup* mediaGroup ) -> MediaGroupLayout* {
    
    for (auto& nav : navElements) {
        
        if (nav.mediaGroup && (nav.mediaGroup == mediaGroup))
            return nav.layout ? dynamic_cast<MediaGroupLayout*>(nav.layout) : nullptr;
    }
    
    return nullptr;
}

auto MediaLayout::showMediaGroupLayout( Emulator::Interface::MediaGroup* mediaGroup ) -> void {
    
    for (auto& nav : navElements) {
        if (nav.mediaGroup && (nav.mediaGroup == mediaGroup)) {
            nav.tvi->setSelected();
            updateSwitchLayout();
            updateOptionsVisibility();
            break;
        }
    }
}

auto MediaLayout::getActiveMediaGroupLayout() -> MediaGroupLayout* {

    auto itemSelected = mediaTree.selected();
    
    if (itemSelected) {
        unsigned navPos = (unsigned)itemSelected->userData();
        if (navPos < navElements.size()) {
            auto& navElement = navElements[navPos];

            return (navElement.mediaGroup && navElement.layout)
            ? dynamic_cast<MediaGroupLayout*>(navElement.layout) : nullptr;
        }
    }
    
    return nullptr;
}

auto MediaLayout::colorListing( unsigned foregroundColor, unsigned backgroundColor ) -> void {

    for (auto& nav : navElements) {
        if (!nav.mediaGroup || !nav.layout)
            continue;

        auto mediaGroupLayout = dynamic_cast<MediaGroupLayout*>(nav.layout);

        if (showListing( mediaGroupLayout ) ) {
            mediaGroupLayout->listings.setForegroundColor( foregroundColor );
            mediaGroupLayout->listings.setBackgroundColor( backgroundColor );

            if (settings->get<bool>("software_preview_commodore_hi", true ))
                mediaGroupLayout->listings.setSelectionColor( backgroundColor, foregroundColor );
            else
                mediaGroupLayout->listings.resetSelectionColor();

            if (dynamic_cast<LIBAMI::Interface*>(emulator)) {
                mediaGroupLayout->listings.setRowForegroundColor( backgroundColor, 0 );
                mediaGroupLayout->listings.setRowBackgroundColor( foregroundColor, 0 );
            } else {
                mediaGroupLayout->listings.resetRowForegroundColor( 0 );
                mediaGroupLayout->listings.resetRowBackgroundColor( 0 );
            }
        }
    }
}

auto MediaLayout::selectionColorListing( ) -> void {
    bool comHi = settings->get<bool>("software_preview_commodore_hi", true );

    for (auto& nav : navElements) {
        if (!nav.mediaGroup || !nav.layout)
            continue;

        auto mediaGroupLayout = dynamic_cast<MediaGroupLayout*>(nav.layout);

        if (showListing( mediaGroupLayout ) ) {
            if (comHi)
                mediaGroupLayout->listings.setSelectionColor(
                    mediaGroupLayout->listings.backgroundColor(), mediaGroupLayout->listings.foregroundColor() );
            else
                mediaGroupLayout->listings.resetSelectionColor();
        }
    }
}

auto MediaLayout::fillListing(Emulator::Interface::Media* media, std::vector<GUIKIT::BrowserWindow::Listing>& listings, bool markPreview) -> void {

    auto layout = getMediaGroupLayout( media->group );

    if (!layout || !showListing(layout))
        return;

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
        layout = getActiveMediaGroupLayout();
        
        if (!layout)
            return;
                
        mediaGroup = layout->mediaGroup; 
        
        block = layout->selectedBlock;
        
    } else {
        mediaGroup = block->media->group;
        
        layout = getMediaGroupLayout( mediaGroup );

        if (!layout)
            return;
    }

    if (!block || !block->selector.pathCombo)
        return;

    GUIKIT::File* file = filePool->get(filePath);
    if (!file)
        return;

    if (!file->exists())
        return FileHelper::errorFileSize(MAX_MEDIUM_SIZE, file->getPath(), message);
    if (!mediaGroup->isHardDisk() && !file->isSizeValid(MAX_MEDIUM_SIZE))
        return FileHelper::errorFileSize(MAX_MEDIUM_SIZE, file->getPath(), message);

    auto& items = file->scanArchive();

    archiveViewer->onCallback = [this, block](GUIKIT::File* file, GUIKIT::File::Item* item) {

        if (!item || (item->info.size == 0) )
            return FileHelper::errorOpen( file, item, message );

        emuThread->lock();
        insertImage( block, file, item );
        if (block->media->group->isDrive()) {
            settings->set<int>("swap_pos", -1, false);
        }
        emuThread->unlock();
    };
    archiveViewer->allowNativeArchive(dynamic_cast<LIBAMI::Interface*>(emulator) ? mediaGroup : nullptr);
    archiveViewer->setView(file, items);
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
    if (media->parent)
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
        if (!nav.mediaGroup )
            continue;

        auto mediaGroupLayout = nav.layout ? dynamic_cast<MediaGroupLayout*>(nav.layout) : nullptr;
        
        if (!mediaGroupLayout || !showListing( mediaGroupLayout ))
            continue;
        
        if (!light)
            mediaGroupLayout->updateListing( mediaGroupLayout->selectedBlock ); 
        
        for (auto block : mediaGroupLayout->blocks) {
            
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
        if (!nav.mediaGroup || !nav.layout)
            continue;

        auto mediaGroupLayout = dynamic_cast<MediaGroupLayout*>(nav.layout);

        mediaGroupLayout->applyFont( fontSize );
    }
}

auto MediaLayout::updateListings( ) -> void {

    for(auto& nav : navElements) {
        if (!nav.mediaGroup || !nav.layout)
            continue;

        auto mediaGroupLayout = dynamic_cast<MediaGroupLayout*>(nav.layout);

        if (showListing( mediaGroupLayout ) )
            mediaGroupLayout->updateListing( mediaGroupLayout->selectedBlock );
    }
}

auto MediaLayout::loadSettings() -> void {

    auto selectedLayout = this->getActiveMediaGroupLayout();

    if (selectedLayout && selectedLayout->mediaGroup->isDisk())
        useTraps.setChecked( settings->get<bool>("use_disk_traps", false) );
    else if (selectedLayout && selectedLayout->mediaGroup->isTape())
        useTraps.setChecked( settings->get<bool>("use_tape_traps", false) );

    for(auto& nav : navElements) {
        if (!nav.mediaGroup)
            continue;

        if (!nav.layout) { // UI has not been built yet
            for(auto& media : nav.mediaGroup->media) {
                auto fSetting = FileSetting::getInstance(emulator, _underscore(media.name) );
                fSetting->update();
            }
            continue;
        }

        auto layout = dynamic_cast<MediaGroupLayout*>(nav.layout);
        
        layout->loadSettings();
        
        auto mediaGroup = layout->mediaGroup;

        if (mediaGroup->isDrive())
            layout->updateVisibility(emulator->getModelValue( emulator->getModelIdOfEnabledDrives(mediaGroup) ), true );
        
        for (auto block : layout->blocks) {
            
            auto fSetting = FileSetting::getInstance(emulator, _underscore(block->media->name) );
            fSetting->update();
            
            if (nav.tvi->selected() || (expansionParent && expansionParent->selected() && (nav.tvi->userData() == expansionParent->userData())))
                updateMediaBlock(block, fSetting);
            else
                block->dirty = true; // we can't update all comboboxes in software at once ... would hang a few seconds

            if ( showListing( layout ) ) {
                
                GUIKIT::File* file = filePool->get(GUIKIT::File::resolveRelativePath(fSetting->path));
                uint8_t* data = nullptr;
               
                if (FileHelper::loadImageDataWhenOk(file, fSetting->id, mediaGroup, data)) {
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
        fontSizeLayout.combo.setSelectionByUserData(previewFontSize);
        updateListingFont(previewFontSize);
    }
    dialogPreviewLayout.updateWidgets(settings, emulator);
    selectionColorListing();
    updateListings();

    swapperLayout->loadSettings();
}

auto MediaLayout::getBlock(Emulator::Interface::Media* media) -> MediaGroupLayout::Block* {
    for (auto& child : moduleSwitch.children) {
        if(dynamic_cast<MediaGroupLayout*>(child.sizable)) {
            auto layout = ((MediaGroupLayout*)child.sizable);
            auto block = layout->getBlock(media);
            if (block)
                return block;
        }
    }
    return nullptr;
}

}

