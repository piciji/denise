
#include "autoloader.h"
#include "fileloader.h"
#include "../tools/filepool.h"
#include "../emuconfig/config.h"
#include "../media/media.h"
#include "../view/view.h"
#include "../tools/filesetting.h"
#include "../config/archiveViewer.h"
#include "../cmd/cmd.h"
#include "../states/states.h"
#include "../firmware/manager.h"

Autoloader* autoloader = nullptr;

auto Autoloader::init( std::vector<std::string> files, bool silentError, Mode mode, unsigned selection, std::string fileName ) -> void {
    ddControl.emulator = nullptr;
    ddControl.mediaGroups.clear();
    ddControl.silentError = silentError;
    ddControl.mode = mode;
    ddControl.selection = selection;
    ddControl.fileName = fileName;
    ddControl.files.clear();
    ddControl.saveFile = nullptr;
    
    unsigned i = 0;
    for( auto& file : files ) {        
        ddControl.files.push_back( file );
        
        if (++i == 7)
            break;
    }        
}

auto Autoloader::postProcessing() -> void {

    if (ddControl.silentError)
        filePool->unloadOrphaned();
    
    auto emuView = EmuConfigView::TabWindow::getView( ddControl.emulator );
    
    if (ddControl.saveFile) {

        program->updateSaveIdent( ddControl.emulator, ddControl.saveFile->getFileName( false ) );

        filePool->assign("savestate", nullptr);
        
        States::getInstance( ddControl.emulator )->load( ddControl.saveFile->getFile() );
        
        return;
    }
    
    if (ddControl.mediaGroups.size() == 0) {
        if (ddControl.silentError)
            program->exit(1);
        
        return;
    }
        
    std::sort(ddControl.mediaGroups.begin(), ddControl.mediaGroups.end(), [ ](const Emulator::Interface::MediaGroup* lhs, const Emulator::Interface::MediaGroup* rhs) {

        if (lhs->isProgram()) return true;
        if (rhs->isProgram()) return false;
        if (lhs->isExpansion()) return true;
        if (rhs->isExpansion()) return false;
        if (lhs->isDisk()) return true;
        return false;
    });	

    auto mediaGroup = ddControl.mediaGroups[0];
    bool autoStart = true;
    bool trapped = false;
    bool forceStandardKernal = false;

    FileSetting* fSetting = nullptr;
    GUIKIT::Settings* settings = program->getSettings( ddControl.emulator );
    
    if (ddControl.mode == Mode::DragnDrop) {
        autoStart = globalSettings->get<bool>("autostart_dragndrop", false);
        if (!autoStart) {
            if (mediaGroup->isExpansion() || mediaGroup->isProgram())
                autoStart = true;
            else if (!activeEmulator || (activeEmulator != ddControl.emulator) )
                autoStart = true;
        }
        if (autoStart) {
            if (mediaGroup->isDisk())
                trapped = settings->get<bool>("use_disk_traps", false);
            else if (mediaGroup->isTape())
                trapped = settings->get<bool>("use_tape_traps", false);
        }
    } else if (ddControl.mode == Mode::Open)
        autoStart = false;
    else if (ddControl.mode == Mode::AutoStart) {
        autoStart = true;
        if (mediaGroup->isDisk())
            trapped = settings->get<bool>("use_disk_traps", false);
        else if (mediaGroup->isTape())
            trapped = settings->get<bool>("use_tape_traps", false);
    } else if (ddControl.mode == Mode::AutoStartPrimary) {
        autoStart = true;
        trapped = false;

    } else if (ddControl.mode == Mode::AutoStartSecondary) {
        autoStart = true;
        trapped = true;

    } else if (ddControl.mode == Mode::AutoStartDblClick) {
        autoStart = true;
        trapped = false;

        if (mediaGroup->isDrive()) {
            if (mediaGroup->isDisk())
                trapped = settings->get<bool>("autostart_traps_on_dblclick", false);
            else if (mediaGroup->isTape())
                trapped = settings->get<bool>("autostart_tape_traps_on_dblclick", false);
        }
    }

    if (trapped) // traps require standard kernals
        forceStandardKernal = true;
    else if (mediaGroup->isTape()) {
        forceStandardKernal = settings->get<bool>("autostart_tape_standard_kernal", false);
    }

    if (!autoStart) {

        if (mediaGroup->isDrive()) {
            if (view && activeEmulator && (ddControl.mode == Mode::Open) )
                view->setFocused(300);
            else if ( emuView && emuView->visible())
                emuView->setFocused();
            
        } else {
            // not autostarted expansion needs settings window
            if (!emuView)
                emuView = EmuConfigView::TabWindow::getView( ddControl.emulator, true );

            emuView->setLayout( EmuConfigView::TabWindow::Layout::Media );

            if (!emuView->visible())
                emuView->setVisible();

            emuView->setFocused();
        }

        if (emuView && emuView->mediaLayout)
            emuView->mediaLayout->showMediaGroupLayout(mediaGroup);

    } else {

        Emulator::Interface::Expansion* useExpansion = nullptr;

		for (auto& _mediaGroup : ddControl.emulator->mediaGroups) {

            unsigned count = countImagesFor(&_mediaGroup);

            if (!count)
                continue;

			if (_mediaGroup.isDrive()) {
				activateDrive( ddControl.emulator, &_mediaGroup, count );
			}
			else if (_mediaGroup.isExpansion()) {
				
				useExpansion = _mediaGroup.expansion;
				
				auto curExpansion = ddControl.emulator->getExpansionById( settings->get<unsigned>("expansion", 0) );
				
				if (curExpansion) {
					for (auto& exp : ddControl.emulator->expansions) {
						if (((exp.mediaGroup == &_mediaGroup) && (exp.mediaGroupExpanded == curExpansion->mediaGroup))
							|| ((exp.mediaGroupExpanded == &_mediaGroup) && (exp.mediaGroup == curExpansion->mediaGroup))) {
							useExpansion = &exp; // set expander
							break;
						}							
					}
				}
				
				settings->set<unsigned>("expansion", useExpansion->id);

                if (useExpansion->isRam()) {
                    fileloader->eject( ddControl.emulator, useExpansion->mediaGroup, true );

                    if (useExpansion->mediaGroupExpanded) {
                        fileloader->eject( ddControl.emulator, useExpansion->mediaGroupExpanded, true );
                    }
                }

                if (emuView && emuView->systemLayout)
                    emuView->systemLayout->setExpansion( useExpansion );
			}
		}
        
        program->power( ddControl.emulator, emuView != nullptr );

        if (!useExpansion)
            program->removeExpansion();

        useExpansion = ddControl.emulator->getExpansion();
        if (trapped && (useExpansion && !useExpansion->isEmpty()))
            trapped = false;
        else if (forceStandardKernal) {
            // temporary disable any speeders
            FirmwareManager::getInstance( ddControl.emulator )->insertDefault();
        }

        if (mediaGroup->selected) {
            ddControl.emulator->selectListing(mediaGroup->selected, ddControl.selection, ddControl.fileName, trapped);
            fSetting = FileSetting::getInstance( ddControl.emulator, _underscore(mediaGroup->selected->name) );
        } else {
            ddControl.emulator->selectListing(&mediaGroup->media[0], ddControl.selection, ddControl.fileName, trapped);
            fSetting = FileSetting::getInstance( ddControl.emulator, _underscore(mediaGroup->media[0].name) );
        }
        
        if (fSetting)
            program->updateSaveIdent( ddControl.emulator, fSetting->file);
        
		if (view) {
			if (mediaGroup->isTape())
				view->updateTapeIcons(Emulator::Interface::TapeMode::Play);

			view->setFocused(300);

			if (!cmd->debug && (mediaGroup->isTape() || mediaGroup->isDisk()) ) {
                program->initAutoWarp(mediaGroup);
			}
		}
    }
}

auto Autoloader::loadFiles() -> void {

    if (ddControl.files.size() == 0)
        return postProcessing();    
    
    auto filePath = ddControl.files[0];
    
    GUIKIT::Vector::eraseVectorPos( ddControl.files, 0 );
    
    GUIKIT::File* file = filePool->get(filePath);
    if (!file)
        return loadFiles();

    if (!file->isSizeValid(MAX_MEDIUM_SIZE)) {
        if (!ddControl.silentError)
            program->errorMediumSize(file, view->message);

        return loadFiles();
    }

    auto& items = file->scanArchive();

	if (archiveViewer) {
		archiveViewer->onCallback = [this, file](GUIKIT::File::Item* item) {
			this->loadFile( file, item );
		};

		archiveViewer->setView( items );
	} else 
		loadFile( file, &items[0] );
}

auto Autoloader::loadFile( GUIKIT::File* file, GUIKIT::File::Item* item ) -> void {

	std::size_t end;
	std::string fileSuffix;

	if (!item || (item->info.size == 0))
		goto errorOpen;

    if (!cmd->noGui) {
        if ( checkForSavestate( file, item ) )
            return loadFiles();
    }
    
	end = item->info.name.find_last_of(".");

	if (end == std::string::npos)
		goto errorOpen;

	fileSuffix = item->info.name.substr(end + 1);

	GUIKIT::String::toLowerCase(fileSuffix);

	for (auto emulator : emulators) {

		if (ddControl.emulator && ddControl.emulator != emulator)
			continue;

		auto emuView = EmuConfigView::TabWindow::getView(emulator);

		for (auto& mediaGroup : emulator->mediaGroups) {

			if (mediaGroup.isHardDisk())
				continue;

			for (auto& mediaSuffix : mediaGroup.suffix) {

				if (mediaSuffix == fileSuffix) {

					if (mediaGroup.isExpansion()) {
						auto analyzedExpansion = emulator->analyzeExpansion(file->archiveData(item->id), item->info.size, fileSuffix);

						if (analyzedExpansion != mediaGroup.expansion)
							continue;
					}

					unsigned alreadyInUse = countImagesFor(&mediaGroup);
                    Emulator::Interface::Media* media = mediaGroup.selected;

					if ((media && alreadyInUse) || (alreadyInUse >= mediaGroup.media.size()))
						return loadFiles();

					ddControl.emulator = emulator;

					if (!media)
						media = &mediaGroup.media[ alreadyInUse ];

					if (emuView && emuView->mediaLayout)
                        emuView->mediaLayout->insertImage(media, file, item);
					else
						fileloader->insertImage( emulator, media, file, item );

                    ddControl.mediaGroups.push_back(&mediaGroup);

					return loadFiles();
				}
			}
		}
	}

errorOpen:
	if (!ddControl.silentError)
		program->errorOpen(file, item, view->message);

	return loadFiles();
}

auto Autoloader::countImagesFor(Emulator::Interface::MediaGroup* mediaGroup) -> unsigned {
    
	unsigned counter = 0;
	for( auto _mG : ddControl.mediaGroups) {
		if (_mG == mediaGroup)
			counter++;
	}   
    
	return counter;
}

auto Autoloader::activateDrive( Emulator::Interface* emulator, Emulator::Interface::MediaGroup* mediaGroup, unsigned requestedCount ) -> void {
	
    if (requestedCount > mediaGroup->media.size())
        requestedCount = mediaGroup->media.size();

    auto modelId = emulator->getModelIdOfEnabledDrives( mediaGroup );

    unsigned counter = emulator->getModelValue( modelId );

	if (counter >= requestedCount)
		return;

    auto settings = program->getSettings( emulator );

    auto model = emulator->getModel( modelId );

    if (model)
        settings->set<unsigned>(_underscore(model->name), requestedCount);

    emulator->setModelValue( modelId, requestedCount );

	settings->remove( "access_floppy" );

    auto emuView = EmuConfigView::TabWindow::getView( emulator );

    if (emuView) {
        if(emuView->systemLayout) emuView->systemLayout->driveModelLayout.updateWidget( modelId );
        if(emuView->mediaLayout) emuView->mediaLayout->updateVisibility( mediaGroup, requestedCount );
    }
}

auto Autoloader::checkForSavestate( GUIKIT::File* file, GUIKIT::File::Item* item ) -> bool {
    
    std::string path = file->getFile();
    auto parts = GUIKIT::String::split( path, '.' );
    bool found = false;
    
    if (parts.size() == 1)
        return false;
    
    if (parts.back() == "images") {
        
        parts.pop_back();
        
        if (parts.back() == "sav") {
            
            path = path.substr(0, path.size() - 7);
            
            found = true;
        }
        
    } else if (parts.back() == "sav")        
        found = true;    
    
    if (!found)
        return false;
        
    uint8_t* data = file->archiveData(item->id);
    unsigned size = item->info.size;
    
    if (file->getPath() != path) {
        file = filePool->get( path );
        
        if (!file->open()) {
            program->errorOpen(file, nullptr, view->message);
            goto End;
        }
            
        data = file->read();
        size = file->getSize();
    }        
    
    for (auto emulator : emulators) {
        
        if (emulator->checkstate( data, size )) {
     
            filePool->assign("savestate", file);
            ddControl.saveFile = file;
            ddControl.emulator = emulator;
            goto End;
        }
    }
    
    view->message->error( trans->get("state_incompatible", {{"%ident%", file->getFile()}}) );
    
End:
    filePool->unloadOrphaned();
    return true;
}
