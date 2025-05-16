
#pragma once

#include "../program.h"

#define FPS_MEASUREMENTS 300

struct DeviceState {    
    Emulator::Interface::Media* media = nullptr;
    bool write;
    unsigned position;
    uint16_t LED;
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
    unsigned updateIntervall = 1000;
    unsigned decimalPoints = 3;
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
    auto updateDeviceState(Emulator::Interface::Media* media, bool write, unsigned position, uint8_t LED, bool motorOff) -> void;
    auto setMessage(const std::string& txt, unsigned duration = 3, bool warn = false) -> void;
    auto updateFPS( bool state ) -> void;
    auto updateVolume( bool state ) -> void;
    auto updateAudioRecord( bool state ) -> void;
    auto updateTapeImage( GUIKIT::Image* image ) -> void;
	auto hideTape() -> void;
    auto updateFrameCounter() -> void;
    auto resetFrameCounter() -> void;
    auto setFpsRefresh() -> void;

    auto updateVisible(unsigned id, bool visible) -> void;
    auto updateText(unsigned id, std::string text, bool alignRight = false, int overrideForegroundColor = -1) -> void;
    auto updateImage(unsigned id, GUIKIT::Image* image) -> void;
    auto updateStatusBar() -> void;
    auto updateDiskDriveSpace() -> void;
    auto initPowerLED() -> void;
    auto updatePowerLED(bool state) -> void;
    auto updatePowerLED() -> void;
    auto setPowerLED() -> void;
    auto hidePowerLED() -> void;
    auto togglePowerLED() -> void;
    auto hasPowerLED() -> bool;
    auto setVolumeSlider(unsigned value) -> void;
    auto setVolumeSlider(Emulator::Interface* emulator) -> void;
    auto updateOnScreenFPS() -> void;
    auto setExpansionClick() -> void;

    GUIKIT::StatusBar* statusBar = nullptr;
    uint16_t control;
    std::vector<DeviceState> deviceStates;
    FpsCounter fpsCounter;

    bool showFPS = false;
    bool showFPSScreen = false;
    bool showVolume = false;

    struct {
        bool enable;
        bool state;
        GUIKIT::Timer timer;
    } powerLED;

    struct {
        std::string txt = "";
        unsigned duration = 0;
        bool warn = false;
        
        auto clear() -> void {
            txt = "";
            duration = 0;
            warn = false;
        }
    } message;
};

extern StatusHandler* statusHandler;