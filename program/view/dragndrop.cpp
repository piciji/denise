
#include "view.h"
#include "../tools/filepool.h"

auto View::setDragnDrop() -> void {
    
    viewport.setDroppable();
    
    setDroppable();
    
    // aspect correct viewport doesn't fill up the complete window.
    // thats why, we have to set drop event on whole window too.
    // but viewport is on top of window, so we simply set drop event on both.
    onDrop = [this]( std::vector<std::string> files ) {
        viewport.onDrop( files );
    };
    
    viewport.onDrop = [this]( std::vector<std::string> files ) {

        autoloadInit( files, false, AutoLoad::DragnDrop );
        
        autoloadFiles();            
    };        
}

auto View::autoloadInit( std::vector<std::string> files, bool silentError, AutoLoad autoLoad, unsigned selection ) -> void {    
    ddControl.emulator = nullptr;
    ddControl.mediaGroups.clear();
    ddControl.silentError = silentError;
    ddControl.autoLoad = autoLoad;
    ddControl.selection = selection;
    ddControl.files.clear();
    
    unsigned i = 0;
    for( auto& file : files ) {        
        ddControl.files.push_back( file );
        
        if (++i == 7)
            break;
    }        
}

auto View::autoloadPostProcessing() -> void {

    if (ddControl.silentError)
        filePool->unloadOrphaned();
    
    if (ddControl.mediaGroups.size() == 0) {
        if (ddControl.silentError)
            program->exit(1);
        
        return;
    }
        
    std::sort(ddControl.mediaGroups.begin(), ddControl.mediaGroups.end(), [ ](const Emulator::Interface::MediaGroup* lhs, const Emulator::Interface::MediaGroup* rhs) {

        if (lhs->isExpansion()) return true;
        if (rhs->isExpansion()) return false;
        if (lhs->isProgram()) return true;
        if (rhs->isProgram()) return false;
        if (lhs->isDisk()) return true;
        return false;
    });

    auto mediaGroup = ddControl.mediaGroups[0];
    bool autoStart = true;
    FileSetting* fSetting = nullptr;
    
    if (ddControl.autoLoad == AutoLoad::DragnDrop) {
        autoStart = globalSettings->get<bool>("autostart_dragndrop", false);
        if (!autoStart) {
            if (mediaGroup->isExpansion() || mediaGroup->isProgram())
                autoStart = true;
            else if (!activeEmulator || (activeEmulator != ddControl.emulator) )
                autoStart = true;
        }
    } else if (ddControl.autoLoad == AutoLoad::Open)
        autoStart = false;
    else if (ddControl.autoLoad == AutoLoad::AutoStart)
        autoStart = true;    

    auto emuConfigView = EmuConfigView::TabWindow::getView(ddControl.emulator);
	
	auto mediaView = MediaView::MediaWindow::getView(ddControl.emulator);
    
    if (!autoStart) {

        if (mediaGroup->isDrive()) {
            if (mediaView->visible())
                mediaView->setFocused();
            
        } else {
            
            if (!mediaView->visible())
                mediaView->setVisible();

            mediaView->setFocused();
        }
        
        mediaView->showMediaGroupLayout(mediaGroup);

    } else {

		for (auto& mediaGroup : ddControl.emulator->mediaGroups) {
			
			if (mediaGroup.isDrive()) {
				
				emuConfigView->systemLayout->activateDrive(&mediaGroup, countImagesFor(&mediaGroup) );				
			}
		}
        
        if (mediaGroup->isExpansion())
            emuConfigView->systemLayout->setExpansion( mediaGroup->expansion );

        program->power( ddControl.emulator );

        if (!mediaGroup->isExpansion())
            program->removeBootableExpansion();        
        
        if (mediaGroup->selected) {
            ddControl.emulator->selectListing(mediaGroup->selected, ddControl.selection);
            fSetting = FileSetting::getInstance( ddControl.emulator, _underscore(mediaGroup->selected->name) );
        } else {
            ddControl.emulator->selectListing(&mediaGroup->media[0], ddControl.selection);
            fSetting = FileSetting::getInstance( ddControl.emulator, _underscore(mediaGroup->media[0].name) );
        }
        
        if (fSetting)
            emuConfigView->configurationsLayout->updateSaveIdent(fSetting->file);  
        
        if (mediaGroup->isTape())
            updateTapeIcons(Emulator::Interface::TapeMode::Play);  
        
        view->setFocused(300);
    }
    
    ddControl.mediaGroups.clear();
}

auto View::autoloadFiles() -> void {

    if (ddControl.files.size() == 0)
        return autoloadPostProcessing();    
    
    auto filePath = ddControl.files[0];
    
    GUIKIT::Vector::eraseVectorPos( ddControl.files, 0 );
    
    GUIKIT::File* file = filePool->get(filePath);
    if (!file)
        return autoloadFiles();

    if (!file->isSizeValid(MAX_MEDIUM_SIZE)) {
        if (!ddControl.silentError)
            program->errorMediumSize(file, message);

        return autoloadFiles();
    }

    auto& items = file->scanArchive();

    archiveViewer->onCallback = [this, file](GUIKIT::File::Item* item) {
        std::size_t end;
        std::string fileSuffix;

        if (!item || (item->info.size == 0))
            goto errorOpen;

        end = item->info.name.find_last_of(".");

        if (end == std::string::npos)
            goto errorOpen;

        fileSuffix = item->info.name.substr(end + 1);

        GUIKIT::String::toLowerCase(fileSuffix);

        for (auto emulator : emulators) {

            if (ddControl.emulator && ddControl.emulator != emulator)
                continue;

            auto mediaView = MediaView::MediaWindow::getView(emulator);

            for (auto& mediaGroup : emulator->mediaGroups) {

                if (mediaGroup.isHardDisk())
                    continue;

                auto mediaSuffixList = mediaGroup.suffix;

                for (auto& mediaSuffix : mediaSuffixList) {

                    if (mediaSuffix == fileSuffix) {
                        
                        Emulator::Interface::Media* media = nullptr;
                        
                        if (media = emulator->getMediaForCustomFileSuffix(fileSuffix)) {
                            // this is a special case for inserting memory dumps, C64 REU
                            EmuConfigView::TabWindow::getView(emulator)->systemLayout->setExpansion( mediaGroup.expansion );
                            ddControl.emulator = emulator;
                            mediaView->insertImage(media, file, item );

                            return autoloadFiles();
                        }
                        
                        if (mediaGroup.isExpansion()) {                                
                            auto analyzedExpansion = emulator->analyzeExpansion( file->archiveData(item->id), item->info.size );
                            
                            if (analyzedExpansion != mediaGroup.expansion)
                                continue;
                        }                                                
                        
                        unsigned alreadyInUse = countImagesFor(&mediaGroup);
                        media = mediaGroup.selected;
                        
                        if ( (media && alreadyInUse) || (alreadyInUse >= mediaGroup.media.size()))
                            return autoloadFiles();

                        ddControl.emulator = emulator;
                        
                        ddControl.mediaGroups.push_back(&mediaGroup);                        
                        
                        if (!media)
                            media = &mediaGroup.media[ alreadyInUse ];                      
                            						
                        mediaView->insertImage(media, file, item );

                        return autoloadFiles();
                    }
                }
            }
        }
        
errorOpen:
        if (!ddControl.silentError)
            program->errorOpen(file, item, message);

        return autoloadFiles();
    };

    archiveViewer->setView(items);
}

auto View::countImagesFor(Emulator::Interface::MediaGroup* mediaGroup) -> unsigned {
    
	unsigned counter = 0;
	for( auto _dG : ddControl.mediaGroups) {
		if (_dG == mediaGroup)
			counter++;
	}   
    
	return counter;
}
