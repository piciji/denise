
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
#include "../audio/manager.h"
#include "../thread/emuThread.h"
#include "../view/status.h"
#include "../emuconfig/layouts/system.h"
#include "../helper/fileHelper.h"
#include "../helper/miscHelper.h"

Autoloader* autoloader = nullptr;

auto Autoloader::init( std::vector<std::string> files, Mode mode, unsigned selection, std::string fileName ) -> void {
    ddControl.emulator = nullptr;
    ddControl.mediaGroups.clear();
    ddControl.errorLevel = 0;
    ddControl.mode = mode;
    ddControl.selection = selection;
    ddControl.fileName = fileName;
    ddControl.files.clear();
    ddControl.saveFile = nullptr;
    ddControl.overrideSpeeder = false;
    
    unsigned i = 0;
    for( auto& file : files ) {        
        ddControl.files.push_back( file );
        
        if (++i == 7)
            break;
    }        
}

auto Autoloader::setEmulator(Emulator::Interface* emulator) -> void {
    ddControl.emulator = emulator;
}

auto Autoloader::overrideSpeeder() -> void {
    ddControl.overrideSpeeder = true;
}

auto Autoloader::postProcessing() -> void {

    if (ddControl.errorLevel >= 1)
        filePool->unloadOrphaned();
    
    auto emuView = EmuConfigView::TabWindow::getView( ddControl.emulator );
    
    if (ddControl.saveFile) {

        FileHelper::updateSaveIdentFromSav( ddControl.emulator, ddControl.saveFile );

        filePool->assign("savestate", nullptr);
        
        States::getInstance( ddControl.emulator )->load( ddControl.saveFile->getFile() );
        
        return;
    }
    
    if (ddControl.mediaGroups.size() == 0) {
        if (ddControl.errorLevel == 2)
            program->exit(1);
        
        return;
    }
        
    std::sort(ddControl.mediaGroups.begin(), ddControl.mediaGroups.end(), [ ](const Emulator::Interface::MediaGroup* lhs, const Emulator::Interface::MediaGroup* rhs) {
        auto getPrio = [](const Emulator::Interface::MediaGroup* group) -> int {
            if (group->isProgram()) return 0;
            if (group->isExpansion()) return 1;
            if (group->isDisk()) return 2;
            if (group->isTape()) return 3;
            if (group->isHardDisk()) return 4;
            return 5;
        };

        return getPrio(lhs) < getPrio(rhs);
    });	

    auto mediaGroup = ddControl.mediaGroups[0];
    bool autoStart = true;
    bool trapped = false;
    bool forceStandardKernal = false;

    FileSetting* fSetting = nullptr;
    GUIKIT::Settings* settings = Program::getSettings( ddControl.emulator );
    
    if (ddControl.mode == Mode::DragnDrop) {
        autoStart = settings->get<bool>("autostart_dragndrop", dynamic_cast<LIBC64::Interface*>(ddControl.emulator));
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
    } else if (ddControl.mode == Mode::Open || ddControl.mode == Mode::OpenWithSlot)
        autoStart = false;
    else if (ddControl.mode == Mode::AutoStart || ddControl.mode == Mode::AutoStartWithSlot) {
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

    if (!dynamic_cast<LIBC64::Interface*>(ddControl.emulator) ) {
        trapped = false;
        ddControl.overrideSpeeder = false;
    }

    if (trapped) // traps require standard kernals
        forceStandardKernal = true;
    else if (mediaGroup->isTape()) {
        forceStandardKernal = settings->get<bool>("autostart_tape_standard_kernal", false);
    }

    if (!autoStart) {

        if (mediaGroup->isDrive()) {
            if (slotMode() || (ddControl.mode == Mode::Open))
                activateDrive( ddControl.emulator, mediaGroup, countImagesFor(mediaGroup) + (slotMode() ? ddControl.selection : 0), activeEmulator == ddControl.emulator );

            if (view) {
                if (activeEmulator && (ddControl.mode == Mode::OpenWithSlot || ddControl.mode == Mode::DragnDrop) ) {

                    if (shouldCaptureMouse(ddControl.emulator, settings))
                        view->prepareCursorHide(200, true);
                    else {
                        view->setForeground();
                        view->setFocused(100 );
                    }
                } else {
                    view->setForeground();
                    view->setFocused(100);
                }
            }

            autoloader->setOnlyForFirstDrive(ddControl.emulator, &mediaGroup->media[0]);
            settings->set<int>("swap_pos", -1, false);
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
				activateDrive( ddControl.emulator, &_mediaGroup, count + (slotMode() ? ddControl.selection : 0) );

			} else if (_mediaGroup.isExpansion()) {
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

                if (emuView && emuView->systemLayout)
                    emuView->systemLayout->setExpansion( useExpansion );
			}
		}

        videoDriver->hideSplashScreen();

        if (!useExpansion)
            MiscHelper::removeExpansion(ddControl.emulator, true);

        if ( dynamic_cast<LIBC64::Interface*>(ddControl.emulator) || (ddControl.emulator != activeEmulator)
            || (activeEmulator->getModelValue( LIBAMI::Interface::ModelIdSystem ) > 0 ) )
            program->power( ddControl.emulator, emuView != nullptr );
        else
            program->reset(ddControl.emulator); // because of A1000 WOM

        if (statusHandler && useExpansion && activeEmulator->isExpansionUnsupported())
            statusHandler->setMessage(trans->getA("unsupported cartridge"), true);

        bool trapsWithSpeeder = trapped && mediaGroup->isDisk() && settings->get<bool>("autostart_speeder_traps", false);

        useExpansion = ddControl.emulator->getExpansion();
        if (trapped && (useExpansion && !useExpansion->isEmpty() && !useExpansion->isFastloader() && !useExpansion->isRS232() && !useExpansion->isTurboCart()))
            trapped = false;
        else if (forceStandardKernal || ddControl.overrideSpeeder) {
            // temporary disable any speeders
            auto fManager = FirmwareManager::getInstance( ddControl.emulator );
            if (fManager->getStoreLevelInUse() > 0)
                fManager->insertDefault( trapsWithSpeeder );
        }

        uint8_t options = (uint8_t)trapped;
        if (ddControl.overrideSpeeder) options |= 0x80;
        else if (trapped && !mediaGroup->selected) {
            if (!trapsWithSpeeder)  options |= 0x80;
            else                    options |= 2;
        }

        if (ddControl.mode == Mode::AutoStartWithSlot) {
            auto sel = ddControl.selection;
            if (sel >= mediaGroup->media.size())
                sel = 0;

            ddControl.emulator->selectListing(&mediaGroup->media[sel], 0, "", options);
            fSetting = FileSetting::getInstance( ddControl.emulator, _underscore(mediaGroup->media[sel].name) );
            set(ddControl.emulator, &mediaGroup->media[0], trapped);
        } else if (mediaGroup->selected) {
            ddControl.emulator->selectListing(mediaGroup->selected, ddControl.selection, ddControl.fileName, trapped);
            fSetting = FileSetting::getInstance( ddControl.emulator, _underscore(mediaGroup->selected->name) );
            set(ddControl.emulator, mediaGroup->selected, trapped);
        } else {
            ddControl.emulator->selectListing(&mediaGroup->media[0], ddControl.selection, ddControl.fileName, options);
            fSetting = FileSetting::getInstance( ddControl.emulator, _underscore(mediaGroup->media[0].name) );
            set(ddControl.emulator, &mediaGroup->media[0], trapped, ddControl.selection);
        }

        if (mediaGroup->isDrive()) {
            settings->set<int>("swap_pos", -1, false);
        }

        if (audioManager)
            audioManager->drive.reset(mediaGroup, true);
        
        if (fSetting)
            FileHelper::updateSaveIdent(ddControl.emulator, fSetting);
        
		if (view) {
			if (mediaGroup->isTape())
				view->updateTapeIcons(Emulator::Interface::TapeMode::Play);

            view->setForeground();

		    bool acqMouse = false;
            if (ddControl.mode == Mode::AutoStartWithSlot || ddControl.mode == Mode::DragnDrop) {
                if (shouldCaptureMouse(ddControl.emulator, settings))
                    acqMouse = true;
		    }
		    if (acqMouse)
		        view->prepareCursorHide(100, true);
		    else
		        view->setFocused(100 );

			if (!cmd->debug && (mediaGroup->isTape() || mediaGroup->isDisk()) ) {
                program->initAutoWarp(mediaGroup);
			}
		}
    }
}

auto Autoloader::shouldCaptureMouse(Emulator::Interface* emulator, GUIKIT::Settings* settings) -> bool {
    return dynamic_cast<LIBAMI::Interface*>(emulator)
        && view->canGrabInputFocusAfterDnDFromOtherApp()
        && program->isAnalogDeviceConnected()
        && settings->get<bool>("dragndrop_capture_mouse",false);
}

auto Autoloader::loadFiles() -> void {

    if (ddControl.files.size() == 0)
        return postProcessing();    
    
    auto filePath = ddControl.files[0];
    
    GUIKIT::Vector::eraseVectorPos( ddControl.files, 0 );
    
    GUIKIT::File* file = filePool->get(filePath);

    if (!file)
        return loadFiles();

    if (!file->exists()) {
        if (ddControl.errorLevel == 0)
            FileHelper::errorFileSize(MAX_MEDIUM_SIZE, file->getPath(), view->message);

        return loadFiles();
    }

    auto& items = file->scanArchive();

	if (archiveViewer) {
        filePool->assign("autoloader", file);
        archiveViewer->onCallback = [this](GUIKIT::File* file, GUIKIT::File::Item* item) {
            bool locked = emuThread->lock(true);
			this->loadFile( file, item );
            filePool->assign("autoloader", nullptr);
            if (locked)
                emuThread->unlock();
		};

        archiveViewer->allowNativeArchive(dynamic_cast<LIBAMI::Interface*>(activeEmulator) ? activeEmulator->getDiskMediaGroup() : nullptr);
		archiveViewer->setView( file, items );
	} else 
		loadFile( file, &items[0] );
}

auto Autoloader::needSlotsForDragnDrop(std::vector<std::string> files) -> unsigned {
    Emulator::Interface::MediaGroup* prefered = nullptr;

    for (auto& path : files) {
        GUIKIT::File* file = filePool->get(path);
        if (!file || !file->exists() || !file->isSizeValid(MAX_MEDIUM_SIZE))
            continue;

        auto& items = file->scanArchive();
        if (!items.size())
            continue;
        GUIKIT::File::Item* item = &items[0];
        if (!item || (item->info.size == 0))
            continue;

        std::string fileSuffix = GUIKIT::String::getExtension(item->info.name, "exe");
        GUIKIT::String::toLowerCase(fileSuffix);

        for (auto emulator : emulators) {
            for (auto& mediaGroup : emulator->mediaGroups) {
                for (auto& mediaSuffix : mediaGroup.suffix) {
                    if (mediaSuffix == fileSuffix) {
                        if (mediaGroup.isExpansion()) {
                            auto analyzedExpansion = emulator->analyzeExpansion(file->archiveData(item->id), item->info.size, fileSuffix);
                            if (analyzedExpansion != mediaGroup.expansion)
                                continue;
                        }

                        if (mediaGroup.isDisk())
                            prefered = &mediaGroup;
                        else if (mediaGroup.isTape()) {
                            if (prefered != emulator->getDiskMediaGroup())
                                prefered = &mediaGroup;
                        } else { // PRG or Expansion
                            if (!prefered)
                                prefered = &mediaGroup;
                        }
                    }
                }
            }
            if (prefered) { // no emu core mixing allowed
                int count = 0;

                if (prefered->isDrive()) {
                    count = emulator->getModelValue( emulator->getModelIdOfEnabledDrives(prefered) );
                    return count ? count : 1;
                }

                for(auto& media : prefered->media) {
                    if (!media.parent)
                        count++;
                }
                return count;
            }
        }
    }
    return 1;
}

auto Autoloader::loadFile( GUIKIT::File* file, GUIKIT::File::Item* item ) -> void {
    std::string fileSuffix;

	if (!item || (item->info.size == 0))
		goto errorOpen;

    if (!cmd->noGui) {
        if ( checkForSavestate( file, item ) )
            return loadFiles();
    }

    fileSuffix = GUIKIT::String::getExtension(item->info.name, "exe");
	GUIKIT::String::toLowerCase(fileSuffix);

	for (auto emulator : emulators) {

		if (ddControl.emulator && ddControl.emulator != emulator)
			continue;

		auto emuView = EmuConfigView::TabWindow::getView(emulator);

		for (auto& mediaGroup : emulator->mediaGroups) {

			for (auto& mediaSuffix : mediaGroup.suffix) {

				if (mediaSuffix == fileSuffix) {

					if (mediaGroup.isExpansion()) {
						auto analyzedExpansion = emulator->analyzeExpansion(file->archiveData(item->id), item->info.size, fileSuffix);

						if (analyzedExpansion != mediaGroup.expansion)
							continue;
					}

					unsigned alreadyInUse = countImagesFor(&mediaGroup);
                    unsigned _pos = alreadyInUse;
                    Emulator::Interface::Media* media = mediaGroup.selected;

                    if (slotMode()) {
                        _pos += ddControl.selection;
                        media = nullptr;
                    }

					if ((media && _pos) || (_pos >= mediaGroup.media.size()))
						return loadFiles();

					if (!media)
						media = &mediaGroup.media[ _pos ];

                    if (media->parent)
                        return loadFiles();

                    ddControl.emulator = emulator;

					if (emuView && emuView->mediaLayout)
                        emuView->mediaLayout->insertImage(media, file, item, alreadyInUse ? 2 : 0);
					else
                        fileloader->insertImage(emulator, media, file, item, alreadyInUse ? 2 : 0);

                    auto settings = Program::getSettings(emulator);
                    auto folderPath = GUIKIT::File::buildRelativePath(file->getPath());
                    settings->set<std::string>(_underscoreEx(mediaGroup.name) + "_folder_auto", folderPath);

                    ddControl.mediaGroups.push_back(&mediaGroup);

					return loadFiles();
				}
			}
		}
	}

errorOpen:
	if (ddControl.errorLevel == 0)
		FileHelper::errorOpen(file, item, view->message);

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

auto Autoloader::activateDrive( Emulator::Interface* emulator, Emulator::Interface::MediaGroup* mediaGroup, unsigned requestedCount, bool updateStatus ) -> void {
	
    if (requestedCount > mediaGroup->media.size())
        requestedCount = mediaGroup->media.size();

    auto modelId = emulator->getModelIdOfEnabledDrives( mediaGroup );

    unsigned counter = emulator->getModelValue( modelId );

	if (counter >= requestedCount)
		return;

    auto settings = Program::getSettings( emulator );

    auto model = emulator->getModel( modelId );

    if (model)
        settings->set<unsigned>(_underscore(model->name), requestedCount);

    emulator->setModelValue( modelId, requestedCount );

    auto emuView = EmuConfigView::TabWindow::getView( emulator );

    if (emuView) {
        if(emuView->systemLayout) emuView->systemLayout->driveModelLayout.updateWidget( modelId );
        if(emuView->mediaLayout) emuView->mediaLayout->updateVisibility( mediaGroup, requestedCount );
    }

    bool halfTrackMode = dynamic_cast<LIBC64::Interface*>(emulator);
    if (updateStatus && statusHandler) {
        for (auto& media: mediaGroup->media) {
            if ((media.id >= counter) && (media.id < requestedCount)) {
                statusHandler->updateDeviceState(&media, false, halfTrackMode ? 0x8000 : 0, 0, true);
            }
        }
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
            FileHelper::errorOpen(file, nullptr, view->message);
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

auto Autoloader::setOnlyForFirstDrive(Emulator::Interface* emulator, Emulator::Interface::Media* media) -> void {
    if(!media->group->isDrive() || (&media->group->media[0] != media))
        return;

    for(auto& use : used) {
        if (use.emulator == emulator) {
            use.media = media;
            return;
        }
    }
    used.push_back({emulator, media, false, 0});
}

auto Autoloader::set(Emulator::Interface* emulator, Emulator::Interface::Media* media, bool trapped, unsigned selection) -> void {
    for(auto& use : used) {
        if (use.emulator == emulator) {
            use.media = media;
            use.trapped = trapped;
            use.selection = selection;
            return;
        }
    }
    used.push_back({emulator, media, trapped, selection});
}

auto Autoloader::get(Emulator::Interface* emulator, bool& trapped, unsigned& selection) -> Emulator::Interface::Media* {
    Emulator::Interface::Media* useMedia = nullptr;

    for(auto& use : used) {
        if (use.emulator == emulator) {
            trapped = use.trapped;
            selection = use.selection;
            useMedia = use.media;
            break;
        }
    }
    if (useMedia) {
        auto fSetting = FileSetting::getInstance( emulator, _underscore(useMedia->name ) );
        if (fSetting->path.empty())
            useMedia = nullptr;
    }

    if (!useMedia) {
        for(auto& group : emulator->mediaGroups) {
            for(auto& _media : group.media) {
                auto fSetting = FileSetting::getInstance(emulator, _underscore(_media.name) );
                if (!fSetting->path.empty()) {
                    useMedia = &_media;
                    selection = 0;
                    GUIKIT::Settings* settings = Program::getSettings( emulator );
                    if (useMedia->group->isDisk())
                        trapped = settings->get<bool>("use_disk_traps", false);
                    else if (useMedia->group->isTape())
                        trapped = settings->get<bool>("use_tape_traps", false);
                    else
                        trapped = false;

                    set(emulator, useMedia, trapped, selection);
                    break;
                }
            }
            if (useMedia)
                break;
        }
    }

    return useMedia;
}

auto Autoloader::getLatestDrive(Emulator::Interface* emulator) -> Emulator::Interface::Media* {
    for(auto& use : used)
        if (use.emulator == emulator) {
            return use.media->group->isDrive() ? use.media : emulator->getDisk(0);
    }
    return emulator->getDisk(0);
}
