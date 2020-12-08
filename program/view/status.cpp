
#include "status.h"
#include "view.h"
#include "../audio/manager.h"

StatusHandler* statusHandler = nullptr;

auto StatusHandler::updateDeviceState( Emulator::Interface::Media* media, bool write, unsigned position, bool LED, bool motorOff ) -> void {

    if (!media)
        return;

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
        statusBar->updateVisible(14, false);
        statusBar->update();
    }
}

auto StatusHandler::updateDRC( bool state ) -> void {
    
    if (!state) {
        statusBar->updateVisible(12, false);
        statusBar->update();
    }
}

auto StatusHandler::updateAudioRecord( bool state ) -> void {
    
    recordAudio = state;
    statusBar->updateVisible(13, recordAudio);
    statusBar->update();
}

auto StatusHandler::updateTapeImage( GUIKIT::Image* image ) -> void {
        
    if (!statusBar)
        return;
    
    if (image == &(view->playhiImage)) {
        for(auto& deviceState : deviceStates) {
            if (deviceState.media->group->isTape()) {
                if (deviceState.motorOff)
                    image = &(view->playhiPauseImage);
                
                break;
            }
        }
    }
    
    statusBar->updateImage( 10, image );        
}

auto StatusHandler::init(GUIKIT::StatusBar* statusBar) -> void {
    statusBar->clear();
    
    this->statusBar = statusBar;
    showFPS = globalSettings->get<bool>("fps", false);
    recordAudio = false;    
    fps = fpsCollect = 0;
    
    GUIKIT::StatusBar::Part part;
    
    part.id = 0;
    part.width = 0;
    part.text = "";
    part.image = nullptr;
    part.onClick = nullptr;
    part.popupMenu = nullptr;
    part.foregroundColor = 0;
    part.overrideForegroundColor = false;
    part.visible = false;        
    
    statusBar->appendPart( part );    // status text
    
    // up to 4 disk drives
    for( unsigned i = 0; i < 4; i++ ) {
        part.id = i * 2 + 1;
        part.width = (i > 1) ? 42 : 38;
        part.image = nullptr;
        statusBar->appendPart( part );    // disk drive track
        part.id = i * 2 + 2;
        part.width = 22;
        part.image = &(view->ledOffImage);
        statusBar->appendPart( part );    // disk   LED
    }
    
    part.id = 9;
    part.width = 26;
    part.image = nullptr;
    statusBar->appendPart( part );    // tape counter
    part.id = 10;
    part.width = 22;
    part.image = &(view->stopImage);
    part.popupMenu = &(view->tapeControlMenu);
    statusBar->appendPart( part );    // tape button icon
    part.id = 11;
    part.width = 22;
    part.image = &(view->ledOffImage);
    part.popupMenu = nullptr;
    statusBar->appendPart( part );    // expansion LED
    part.id = 12;
    part.width = 220;
    part.image = nullptr;
    statusBar->appendPart( part );    // DRC Status
    part.id = 13;
    part.width = 22;
    part.image = &(view->recordhiImage);
    statusBar->appendPart( part );    // REC Status
    part.id = 14;
    part.width = 40;
    part.image = nullptr;
    statusBar->appendPart( part );    // FPS
}

auto StatusHandler::transferToOSD( std::string text ) -> void {
    auto option = globalSettings->get("video_screen_text", 0, {0u, 2u});

    if (option == 0) {
        videoDriver->showMessage("");

    } else if (option == 1) {
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
        statusBar->updateText(0, message.txt, message.critical, 0xe92828 );        

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

                    auto mode = activeEmulator->getTapeControl( media );

                    typedef Emulator::Interface::TapeMode TapeMode;

                    if (mode == TapeMode::Play)
                        statusBar->updateImage( 10, deviceState.motorOff ? &(view->playhiPauseImage) : &(view->playhiImage) );
                    
                } else if (group->isExpansion()) {

                    GUIKIT::Image* image = &(view->ledOffImage);

                    if (deviceState.inputsPerFrame) {
                        deviceState.inputsPerFrame--;
                        clearMask &= ~2;
                        deviceState.update = true;
                    }
                    
                    if ((deviceState.LED >> deviceState.inputsPerFrame) & 1)
                        image = &(view->ledGreenImage); 

                    statusBar->updateImage(11, image);
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
                statusBar->updateText(12, out);

            if (message.txt.empty())
                OSDText += out;
        }
        
        if (recordAudio && message.txt.empty())
            OSDText += " REC ";
        
        if (showFPS) {
            std::string _FPS = std::to_string(fps);

            if (fpsCounterUpdate())
                statusBar->updateText(14, _FPS);

            if (message.txt.empty())
                OSDText += " " + _FPS;            
        }
    }    
        
    statusBar->update();    
    
    clearUpdates( clearMask );
    
    transferToOSD( OSDText );
}
