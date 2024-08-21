
#include "status.h"
#include "view.h"
#include "../audio/manager.h"
#include "../cmd/cmd.h"
#include "../tools/chronos.h"
#include "../thread/emuThread.h"
#include "../emuconfig/config.h"

StatusHandler* statusHandler = nullptr;

auto StatusHandler::updateDeviceState( Emulator::Interface::Media* media, bool write, unsigned position, bool LED, bool motorOff ) -> void {

    setDeviceUpdate();
    
    for (auto& deviceState : deviceStates) {
        if (deviceState.media == media) {
            deviceState.write = write;
            deviceState.position = position;
            deviceState.LED <<= 1;
            deviceState.LED |= LED;
            deviceState.inputsPerFrame++;
            deviceState.motorOff = motorOff;
            deviceState.update = true;
            return;
        }
    }

    deviceStates.push_back({media, write, position, LED, motorOff, 1, true});
}

auto StatusHandler::setMessage(std::string txt, unsigned duration, bool critical ) -> void {

    if (txt == "")
        duration = 0;
    else {
        if (duration == 0)
            duration = 1;

        duration *= (unsigned)audioManager->inputFPS;
    }

    message.txt = txt;
    message.duration = duration;
    message.critical = critical;
    
    setMessageUpdate();
}

auto StatusHandler::clear() -> void {    
    statusBar->hideContent();
    statusBar->updateText(15, "");
    statusBar->updateVisible(18, showVolume);
    statusBar->update();
    message.clear();
    clearUpdates();
    statusHandler->setMessage("");
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
    }
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

    if (message.duration) {
        if (--message.duration == 0) {
            message.clear();
            setMessageUpdate();
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
        statusBar->updateSlider(18, volume / 20);
    audioManager->setVolume();
    updateVisible(18, showVolume = state);
    updateStatusBar();
    emuThread->unlock();
}

auto StatusHandler::updateDRC( bool state ) -> void {
    emuThread->lockStatus();
    if (!state) {
        clearUpdates( 8 );
        updateVisible(13, false);
        updateStatusBar();
    }
    emuThread->unlockStatus();
}

auto StatusHandler::updateAudioRecord( bool state ) -> void {
    emuThread->lockStatus();
    recordAudio = state;
    updateVisible(14, recordAudio);
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

auto StatusHandler::updatePowerLED(bool state) -> void {
    powerLED.state = state;
    if (!hasPowerLED() || powerLED.timer.enabled())
        return;

    setPowerLED();
}

auto StatusHandler::setPowerLED() -> void {
    GUIKIT::Image* image;
    int model = activeEmulator->getModelValue( LIBAMI::Interface::ModelId::ModelIdSystem );

    if (powerLED.state)
        image = (model > 1) ? &(view->ledGreen2Image) : &(view->ledRed2Image);
    else
        image = (model > 1) ? &(view->ledGreen2DimImage) : &(view->ledOffImage);

    emuThread->lockStatus();
    updateVisible(16, true);
    updateImage( 17, image );
    updateStatusBar();
    emuThread->unlockStatus();
}

auto StatusHandler::hidePowerLED() -> void {
    emuThread->lockStatus();
    updateVisible(16, false);
    updateVisible( 17, false );
    updateStatusBar();
    emuThread->unlockStatus();
}

auto StatusHandler::hasPowerLED() -> bool {
    return powerLED.enable && activeEmulator && dynamic_cast<LIBAMI::Interface*>(activeEmulator);
}

auto StatusHandler::togglePowerLED() -> void {
    powerLED.enable ^= 1;
    globalSettings->set<bool>("power_led", powerLED.enable);
    hasPowerLED() ? setPowerLED() : hidePowerLED();
    if (powerLED.timer.enabled())
        powerLED.timer.setEnabled(false);
}

auto StatusHandler::initPowerLED() -> void {
    powerLED.state = true;
    powerLED.timer.setEnabled( powerLED.enable && activeEmulator && dynamic_cast<LIBAMI::Interface*>(activeEmulator) );
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

auto StatusHandler::init(GUIKIT::StatusBar* statusBar) -> void {
    statusBar->clear();
    
    this->statusBar = statusBar;
    showFPS = globalSettings->get<bool>("fps", false);
    powerLED.enable = globalSettings->get<bool>("power_led", true);
    showVolume = globalSettings->get<bool>("volume_control", true );
    recordAudio = false;
    fpsCounter.decimalPoints = 3;
	control = 0;

    statusBar->append( 0, "1000.999", nullptr, &(view->speedControlMenu ) );    // FPS
    statusBar->append( 16, "Power", nullptr, &(view->power.menu ) );
    statusBar->append( 17, &(view->ledGreenImage), nullptr, &(view->power.menu ) ); // Power LED
    // up to 4 disk drives
    statusBar->append( 1, "8 00.0", nullptr, &(view->diskControlMenus[0].menu) ); // disk drive track
    statusBar->append( 2, &(view->ledOffImage), nullptr, &(view->diskControlMenus[0].menu) );    // disk LED
    statusBar->append( 3, "9 00.0", nullptr, &(view->diskControlMenus[1].menu) ); // disk drive track
    statusBar->append( 4, &(view->ledOffImage), nullptr, &(view->diskControlMenus[1].menu) );    // disk LED
    statusBar->append( 5, "10 00.0", nullptr, &(view->diskControlMenus[2].menu) ); // disk drive track
    statusBar->append( 6, &(view->ledOffImage), nullptr, &(view->diskControlMenus[2].menu) );    // disk LED
    statusBar->append( 7, "11 00.0", nullptr, &(view->diskControlMenus[3].menu) ); // disk drive track
    statusBar->append( 8, &(view->ledOffImage), nullptr, &(view->diskControlMenus[3].menu) );    // disk LED
    
    statusBar->append( 9, "000", nullptr, &(view->tapeControlMenu) );    // tape counter
    statusBar->append( 10, &(view->stopStatusImage), nullptr, &(view->tapeControlMenu) );    // tape button icon
	statusBar->append( 11, "CRT" );    // expansion label
    statusBar->append( 12, &(view->ledOffImage) );    // expansion LED
    statusBar->append( 13, "DRC DRC DRC DRC DRC DRC DRC DRC D" );    // DRC Status
    statusBar->append( 14, &(view->recordStatusImage) );    // REC Status
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
    statusBar->updateSeparator( 13, true );
    statusBar->updateSeparator( 14, true );
    statusBar->updateSeparator( 17, true );
    statusBar->updateSeparator( 18, true );

    powerLED.timer.setInterval(1000);
    powerLED.timer.onFinished = [this]() {
        powerLED.timer.setEnabled(false);
        if (hasPowerLED())
            setPowerLED();
    };
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
            
auto StatusHandler::transferToOSD( std::string text ) -> void {
	static auto option = globalSettings->getOrInit("video_screen_text", 0, {0u, 2u});

    if (*option == 0) {
        videoDriver->showMessage("");

    } else if (*option == 1) {
        if (!view->statusVisible())
            videoDriver->showMessage( text, message.critical );
        else
            videoDriver->showMessage("");        

    } else
        videoDriver->showMessage( text, message.critical );
}

auto StatusHandler::update() -> void {
    uint16_t clearMask = ~0;

    emuThread->lockStatus();
    
    std::string OSDText = message.txt;
    
    if (messageUpdate())        
        updateText(15, message.txt, false, message.critical ? 0xe92828 : -1 );

    if (activeEmulator) {
        if (deviceUpdate()) {
            for(auto& deviceState : deviceStates) {

                if (!deviceState.update)
                    continue;
                
                deviceState.update = false;

                auto media = deviceState.media;
                auto group = media->group;

                if (group->isDisk()) {
                    std::string name = media->name;
                    auto chunks = GUIKIT::String::split( media->name, ' ' );
                    if (chunks.size() > 1)
                        name = chunks.back();

                    name += " " + GUIKIT::String::prependZero( std::to_string((unsigned)(deviceState.position / 2)), 2 );

                    if (dynamic_cast<LIBC64::Interface*> (activeEmulator))
                        name += (deviceState.position & 1) ? ".5" : ".0";
                    else
                        name += (deviceState.position & 1) ? ":1" : ":0";

                    updateText(media->id * 2 + 1, name);

                    GUIKIT::Image* image = &(view->ledOffImage);
                    if (deviceState.LED & 1) {
                        if (deviceState.write)
                            image = &(view->ledRedImage);
                        else {
                            int _col = 0;
                            if (dynamic_cast<LIBAMI::Interface*>(activeEmulator)) {
                                _col = activeEmulator->getModelValue(LIBAMI::Interface::ModelId::ModelIdSystem);
                                image = (_col > 1) ? &(view->ledYellowImage) : &(view->ledGreen2Image);
                            } else
                                image = &(view->ledGreenImage);
                        }
                    }
                    if (activeVideoManager->driveLedParam) {
                        activeVideoManager->driveLedParam->value = deviceState.LED & 1;
                    }

                    updateImage(media->id * 2 + 2, image);

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

                    if (deviceState.inputsPerFrame) {
                        deviceState.inputsPerFrame--;
                        clearMask &= ~2;
                        deviceState.update = true;
                    }
                    
                    if ((deviceState.LED >> deviceState.inputsPerFrame) & 1)
                        image = &(view->ledGreenImage); 

					updateVisible(11, true);
                    updateImage(12, image);
                }                
            }
        }                        

        auto& drcS = audioManager->statistics;

        if (drcS.enable) {
            std::string out = "DRC: ";
            out += GUIKIT::String::formatFloatingPoint(drcS.current, 2) + "% ";
            out += "[ " + GUIKIT::String::formatFloatingPoint(drcS.min, 2) + " : " + GUIKIT::String::formatFloatingPoint(drcS.max, 2) + " ]";
            out += " Ø " + GUIKIT::String::formatFloatingPoint(drcS.average, 2) + "%";

            if (drcBufferUpdate())
                updateText(13, out, true);

            if (message.txt.empty())
                OSDText += out;
        }
        
        if (recordAudio && message.txt.empty())
            OSDText += " REC ";
        
        if (showFPS) {
            unsigned decimalPoints = fpsCounter.decimalPoints;
            std::string _FPS = decimalPoints
                    ? GUIKIT::String::formatFloatingPoint(fpsCounter.fps, decimalPoints)
                    : std::to_string((unsigned)round(fpsCounter.fps));

            if (fpsCounterUpdate())
                updateText(0, _FPS);

            if (message.txt.empty())
                OSDText += " " + _FPS;            
        }
    }

    updateStatusBar();
    
    clearUpdates( clearMask );

    emuThread->unlockStatus();

    if (!cmd->noDriver)
		transferToOSD( OSDText );
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
