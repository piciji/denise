
#include "manager.h"

std::vector<InputMapping*> InputManager::hotkeyTriggers;

auto InputManager::setHotkeys() -> void {
    hotkeys.push_back( {Hotkey::Id::Pause, "Pause"} );
    hotkeys.push_back( {Hotkey::Id::Fullscreen, "Fullscreen"} );
    hotkeys.push_back( {Hotkey::Id::ToggleFastForward, "Toggle_fastforward"} );
    hotkeys.push_back( {Hotkey::Id::ToggleFastForwardAggressive, "Toggle_fastforward_aggressive"} );
    hotkeys.push_back( {Hotkey::Id::CaptureMouse, "Capture_mouse"} );        
    
    hotkeys.push_back( {Hotkey::Id::ToggleMenu, "Toggle_menu"} );
    hotkeys.push_back( {Hotkey::Id::ToggleStatus, "Toggle_status"} );	
	
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
	customHotkeys.push_back( {Hotkey::Id::Power, "power", true} );
	customHotkeys.push_back( {Hotkey::Id::SoftReset, "Soft Reset", true} );
    customHotkeys.push_back( {Hotkey::Id::MultiLoad, "multi load", true} );

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
	}	
    
	customHotkeys.push_back( {Hotkey::Id::Software, "Software", true} );	
    customHotkeys.push_back( {Hotkey::Id::System, "System", true} );
	customHotkeys.push_back( {Hotkey::Id::Control, "Control", true} );
	customHotkeys.push_back( {Hotkey::Id::States, "States", true} );
	customHotkeys.push_back( {Hotkey::Id::Presentation, "Presentation", true} );
	customHotkeys.push_back( {Hotkey::Id::Palette, "Palette", true} );
    customHotkeys.push_back( {Hotkey::Id::Firmware, "Firmware", true} );
	customHotkeys.push_back( {Hotkey::Id::Border, "Border", true} );
    customHotkeys.push_back( {Hotkey::Id::DiskSwapper, "Disk_swapper", true} );                        
}

auto InputManager::fireHotkey(Emulator::Interface* emulator, Hotkey::Id id) -> void {
    
    typedef LIBC64::Interface C64Interface;
    typedef LIBAMI::Interface AmigaInterface;
    
    switch ( id ) {
		case Hotkey::Id::SwapInputDevices: {
			auto connector1 = emulator->getConnector( 0 );
            auto connectedDevice1 = emulator->getConnectedDevice( connector1 );
            
            auto connector2 = emulator->getConnector( 1 );
            auto connectedDevice2 = emulator->getConnectedDevice( connector2 );
            
            emulator->connect( connector1, connectedDevice2 );
            emulator->connect( connector2, connectedDevice1 );
			
			view->checkInputDevice( emulator, connector1, connectedDevice2 );
			view->checkInputDevice( emulator, connector2, connectedDevice1 );
			
			EmuConfigView::TabWindow::getView(emulator)->inputLayout->updateConnectorButtons();
		} break;
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
			
		case Hotkey::Power:
			program->power(emulator);
			break;
			
		case Hotkey::SoftReset:
			program->reset(emulator);
			break;
			
        case Hotkey::MultiLoad: {                
            MediaView::MediaWindow::getView( emulator )->multiLoad();
            break;
        }
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
        case Hotkey::Id::Software:
        case Hotkey::Id::Presentation:
        case Hotkey::Id::Palette:
        case Hotkey::Id::Border:
        case Hotkey::Id::Firmware:
        case Hotkey::Id::System:
        case Hotkey::Id::Control:
		case Hotkey::Id::States:	
            openMenu( emulator, id );
            break;
        case Hotkey::Id::Pause:
            program->isPause ^= 1;
            audioDriver->clear();
            break;
        case Hotkey::IncSlot:
        case Hotkey::DecSlot: 
            States::getInstance( emulator )->changeSlot( id == Hotkey::DecSlot );
            break;
        case Hotkey::Loadstate:
            States::getInstance( emulator )->load();
            break;
        case Hotkey::Savestate:
            States::getInstance( emulator )->save();
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
                activeEmulator->controlTape( media, TapeMode::Record );
                status->addMessage( trans->get("tape_record_state") );						
                view->updateTapeIcons( TapeMode::Record );
                if (setting->writeProtect)
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
        case Hotkey::Id::ToggleSidFilter: {
            if (!activeEmulator || !dynamic_cast<LIBC64::Interface*>(activeEmulator))
                break;
            auto view = EmuConfigView::TabWindow::getView( activeEmulator );
            bool state = view->systemLayout->toggleFeature( C64Interface::FeatureIdFilter );
            status->addMessage( trans->get( state ? "sid_filter_on" : "sid_filter_off" ) );
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
	
	if (hotkeyTriggers.size() == 0)
		return;
	
	std::vector<InputMapping*> useTrigger;
	InputMapping* viewOpen = nullptr;
	InputMapping* fastForward = nullptr;
	InputMapping* stateHandler = nullptr;
	InputMapping* deviceSwapper = nullptr;
	InputMapping* starter = nullptr;
    InputMapping* multiLoad = nullptr;
	
	auto useEmu = activeEmulator;
	
	for( auto trigger : hotkeyTriggers ) {
		
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
			case Hotkey::Id::States:
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
				
            case Hotkey::MultiLoad:
				if (!useEmu) 
					useEmu = program->getLastUsedEmu();				

				if (!multiLoad)
					multiLoad = trigger;				
				
				else if (useEmu == trigger->inputManager->emulator)
					multiLoad = trigger;
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
    
    if (multiLoad)
		useTrigger.push_back( multiLoad );
	
	for( auto trigger : useTrigger )			
		fireHotkey( trigger->inputManager ? trigger->inputManager->emulator : nullptr, (Hotkey::Id)trigger->hotkeyId );   
	
	hotkeyTriggers.clear();		
}

auto InputManager::activateHotkey(Hotkey::Id id, Emulator::Interface* emulator) -> void {
	
	for (auto manager : inputManagers) {
		if (emulator && emulator != manager->emulator)
			continue;
		
		for( auto mapping : manager->mappings ) {
			
			if (mapping->emuDevice)
				continue;
			
			if (mapping->hotkeyId == id) {
				hotkeyTriggers.push_back( mapping );
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
    
    auto configView = EmuConfigView::TabWindow::getView( emulator );
    
    switch(id) {
        case Hotkey::Id::Presentation:
            configView->showDelayed( EmuConfigView::TabWindow::Layout::Presentation ); break;
        case Hotkey::Id::Border:
            configView->showDelayed( EmuConfigView::TabWindow::Layout::Border ); break;
        case Hotkey::Id::Palette:
            configView->showDelayed( EmuConfigView::TabWindow::Layout::Palette ); break;
        case Hotkey::Id::DiskSwapper:
            configView->showDelayed( EmuConfigView::TabWindow::Layout::Swapper ); break;
        case Hotkey::Id::Software:
            MediaView::MediaWindow::getView( emulator )->showDelayed(); break;
        case Hotkey::Id::System:
            configView->showDelayed(EmuConfigView::TabWindow::Layout::System); break;
        case Hotkey::Id::Firmware:
            configView->showDelayed( EmuConfigView::TabWindow::Layout::Firmware ); break;
        case Hotkey::Id::Control:
            configView->showDelayed( EmuConfigView::TabWindow::Layout::Control ); break;
		case Hotkey::Id::States:
            configView->showDelayed( EmuConfigView::TabWindow::Layout::States ); break;
    }
}
