
#include "manager.h"
#include "../tools/DiskFinder.h"
#include "../view/status.h"
#include "../audio/manager.h"
#include "../thread/emuThread.h"

std::vector<InputMapping*> InputManager::hotkeyTriggers;

auto InputManager::setHotkeys() -> void {
    hotkeys.push_back( {Hotkey::Id::Pause, "Pause"} );
    hotkeys.push_back( {Hotkey::Id::Fullscreen, "Fullscreen"} );
    hotkeys.push_back( {Hotkey::Id::ToggleFastForward, "Toggle_fastforward"} );
    hotkeys.push_back( {Hotkey::Id::ToggleFastForwardAggressive, "Toggle_fastforward_aggressive"} );
    hotkeys.push_back( {Hotkey::Id::CaptureMouse, "Capture_mouse"} );        
    
    hotkeys.push_back( {Hotkey::Id::ToggleMenu, "Toggle_menu"} );
    hotkeys.push_back( {Hotkey::Id::ToggleStatus, "Toggle_status"} );	
	
    hotkeys.push_back( {Hotkey::Id::RunAheadUp, "runahead up"} );	
    hotkeys.push_back( {Hotkey::Id::RunAheadDown, "runahead down"} );	
    hotkeys.push_back( {Hotkey::Id::RunAheadToggleMode, "runahead toggle mode"} );	
    
    hotkeys.push_back( {Hotkey::Id::ToggleRenderer, "Toggle renderer"} );	
    hotkeys.push_back( {Hotkey::Id::AudioRecord, "audio record"} );

    hotkeys.push_back( {Hotkey::Id::Freeze, "freeze button"} );

    hotkeys.push_back( {Hotkey::Id::SyncStatus, "Sync status"} );

    hotkeys.push_back( {Hotkey::Id::FloppyAccess, "select_disk_drive"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwap0, "Disk_swapper_call0"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwap1, "Disk_swapper_call1"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwap2, "Disk_swapper_call2"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwap3, "Disk_swapper_call3"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwap4, "Disk_swapper_call4"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwap5, "Disk_swapper_call5"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwap6, "Disk_swapper_call6"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwap7, "Disk_swapper_call7"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwap8, "Disk_swapper_call8"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwap9, "Disk_swapper_call9"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwap10, "Disk_swapper_call10"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwap11, "Disk_swapper_call11"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwap12, "Disk_swapper_call12"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwap13, "Disk_swapper_call13"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwap14, "Disk_swapper_call14"} );
}

auto InputManager::setCustomHotkeys() -> void {
	
	customHotkeys.push_back( {Hotkey::Id::Loadstate, "Loadstate", true} );
	customHotkeys.push_back( {Hotkey::Id::Savestate, "Savestate", true} );
	customHotkeys.push_back( {Hotkey::Id::IncSlot, "Incslot", true} );
    customHotkeys.push_back( {Hotkey::Id::DecSlot, "Decslot", true} );
	customHotkeys.push_back( {Hotkey::Id::SwapInputDevices, "swap_ports", true} );
	customHotkeys.push_back( {Hotkey::Id::Power, "Hard Reset", true} );
	customHotkeys.push_back( {Hotkey::Id::SoftReset, "Soft Reset", true} );
    customHotkeys.push_back( {Hotkey::Id::AnyLoad, "load software", true} );
    customHotkeys.push_back( {Hotkey::Id::ToggleBorder, "toggle border", true} );

	if (dynamic_cast<LIBC64::Interface*>(emulator) ) {
		customHotkeys.push_back( {Hotkey::Id::ToggleSidFilter, "sid_filter_toggle", false} );
		customHotkeys.push_back( {Hotkey::Id::SwapSid, "Swap_sid", false} );
		customHotkeys.push_back( {Hotkey::Id::DigiBoost, "Digi_boost", false} );	
		customHotkeys.push_back( {Hotkey::Id::AdjustBiasUp, "adjust_bias_up", false} );	
		customHotkeys.push_back( {Hotkey::Id::AdjustBiasDown, "adjust_bias_down", false} );	
		
		customHotkeys.push_back( {Hotkey::Id::PlayTape, "tape_play_key", false} );
		customHotkeys.push_back( {Hotkey::Id::StopTape, "tape_stop_key", false} );
		customHotkeys.push_back( {Hotkey::Id::RecordTape, "tape_record_key", false} );
		customHotkeys.push_back( {Hotkey::Id::ForwardTape, "tape_forward_key", false} );
		customHotkeys.push_back( {Hotkey::Id::RewindTape, "tape_rewind_key", false} );
		customHotkeys.push_back( {Hotkey::Id::ResetTapeCounter, "tape_counter_reset_key", false} );
        customHotkeys.push_back( {Hotkey::Id::EF3Menu, "ef3 menu button", false} );
	}	
    
	customHotkeys.push_back( {Hotkey::Id::Software, "Software", true} );	
    customHotkeys.push_back( {Hotkey::Id::System, "System", true} );
	customHotkeys.push_back( {Hotkey::Id::Control, "Control", true} );
	customHotkeys.push_back( {Hotkey::Id::Configurations, "Configurations", true} );
	customHotkeys.push_back( {Hotkey::Id::Presentation, "Presentation", true} );
	customHotkeys.push_back( {Hotkey::Id::Palette, "Palette", true} );
    customHotkeys.push_back( {Hotkey::Id::Firmware, "Firmware", true} );
	customHotkeys.push_back( {Hotkey::Id::Border, "Border", true} );
    customHotkeys.push_back( {Hotkey::Id::DiskSwapper, "Disk_swapper", true} );
}

auto InputManager::fireHotkey(Emulator::Interface* emulator, Hotkey::Id id) -> void {
    
    typedef LIBC64::Interface C64Interface;
    typedef LIBAMI::Interface AmigaInterface;
    
    auto settings = program->getSettings( activeEmulator );
    
    switch ( id ) {
        case Hotkey::Id::AudioRecord: {
            if (!activeEmulator)
                break;

            auto emuView = EmuConfigView::TabWindow::getView(activeEmulator);
            if (emuView && emuView->audioLayout) {
                emuView->audioLayout->toggleRecord();
            } else {
                emuThread->lock();
                if (audioManager->record.run()) {
                    audioManager->record.finish();
                } else {
                    std::string errorText;
                    if (!audioManager->record.start(activeEmulator, errorText)) {
                        statusHandler->setMessage(errorText, 3, true);
                    }
                }
            }

        } break;
        case Hotkey::Id::RunAheadDown:
        case Hotkey::Id::RunAheadUp: {
            if (!activeEmulator)
                break;   
            
            unsigned pos = settings->get<unsigned>( "runahead", 0, {0u, 10u});
            bool down = id == Hotkey::Id::RunAheadDown;
            
            if ( down && (pos == 0) )
                break;
            else if ( !down && (pos == 10) )
                break;

            pos += down ? -1 : 1;
            settings->set<unsigned>( "runahead", pos);
            emuThread->lock();
            audioManager->drive.reset();
            activeEmulator->runAhead( pos );
            emuThread->unlock();
            auto emuView = EmuConfigView::TabWindow::getView(activeEmulator);
            if (emuView && emuView->miscLayout)
                emuView->miscLayout->setRunAhead( pos );

            statusHandler->setMessage( trans->get( "runahead input latency", {{"%count%", std::to_string(pos) }} ) );  

        } break;
            
        case Hotkey::Id::ToggleRenderer: {
            if (!activeEmulator)
                break;

            auto emuView = EmuConfigView::TabWindow::getView(activeEmulator);
            if (emuView && emuView->systemLayout)
                emuView->systemLayout->performanceModelLayout.toggleCheckbox( activeEmulator->getModelIdOfCycleRenderer() );
            else {
                auto model = activeEmulator->getModel( activeEmulator->getModelIdOfCycleRenderer() );
                if (model) {
                    emuThread->lock();
                    bool val = activeEmulator->getModelValue( model->id );
                    settings->set<bool>( _underscore(model->name), !val );
                    activeEmulator->setModelValue( model->id, !val );
                    program->fastForward(false);
                    program->power(activeEmulator);
                }
            }
        } break;
        
        case Hotkey::Id::RunAheadToggleMode: {
            if (!activeEmulator)
                break;
            
            bool state = settings->get<bool>( "runahead_performance", false);
            state ^= 1;
            settings->set<bool>( "runahead_performance", state);
            emuThread->lock();
            activeEmulator->runAheadPerformance( state );
            emuThread->unlock();

            auto emuView = EmuConfigView::TabWindow::getView(activeEmulator);
            if (emuView && emuView->miscLayout)
                emuView->miscLayout->setRunAheadPerformance( state );
            
            statusHandler->setMessage( trans->get( !state ? "runahead accuracy mode" : "runahead performance mode" ) );  
        } break;
        
		case Hotkey::Id::SwapInputDevices: {
            emuThread->lock();
			auto connector1 = emulator->getConnector( 0 );
            auto connectedDevice1 = emulator->getConnectedDevice( connector1 );
            
            auto connector2 = emulator->getConnector( 1 );
            auto connectedDevice2 = emulator->getConnectedDevice( connector2 );
            
            emulator->connect( connector1, connectedDevice2 );
            emulator->connect( connector2, connectedDevice1 );

            settings->set<unsigned>( _underscore(connector1->name), connectedDevice2->id);
            settings->set<unsigned>( _underscore(connector2->name), connectedDevice1->id);

            view->checkInputDevice( emulator, connector1, connectedDevice2 );
			view->checkInputDevice( emulator, connector2, connectedDevice1 );

            auto emuView = EmuConfigView::TabWindow::getView(activeEmulator);
            if (emuView && emuView->inputLayout)
                emuView->inputLayout->updateConnectorButtons();
		} break;
        case Hotkey::Id::ToggleFastForward:
        case Hotkey::Id::ToggleFastForwardAggressive:
            emuThread->lock();
            program->toggleFastForward( id == Hotkey::Id::ToggleFastForwardAggressive );
            break;
        
        case Hotkey::Id::Fullscreen:
            view->setFullScreen( !view->fullScreen() );
            break;
			
		case Hotkey::Power:
            emuThread->lock();
			program->power(emulator);
			break;
			
		case Hotkey::SoftReset:
            emuThread->lock();
			program->reset(emulator);
			break;
			
        case Hotkey::AnyLoad:           
            view->setAnyload( emulator );
            break;
            
        case Hotkey::Id::CaptureMouse:
            if (inputDriver->mIsAcquired()) {
                inputDriver->mUnacquire();					
            } else if (view->fullScreen()) {
                inputDriver->mAcquire();
            } else if (program->isRunning && program->isAnalogDeviceConnected()) {
                inputDriver->mAcquire();
            }
            break;
        case Hotkey::Id::DiskSwapper:
        case Hotkey::Id::Software:
        case Hotkey::Id::Presentation:
        case Hotkey::Id::Palette:
        case Hotkey::Id::Border:
        case Hotkey::Id::Firmware:
        case Hotkey::Id::System:
        case Hotkey::Id::Control:
		case Hotkey::Id::Configurations:	
            openMenu( emulator, id );
            break;
        case Hotkey::Id::SyncStatus: {
            std::string _str = videoDriver->hasThreaded() ? "Threaded  " : "";

            if (videoDriver->hasSynchronized()) {
                _str += "Vsync";
                _str += VideoManager::frameRenderTrigger > 1 ? "(" + std::to_string(VideoManager::frameRenderTrigger) + "x)  " : "  ";
            }

            _str += audioDriver->hasSynchronized() ? "Audiosync  " : "";
            _str += audioManager->dynamicRateControl ? "DRC  " : "";
            float monitorFrequency = GUIKIT::Monitor::getCurrentRefreshRate();
            _str += GUIKIT::String::convertDoubleToString(monitorFrequency, 3 );
            statusHandler->setMessage(_str, 6);
        } break;
        case Hotkey::Id::Pause:
            view->togglePause();
            view->updatePauseCheck();
            break;
        case Hotkey::IncSlot:
        case Hotkey::DecSlot: 
            States::getInstance( emulator )->changeSlot( id == Hotkey::DecSlot );
            break;
        case Hotkey::Loadstate:
            emuThread->lock();
            States::getInstance( emulator )->load();
            break;
        case Hotkey::Savestate:
            emuThread->lock();
            States::getInstance( emulator )->save();
            break;
        case Hotkey::ToggleMenu:
            if(!view->fullScreen()) view->updateMenuBar( true );
            break;
        case Hotkey::ToggleStatus:
            if(!view->exclusiveFullscreen()) view->updateStatusBar( true );
            break;

        case Hotkey::Id::ToggleBorder: {
            if (!activeEmulator)
                break;

            typedef Emulator::Interface::CropType CropType;
            auto cropType = settings->get<unsigned>("crop_type", (unsigned)CropType::Off);
            if (++cropType > 4) {
                cropType = 0;
            }
            if (cropType == 3)
                cropType = 4;

            auto emuView = EmuConfigView::TabWindow::getView( activeEmulator );

            if (emuView && emuView->borderLayout) {
                if ((CropType)cropType == CropType::Off) emuView->borderLayout->cropOff.activate();
                else if ((CropType)cropType == CropType::Monitor) emuView->borderLayout->cropMonitor.activate();
                else if ((CropType)cropType == CropType::Auto) emuView->borderLayout->cropAuto.activate();
                else if ((CropType)cropType == CropType::Free) emuView->borderLayout->cropFree.activate();
            } else {
                settings->set<unsigned>("crop_type", cropType);
                emuThread->lock();
                program->updateCrop(activeEmulator);
            }

        } break;

        case Hotkey::ResetTapeCounter:
        case Hotkey::PlayTape:
        case Hotkey::RecordTape:
        case Hotkey::StopTape:
        case Hotkey::ForwardTape:
        case Hotkey::RewindTape: {
            if (!activeEmulator || !dynamic_cast<LIBC64::Interface*>(activeEmulator) )
                break;

            program->informDriveLoading(false);

            emuThread->lock();
            auto media = activeEmulator->getTape( 0 );
            if (!media)
                break;

            unsigned driveCount = activeEmulator->getModelValue( activeEmulator->getModelIdOfEnabledDrives( media->group ) );

            if (driveCount == 0) {
                statusHandler->setMessage( trans->get("tape_disconnect"), 3, true );
                break;
            }                        

            typedef Emulator::Interface::TapeMode TapeMode;

            if (id == Hotkey::PlayTape) {
                activeEmulator->controlTape( media, TapeMode::Play );
                statusHandler->setMessage( trans->get("tape_play_state") );
                view->updateTapeIcons( TapeMode::Play );
            } else if (id == Hotkey::StopTape) {
                activeEmulator->controlTape( media, TapeMode::Stop );
                statusHandler->setMessage( trans->get("tape_stop_state") );
                view->updateTapeIcons( TapeMode::Stop );
            } else if (id == Hotkey::RecordTape) {              
                activeEmulator->controlTape( media, TapeMode::Record );
                statusHandler->setMessage( trans->get("tape_record_state") );						
                view->updateTapeIcons( TapeMode::Record );
                if (activeEmulator->isWriteProtected( media ))
                    statusHandler->setMessage( trans->get("tape_record_wp_state"), 3, true );						

            } else if (id == Hotkey::ForwardTape) {
                activeEmulator->controlTape( media, TapeMode::Forward );
                //status->addMessage( trans->get("tape_forward_state") );                        
                view->updateTapeIcons( TapeMode::Forward );
            } else if (id == Hotkey::RewindTape) {
                activeEmulator->controlTape( media, TapeMode::Rewind );
                //status->addMessage( trans->get("tape_rewind_state") );
                view->updateTapeIcons( TapeMode::Rewind );
            } else if (id == Hotkey::ResetTapeCounter) {
                activeEmulator->controlTape( media, TapeMode::ResetCounter );
                statusHandler->setMessage( trans->get("tape_counter_reset") );
            } 															
            break;
        }
        case Hotkey::Id::DigiBoost: {
            if (!activeEmulator || !dynamic_cast<LIBC64::Interface*>(activeEmulator))
                break;

            auto emuView = EmuConfigView::TabWindow::getView( activeEmulator );
            bool state = false;

            if (emuView && emuView->audioLayout)
                state = emuView->audioLayout->settingsLayout.toggleCheckbox( C64Interface::ModelIdDigiboost );
            else {
                emuThread->lock();
                auto model = activeEmulator->getModel( C64Interface::ModelIdDigiboost );
                if (model) {
                    state = activeEmulator->getModelValue( model->id );
                    state ^= 1;
                    settings->set<bool>( _underscore(model->name), state );
                    activeEmulator->setModelValue( model->id, state );
                }
            }

            statusHandler->setMessage( trans->get( state ? "digiboost_on" : "digiboost_off" ) );
        } break;
        case Hotkey::Id::SwapSid: {
            if (!activeEmulator || !dynamic_cast<LIBC64::Interface*>(activeEmulator))
                break;
            auto emuView = EmuConfigView::TabWindow::getView( activeEmulator );
            unsigned val;

            if (emuView && emuView->systemLayout) {
                val = emuView->systemLayout->modelLayout.nextOption( C64Interface::ModelIdSid );
            } else {
                emuThread->lock();
                auto model = activeEmulator->getModel( C64Interface::ModelIdSid );
                if (model) {
                    val = activeEmulator->getModelValue( model->id );
                    val++;
                    if (val == model->options.size())
                        val = 0;
                    settings->set<int>( _underscore(model->name), val );
                    activeEmulator->setModelValue( model->id, val );
                }
            }

            if (emuView->audioLayout) {
                emuView->audioLayout->settingsLayout.updateWidget( C64Interface::ModelIdSid );
            }

            statusHandler->setMessage( trans->get( val == 1 ? "sid_6581_on" : "sid_8580_on" ) );
        } break;
        case Hotkey::Id::ToggleSidFilter: {
            if (!activeEmulator || !dynamic_cast<LIBC64::Interface*>(activeEmulator))
                break;
            auto emuView = EmuConfigView::TabWindow::getView( activeEmulator );
            bool state = false;

            if (emuView && emuView->audioLayout)
                state = emuView->audioLayout->settingsLayout.toggleCheckbox( C64Interface::ModelIdFilter );
            else {
                emuThread->lock();
                auto model = activeEmulator->getModel( C64Interface::ModelIdFilter );
                if (model) {
                    state = activeEmulator->getModelValue( model->id );
                    state ^= 1;
                    settings->set<bool>( _underscore(model->name), state );
                    activeEmulator->setModelValue( model->id, state );
                }
            }
            statusHandler->setMessage( trans->get( state ? "sid_filter_on" : "sid_filter_off" ) );
        } break;
        case Hotkey::AdjustBiasUp:
        case Hotkey::AdjustBiasDown: {
            if (!activeEmulator || !dynamic_cast<LIBC64::Interface*>(activeEmulator))
                break;
            
            int _sid = activeEmulator->getModelValue( C64Interface::ModelIdSid );
            auto emuView = EmuConfigView::TabWindow::getView( activeEmulator );
            int state;

            if (emuView && emuView->audioLayout)
                state = emuView->audioLayout->settingsLayout.stepRange( _sid == 0 ? C64Interface::ModelIdBias8580 : C64Interface::ModelIdBias6581,
                        id == Hotkey::AdjustBiasUp ? 100: -100 );
            else {
                emuThread->lock();
                auto model = activeEmulator->getModel( _sid == 0 ? C64Interface::ModelIdBias8580 : C64Interface::ModelIdBias6581);

                if (model) {
                    state = activeEmulator->getModelValue( model->id );
                    state += id == Hotkey::AdjustBiasUp ? 100: -100;
                    state = std::max( model->range[0], std::min( state, model->range[1] ) );
                    settings->set<int>( _underscore(model->name), state );
                    activeEmulator->setModelValue( model->id, state );
                }
            }
            statusHandler->setMessage( trans->get( "sid_bias_change", {{"%state%", std::to_string(state) }} ) );                    
        } break;
        
        case Hotkey::Id::FloppyAccess: {
            if (!activeEmulator)
                break;

            emuThread->lock();
            auto defaultMedia = activeEmulator->getDisk( 0 );

            if (!defaultMedia)
                break;

            auto mediaGroup = defaultMedia->group;

            auto mediaId = settings->get<unsigned>("access_floppy", 0u, {0u, (unsigned)mediaGroup->media.size() - 1u});
            unsigned enabledCount = activeEmulator->getModelValue( activeEmulator->getModelIdOfEnabledDrives(mediaGroup) );
            if (enabledCount > mediaGroup->media.size())
                enabledCount = mediaGroup->media.size();

            mediaId++; // switch to next

            auto media = defaultMedia;

            if ( ( mediaId < mediaGroup->media.size() ) && ( mediaId < enabledCount ) )
                media = activeEmulator->getDisk( mediaId );                    

            settings->set<unsigned>( "access_floppy", media->id, false);
            statusHandler->setMessage( trans->get("access_floppy", {{"%drive%", media->name}}) );								                    
            break;
        }

        case Hotkey::Freeze:
            emuThread->lock();
            if (activeEmulator)
                activeEmulator->freezeButton();
            break;

        case Hotkey::EF3Menu:
            emuThread->lock();
            if (activeEmulator)
                activeEmulator->customCartridgeButton();
            break;
        
        case Hotkey::DiskSwap0: case Hotkey::DiskSwap1: case Hotkey::DiskSwap2:
        case Hotkey::DiskSwap3: case Hotkey::DiskSwap4: case Hotkey::DiskSwap5:
        case Hotkey::DiskSwap6: case Hotkey::DiskSwap7: case Hotkey::DiskSwap8:
        case Hotkey::DiskSwap9: case Hotkey::DiskSwap10: case Hotkey::DiskSwap11:
        case Hotkey::DiskSwap12: case Hotkey::DiskSwap13: case Hotkey::DiskSwap14: {
            if (!activeEmulator)
                break;

            auto mediaId = settings->get<unsigned>("access_floppy", 0u, {0u, 3u});
            GUIKIT::File* file;

            emuThread->lock();
            auto media = activeEmulator->getDisk( mediaId );
            if (!media)
                break;                                        

            uint8_t* data;						

            auto swapPos = id - Hotkey::DiskSwap0;
            FileSetting* fSetting = FileSetting::getInstance( activeEmulator, "swapper_" + std::to_string(swapPos) );
            
            FileSetting fs;
            if (fSetting->path.empty()) {                
                fSetting = &fs;
                // auto create 
                auto srcSetting = FileSetting::getInstance(activeEmulator, _underscore(media->name) );
                
                if (srcSetting->path.empty())
                    break;
                
                DiskFinder diskFinder( srcSetting->path );
                
                auto result = diskFinder.findNext( swapPos );

                if (result != "") {
                    fSetting->file = result;
                    fSetting->path = diskFinder.filePath + result;
                    fSetting->id = 0;
                    fSetting->writeProtect = false;
                }                                               
            }
            
            file = filePool->get( fSetting->path );

            if (!file || !file->isSizeValid(MAX_MEDIUM_SIZE) ||                
                ((data = file->archiveData(fSetting->id)) == nullptr)
            ) {  
                statusHandler->setMessage(trans->get("file_open_error", {{ "%path%", fSetting->file }}), 2, true);
                break;
            }

            //activeEmulator->ejectDisk( media );
            activeEmulator->insertDisk(media, data, file->archiveDataSize(fSetting->id), true);
            activeEmulator->writeProtectDisk(media, (file->isArchived() || file->isReadOnly()) ? true : fSetting->writeProtect);
            media->guid = uintptr_t(file);
            auto emuView = EmuConfigView::TabWindow::getView( activeEmulator );
            if (emuView && emuView->mediaLayout)
                emuView->mediaLayout->updateWriteProtection( media, fSetting->writeProtect );

            filePool->assign( _ident(activeEmulator, media->name), file);
            filePool->assign( _ident(activeEmulator, "swapper_" + std::to_string(swapPos)), file);
            filePool->unloadOrphaned();
            program->updateSaveIdent( activeEmulator, fSetting->file );

            States::getInstance( activeEmulator )->updateImage( fSetting, media );
            statusHandler->setMessage( trans->get("insert_floppy", {{"%drive%", media->name},{"%file%", fSetting->file}}) );		
            break;	
        }
    }
    emuThread->unlock();
}

auto InputManager::pollHotkeys() -> void {

    emuThread->lockHotkeys();
    if (hotkeyTriggers.size() == 0) {
        emuThread->unlockHotkeys();
        return;
    }
    auto _hotkeyTriggers = hotkeyTriggers;
    hotkeyTriggers.clear();
    emuThread->unlockHotkeys();

    if (!Program::hasFocus()) {
        return;
    }

	std::vector<InputMapping*> useTrigger;
	InputMapping* viewOpen = nullptr;
	InputMapping* fastForward = nullptr;
	InputMapping* stateHandler = nullptr;
	InputMapping* deviceSwapper = nullptr;
	InputMapping* starter = nullptr;
    InputMapping* anyLoad = nullptr;
	
	auto useEmu = activeEmulator;
	
	for( auto trigger : _hotkeyTriggers ) {
		
		switch(trigger->hotkeyId) {
			case Hotkey::Id::SwapInputDevices:
				if (!useEmu) 
					useEmu = program->getLastUsedEmu();	
								
				if (!deviceSwapper)
					deviceSwapper = trigger;				
				else if (useEmu == trigger->inputManager->emulator)
					deviceSwapper = trigger;
				
				break;
			
			case Hotkey::Id::DiskSwapper:
			case Hotkey::Id::Software:
			case Hotkey::Id::Presentation:
			case Hotkey::Id::Palette:
			case Hotkey::Id::Border:
			case Hotkey::Id::Firmware:
			case Hotkey::Id::System:
			case Hotkey::Id::Control:
			case Hotkey::Id::Configurations:
				if (!useEmu) 
					useEmu = program->getLastUsedEmu();				
				
				if (!viewOpen)
					viewOpen = trigger;				
				
				else if (useEmu == trigger->inputManager->emulator)
					viewOpen = trigger;
				
				break;
				
			case Hotkey::Id::ToggleFastForward:
			case Hotkey::Id::ToggleFastForwardAggressive:
				if(!fastForward)
					fastForward = trigger;
				break;
				
			case Hotkey::IncSlot:
			case Hotkey::DecSlot: 
			case Hotkey::Loadstate:
			case Hotkey::Savestate:
				if (!useEmu) 
					useEmu = program->getLastUsedEmu();				

				if (!stateHandler)
					stateHandler = trigger;				
				
				else if (useEmu == trigger->inputManager->emulator)
					stateHandler = trigger;
				break;
				
			case Hotkey::Power:
			case Hotkey::SoftReset:
				if (!useEmu) 
					useEmu = program->getLastUsedEmu();				

				if (!starter)
					starter = trigger;				
				
				else if (useEmu == trigger->inputManager->emulator)
					starter = trigger;
				break;
				
            case Hotkey::AnyLoad:
				if (!useEmu) 
					useEmu = program->getLastUsedEmu();				

				if (!anyLoad)
					anyLoad = trigger;				
				
				else if (useEmu == trigger->inputManager->emulator)
					anyLoad = trigger;
				break;
                
			default:
				if (!GUIKIT::Vector::find( useTrigger, trigger ))
					useTrigger.push_back( trigger );
				break;			
		}		
	}
	
	if (viewOpen)
		useTrigger.push_back( viewOpen );

	if(fastForward)
		useTrigger.push_back( fastForward );
	
	if(stateHandler)
		useTrigger.push_back( stateHandler );
	
	if(deviceSwapper)
		useTrigger.push_back( deviceSwapper );
	
	if (starter)
		useTrigger.push_back( starter );
    
    if (anyLoad)
		useTrigger.push_back( anyLoad );
    
	for( auto trigger : useTrigger )			
		fireHotkey( trigger->inputManager ? trigger->inputManager->emulator : nullptr, (Hotkey::Id)trigger->hotkeyId );   		
}

auto InputManager::activateHotkey(Hotkey::Id id, Emulator::Interface* emulator) -> void {
	
	for (auto manager : inputManagers) {
		if (emulator && emulator != manager->emulator)
			continue;
		
		for( auto mapping : manager->mappings ) {
			
			if (mapping->emuDevice)
				continue;
			
			if (mapping->hotkeyId == id) {
                emuThread->lockHotkeys();
				hotkeyTriggers.push_back( mapping );
                emuThread->unlockHotkeys();
				return;
			}
		}
	}	
}

auto InputManager::unmapHotkeys() -> void {
    for(auto& hotkey : hotkeys) {
                
        auto mapping = (InputMapping*)hotkey.guid;
        
        mapping->init();

        if (mapping->alternate)
            mapping->alternate->init();
    }
	
	InputManager::updateAllMappingsInUse();
}

auto InputManager::unmapCustomHotkeys() -> void {
	
	for(auto& hotkey : customHotkeys) {
		
		auto mapping = (InputMapping*)hotkey.guid;
        
        mapping->init();

        if (mapping->alternate)
            mapping->alternate->init();
	}
	// there are emulator specific hotkeys which are active for another emulator cores, like 'load state'
	InputManager::updateAllMappingsInUse(true);
}

auto InputManager::openMenu( Emulator::Interface* emulator, Hotkey::Id id ) -> void {
    if (!emulator)
        return;
    
    auto emuView = EmuConfigView::TabWindow::getView( emulator, true );
    
    switch(id) {
        case Hotkey::Id::Presentation:
            emuView->showDelayed( EmuConfigView::TabWindow::Layout::Presentation ); break;
        case Hotkey::Id::Border:
            emuView->showDelayed( EmuConfigView::TabWindow::Layout::Border ); break;
        case Hotkey::Id::Palette:
            emuView->showDelayed( EmuConfigView::TabWindow::Layout::Palette ); break;
        case Hotkey::Id::DiskSwapper:
            if (!emuView->mediaLayout)
                emuView->prepareLayout(EmuConfigView::TabWindow::Layout::Media);

            emuView->mediaLayout->setDiskSwapperView();
            emuView->showDelayed( EmuConfigView::TabWindow::Layout::Media );
            break;
        case Hotkey::Id::Software:
            if (!emuView->mediaLayout)
                emuView->prepareLayout(EmuConfigView::TabWindow::Layout::Media);

            emuView->mediaLayout->setMediaView();
            emuView->showDelayed( EmuConfigView::TabWindow::Layout::Media );
            break;
        case Hotkey::Id::System:
            emuView->showDelayed(EmuConfigView::TabWindow::Layout::System); break;
        case Hotkey::Id::Firmware:
            emuView->showDelayed( EmuConfigView::TabWindow::Layout::Firmware ); break;
        case Hotkey::Id::Control:
            emuView->showDelayed( EmuConfigView::TabWindow::Layout::Control ); break;
		case Hotkey::Id::Configurations:
            emuView->showDelayed( EmuConfigView::TabWindow::Layout::Configurations ); break;
    }
}
