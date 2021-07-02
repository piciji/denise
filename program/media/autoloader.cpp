
#include "autoloader.h"
#include "../tools/filepool.h"
#include "../emuconfig/config.h"
#include "../media/media.h"
#include "../view/view.h"
#include "../tools/filepool.h"
#include "../tools/filesetting.h"
#include "../config/archiveViewer.h"
#include "../cmd/cmd.h"
#include "../states/states.h"

Autoloader* autoloader = nullptr;

auto Autoloader::init( std::vector<std::string> files, bool silentError, Mode mode, unsigned selection ) -> void {    
    ddControl.emulator = nullptr;
    ddControl.mediaGroups.clear();
    ddControl.silentError = silentError;
    ddControl.mode = mode;
    ddControl.selection = selection;
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

        if (emuView)
            emuView->configurationsLayout->updateSaveIdent( ddControl.saveFile->getFileName( false ) );
        
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
    FileSetting* fSetting = nullptr;
    
    if (ddControl.mode == Mode::DragnDrop) {
        autoStart = globalSettings->get<bool>("autostart_dragndrop", false);
        if (!autoStart) {
            if (mediaGroup->isExpansion() || mediaGroup->isProgram())
                autoStart = true;
            else if (!activeEmulator || (activeEmulator != ddControl.emulator) )
                autoStart = true;
        }
    } else if (ddControl.mode == Mode::Open)
        autoStart = false;
    else if (ddControl.mode == Mode::AutoStart)
        autoStart = true;    	    
    
    if (!autoStart) {

        if (mediaGroup->isDrive()) {
            if (emuView->visible())
                emuView->setFocused();
            
        } else {
            
            if (!emuView->visible())
                emuView->setVisible();

            emuView->setFocused();
        }
        
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
				
				auto curExpansion = ddControl.emulator->getExpansionById( program->getSettings( ddControl.emulator )->get<unsigned>("expansion", 0) );
				
				if (curExpansion) {
					for (auto& exp : ddControl.emulator->expansions) {
						if (((exp.mediaGroup == &_mediaGroup) && (exp.mediaGroupExpanded == curExpansion->mediaGroup))
							|| ((exp.mediaGroupExpanded == &_mediaGroup) && (exp.mediaGroup == curExpansion->mediaGroup))) {
							useExpansion = &exp; // set expander
							break;
						}							
					}
				}
				
				program->getSettings( ddControl.emulator )->set<unsigned>("expansion", useExpansion->id);

                if (emuView) {
                    if (useExpansion->isRam()) {
                        emuView->mediaLayout->eject( useExpansion->mediaGroup, true);
						if (useExpansion->mediaGroupExpanded)
							emuView->mediaLayout->eject( useExpansion->mediaGroupExpanded, true);
					}

                    emuView->systemLayout->setExpansion( useExpansion );
                }
			}
		}
        
        program->power( ddControl.emulator, emuView );

        if (!useExpansion)
            program->removeExpansion();        
        
        if (mediaGroup->selected) {
            ddControl.emulator->selectListing(mediaGroup->selected, ddControl.selection);
            fSetting = FileSetting::getInstance( ddControl.emulator, _underscore(mediaGroup->selected->name) );
        } else {
            ddControl.emulator->selectListing(&mediaGroup->media[0], ddControl.selection);
            fSetting = FileSetting::getInstance( ddControl.emulator, _underscore(mediaGroup->media[0].name) );
        }
        
        if (emuView && fSetting)
            emuView->configurationsLayout->updateSaveIdent(fSetting->file);  
        
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

					if (emuView)
						emuView->mediaLayout->insertImage(media, file, item);
					else
						insertImage( emulator, media, file, item );

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

auto Autoloader::insertImage(Emulator::Interface* emulator, Emulator::Interface::Media* media, GUIKIT::File* file, GUIKIT::File::Item* item) -> void {
	
	auto mediaGroup = media->group;
	
	auto settings = program->getSettings( emulator );
	
	auto fSetting = FileSetting::getInstance( emulator, _underscore(media->name ) );
	
	unsigned size = file->archiveDataSize(item->id);

	auto data = mediaGroup->isTape() && !file->isArchived() ? nullptr
		: file->archiveData(item->id);

	if (!mediaGroup->isExpansion()) {
		emulator->ejectMedium(media);

		media->guid = uintptr_t(file);
		emulator->insertMedium(media, data, size);
		emulator->writeProtect(media, false);
		filePool->assign(_ident(emulator, media->name), file);
	} else {
		
		if (mediaGroup->expansion->pcbs.size())		
			settings->set<unsigned>( _underscore(media->name) + "_pcb", 0);
	}
	
	emulator->getListing(media);  
	
	if (mediaGroup->selected && !media->secondary )
		settings->set<unsigned>( _underscore(mediaGroup->name) + "_selected", media->id);

	filePool->assign(_ident(emulator, media->name + "store"), file);
	filePool->unloadOrphaned();

	fSetting->setPath(file->getFile());
	fSetting->setFile(item->info.name);
	fSetting->setId(item->id);
	fSetting->setWriteProtect(false);
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
        emuView->systemLayout->driveModelLayout.updateWidget( modelId );
        emuView->mediaLayout->updateVisibility( mediaGroup, requestedCount );
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
