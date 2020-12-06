
#pragma once

#include "../program.h"

struct DeviceState {    
    Emulator::Interface::Media* media = nullptr;
    bool write;
    unsigned position;
    uint8_t LED;    
    bool motorOff;
    uint8_t inputsPerFrame;
    bool update = true;
};

struct StatusHandler {
    
    auto messageUpdate() -> bool { return control & 1; }
    auto deviceUpdate() -> bool { return control & 2; }         
    auto drcBufferUpdate() -> bool { return control & 8; }
    auto fpsCounterUpdate() -> bool { return control & 0x10; }   
    
    auto setMessageUpdate() -> void { control |= 1; }
    auto setDeviceUpdate() -> void { control |= 2; }
    auto setDrcBufferUpdate() -> void  { control |= 8; }
    auto setFpsCounterUpdate() -> void  { control |= 0x10; }

    auto clearUpdates( uint16_t mask = ~0 ) -> void { control &= ~mask; }
    auto hasUpdates() -> bool { return !!control; }

    auto init(GUIKIT::StatusBar* statusBar) -> void;
    auto clear() -> void;
    auto update() -> void;
    auto updateDeviceState(Emulator::Interface::Media* media, bool write, unsigned position, bool LED, bool motorOff) -> void;
    auto setMessage(std::string txt, unsigned duration = 3, bool critical = false) -> void;
    auto transferToOSD( std::string text ) -> void;
    auto updateFPS( bool state ) -> void;
    auto updateDRC( bool state ) -> void;
    auto updateAudioRecord( bool state ) -> void;
    auto updateTapeImage( GUIKIT::Image* image ) -> void;

    GUIKIT::StatusBar* statusBar = nullptr;
    uint16_t control;
    std::vector<DeviceState> deviceStates;

    bool showFPS = false;
    bool recordAudio = false;

    struct {
        std::string txt = "";
        unsigned duration = 0;
        bool critical = false;
        
        auto clear() -> void {
            txt = "";
            duration = 0;
            critical = false;
        }
    } message;
};

extern StatusHandler* statusHandler;