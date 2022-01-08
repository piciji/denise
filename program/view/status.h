
#pragma once

#include "../program.h"

#define FPS_MEASUREMENTS 300

struct DeviceState {    
    Emulator::Interface::Media* media = nullptr;
    bool write;
    unsigned position;
    uint8_t LED;    
    bool motorOff;
    uint8_t inputsPerFrame;
    bool update = true;
};

struct FpsCounter {
    uint64_t deltas[FPS_MEASUREMENTS];
    uint64_t sum;
    uint32_t pos;
    uint64_t last;
    uint32_t measures;
    float fps;
    unsigned updateDelay;
};

struct StatusHandler {
    
    auto messageUpdate() const -> bool { return control & 1; }
    auto deviceUpdate() const -> bool { return control & 2; }
    auto drcBufferUpdate() const -> bool { return control & 8; }
    auto fpsCounterUpdate() const -> bool { return control & 0x10; }
    
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
	auto hideTape() -> void;
    auto updateFrameCounter() -> void;
    auto resetFrameCounter() -> void;

    auto updateVisible(unsigned id, bool visible) -> void;
    auto updateText(unsigned id, std::string text, bool alignRight = false, int overrideForegroundColor = -1) -> void;
    auto updateImage(unsigned id, GUIKIT::Image* image) -> void;
    auto updateStatusBar() -> void;

    GUIKIT::StatusBar* statusBar = nullptr;
    uint16_t control;
    std::vector<DeviceState> deviceStates;
    FpsCounter fpsCounter;

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