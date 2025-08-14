
#include "manager.h"
#include "../view/status.h"
#include "../audio/manager.h"
#include "../thread/emuThread.h"
#include "../media/fileloader.h"
#include "../media/autoloader.h"

std::vector<InputMapping*> InputManager::hotkeyTriggers;

auto InputManager::setHotkeys() -> void {
    hotkeys.push_back( {Hotkey::Id::Pause, "Pause"} );
    hotkeys.push_back( {Hotkey::Id::Fullscreen, "Fullscreen"} );
    hotkeys.push_back( {Hotkey::Id::ToggleWarp, "Toggle Warp"} );
    hotkeys.push_back( {Hotkey::Id::ToggleWarpAggressive, "Toggle Warp Aggressive"} );
    
    hotkeys.push_back( {Hotkey::Id::ToggleMenu, "Toggle_menu"} );
    hotkeys.push_back( {Hotkey::Id::ToggleStatus, "Toggle_status"} );	
	
    hotkeys.push_back( {Hotkey::Id::RunAheadUp, "runahead up"} );	
    hotkeys.push_back( {Hotkey::Id::RunAheadDown, "runahead down"} );	
    hotkeys.push_back( {Hotkey::Id::RunAheadToggleMode, "runahead toggle mode"} );	
    
    hotkeys.push_back( {Hotkey::Id::ToggleCycleRenderer, "Toggle Cycle renderer"} );	
    hotkeys.push_back( {Hotkey::Id::AudioRecord, "audio record"} );

    hotkeys.push_back( {Hotkey::Id::Freeze, "freeze button"} );
    hotkeys.push_back( {Hotkey::Id::Rotation, "image rotation"} );

    hotkeys.push_back( {Hotkey::Id::SyncStatus, "Sync status"} );
    hotkeys.push_back( {Hotkey::Id::ThreadedRenderer, "Threaded Renderer"} );
    hotkeys.push_back( {Hotkey::Id::Quit, "exit"} );

    hotkeys.push_back( {Hotkey::Id::ToggleSCVideo, "toggle S/C-Video"} );
    hotkeys.push_back( {Hotkey::Id::ToggleShader, "toggle Shader"} );
    hotkeys.push_back( {Hotkey::Id::ToggleBorder, "toggle border"} );
    hotkeys.push_back( {Hotkey::Id::ToggleBorderPrev, "toggle border prev"} );
    hotkeys.push_back( {Hotkey::Id::ToggleScaling, "toggle scaling"} );
    hotkeys.push_back( {Hotkey::Id::ToggleFPS, "show_fps"} );
    hotkeys.push_back( {Hotkey::Id::ToggleAudioStats, "show_audio_buffer"} );
    hotkeys.push_back( {Hotkey::Id::ApplyWindowSize, "apply window size"} );
    hotkeys.push_back( {Hotkey::Id::CropWindow, "crop window"} );
    hotkeys.push_back({ Hotkey::Id::TakeScreenShot, "take screenshot" });

    hotkeys.push_back( {Hotkey::Id::FloppyAccess, "select_disk_drive"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwapUp, "swapper up"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwapDown, "swapper down"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwap0, "swap media0"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwap1, "swap media1"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwap2, "swap media2"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwap3, "swap media3"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwap4, "swap media4"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwap5, "swap media5"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwap6, "swap media6"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwap7, "swap media7"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwap8, "swap media8"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwap9, "swap media9"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwap10, "swap media10"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwap11, "swap media11"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwap12, "swap media12"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwap13, "swap media13"} );
    hotkeys.push_back( {Hotkey::Id::DiskSwap14, "swap media14"} );

    // not assignable, not saveable
    hiddenHotkeys.push_back( {Hotkey::Id::Warp, ""} );
    hiddenHotkeys.push_back( {Hotkey::Id::WarpOff, ""} );
}

auto InputManager::setCustomHotkeys() -> void {

    customHotkeys.push_back( {Hotkey::Id::CaptureMouse, "Capture_mouse", false} );
	customHotkeys.push_back( {Hotkey::Id::Loadstate, "Loadstate", true} );
	customHotkeys.push_back( {Hotkey::Id::Savestate, "Savestate", true} );
	customHotkeys.push_back( {Hotkey::Id::IncSlot, "Incslot", true} );
    customHotkeys.push_back( {Hotkey::Id::DecSlot, "Decslot", true} );
	customHotkeys.push_back( {Hotkey::Id::Power, "Hard Reset", true} );
    if (dynamic_cast<LIBC64::Interface*>(emulator) )
        customHotkeys.push_back( {Hotkey::Id::PowerWithUnplugCart, "Hard Reset + Unplug Cart", false} );
	customHotkeys.push_back( {Hotkey::Id::SoftReset, "Soft Reset", true} );
    customHotkeys.push_back( {Hotkey::Id::AnyLoad, "load software", true} );

	if (dynamic_cast<LIBC64::Interface*>(emulator) ) {
	    customHotkeys.push_back( {Hotkey::Id::SwapPortDevices, "swap Ports", false} );
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
	    customHotkeys.push_back( {Hotkey::Id::ToggleSuperCpuTurbo, "SuperCPU Turbo", false} );
	} else
        customHotkeys.push_back( {Hotkey::Id::SwapJoypadsPort2, "swap joypads Port2", false} );
    
	customHotkeys.push_back( {Hotkey::Id::Software, "Software", true} );	
    customHotkeys.push_back( {Hotkey::Id::System, "System", true} );
	customHotkeys.push_back( {Hotkey::Id::Control, "Control", true} );
	customHotkeys.push_back( {Hotkey::Id::Configurations, "Configurations", true} );
	customHotkeys.push_back( {Hotkey::Id::Presentation, "Presentation", true} );
	customHotkeys.push_back( {Hotkey::Id::Palette, "Palette", true} );
    customHotkeys.push_back( {Hotkey::Id::Firmware, "Firmware", true} );
    customHotkeys.push_back( {Hotkey::Id::Audio, "Audio", true} );
	customHotkeys.push_back( {Hotkey::Id::Geometry, "Geometry", true} );
    customHotkeys.push_back( {Hotkey::Id::DiskSwapper, "swapper", true} );
    customHotkeys.push_back( {Hotkey::Id::AutoStart, "autostart media", true} );
}

auto InputManager::fireHotkey(InputMapping* trigger) -> void {
    Emulator::Interface* emulator = trigger->inputManager ? trigger->inputManager->emulator : nullptr;
    if (emulator && !globalSettings->get<bool>("core_" + emulator->ident, true))
        return;

    Hotkey::Id id = (Hotkey::Id)trigger->hotkeyId;

    typedef LIBC64::Interface C64Interface;
    typedef LIBAMI::Interface AmigaInterface;
    
    auto settings = program->getSettings( activeEmulator );
    
    switch ( id ) {
        case Hotkey::Id::Rotation: {
            emuThread->lock();
            auto emuView = EmuConfigView::TabWindow::getView(activeEmulator);
            DRIVER::Rotation rotation = videoDriver->getRotation();

            if (rotation == DRIVER::ROT_0) rotation = DRIVER::ROT_90;
            else if (rotation == DRIVER::ROT_90) rotation = DRIVER::ROT_180;
            else if (rotation == DRIVER::ROT_180) rotation = DRIVER::ROT_270;
            else if (rotation == DRIVER::ROT_270) rotation = DRIVER::ROT_0;

            settings->set<unsigned>("rotation", (unsigned)rotation);
            videoDriver->setRotation(rotation);
            if (emuView && emuView->geometryLayout)
                emuView->geometryLayout->setRotation(rotation);

        } break;

        case Hotkey::Id::AudioRecord: {
            if (!activeEmulator)
                break;

            program->toggleRecord();
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

            statusHandler->setMessage( trans->get( "runahead input latency", {{"%count%", std::to_string(pos) }} ), false, true );

        } break;
            
        case Hotkey::Id::ToggleCycleRenderer: {
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
                    program->setWarp(false);
                    program->power(activeEmulator);
                }
            }
        } break;
        
        case Hotkey::Id::RunAheadToggleMode: {
            if (!activeEmulator)
                break;
            
            bool state = settings->get<bool>( "runahead_performance", dynamic_cast<LIBAMI::Interface*>(activeEmulator));
            state ^= 1;
            settings->set<bool>( "runahead_performance", state);
            emuThread->lock();
            activeEmulator->runAheadPerformance( state );
            emuThread->unlock();

            auto emuView = EmuConfigView::TabWindow::getView(activeEmulator);
            if (emuView && emuView->miscLayout)
                emuView->miscLayout->setRunAheadPerformance( state );
            
            statusHandler->setMessage( trans->get( !state ? "runahead accuracy mode" : "runahead performance mode" ), false, true );
        } break;

        case Hotkey::Id::SwapJoypadsPort2: {
            if (activeEmulator != emulator)
                break;

            emuThread->lock();
            auto settings = program->getSettings( emulator );
            auto connector = emulator->getConnector( 1 );
            auto connectedDevice = emulator->getConnectedDevice( connector );

            for (auto& device : emulator->devices) {
                if (!device.isJoypad())
                    continue;
                if (!connectedDevice || !connectedDevice->isJoypad()) {
                    connectedDevice = &device;
                    break;
                }
                if (connectedDevice != &device) {
                    connectedDevice = &device;
                    break;
                }
            }

            emulator->connect( connector, connectedDevice );
            auto manager = InputManager::getManager(emulator);
            manager->updateMappingsInUse();
            settings->set<unsigned>( _underscore(connector->name), connectedDevice->id);
            view->checkInputDevice(emulator, connector, connectedDevice);

            auto emuView = EmuConfigView::TabWindow::getView(emulator);
            if (emuView && emuView->inputLayout)
                emuView->inputLayout->updateConnectorButtons();
        } break;
		case Hotkey::Id::SwapPortDevices: {
		    if (activeEmulator != emulator)
		        break;

            emuThread->lock();
            auto settings = program->getSettings( emulator );
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

            auto emuView = EmuConfigView::TabWindow::getView(emulator);
            if (emuView && emuView->inputLayout)
                emuView->inputLayout->updateConnectorButtons();
		} break;
        case Hotkey::Id::ToggleWarp:
        case Hotkey::Id::ToggleWarpAggressive:
            emuThread->lock();
            program->toggleWarp( id == Hotkey::Id::ToggleWarpAggressive );
            break;

        case Hotkey::Id::Warp:
            if (!program->warp.active) {
                emuThread->lock();
                program->setWarp(true, program->warp.aggressive);
            }
            break;

        case Hotkey::Id::WarpOff:
            if (program->warp.active) {
                emuThread->lock();
                program->setWarp(false);
            }
            break;

        case Hotkey::Id::Fullscreen:
            emuThread->lock();
            view->switchFullScreen( !view->fullScreen() );
            break;

        case Hotkey::Id::Quit: {
            auto tempTimer = new GUIKIT::Timer;
            tempTimer->setInterval(5);
            tempTimer->onFinished = [tempTimer]() {
                tempTimer->setEnabled(false);
                view->onClose();
            };
            tempTimer->setEnabled();
        } break;
			
		case Hotkey::Power:
            emuThread->lock();
			program->power(emulator);
			break;

        case Hotkey::PowerWithUnplugCart:
            emuThread->lock();
            program->power(emulator);
            program->removeExpansion(false);
            break;
			
		case Hotkey::SoftReset:
            emuThread->lock();
			program->reset(emulator);
			break;

        case Hotkey::ToggleFPS: {
            if (statusHandler) {
                statusHandler->showFPSScreen ^= 1;
                if (!statusHandler->showFPSScreen)
                    videoDriver->showScreenText( "", 0 );
                else
                    statusHandler->setFpsCounterUpdate();

                auto fpsScreen = globalSettings->get<unsigned>("fps_screen", 0);

                if (statusHandler->showFPSScreen)
                    fpsScreen |= 1 << (view->fullScreen() ? 1 : 0);
                else
                    fpsScreen &= ~(1 << (view->fullScreen() ? 1 : 0));

                globalSettings->set<unsigned>("fps_screen", fpsScreen);
            }
        } break;

        case Hotkey::ToggleAudioStats: {
            if (audioManager) {
                audioManager->statistics.enable ^= 1;
                if (!audioManager->statistics.enable)
                    videoDriver->showScreenText( "", 0 );
                else
                    statusHandler->setDrcBufferUpdate();
            }
        } break;
			
        case Hotkey::AnyLoad:           
            view->setAnyload( emulator );
            break;
            
        case Hotkey::Id::CaptureMouse:
            if (inputDriver->mIsAcquired()) {
                inputDriver->mUnacquire();					
            } else if (view->fullScreen()) {
                // dinput needs this, when grab button is mapped to mouse
                view->prepareCursorHide(200);
                //inputDriver->mAcquire();
            } else if (!program->isPause && program->isAnalogDeviceConnected()) {
                view->prepareCursorHide(200);
                // inputDriver->mAcquire();
                view->setFocused();
            }
            break;
        case Hotkey::Id::DiskSwapper:
        case Hotkey::Id::Software:
        case Hotkey::Id::Presentation:
        case Hotkey::Id::Palette:
        case Hotkey::Id::Geometry:
        case Hotkey::Id::Firmware:
        case Hotkey::Id::System:
        case Hotkey::Id::Control:
		case Hotkey::Id::Configurations:
        case Hotkey::Id::Audio:
            openMenu( emulator, id );
            break;
        case Hotkey::Id::SyncStatus: {
            std::string _str = videoDriver->hasThreaded() ? "Threaded  " : "";

            if (videoDriver->hasSynchronized()) {
                _str += "Vsync";
                _str += VideoManager::frameRenderTrigger > 1 ? "(" + std::to_string(VideoManager::frameRenderTrigger) + "x)  " : "  ";
            }

            _str += videoDriver->hasVRR() ? "VRR  " : "";
            _str += audioDriver->hasSynchronized() ? "Audiosync  " : "";
            _str += audioManager->dynamicRateControl ? "DRC  " : "";
            float monitorFrequency = GUIKIT::Monitor::getCurrentRefreshRate();
            _str += GUIKIT::String::convertDoubleToString(monitorFrequency, 3 );
            emuThread->lock();
            if (statusHandler)
                statusHandler->setMessage(_str, false, true, 4);
        } break;
        case Hotkey::Id::TakeScreenShot:
            if (view) {
                emuThread->lock();
                view->takeScreenshot();
            }
            break;
        case Hotkey::Id::ThreadedRenderer: {
            auto _settings = program->getSettings( activeEmulator );
            unsigned tr = _settings->get<unsigned>("threaded_renderer", 1);
            if (++tr == 3)
                tr = 0;

            _settings->set<unsigned>("threaded_renderer", tr);

            auto emuView = EmuConfigView::TabWindow::getView( activeEmulator );
            if (emuView && emuView->videoLayout) {
                switch (tr) {
                    default:
                    case 0: emuView->videoLayout->layBase.view.option.trOff.setChecked(); break;
                    case 1: emuView->videoLayout->layBase.view.option.trOn.setChecked(); break;
                    case 2: emuView->videoLayout->layBase.view.option.trAuto.setChecked(); break;
                }
            }

            emuThread->lock();
            if (statusHandler) {
                switch (tr) {
                    default:
                    case 0: statusHandler->setMessage( trans->getA("Threaded Renderer") + " " + trans->getA("disabled"), false, true ); break;
                    case 1: statusHandler->setMessage(trans->getA("Threaded Renderer") + " " + trans->getA("enabled"), false, true); break;
                    case 2: statusHandler->setMessage(trans->getA("Threaded Renderer") + " " + trans->getA("auto"), false, true ); break;
                }
            }

            VideoManager::setSynchronize();
        } break;
        case Hotkey::Id::ToggleShader:
            if(!videoDriver->shaderSupport())
                break;
        case Hotkey::Id::ToggleSCVideo: {
            if (!activeEmulator)
                break;
            emuThread->lock();
            unsigned _mode = (id == Hotkey::Id::ToggleSCVideo) ? (unsigned)VideoManager::CrtMode::Cpu : (unsigned)VideoManager::CrtMode::Gpu;
            unsigned _current = settings->get<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::None);

            if (id == Hotkey::Id::ToggleSCVideo && _current == (unsigned)VideoManager::CrtMode::Cpu) {
                _mode = (unsigned)VideoManager::CrtMode::None;
            } else if (id == Hotkey::Id::ToggleShader && _current == (unsigned)VideoManager::CrtMode::Gpu) {
                _mode = (unsigned)VideoManager::CrtMode::None;
            }

            settings->set<unsigned>("video_crt", _mode);
            program->setWarp( false );
            auto emuView = EmuConfigView::TabWindow::getView( activeEmulator );
            if (emuView && emuView->videoLayout)
                emuView->videoLayout->loadSettings();
            else
                VideoManager::getInstance( activeEmulator )->reloadSettings(false);

            if (statusHandler) {
                std::string txt = "RGB";
                if (_mode == (unsigned)VideoManager::CrtMode::Cpu) txt = "S/C-Video CPU";
                else if (_mode == (unsigned)VideoManager::CrtMode::Gpu) txt = "Shader GPU";
                statusHandler->setMessage( trans->getA(txt), false, true );
            }

        } break;
        case Hotkey::Id::Pause:
            view->togglePause();
            view->updatePauseCheck();
            break;
        case Hotkey::IncSlot:
        case Hotkey::DecSlot:
            if (emulator == activeEmulator) {
                emuThread->lock();
                States::getInstance( activeEmulator )->changeSlot( id == Hotkey::DecSlot );
            }
            break;
        case Hotkey::Loadstate:
            emuThread->lock();
            States::getInstance( emulator )->load();
            break;
        case Hotkey::Savestate:
            if (emulator == activeEmulator) {
                emuThread->lock();
                States::getInstance( activeEmulator )->save();
            }
            break;
        case Hotkey::ToggleMenu:
            if(!view->fullScreen()) view->updateMenuBar( true );
            break;
        case Hotkey::ToggleStatus:
            emuThread->lock();
            if(!videoDriver || !videoDriver->hasExclusiveFullscreen()) view->updateStatusBar( true );
            break;

        case Hotkey::ApplyWindowSize:
            if (!view || view->fullScreen())
                break;

            emuThread->lock();
            globalSettings->set<unsigned>("screen_width", globalSettings->get<unsigned>("view_hold_width", 800));
            globalSettings->set<unsigned>("screen_height", globalSettings->get<unsigned>("view_hold_height", 600));

            view->updateGeometry(true);
            break;

        case Hotkey::CropWindow:
            if (view)
                view->adjustToEmu(true);
            break;

        case Hotkey::Id::ToggleScaling: {
            auto hotkeyState = settings->get<unsigned>( "scale_hotkey", program->getScaleHotkeyDefault() );
            int aspectMode = settings->get<int>("aspect_mode", 1, {0, 3});
            auto aspectModeOld = aspectMode;

            do {
                aspectMode++;
                if (aspectMode > 3)
                    aspectMode = 0;

                if (hotkeyState & (1 << aspectMode))
                    break;

            } while (aspectMode != aspectModeOld);

            if (aspectMode == aspectModeOld)
                break;

            auto emuView = EmuConfigView::TabWindow::getView( activeEmulator );
            if (emuView && emuView->geometryLayout) {
                switch(aspectMode) {
                    case 0: emuView->geometryLayout->ratioLayout.control.window.activate(); break;
                    case 1: emuView->geometryLayout->ratioLayout.control.tv.activate(); break;
                    case 2: emuView->geometryLayout->ratioLayout.control.native.activate(); break;
                    case 3: emuView->geometryLayout->ratioLayout.control.nativeFree.activate(); break;
                }
                emuThread->lock();
            } else {
                emuThread->lock();
                settings->set<unsigned>("aspect_mode", aspectMode);
                program->setVideoDimension(activeEmulator);
                view->updateViewport();
            }
            statusHandler->setMessage( program->getScaleMessage(activeEmulator, aspectMode), false, true );
        } break;

        case Hotkey::Id::ToggleBorder:
        case Hotkey::Id::ToggleBorderPrev: {
            bool prev = id == Hotkey::Id::ToggleBorderPrev;
            typedef Emulator::Interface::CropType CropType;
            int cropType = settings->get<int>("crop_type", (unsigned)CropType::Monitor, {0u, 11u});
            auto hotkeyState = settings->get<unsigned>( "border_hotkey", program->getCropHotkeyDefault() );
            auto cropTypeOld = cropType;

            do {
                cropType += prev ? -1 : 1;
                if (cropType > 11)
                    cropType = 0;
                else if (cropType < 0)
                    cropType = 11;

                if (hotkeyState & (1 << cropType))
                    break;

            } while (cropType != cropTypeOld);

            if (cropType == cropTypeOld)
                break;

            auto emuView = EmuConfigView::TabWindow::getView( activeEmulator );

            if (emuView && emuView->geometryLayout) {
                if ((CropType)cropType == CropType::Off) emuView->geometryLayout->cropLayout.type1.cropOff.activate();
                else if ((CropType)cropType == CropType::Monitor) emuView->geometryLayout->cropLayout.type1.cropMonitor.activate();
                else if ((CropType)cropType == CropType::AutoRatio) emuView->geometryLayout->cropLayout.type1.cropAutoAspect.activate();
                else if ((CropType)cropType == CropType::Auto) emuView->geometryLayout->cropLayout.type1.cropAuto.activate();
                else if ((CropType)cropType == CropType::AllSidesRatio) emuView->geometryLayout->cropLayout.type2.cropAllSidesAspect.activate();
                else if ((CropType)cropType == CropType::AllSides) emuView->geometryLayout->cropLayout.type2.cropAllSides.activate();
                else if ((CropType)cropType == CropType::Free) emuView->geometryLayout->cropLayout.type3.cropFree1.activate();

                else if (cropType == ((int)CropType::Free + 1) ) emuView->geometryLayout->cropLayout.type3.cropFree2.activate();
                else if (cropType == ((int)CropType::Free + 2) ) emuView->geometryLayout->cropLayout.type3.cropFree3.activate();
                else if (cropType == ((int)CropType::Free + 3) ) emuView->geometryLayout->cropLayout.type4.cropFree4.activate();
                else if (cropType == ((int)CropType::Free + 4) ) emuView->geometryLayout->cropLayout.type4.cropFree5.activate();
                else if (cropType == ((int)CropType::Free + 5) ) emuView->geometryLayout->cropLayout.type4.cropFree6.activate();
                emuThread->lock();
            } else {
                settings->set<unsigned>("crop_type", cropType);
                emuThread->lock();
                program->updateCrop(activeEmulator);
            }
            statusHandler->setMessage(program->getCropMessage(activeEmulator, (CropType)cropType), false, true);
        } break;

        case Hotkey::ResetTapeCounter:
        case Hotkey::PlayTape:
        case Hotkey::RecordTape:
        case Hotkey::StopTape:
        case Hotkey::ForwardTape:
        case Hotkey::RewindTape: {
            if (!activeEmulator || !dynamic_cast<LIBC64::Interface*>(activeEmulator) )
                break;

            program->hintAutoWarp(0);

            emuThread->lock();
            auto media = activeEmulator->getTape( 0 );
            if (!media)
                break;

            unsigned driveCount = activeEmulator->getModelValue( activeEmulator->getModelIdOfEnabledDrives( media->group ) );

            if (driveCount == 0) {
                statusHandler->setMessage( trans->get("tape_disconnect"), true );
                break;
            }                        

            typedef Emulator::Interface::TapeMode TapeMode;

            if (id == Hotkey::PlayTape) {
                activeEmulator->controlTape( media, TapeMode::Play );
                statusHandler->setMessage(trans->get("tape_play_state"), false, true);
                view->updateTapeIcons( TapeMode::Play );
            } else if (id == Hotkey::StopTape) {
                activeEmulator->controlTape( media, TapeMode::Stop );
                statusHandler->setMessage(trans->get("tape_stop_state"), false, true);
                view->updateTapeIcons( TapeMode::Stop );
            } else if (id == Hotkey::RecordTape) {              
                activeEmulator->controlTape( media, TapeMode::Record );
                statusHandler->setMessage(trans->get("tape_record_state"), false, true);
                view->updateTapeIcons( TapeMode::Record );
                if (activeEmulator->isWriteProtected( media ))
                    statusHandler->setMessage( trans->get("tape_record_wp_state"), true );						

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
                statusHandler->setMessage(trans->get("tape_counter_reset"), false, true);
            } 															
            break;
        }
        case Hotkey::Id::DigiBoost: {
            if (!activeEmulator || !dynamic_cast<LIBC64::Interface*>(activeEmulator))
                break;

            auto emuView = EmuConfigView::TabWindow::getView( activeEmulator );
            bool state = false;

            if (emuView && emuView->audioLayout) {
                state = emuView->audioLayout->settingsLayout.toggleCheckbox( C64Interface::ModelIdDigiboost );
                emuThread->lock();
            } else {
                emuThread->lock();
                auto model = activeEmulator->getModel( C64Interface::ModelIdDigiboost );
                if (model) {
                    state = activeEmulator->getModelValue( model->id );
                    state ^= 1;
                    settings->set<bool>( _underscore(model->name), state );
                    activeEmulator->setModelValue( model->id, state );
                }
            }

            statusHandler->setMessage(trans->get(state ? "digiboost_on" : "digiboost_off"), false, true);
        } break;
        case Hotkey::Id::SwapSid: {
            if (!activeEmulator || !dynamic_cast<LIBC64::Interface*>(activeEmulator))
                break;
            auto emuView = EmuConfigView::TabWindow::getView( activeEmulator );
            unsigned val;

            if (emuView && emuView->systemLayout) {
                val = emuView->systemLayout->modelLayout.nextOption( C64Interface::ModelIdSid );
                emuThread->lock();
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

            if (emuView && emuView->audioLayout) {
                emuView->audioLayout->settingsLayout.updateWidget( C64Interface::ModelIdSid );
            }

            statusHandler->setMessage(trans->get(val == 1 ? "sid_6581_on" : "sid_8580_on"), false, true);
        } break;
        case Hotkey::Id::ToggleSidFilter: {
            if (!activeEmulator || !dynamic_cast<LIBC64::Interface*>(activeEmulator))
                break;
            auto emuView = EmuConfigView::TabWindow::getView( activeEmulator );
            bool state = false;

            if (emuView && emuView->audioLayout) {
                state = emuView->audioLayout->settingsLayout.toggleCheckbox( C64Interface::ModelIdFilter );
                emuThread->lock();
            } else {
                emuThread->lock();
                auto model = activeEmulator->getModel( C64Interface::ModelIdFilter );
                if (model) {
                    state = activeEmulator->getModelValue( model->id );
                    state ^= 1;
                    settings->set<bool>( _underscore(model->name), state );
                    activeEmulator->setModelValue( model->id, state );
                }
            }
            statusHandler->setMessage(trans->get(state ? "sid_filter_on" : "sid_filter_off"), false, true);
        } break;
        case Hotkey::AdjustBiasUp:
        case Hotkey::AdjustBiasDown: {
            if (!activeEmulator || !dynamic_cast<LIBC64::Interface*>(activeEmulator))
                break;
            
            int _sid = activeEmulator->getModelValue( C64Interface::ModelIdSid );
            auto emuView = EmuConfigView::TabWindow::getView( activeEmulator );
            int state;

            if (emuView && emuView->audioLayout) {
                state = emuView->audioLayout->settingsLayout.stepRange( _sid == 0 ? C64Interface::ModelIdBias8580 : C64Interface::ModelIdBias6581,
                        id == Hotkey::AdjustBiasUp ? 100: -100 );

                emuThread->lock();
            } else {
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
            statusHandler->setMessage(trans->get("sid_bias_change", { {"%state%", std::to_string(state) } }), false, true);
        } break;
        
        case Hotkey::Id::FloppyAccess: {
            if (!activeEmulator)
                break;

            emuThread->lock();
            auto mediaId = settings->get<unsigned>("access_floppy", 0u, {0u, 3u});
            mediaId++;
            auto media = activeEmulator->getEnabledDisk(mediaId);
            if (!media)
                break;

            settings->set<unsigned>( "access_floppy", media->id, false);
            statusHandler->setMessage(trans->get("access_floppy", { {"%drive%", media->name} }), false, true);
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

        case Hotkey::ToggleSuperCpuTurbo:
            statusHandler->setExpansionClick();
            break;

        case Hotkey::AutoStart: {
            emuThread->lock();
            bool trapped;
            unsigned selection;
            auto media = autoloader->get(emulator, trapped, selection);
            if (!media)
                break;

            auto fSetting = FileSetting::getInstance( emulator, _underscore(media->name ) );
            if (fSetting->path.empty())
                break;

            fileloader->autoload(emulator, media, selection, trapped );
        } break;
        
        case Hotkey::DiskSwap0: case Hotkey::DiskSwap1: case Hotkey::DiskSwap2:
        case Hotkey::DiskSwap3: case Hotkey::DiskSwap4: case Hotkey::DiskSwap5:
        case Hotkey::DiskSwap6: case Hotkey::DiskSwap7: case Hotkey::DiskSwap8:
        case Hotkey::DiskSwap9: case Hotkey::DiskSwap10: case Hotkey::DiskSwap11:
        case Hotkey::DiskSwap12: case Hotkey::DiskSwap13: case Hotkey::DiskSwap14:
        case Hotkey::DiskSwapUp: case Hotkey::DiskSwapDown: {
            if (!activeEmulator)
                break;

            int swapPos = settings->get<int>("swap_pos", -1);

            if (id == Hotkey::DiskSwapUp) {
                if (swapPos == -1)
                    swapPos = fileloader->getSwapPos(activeEmulator);
                swapPos++;
            } else if (id == Hotkey::DiskSwapDown) {
                if (swapPos == -1)
                    swapPos = fileloader->getSwapPos(activeEmulator);
                swapPos--;
            } else
                swapPos = id - Hotkey::DiskSwap0;

            if (swapPos < 0) swapPos = SWAPPER_SLOTS - 1;
            else if (swapPos >= SWAPPER_SLOTS) swapPos = 0;

            emuThread->lock();
            fileloader->insertSwapDisk(activeEmulator, swapPos);
            break;
        }
        case Hotkey::Autofire: {
            if (!activeEmulator)
                break;
            emuThread->lock();
            auto manager = trigger->inputManager;
            auto mapping = trigger->shadowMap[0];

            manager->toggleAutofire(mapping);

            statusHandler->setMessage(trans->get(manager->isAutofireActive(mapping) ? "Autofire active" : "Autofire inactive"), false, true, 4);

            auto emuView = EmuConfigView::TabWindow::getView( activeEmulator );
            if (emuView && emuView->inputLayout)
                emuView->inputLayout->updatedAutofireButtonHints();

        } break;
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

	std::vector<InputMapping*> useTrigger;
	InputMapping* viewOpen = nullptr;
	InputMapping* warp = nullptr;
    InputMapping* warpAutostart = nullptr;
	InputMapping* stateHandler = nullptr;
	InputMapping* starter = nullptr;
    InputMapping* anyLoad = nullptr;
    InputMapping* lastAutostart = nullptr;
    InputMapping* swapAutofire = nullptr;
	
	auto useEmu = activeEmulator;
    if (!useEmu)
        useEmu = program->getLastUsedEmu();

    for( auto trigger : _hotkeyTriggers ) {
		
		switch(trigger->hotkeyId) {
            case Hotkey::Id::Warp:
            case Hotkey::Id::WarpOff:
                warpAutostart = trigger; // use last event
                break;

			case Hotkey::Id::DiskSwapper:
			case Hotkey::Id::Software:
			case Hotkey::Id::Presentation:
			case Hotkey::Id::Palette:
			case Hotkey::Id::Geometry:
			case Hotkey::Id::Firmware:
			case Hotkey::Id::System:
			case Hotkey::Id::Control:
			case Hotkey::Id::Configurations:
            case Hotkey::Id::Audio:
				if (!viewOpen)
					viewOpen = trigger;				
				
				else if (useEmu == trigger->inputManager->emulator)
					viewOpen = trigger;
				
				break;
				
			case Hotkey::Id::ToggleWarp:
			case Hotkey::Id::ToggleWarpAggressive:
				if(!warp)
					warp = trigger;
				break;

			case Hotkey::IncSlot:
			case Hotkey::DecSlot:
			case Hotkey::Loadstate:
			case Hotkey::Savestate:
				if (!stateHandler)
					stateHandler = trigger;

				else if (useEmu == trigger->inputManager->emulator)
					stateHandler = trigger;
				break;

			case Hotkey::Power:
			case Hotkey::SoftReset:
				if (!starter)
					starter = trigger;				
				
				else if (useEmu == trigger->inputManager->emulator)
					starter = trigger;
				break;
				
            case Hotkey::AnyLoad:
				if (!anyLoad)
					anyLoad = trigger;				
				
				else if (useEmu == trigger->inputManager->emulator)
					anyLoad = trigger;
				break;

            case Hotkey::AutoStart:
                if (!lastAutostart)
                    lastAutostart = trigger;

                else if (useEmu == trigger->inputManager->emulator)
                    lastAutostart = trigger;
                break;

		    case Hotkey::Autofire:
		        swapAutofire = trigger;
		        break;
                
			default:
				if (!GUIKIT::Vector::find( useTrigger, trigger ))
					useTrigger.push_back( trigger );
				break;			
		}		
	}

    if (!warpAutostart || warp) {
        if (!Program::hasFocus() && !swapAutofire)
            return;
    }
	
	if (viewOpen)
		useTrigger.push_back( viewOpen );

	if(warp)
		useTrigger.push_back( warp );
    else if(warpAutostart)
        useTrigger.push_back( warpAutostart );
	
	if(stateHandler)
		useTrigger.push_back( stateHandler );

    if(lastAutostart)
        useTrigger.push_back( lastAutostart );

	if (starter)
		useTrigger.push_back( starter );
    
    if (anyLoad)
		useTrigger.push_back( anyLoad );

    if (swapAutofire)
        useTrigger.push_back( swapAutofire );

	for( auto trigger : useTrigger )
        fireHotkey( trigger );
}

auto InputManager::activateHotkey(Hotkey::Id id, Emulator::Interface* emulator) -> void {
	
	for (auto manager : inputManagers) {
		if (emulator && emulator != manager->emulator)
			continue;
		
		for( auto mapping : manager->mappings ) {
			
			if (mapping->emuDevice)
				continue;

            if (emulator && !mapping->inputManager)
                continue;

            if (emulator && mapping->inputManager->emulator != emulator)
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

auto InputManager::activateHiddenHotkey(Hotkey::Id id) -> void {
    for (auto& item: hiddenHotkeys) {
        if (item.id == id) {
            emuThread->lockHotkeys();
            hotkeyTriggers.push_back((InputMapping*) item.guid);
            emuThread->unlockHotkeys();
            break;
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
        case Hotkey::Id::Geometry:
            emuView->showDelayed( EmuConfigView::TabWindow::Layout::Geometry ); break;
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

            //emuView->mediaLayout->setMediaView();
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
        case Hotkey::Id::Audio:
            emuView->showDelayed( EmuConfigView::TabWindow::Layout::Audio ); break;
        default:
            break;
    }
}
