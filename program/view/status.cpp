
#include "status.h"
#include "view.h"
#include "../audio/manager.h"
#include "../cmd/cmd.h"
#include "../tools/chronos.h"
#include "../thread/emuThread.h"
#include "../emuconfig/config.h"
#include "../media/media.h"
#include "../input/manager.h"
#include "../emuconfig/layouts/audio.h"

StatusHandler* statusHandler = nullptr;

auto StatusHandler::updateLEDState(Emulator::Interface::LedId ledId, uint8_t state) -> void {
    setLedUpdate();

    for (auto& ledState : ledStates) {
        if (ledState.ledId == ledId) {
            ledState.inputQueue <<= 2;
            ledState.inputQueue |= state & 3;
            if (++ledState.inputs > 16)
                ledState.inputs = 1;
            return;
        }
    }
}

auto StatusHandler::updateDeviceState( Emulator::Interface::Media* media, bool write, unsigned position, uint8_t LED, bool motorOff ) -> void {
    setDeviceUpdate();
    
    for (auto& deviceState : deviceStates) {
        if (deviceState.media == media) {
            deviceState.write = write;
            deviceState.position = position;
            if (LED & 0x80) { // can be changed more times a frame
                deviceState.LED <<= 2;
                deviceState.LED |= LED & 3;
                deviceState.inputsPerFrame++;
                if (LED & 0x40) { // flicker
                    deviceState.LED <<= 2;
                    deviceState.inputsPerFrame++;
                }
            } else {
                deviceState.LED = LED & 3;
                deviceState.inputsPerFrame = 0;
            }

            deviceState.motorOff = motorOff;
            deviceState.update = true;
            return;
        }
    }

    uint16_t _led = LED & 3;
    uint8_t _inputsPerFrame = (LED & 0x80) ? 1 : 0;

    deviceStates.push_back({media, write, position, _led, motorOff, _inputsPerFrame, true});
}

auto StatusHandler::setMessage(const std::string& txt, bool warn, bool force, unsigned duration) -> void {
    if (!force && showOnlyUrgentMessages && !warn)
        return;

    if (txt.empty())
        duration = 0;

    message.txt = txt;
    message.duration = duration;
    message.warn = warn;
    
    setMessageUpdate();
}

auto StatusHandler::clear() -> void {    
    statusBar->hideContent();
    statusBar->updateText(15, "");
    statusBar->updateVisible(18, showVolume);
    statusBar->update();
    message.clear();
    clearUpdates();
    setMessage("", false, true);
}

auto StatusHandler::resetFrameCounter() -> void {
    if (!activeEmulator)
        return;

    auto& stats = activeEmulator->getStatsForSelectedRegion();

    float speed;
    bool percent;
    view->getSpeedBySelectedProfile(speed, percent);

    if (percent) {
        fpsCounter.fps = (stats.fps * (double)speed) / 100.0;

    } else {
        fpsCounter.fps = speed;
    }

    fpsCounter.sum = 0;
    fpsCounter.pos = 0;
    fpsCounter.measures = 0;
    fpsCounter.updateDelay = 0;
    fpsCounter.last = Chronos::getTimestampInMicroseconds();
    program->loopFrames = 0;
}

auto StatusHandler::setFpsRefresh() -> void {
    if (!activeEmulator)
        return;
    auto settings = program->getSettings( activeEmulator );
    fpsCounter.updateIntervall = settings->get<unsigned>("fps_refresh", 1000, {200u, 5000u});

    auto pointsBefore = fpsCounter.decimalPoints;
    fpsCounter.decimalPoints = settings->get<unsigned>("fps_decimal_point", 3u, {0u, 3u});

    if (pointsBefore != fpsCounter.decimalPoints) {
        switch(fpsCounter.decimalPoints) {
            case 0: statusBar->updateDimension( 0, "1000" ); break;
            case 1: statusBar->updateDimension( 0, "1000.9" ); break;
            case 2: statusBar->updateDimension( 0, "1000.99" ); break;
            case 3: statusBar->updateDimension( 0, "1000.999" ); break;
        }
        videoDriver->showScreenText( "", 0 );
        statusHandler->setFpsCounterUpdate();
    }
}

auto StatusHandler::updateOnScreenFPS() -> void {
    auto fpsScreen = globalSettings->get<unsigned>("fps_screen", 0);

    bool _showFPSScreen = showFPSScreen;
    if (fpsScreen & (1 << (view->fullScreen() ? 1 : 0)))    showFPSScreen = true;
    else                                                    showFPSScreen = false;

    if (_showFPSScreen == showFPSScreen)
        return;

    if (!showFPSScreen)
        videoDriver->showScreenText( "", 0 );
    else
        statusHandler->setFpsCounterUpdate();
}

auto StatusHandler::updateFrameCounter() -> void {
    float deviation;

    if (fpsCounter.measures == FPS_MEASUREMENTS) {
        fpsCounter.sum -= fpsCounter.deltas[fpsCounter.pos];
        deviation = 0.99;
    } else {
        fpsCounter.measures++;
        deviation = 0.8;
    }

    uint64_t cur = Chronos::getTimestampInMicroseconds();

    uint64_t delta = cur - fpsCounter.last;

    fpsCounter.deltas[fpsCounter.pos++] = delta;

    fpsCounter.sum += delta;

    fpsCounter.fps = (deviation * fpsCounter.fps) + (1.0 - deviation) * ((double)fpsCounter.measures / ((double)fpsCounter.sum / 1000000.0) );

    if (fpsCounter.pos == FPS_MEASUREMENTS) {
        fpsCounter.pos = 0;
    }

    fpsCounter.last = cur;

    unsigned limit = ((unsigned)fpsCounter.fps * fpsCounter.updateIntervall) / 1000;

    if (++fpsCounter.updateDelay >= limit) {
        if (!VideoManager::synchronized)
            // check input polling and message loop every 50 ms
            program->loopFrames = ((unsigned)fpsCounter.fps * 50 ) / 1000;
        else
            // check input polling every frame
            program->loopFrames = 0;

        fpsCounter.updateDelay = 0;
        if (!cmd->noGui) {
            setFpsCounterUpdate();
        }
    }
}

auto StatusHandler::updateFPS( bool state ) -> void {
    emuThread->lockStatus();
    showFPS = state;
    if (!showFPS) {
        updateVisible(0, false);
        updateStatusBar();
    }
    emuThread->unlockStatus();
}

auto StatusHandler::updateVolume( bool state ) -> void {
    unsigned volume = 100;

    if (activeEmulator) {
        auto settings = program->getSettings(activeEmulator);
        volume = settings->get<unsigned>("audio_volume", 100u, {0u, 100u});
    }

    emuThread->lock();
    if (state)
        statusBar->updateSlider(18, volume / 5);
    audioManager->setVolume();
    updateVisible(18, showVolume = state);
    updateStatusBar();
    emuThread->unlock();
}

auto StatusHandler::updateAudioRecord( bool state ) -> void {
    emuThread->lockStatus();
    updateVisible(14, state);
    updateStatusBar();
    emuThread->unlockStatus();
}

auto StatusHandler::updateTapeImage( GUIKIT::Image* image ) -> void {
        
    if (!statusBar)
        return;
    
    if (image != &(view->stopStatusImage)) {

        for(auto& deviceState : deviceStates) {
            if (deviceState.media->group->isTape()) {
                // check if tape is paused
                setDeviceUpdate();
                deviceState.update = true;
                //if (deviceState.motorOff)
                //   image = &(view->playPauseStatusImage);
                
                return;
            }
        }
    }

    emuThread->lockStatus();

    updateImage( 10, image );
    updateStatusBar();

    emuThread->unlockStatus();
}

auto StatusHandler::setExpansionClick() -> void {
    auto expansion = activeEmulator->getExpansion();

    if (program->hasSuperCpuActive()) {
        emuThread->lock();
        bool state = activeEmulator->getExpansionJumper(expansion->mediaGroup->selected, 0);
        activeEmulator->setExpansionJumper(expansion->mediaGroup->selected, 0, !state);
        setMessage( trans->get( state ? "SuperCPU 1Mhz" : "SuperCPU turbo"), false, true);

        auto& jumper = expansion->jumpers[0];
        auto emuView = EmuConfigView::TabWindow::getView( activeEmulator );
        if (emuView && emuView->mediaLayout) {
            auto block = emuView->mediaLayout->getBlock(expansion->mediaGroup->selected);
            if (block)
                block->selector.jumpers[0]->setChecked(!state);
        }
        std::string saveIdent = _underscore( expansion->mediaGroup->selected->name + "_jumper_" + jumper.name );
        program->getSettings(activeEmulator)->set<bool>( saveIdent, !state);
        emuThread->unlock();
    }
}

auto StatusHandler::hideTape() -> void {
	updateVisible(9, false);
	updateVisible(10, false);
    updateStatusBar();
}

auto StatusHandler::setVolumeSlider(Emulator::Interface* emulator) -> void {
    auto settings = program->getSettings(emulator);
    auto volume = settings->get<unsigned>("audio_volume", 100, {0, 100});

    setVolumeSlider(volume);
}

auto StatusHandler::setVolumeSlider(unsigned value) -> void {
    if (showVolume)
        statusBar->updateSlider(18, value / 5);
}

auto StatusHandler::enableLEDs() -> void {
    for (auto& ledState : ledStates) {
        ledState.state = false;
        ledState.inputQueue = 0;
        ledState.inputs = 0;
        enableLED(ledState);
    }
}

auto StatusHandler::toggleLED(Emulator::Interface::LedId ledId) -> void {
    auto settings = program->getSettings(activeEmulator);

    for (auto& ledState : ledStates) {
        if (ledState.ledId == ledId) {
            bool state = settings->get<bool>(ledState.name, false);
            state ^= 1;
            settings->set<bool>(ledState.name, state);
            enableLED(ledState);
            break;
        }
    }
}

auto StatusHandler::enableLED(LEDState& ledState) -> void {
    GUIKIT::Settings* settings = nullptr;
    if (activeEmulator)
        settings = program->getSettings(activeEmulator);

    bool enabled = settings ? settings->get<bool>(ledState.name, false) : false;

    if (enabled && (ledState.ledId == Emulator::Interface::LedId::MHz2) && program->hasSuperCpuActive())
        enabled = false;

    ledState.enabled = enabled;

    emuThread->lockStatus();
    unsigned sId = getStatusId(ledState.ledId);
    updateVisible(sId - 1, enabled);
    if (enabled)
        updateImage(sId, getLEDImage(ledState));
    else
        updateVisible(sId, false);

    updateStatusBar();
    emuThread->unlockStatus();
}

auto StatusHandler::getLEDImage(LEDState& ledState) -> GUIKIT::Image* {
    switch (ledState.ledId) {
        case Emulator::Interface::LedId::Power:
            switch (ledState.state & 3) {
                default:
                case 0: return &(view->ledOffImage);
                case 1: return &(view->ledRed2Image);
                case 2: return &(view->ledGreen2DimImage);
                case 3: return &(view->ledGreen2Image);
            }
        case Emulator::Interface::LedId::CapsLock:
            switch (ledState.state & 3) {
                default:
                case 0: return &(view->ledOffRoundImage);
                case 1: return &(view->ledRedRoundImage);
                case 2: return &(view->ledOffRoundImage);
                case 3: return &(view->ledGreenRoundImage);
            }
        case Emulator::Interface::LedId::MHz2:
            switch (ledState.state & 3) {
                default:
                case 0: return &(view->ledOffImage);
                case 1: return &(view->ledGreen2Image);
                case 2: return &(view->ledOffImage);
                case 3: return &(view->ledGreen2Image);
            }
        default:
            break;
    }
    return nullptr;

    // switch (ledState.state & 3) {
    //     default:
    //     case 0: return ledState.ledId == Emulator::Interface::LedId::Power ? &(view->ledOffImage) : &(view->ledOffRoundImage);
    //     case 1: return ledState.ledId == Emulator::Interface::LedId::Power ? &(view->ledRed2Image) : &(view->ledRedRoundImage);
    //     case 2: return ledState.ledId == Emulator::Interface::LedId::Power ? &(view->ledGreen2DimImage) : &(view->ledOffRoundImage);
    //     case 3: return ledState.ledId == Emulator::Interface::LedId::Power ? &(view->ledGreen2Image) : &(view->ledGreenRoundImage);
    // }
}

auto StatusHandler::init(GUIKIT::StatusBar* statusBar) -> void {
    statusBar->clear();
    updateOnScreenFPS();
    
    this->statusBar = statusBar;
    showFPS = globalSettings->get<bool>("fps", false);
    showVolume = globalSettings->get<bool>("volume_control", true );
    fpsCounter.decimalPoints = 3;

    ledStates.push_back({ Emulator::Interface::LedId::Power, "power_led", false, 0, 0, 0 });
    ledStates.push_back({ Emulator::Interface::LedId::CapsLock, "caps_led", false, 0, 0, 0 });
    ledStates.push_back({ Emulator::Interface::LedId::MHz2, "2_mhz", false, 0, 0, 0 });

	control = 0;

    statusBar->append( 0, "1000.999", nullptr, &(view->speedControlMenu ) );    // FPS
    statusBar->append( 16, "Power", nullptr, &(view->power.menu ) );
    statusBar->append( 17, &(view->ledGreenImage), nullptr, &(view->power.menu ) ); // Power LED
    
    statusBar->append( 1, "8 00.0", nullptr, &(view->diskControlMenus[0].menu) ); // disk drive track
    statusBar->append( 2, &(view->ledOffImage), nullptr, &(view->diskControlMenus[0].menu) );    // disk LED
    statusBar->append( 3, "9 00.0", nullptr, &(view->diskControlMenus[1].menu) ); // disk drive track
    statusBar->append( 4, &(view->ledOffImage), nullptr, &(view->diskControlMenus[1].menu) );    // disk LED
    statusBar->append( 5, "10 00.0", nullptr, &(view->diskControlMenus[2].menu) ); // disk drive track
    statusBar->append( 6, &(view->ledOffImage), nullptr, &(view->diskControlMenus[2].menu) );    // disk LED
    statusBar->append( 7, "11 00.0", nullptr, &(view->diskControlMenus[3].menu) ); // disk drive track
    statusBar->append( 8, &(view->ledOffImage), nullptr, &(view->diskControlMenus[3].menu) );    // disk LED

    statusBar->append( 19, "DH0 00000");
    statusBar->append( 20, &(view->ledOffImage));
    statusBar->append( 21, "DH1 00000");
    statusBar->append( 22, &(view->ledOffImage));
    statusBar->append( 23, "DH2 00000");
    statusBar->append( 24, &(view->ledOffImage));
    statusBar->append( 25, "DH3 00000");
    statusBar->append( 26, &(view->ledOffImage));

    statusBar->append( 27, "Caps" );
    statusBar->append( 28, &(view->ledOffImage));

    statusBar->append( 29, "2 MHz", [this]() {
        emuThread->lock();
        program->toggle2Mhz();
        emuThread->unlock();
    } );
    statusBar->append( 30, &(view->ledOffImage), [this]() {
        emuThread->lock();
        program->toggle2Mhz();
        emuThread->unlock();
    });
    
    statusBar->append( 9, "000", nullptr, &(view->tapeControlMenu) );    // tape counter
    statusBar->append( 10, &(view->stopStatusImage), nullptr, &(view->tapeControlMenu) );    // tape button icon
	statusBar->append( 11, "CRT", [this]() {
	    if (activeEmulator)
	        this->setExpansionClick();
    } );
    statusBar->append( 12, &(view->ledOffImage), [this]() {
        if (activeEmulator)
            this->setExpansionClick();
    } );    // expansion LED
    statusBar->append(14, &(view->recordStatusImage), []() { program->toggleRecord(); });    // REC Status
    statusBar->append( 18, 21, 60, [](unsigned position) {
        if (!activeEmulator)
            return;
        auto settings = program->getSettings(activeEmulator);
        settings->set<unsigned>("audio_volume", position * 5);

        emuThread->lock();
        auto emuView = EmuConfigView::TabWindow::getView(activeEmulator);
        if (emuView && emuView->audioLayout)
            emuView->audioLayout->updateVolumeSlider();

        audioManager->setVolume();
        emuThread->unlock();
    } );
    statusBar->updateSlider(18, 100 / 5);
    statusBar->updateVisible(18, showVolume);

    statusBar->append( 15, "" );    // status text
    statusBar->updateVisible(15, true);

    statusBar->updateSeparator( 0, true );
    statusBar->updateSeparator( 2, true );
    statusBar->updateSeparator( 4, true );
    statusBar->updateSeparator( 6, true );
    statusBar->updateSeparator( 8, true );
    statusBar->updateSeparator( 10, true );
    statusBar->updateSeparator( 12, true );
    statusBar->updateSeparator( 14, true );
    statusBar->updateSeparator( 17, true );
    statusBar->updateSeparator( 18, true );
    statusBar->updateSeparator( 20, true);
    statusBar->updateSeparator( 22, true);
    statusBar->updateSeparator( 24, true);
    statusBar->updateSeparator( 26, true);
    statusBar->updateSeparator( 28, true);
    statusBar->updateSeparator( 30, true);
}

auto StatusHandler::reset() -> void {
    resetFrameCounter();
    setFpsRefresh();
    setVolumeSlider(activeEmulator);
    enableLEDs();
    setMessageLevel();
}

auto StatusHandler::setMessageLevel() -> void {
    auto settings = program->getSettings(activeEmulator);
    showOnlyUrgentMessages = settings->get<bool>("only_urgent_messages", false);
}

auto StatusHandler::updateDiskDriveSpace() -> void {
    if (dynamic_cast<LIBAMI::Interface*>(activeEmulator)) {
        statusBar->updateDimension( 1, "DF0 00:0" );
        statusBar->updateDimension( 3, "DF1 00:0" );
        statusBar->updateDimension( 5, "DF2 00:0" );
        statusBar->updateDimension( 7, "DF3 00:0" );
    } else {
        statusBar->updateDimension( 1, "8 00.0" );
        statusBar->updateDimension( 3, "9 00.0" );
        statusBar->updateDimension( 5, "10 00.0" );
        statusBar->updateDimension( 7, "11 00.0" );
    }
}

auto StatusHandler::update() -> void {
    uint16_t clearMask = ~0;
    auto& drcS = audioManager->statistics;

    emuThread->lockStatus();

    if (activeEmulator) {
        if (deviceUpdate()) {
            for(auto& deviceState : deviceStates) {

                if (!deviceState.update)
                    continue;

                if (deviceState.inputsPerFrame)
                    deviceState.inputsPerFrame--;

                if (deviceState.inputsPerFrame)
                    clearMask &= ~2;
                else
                    deviceState.update = false;

                auto media = deviceState.media;
                auto group = media->group;

                if (group->isDisk()) {
                    std::string name = media->name;
                    auto chunks = GUIKIT::String::split( media->name, ' ' );
                    if (chunks.size() > 1)
                        name = chunks.back();

                    unsigned track = (deviceState.position >> 1) & 0xff;
                    name += " " + GUIKIT::String::prependZero( std::to_string(track), 2 );

                    if (deviceState.position & 0x8000)
                        name += (deviceState.position & 1) ? ".5" : ".0";
                    else
                        name += (deviceState.position & 1) ? ":1" : ":0";

                    updateText(media->id * 2 + 1, name);

                    GUIKIT::Image* image = &(view->ledOffImage);
                    uint8_t _led = (deviceState.LED >> (deviceState.inputsPerFrame << 1) ) & 3;

                    if (_led) {
                        if (deviceState.write) {
                            if (dynamic_cast<LIBC64::Interface*>(activeEmulator))
                                image = (_led & 1) ? &(view->ledRedImage) : &(view->ledYellowImage);
                            else
                                image = &(view->ledRedImage);
                        } else {
                            if (dynamic_cast<LIBAMI::Interface*>(activeEmulator))
                                image = (_led & 1) ? &(view->ledYellowImage) : &(view->ledGreen2Image);
                            else
                                image = (_led & 1) ? &(view->ledGreenImage) : &(view->ledRed2Image);
                        }
                    }

                    if (activeVideoManager->driveLedParam)
                        activeVideoManager->driveLedParam->value = (_led & 3) ? 1 : 0;

                    updateImage(media->id * 2 + 2, image);

                } else if (group->isHardDisk()) {
                    std::string name = media->name + " ";
                    unsigned cylinder = deviceState.position & 0xffff;
                    name += std::to_string(cylinder);

                    updateText(media->id * 2 + 19, name);

                    GUIKIT::Image* image = &(view->ledOffImage);
                    uint8_t _led = (deviceState.LED >> (deviceState.inputsPerFrame << 1)) & 3;
                    if (_led) {
                        if (deviceState.write)
                            image = &(view->ledRedImage);
                        else
                            image = (_led & 1) ? &(view->ledYellowImage) : &(view->ledGreen2Image);
                    }

                    if (activeVideoManager->driveLedParam)
                        activeVideoManager->driveLedParam->value = (_led & 3) ? 1 : 0;

                    updateImage(media->id * 2 + 20, image);
                            
                } else if (group->isTape()) {

                    std::string name = GUIKIT::String::prependZero( std::to_string( deviceState.position ), 3 );

                    updateText(9, name);

                    // we don't use the tape mode of emulation core, because it doesn't match the "tape button press" state
                    // in all cases, e.g. when tape is forwarded until end, mode changes to "stop" but play button keeps in pressed state.
                    if ( view->tapePlayItem.icon() == &view->playhiImage )
                        updateImage( 10, deviceState.motorOff ? &(view->playPauseStatusImage) : &(view->playStatusImage) );
                    
                    else if ( view->tapeForwardItem.icon() == &view->forwardhiImage )
                        updateImage( 10, deviceState.motorOff ? &(view->forwardPauseStatusImage) : &(view->forwardStatusImage) );

                    else if ( view->tapeRewindItem.icon() == &view->rewindhiImage )
                        updateImage( 10, deviceState.motorOff ? &(view->rewindPauseStatusImage) : &(view->rewindStatusImage) );
                    
                    else if ( view->tapeRecordItem.icon() == &view->recordhiImage )
                        updateImage( 10, deviceState.motorOff ? &(view->recordPauseStatusImage) : &(view->recordStatusImage) );
                    
                } else if (group->isExpansion()) {

                    GUIKIT::Image* image = &(view->ledOffImage);
                    
                    if ((deviceState.LED >> (deviceState.inputsPerFrame << 1) ) & 3)
                        image = &(view->ledGreenImage); 

					updateVisible(11, true);
                    updateImage(12, image);
                }                
            }
        }

        if (ledUpdate()) {
            for(auto& ledState : ledStates) {
                if (!ledState.inputs)
                    continue;

                ledState.inputs--;

                if (ledState.inputs)
                    clearMask &= ~4;

                ledState.state = (ledState.inputQueue >> (ledState.inputs << 1)) & 3;

                if (ledState.enabled)
                    updateImage(getStatusId(ledState.ledId), getLEDImage(ledState));
            }
        }

        std::string _FPS;
        if (fpsCounterUpdate() && (showFPS || showFPSScreen) ) {
            _FPS = fpsCounter.decimalPoints
                    ? GUIKIT::String::formatFloatingPoint(fpsCounter.fps, fpsCounter.decimalPoints)
                    : std::to_string((unsigned)round(fpsCounter.fps));

            if (showFPS)
                updateText(0, _FPS);
        }

        if (messageUpdate()) {
            videoDriver->showScreenText( message.txt, message.duration, message.warn );

        } else if (drcS.enable) {
            if (drcBufferUpdate()) {
                std::string OSDText = "DRC: ";
                OSDText += GUIKIT::String::formatFloatingPoint(drcS.current, 2) + "% ";
                OSDText += "[ " + GUIKIT::String::formatFloatingPoint(drcS.min, 2) + " : " + GUIKIT::String::formatFloatingPoint(drcS.max, 2) + " ]";
                OSDText += " Ø " + GUIKIT::String::formatFloatingPoint(drcS.average, 2) + "%";
                videoDriver->showScreenText( OSDText, 0 );
            }
        } else if (showFPSScreen) {
            if (fpsCounterUpdate())
                videoDriver->showScreenText( _FPS, 0 );
        }
    }

    updateStatusBar();
    
    clearUpdates( clearMask );

    emuThread->unlockStatus();
}

inline auto StatusHandler::getStatusId(Emulator::Interface::LedId ledId) -> unsigned {
    switch (ledId) {
        case Emulator::Interface::LedId::Power:     return 17;
        case Emulator::Interface::LedId::CapsLock:  return 28;
        case Emulator::Interface::LedId::MHz2:      return 30;
        default:
            return 0;
    }
}

auto StatusHandler::updateVisible(unsigned id, bool visible) -> void {
    if (emuThread->enabled)
        emuThread->addStatusUpdate(id, (int)visible);
    else
        statusBar->updateVisible(id, visible);
}

auto StatusHandler::updateText(unsigned id, std::string text, bool alignRight, int overrideForegroundColor) -> void {
    if (emuThread->enabled)
        emuThread->addStatusUpdate(id, -1, nullptr, text, alignRight, overrideForegroundColor);
    else
        statusBar->updateText(id, text, alignRight, overrideForegroundColor);
}

auto StatusHandler::updateImage(unsigned id, GUIKIT::Image* image) -> void {
    if (emuThread->enabled)
        emuThread->addStatusUpdate(id, -1, image);
    else
        statusBar->updateImage(id, image);
}

auto StatusHandler::updateStatusBar() -> void {
    if (!emuThread->enabled)
        statusBar->update();
}
