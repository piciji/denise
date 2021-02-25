
#include "status.h"
#include "view.h"
#include "../audio/manager.h"
#include "../cmd/cmd.h"

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
    
    message.txt = txt;
    message.duration = duration;
    message.critical = critical;
    
    setMessageUpdate();
}

auto StatusHandler::clear() -> void {    
    statusBar->hideContent();
    statusBar->updateText(0, "");
    statusBar->update();
    message.clear();
    clearUpdates();
    videoDriver->showMessage( "" );
    fps = fpsCollect = 0;
}

auto StatusHandler::countFrames() -> void {
    fpsCollect++;
    time( &curr_t );

    if (curr_t != prev_t) {
        fps = fpsCollect;
        fpsCollect = 0;
        
		if (!cmd->noGui)
			setFpsCounterUpdate();

        if (!VideoManager::synchronized)
            // check input polling and message loop every 50 ms
            program->loopFrames = (fps * 50 ) / 1000;
        else
            // check input polling every frame
            program->loopFrames = 0;
    }
    prev_t = curr_t;
}

auto StatusHandler::updateFPS( bool state ) -> void {
    
    showFPS = state;
    if (!showFPS) {
        statusBar->updateVisible(15, false);
        statusBar->update();
    }
}

auto StatusHandler::updateDRC( bool state ) -> void {
    
    if (!state) {
        statusBar->updateVisible(13, false);
        statusBar->update();
    }
}

auto StatusHandler::updateAudioRecord( bool state ) -> void {
    
    recordAudio = state;
    statusBar->updateVisible(14, recordAudio);
    statusBar->update();
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
    
    statusBar->updateImage( 10, image );        
}

auto StatusHandler::hideTape() -> void {
	statusBar->updateVisible(9, false);
	statusBar->updateVisible(10, false);
	statusBar->update();
}

auto StatusHandler::init(GUIKIT::StatusBar* statusBar) -> void {
    statusBar->clear();
    
    this->statusBar = statusBar;
    showFPS = globalSettings->get<bool>("fps", false);
    recordAudio = false;    
    fps = fpsCollect = 0; 
	control = 0;

    statusBar->append( 0, "" );    // status text
	statusBar->updateVisible(0, true);
	    
    // up to 4 disk drives
    statusBar->append( 1, "8 00.0", nullptr, &(view->diskControlMenus[0].menu) ); // disk drive track
    statusBar->append( 2, &(view->ledOffImage), nullptr, &(view->diskControlMenus[0].menu) );    // disk LED
    statusBar->append( 3, "9 00.0", nullptr, &(view->diskControlMenus[1].menu) ); // disk drive track
    statusBar->append( 4, &(view->ledOffImage), nullptr, &(view->diskControlMenus[1].menu) );    // disk LED
    statusBar->append( 5, "10 00.0", nullptr, &(view->diskControlMenus[2].menu) ); // disk drive track
    statusBar->append( 6, &(view->ledOffImage), nullptr, &(view->diskControlMenus[2].menu) );    // disk LED
    statusBar->append( 7, "11 00.0", nullptr, &(view->diskControlMenus[3].menu) ); // disk drive track
    statusBar->append( 8, &(view->ledOffImage), nullptr, &(view->diskControlMenus[3].menu) );    // disk LED
    
    statusBar->append( 9, "000" );    // tape counter
    statusBar->append( 10, &(view->stopStatusImage), nullptr, &(view->tapeControlMenu) );    // tape button icon
	statusBar->append( 11, "CRT" );    // expansion label
    statusBar->append( 12, &(view->ledOffImage) );    // expansion LED
    statusBar->append( 13, "DRC DRC DRC DRC DRC DRC DRC DRC D" );    // DRC Status
    statusBar->append( 14, &(view->recordStatusImage) );    // REC Status
    statusBar->append( 15, "1000" );    // FPS

    statusBar->updateSeparator( 0, true );
    statusBar->updateSeparator( 2, true );
    statusBar->updateSeparator( 4, true );
    statusBar->updateSeparator( 6, true );
    statusBar->updateSeparator( 8, true );
    statusBar->updateSeparator( 10, true );
    statusBar->updateSeparator( 12, true );
    statusBar->updateSeparator( 13, true );
    statusBar->updateSeparator( 14, true );
    statusBar->updateSeparator( 15, true );
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

    view->renderPlaceholder();   
}

auto StatusHandler::update() -> void {

    uint16_t clearMask = ~0;
    
    if (fpsCounterUpdate()) {
        if (message.duration) {
            if (--message.duration == 0) {
                message.clear();
                setMessageUpdate();
            }
        }
    }
    
    std::string OSDText = message.txt;
    
    if (messageUpdate())        
        statusBar->updateText(0, message.txt, true, message.critical ? 0xe92828 : -1 );

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

                    if (dynamic_cast<LIBC64::Interface*> (activeEmulator)) {
                        name += " " + GUIKIT::String::prependZero( std::to_string((unsigned)(deviceState.position / 2)), 2 );

                        name += (deviceState.position & 1) ? ".5" : ".0";
                    } else
                        name += GUIKIT::String::prependZero( std::to_string( deviceState.position ), 2 );                

                    statusBar->updateText(media->id * 2 + 1, name);

                    GUIKIT::Image* image = &(view->ledOffImage);
                    if (deviceState.LED & 1)
                        image = deviceState.write ? &(view->ledRedImage) : &(view->ledGreenImage);

                    statusBar->updateImage(media->id * 2 + 2, image);

                } else if (group->isTape()) {

                    std::string name = GUIKIT::String::prependZero( std::to_string( deviceState.position ), 3 );

                    statusBar->updateText(9, name);

                    // we don't use the tape mode of emulation core, because it doesn't match the "tape button press" state
                    // in all cases, e.g. when tape is forwarded until end, mode changes to "stop" but play button keeps in pressed state.
                    if ( view->tapePlayItem.icon() == &view->playhiImage )
                        statusBar->updateImage( 10, deviceState.motorOff ? &(view->playPauseStatusImage) : &(view->playStatusImage) );
                    
                    else if ( view->tapeForwardItem.icon() == &view->forwardhiImage )
                        statusBar->updateImage( 10, deviceState.motorOff ? &(view->forwardPauseStatusImage) : &(view->forwardStatusImage) );

                    else if ( view->tapeRewindItem.icon() == &view->rewindhiImage )
                        statusBar->updateImage( 10, deviceState.motorOff ? &(view->rewindPauseStatusImage) : &(view->rewindStatusImage) );
                    
                    else if ( view->tapeRecordItem.icon() == &view->recordhiImage )
                        statusBar->updateImage( 10, deviceState.motorOff ? &(view->recordPauseStatusImage) : &(view->recordStatusImage) );
                    
                } else if (group->isExpansion()) {

                    GUIKIT::Image* image = &(view->ledOffImage);

                    if (deviceState.inputsPerFrame) {
                        deviceState.inputsPerFrame--;
                        clearMask &= ~2;
                        deviceState.update = true;
                    }
                    
                    if ((deviceState.LED >> deviceState.inputsPerFrame) & 1)
                        image = &(view->ledGreenImage); 

					statusBar->updateVisible(11, true);
                    statusBar->updateImage(12, image);
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
                statusBar->updateText(13, out, true);

            if (message.txt.empty())
                OSDText += out;
        }
        
        if (recordAudio && message.txt.empty())
            OSDText += " REC ";
        
        if (showFPS) {
            std::string _FPS = std::to_string(fps);

            if (fpsCounterUpdate())
                statusBar->updateText(15, _FPS, true);

            if (message.txt.empty())
                OSDText += " " + _FPS;            
        }
    }    
        
    statusBar->update();    
    
    clearUpdates( clearMask );
    
	if (!cmd->noDriver)
		transferToOSD( OSDText );
}
