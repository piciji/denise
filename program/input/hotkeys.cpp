
#include "manager.h"

std::vector<Hotkey::Id> InputManager::hotkeyTriggers;

auto InputManager::setHotkeys() -> void {
    hotkeys.push_back( {Hotkey::Id::Pause, "Pause"} );
    hotkeys.push_back( {Hotkey::Id::Fullscreen, "Fullscreen"} );
    hotkeys.push_back( {Hotkey::Id::ToggleFastForward, "Toggle_fastforward"} );
    hotkeys.push_back( {Hotkey::Id::ToggleFastForwardAggressive, "Toggle_fastforward_aggressive"} );
    hotkeys.push_back( {Hotkey::Id::CaptureMouse, "Capture_mouse"} );    
	hotkeys.push_back( {Hotkey::Id::Drives, "Drives"} );	
    hotkeys.push_back( {Hotkey::Id::System, "System"} );
    hotkeys.push_back( {Hotkey::Id::Firmware, "Firmware"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwapper, "Disk_swapper"} );
    hotkeys.push_back( {Hotkey::Id::States, "States"} );    
    hotkeys.push_back( {Hotkey::Id::Video, "Video"} );
    hotkeys.push_back( {Hotkey::Id::Palette, "Palette"} );
    hotkeys.push_back( {Hotkey::Id::Border, "Border"} );
    hotkeys.push_back( {Hotkey::Id::Input, "Input"} );
    hotkeys.push_back( {Hotkey::Id::Savestate, "Savestate"} );
    hotkeys.push_back( {Hotkey::Id::Loadstate, "Loadstate"} );
    hotkeys.push_back( {Hotkey::Id::IncSlot, "Incslot"} );
    hotkeys.push_back( {Hotkey::Id::DecSlot, "Decslot"} );
    hotkeys.push_back( {Hotkey::Id::ToggleMenu, "Toggle_menu"} );
    hotkeys.push_back( {Hotkey::Id::ToggleStatus, "Toggle_status"} );	
	hotkeys.push_back( {Hotkey::Id::ActivateFilter, "Activate_filter"} );
	hotkeys.push_back( {Hotkey::Id::SwapSid, "Swap_sid"} );
	hotkeys.push_back( {Hotkey::Id::DigiBoost, "Digi_boost"} );	
    hotkeys.push_back( {Hotkey::Id::AdjustBiasUp, "adjust_bias_up"} );	
    hotkeys.push_back( {Hotkey::Id::AdjustBiasDown, "adjust_bias_down"} );	
	hotkeys.push_back( {Hotkey::Id::PlayTape, "tape_play_key"} );
	hotkeys.push_back( {Hotkey::Id::StopTape, "tape_stop_key"} );
	hotkeys.push_back( {Hotkey::Id::RecordTape, "tape_record_key"} );
    hotkeys.push_back( {Hotkey::Id::ForwardTape, "tape_forward_key"} );
    hotkeys.push_back( {Hotkey::Id::RewindTape, "tape_rewind_key"} );
	hotkeys.push_back( {Hotkey::Id::ResetTapeCounter, "tape_counter_reset_key"} );
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

auto InputManager::fireHotkey(Hotkey::Id id) -> void {
    
    typedef LIBC64::Interface C64Interface;
    typedef LIBAMI::Interface AmigaInterface;
    
    switch ( id ) {
        case Hotkey::Id::ToggleFastForward:
        case Hotkey::Id::ToggleFastForwardAggressive: {
            if (!activeEmulator)
                break;                        
            
            bool ff = settings->get<bool>("fast_forward", false);
            bool ffa = settings->get<bool>("fast_forward_aggressive", false);   
            bool aggressive = id == Hotkey::Id::ToggleFastForwardAggressive;

            if ( (!aggressive && ffa) || (aggressive && ff) ) {
                // switch modes (already active)
                unsigned val = (unsigned)Emulator::Interface::FastForward::NoAudioOut | (unsigned)Emulator::Interface::FastForward::ReduceVideoOutput;
                if (id == Hotkey::Id::ToggleFastForwardAggressive)
                    val |= (unsigned)Emulator::Interface::FastForward::NoVideoSequencer;

                activeEmulator->fastForward( val );
                settings->set<bool>("fast_forward_aggressive", aggressive, false);
                settings->set<bool>("fast_forward", !aggressive, false);
                
            } else                
                program->fastForward( !ff && !ffa, id == Hotkey::Id::ToggleFastForwardAggressive);
                  
        } break;        
        
        case Hotkey::Id::Fullscreen:
            view->setFullScreen( !view->fullScreen() );
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
        case Hotkey::Id::FloppyAccess: {
            if (!activeEmulator)
                break;

            auto defaultMedia = activeEmulator->getDisk( 0 );

            // emulated system don't support floppy drives
            if (!defaultMedia)
                break;

            auto mediaGroup = defaultMedia->group;

            auto mediaId = settings->get<unsigned>(program->ident(activeEmulator, "access_floppy"), 0u, {0u, (unsigned)mediaGroup->media.size() - 1u});
            unsigned enabledCount = settings->get<unsigned>( program->ident(activeEmulator, mediaGroup->name + "_count"), mediaGroup->defaultUsage());
            if (enabledCount > mediaGroup->media.size())
                enabledCount = mediaGroup->defaultUsage();

            mediaId++; // switch to next

            auto media = defaultMedia;

            if ( ( mediaId < mediaGroup->media.size() ) && ( mediaId < enabledCount ) )
                media = activeEmulator->getDisk( mediaId );                    

            settings->set<unsigned>(program->ident(activeEmulator, "access_floppy"), media->id, false);
            status->addMessage( trans->get("access_floppy", {{"%drive%", media->name}}) );								                    
            break;
        }
        case Hotkey::Id::DiskSwapper:
        case Hotkey::Id::Drives:
        case Hotkey::Id::Video:
        case Hotkey::Id::Palette:
        case Hotkey::Id::Border:
        case Hotkey::Id::Firmware:
        case Hotkey::Id::System:
        case Hotkey::Id::Input:
            openMenu( id );
            break;        
        case Hotkey::Id::States:		            
            EmuConfigView::TabWindow::getView( States::getInstanceAuto()->emulator )
                ->showDelayed( EmuConfigView::TabWindow::Layout::States );
            break;
        case Hotkey::Id::Pause:
            program->isPause ^= 1;
            audioDriver->clear();
            break;
        case Hotkey::IncSlot:
        case Hotkey::DecSlot: 
            States::getInstanceAuto( )->changeSlot( id == Hotkey::DecSlot );
            break;
        case Hotkey::Loadstate:
            States::getInstanceAuto( )->load();
            break;
        case Hotkey::Savestate:
            States::getInstanceAuto( )->save();
            break;
        case Hotkey::ToggleMenu:
            if(!view->fullScreen()) view->updateMenuBar( true );
            break;
        case Hotkey::ToggleStatus:
            if(!view->fullScreen()) view->updateStatusBar( true );
            break;

        case Hotkey::ResetTapeCounter:
        case Hotkey::PlayTape:
        case Hotkey::RecordTape:
        case Hotkey::StopTape:
        case Hotkey::ForwardTape:
        case Hotkey::RewindTape: {
            if (!activeEmulator || !dynamic_cast<LIBC64::Interface*>(activeEmulator) )
                break;

            auto media = activeEmulator->getTape( 0 );
            if (!media)
                break;

            unsigned driveCount = activeEmulator->getDrivesConnected( media->group );            

            if (driveCount == 0) {
                status->addMessage( trans->get("tape_disconnect"), 3, true );
                return;
            }                        

            typedef Emulator::Interface::TapeMode TapeMode;
            
            auto setting = FileSetting::getInstance( program->ident(activeEmulator, media->name ) );

            if (id == Hotkey::PlayTape) {
                activeEmulator->controlTape( media, TapeMode::Play );
                status->addMessage( trans->get("tape_play_state") );
                view->updateTapeIcons( TapeMode::Play );
            } else if (id == Hotkey::StopTape) {
                activeEmulator->controlTape( media, TapeMode::Stop );
                status->addMessage( trans->get("tape_stop_state") );
                view->updateTapeIcons( TapeMode::Stop );
            } else if (id == Hotkey::RecordTape) {
                if (!setting->writeProtect) {
                    activeEmulator->controlTape( media, TapeMode::Record );
                    status->addMessage( trans->get("tape_record_state") );						
                    view->updateTapeIcons( TapeMode::Record );
                } else
                    status->addMessage( trans->get("tape_record_wp_state"), 3, true );						

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
                status->addMessage( trans->get("tape_counter_reset") );
            } 															
            break;
        }
        case Hotkey::Id::DigiBoost: {
            if (!activeEmulator || !dynamic_cast<LIBC64::Interface*>(activeEmulator))
                break;

            auto view = EmuConfigView::TabWindow::getView( activeEmulator );
            bool state = view->systemLayout->toggleFeature( C64Interface::FeatureIdDigiboost );
            status->addMessage( trans->get( state ? "digiboost_on" : "digiboost_off" ) );
        } break;
        case Hotkey::Id::SwapSid: {
            if (!activeEmulator || !dynamic_cast<LIBC64::Interface*>(activeEmulator))
                break;
            auto view = EmuConfigView::TabWindow::getView( activeEmulator );
            bool state = view->systemLayout->toggleFeature( C64Interface::FeatureIdSid );
            status->addMessage( trans->get( state ? "sid_6581_on" : "sid_8580_on" ) );
        } break;
        case Hotkey::Id::ActivateFilter: {
            if (!activeEmulator || !dynamic_cast<LIBC64::Interface*>(activeEmulator))
                break;
            auto view = EmuConfigView::TabWindow::getView( activeEmulator );
            bool state = view->systemLayout->toggleFeature( C64Interface::FeatureIdFilter );
            status->addMessage( trans->get( state ? "audio_filter_on" : "audio_filter_off" ) );
        } break;
        case Hotkey::AdjustBiasUp:
        case Hotkey::AdjustBiasDown: {
            if (!activeEmulator || !dynamic_cast<LIBC64::Interface*>(activeEmulator))
                break;
            auto view = EmuConfigView::TabWindow::getView( activeEmulator );
            int state = view->systemLayout->updateFeature( C64Interface::FeatureIdBias, id == Hotkey::AdjustBiasUp ? 100: -100 );
            status->addMessage( trans->get( "sid_bias_change", {{"%state%", std::to_string(state) }} ) );                    
        } break;
        case Hotkey::DiskSwap0: case Hotkey::DiskSwap1: case Hotkey::DiskSwap2:
        case Hotkey::DiskSwap3: case Hotkey::DiskSwap4: case Hotkey::DiskSwap5:
        case Hotkey::DiskSwap6: case Hotkey::DiskSwap7: case Hotkey::DiskSwap8:
        case Hotkey::DiskSwap9: case Hotkey::DiskSwap10: case Hotkey::DiskSwap11:
        case Hotkey::DiskSwap12: case Hotkey::DiskSwap13: case Hotkey::DiskSwap14: {
            if (!activeEmulator)
                break;

            auto mediaId = settings->get<unsigned>(program->ident(activeEmulator, "access_floppy"), 0u, {0u, 3u});

            auto media = activeEmulator->getDisk( mediaId );
            if (!media)
                break;                                        

            uint8_t* data;						

            auto swapPos = id - Hotkey::DiskSwap0;
            auto setting = FileSetting::getInstance( program->ident(activeEmulator, "swapper_" + std::to_string(swapPos)) );
            GUIKIT::File* file = filePool->get( setting->path );

            if (!file || !file->isSizeValid(MAX_MEDIUM_SIZE) ||                
                ((data = file->archiveData(setting->id)) == nullptr)
            ) {  
                status->addMessage(trans->get("file_open_error", {{ "%path%", setting->file }}), 2, true);
                break;
            }

            activeEmulator->ejectDisk( media );
            activeEmulator->insertDisk(media, data, file->archiveDataSize(setting->id));
            activeEmulator->writeProtectDisk(media, file->isArchived() ? true : setting->writeProtect);
            media->guid = uintptr_t(file);
            filePool->assign(program->ident(activeEmulator, media->name), file);
            filePool->assign(program->ident(activeEmulator, "swapper_" + std::to_string(swapPos)), file);
            filePool->unloadOrphaned();
            States::getInstance( activeEmulator )->updateImage( setting, media );
            status->addMessage( trans->get("insert_floppy", {{"%media%", media->name},{"%file%", setting->file}}) );		
            break;	
        }
    }
    
}

auto InputManager::pollHotkeys() -> void {
    bool _ignoreSecond = false;
    
    for( auto& hotkey : hotkeys ) {
        InputMapping* mapping = (InputMapping*)hotkey.guid;
        
        if ( mapping->state ) {
            if (hotkey.id == Hotkey::Id::ToggleFastForward || hotkey.id == Hotkey::Id::ToggleFastForwardAggressive) {
                if (_ignoreSecond)
                    continue;
            
                _ignoreSecond = true;
            }
            fireHotkey( hotkey.id );        
        }
    }
    
    if (hotkeyTriggers.size() == 0)
        return;
    
    for( auto& id : hotkeyTriggers )
        fireHotkey( id );        
    
    hotkeyTriggers.clear();
}

auto InputManager::activateHotkey(Hotkey::Id id) -> void {
    hotkeyTriggers.push_back( id );
}

auto InputManager::unmapHotkeys() -> void {
    for(auto& hotkey : hotkeys) {
                
        auto mapping = (InputMapping*)hotkey.guid;
        
        mapping->init();

        if (mapping->alternate)
            mapping->alternate->init();
    }
    for (auto manager : inputManagers)
        manager->updateMappingsInUse();
}

auto InputManager::openMenu( Hotkey::Id id ) -> void {
    Emulator::Interface* emu = activeEmulator;
    
    if (!emu) {
        auto ident = settings->get<std::string>("last_used_emu", "C64");
        
        for(auto emulator : emulators) {
            
            if (emulator->ident == ident) {
                emu = emulator;
                break;
            }
        }
    }
    
    if (!emu)
        return;
    
    auto configView = EmuConfigView::TabWindow::getView( emu );
    
    switch(id) {
        case Hotkey::Id::Video:
            configView->showDelayed( EmuConfigView::TabWindow::Layout::Video ); break;
        case Hotkey::Id::Border:
            configView->showDelayed( EmuConfigView::TabWindow::Layout::Border ); break;
        case Hotkey::Id::Palette:
            configView->showDelayed( EmuConfigView::TabWindow::Layout::Palette ); break;
        case Hotkey::Id::DiskSwapper:
            configView->showDelayed( EmuConfigView::TabWindow::Layout::Swapper ); break;
        case Hotkey::Id::Drives:
            configView->showDelayed( EmuConfigView::TabWindow::Layout::Media ); break;
        case Hotkey::Id::System:
            configView->showDelayed(EmuConfigView::TabWindow::Layout::System); break;
        case Hotkey::Id::Firmware:
            configView->showDelayed( EmuConfigView::TabWindow::Layout::Firmware ); break;
        case Hotkey::Id::Input:
            configView->showDelayed( EmuConfigView::TabWindow::Layout::Input ); break;
    }
}
