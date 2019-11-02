
#include "view.h"
#include "../tools/filepool.h"
#include "../config/archiveViewer.h"
#include "../emuconfig/config.h"

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

        autoloadInit( files, false );
        
        autoloadFiles();            
    };        
}

auto View::autoloadInit( std::vector<std::string>& files, bool silentError ) -> void {    
    ddControl.emulator = nullptr;
    ddControl.mediaGroups.clear();
    ddControl.silentError = silentError;
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
        if (lhs->isMemory()) return true;
        if (rhs->isMemory()) return false;
        if (lhs->isDisk()) return true;
        return false;
    });

    auto autoStart = settings->get<bool>("autostart_dragndrop", false);

    auto emuConfigView = EmuConfigView::TabWindow::getView(ddControl.emulator);

    auto mediaGroup = ddControl.mediaGroups[0];

    if (!autoStart && (activeEmulator == ddControl.emulator)) {

        emuConfigView->show(EmuConfigView::TabWindow::Layout::Media);

        emuConfigView->mediaLayout->showMediaGroupLayout(mediaGroup);

    } else {

		for (auto& mediaGroup : ddControl.emulator->mediaGroups) {
			
			if (mediaGroup.isDrive()) {
				
				emuConfigView->systemLayout->activateDrive(&mediaGroup, countImagesFor(&mediaGroup) );				
			}
		}
	                                            
        emuConfigView->systemLayout->handleExpansionIfAutoBoot( mediaGroup->isExpansion() );
        
        program->power(ddControl.emulator);

        if (mediaGroup->isMemory()) {
            for (auto& media : mediaGroup->media)
                ddControl.emulator->selectListing(&media, 0); 
        } else                
            ddControl.emulator->selectListing(&mediaGroup->media[0], 0);

        if (mediaGroup->isTape())
            updateTapeIcons(Emulator::Interface::TapeMode::Play);

        view->setFocused(300);
    }
}

auto View::autoloadFiles() -> void {

    if (ddControl.files.size() == 0)
        return autoloadPostProcessing();    
    
    auto filePath = ddControl.files[0];
    
    GUIKIT::Vector::eraseVectorPos( ddControl.files, 0 );
    
    GUIKIT::File* file = filePool->get(filePath);
    if (!file)
        return autoloadFiles();

    if (file->getSize() > MAX_ARCHIVE_SIZE) {
        if (!ddControl.silentError)
            program->errorArchiveSize(file, message);

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

            auto emuConfigView = EmuConfigView::TabWindow::getView(emulator);

            for (auto& mediaGroup : emulator->mediaGroups) {

                if (mediaGroup.isHardDisk())
                    continue;
                
                if (mediaGroup.isExpansion() && !mediaGroup.getExpansion()->isGame())
                    // todo: distinguish between more expansion types
                    continue;

                auto mediaSuffixList = mediaGroup.suffix;

                for (auto& mediaSuffix : mediaSuffixList) {

                    if (mediaSuffix == fileSuffix) {
                        
                        unsigned alreadyInUse = countImagesFor(&mediaGroup);
                        Emulator::Interface::Media* media = mediaGroup.selected;
                        
                        if ( (media && alreadyInUse) || (alreadyInUse >= mediaGroup.media.size()))
                            return autoloadFiles();

                        if (!mediaGroup.isTape() && !mediaGroup.isMemory() && (item->info.size > MAX_MEDIUM_SIZE))
                            goto errorSize;

                        ddControl.emulator = emulator;
                        
                        ddControl.mediaGroups.push_back(&mediaGroup);                        
                        
                        if (!media) {                          
                            if (mediaGroup.isMemory())
                                // todo: distinguish between more memory types
                                media = (item->info.size >= (128 * 1024)) ? &mediaGroup.media[1] : &mediaGroup.media[0];
                            else
                                media = &mediaGroup.media[ alreadyInUse ];                                                
                        }
                            						
                        emuConfigView->mediaLayout->insertImage(media, file, item );

                        return autoloadFiles();
                    }
                }
            }
        }
        
errorOpen:
        if (!ddControl.silentError)
            program->errorOpen(file, item, message);

        return autoloadFiles();

errorSize:
        if (!ddControl.silentError)
            program->errorMediumSize(item, message);

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
