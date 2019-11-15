
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
    ddControl.driveGroups.clear();
    ddControl.silentError = silentError;
    ddControl.files.clear();
    
    unsigned i = 0;
    for( auto& file : files ) {        
        ddControl.files.push_back( file );
        
        if (++i == 4)
            break;
    }        
}

auto View::autoloadPostProcessing() -> void {

    if (ddControl.silentError)
        filePool->unloadOrphaned();
    
    if (ddControl.driveGroups.size() == 0) {
        if (ddControl.silentError)
            program->exit(1);
        
        return;
    }
        

    std::sort(ddControl.driveGroups.begin(), ddControl.driveGroups.end(), [ ](const Emulator::Interface::DriveGroup* lhs, const Emulator::Interface::DriveGroup* rhs) {

        if (lhs->isModuleSlot()) return true;
        if (rhs->isModuleSlot()) return false;
        if (lhs->isMemory()) return true;
        if (rhs->isMemory()) return false;
        if (lhs->isDiskDrive()) return true;
        return false;
    });

    auto autoStart = settings->get<bool>("autostart_dragndrop", false);

    auto emuConfigView = EmuConfigView::TabWindow::getView(ddControl.emulator);

    auto driveGroup = ddControl.driveGroups[0];

    if (!autoStart && (activeEmulator == ddControl.emulator)) {

        emuConfigView->show(EmuConfigView::TabWindow::Layout::Drives);

        emuConfigView->drivesLayout->showDriveGroupLayout(driveGroup);

    } else {

        if (!driveGroup->isModuleSlot()) {
            
            auto moduleDrive = ddControl.emulator->getModuleSlot(0);
            
            if (moduleDrive)
                 emuConfigView->drivesLayout->eject( moduleDrive->group );
        }                                               

        program->power(ddControl.emulator);

        ddControl.emulator->selectListing(driveGroup->type, 0, 0);

        if (driveGroup->isTapeDrive())
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

            for (auto& driveGroup : emulator->driveGroups) {

                if (driveGroup.isHardDrive())
                    continue;

                auto driveSuffixList = driveGroup.suffix;

                for (auto& driveSuffix : driveSuffixList) {

                    if (driveSuffix == fileSuffix) {

                        if (GUIKIT::Vector::find(ddControl.driveGroups, &driveGroup))
                            // only one file per group allowed
                            return autoloadFiles();

                        if (!driveGroup.isTapeDrive() && (item->info.size > MAX_MEDIUM_SIZE))
                            goto errorSize;

                        ddControl.emulator = emulator;
                        ddControl.driveGroups.push_back(&driveGroup);

                        auto autoStart = settings->get<bool>("autostart_dragndrop", false);

                        if (!autoStart && (activeEmulator == emulator));
                        else
                            emuConfigView->systemLayout->activateDrive(driveGroup);

                        emuConfigView->drivesLayout->insertImage({file, item, &driveGroup, nullptr});

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