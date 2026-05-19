
#pragma once

#include <cstdint>

#include "debuggerSnapshot.h"

namespace Emulator {
    struct Serializer;
}

namespace LIBC64 {

struct System;
struct SidManager;
struct USBSIDPico;

struct Sid {
    enum Type { MOS_6581 = 0, MOS_8580 = 1 } type;

    unsigned nr;
    System* system;
    SidManager& sidManager;
    USBSIDPico& usbSIDPico;

    bool leftChannel = true;
    bool rightChannel = true;
    uint8_t ioPos;
    uint16_t ioMask;

    double sampleRate;

    int curve6581 = 5000;
    int curve8580 = 5000;
    int range6581 = 5000;
    uint8_t waveStrength = 0;

    bool digiBoost = false;
    bool useFilter = false;

    DebuggerSnapshot::SID snapshot;

    Sid(unsigned nr, System* system, SidManager& sidManager, USBSIDPico& usbSIDPico, Type type) :
    system( system ),
    sidManager( sidManager ),
    usbSIDPico( usbSIDPico ) {
        this->type = type;
        this->nr = nr;
    }

    virtual ~Sid() = default;

    virtual auto reset() -> void = 0;

    virtual auto setType( Type type ) -> void = 0;

    virtual auto getType() -> Type { return type; }

    virtual auto setDigiBoost( bool state ) -> void = 0;

    virtual auto hasDigiBoost() -> bool { return digiBoost; }

    virtual auto readIO( uint8_t addr ) -> uint8_t = 0;

    virtual auto peekIO( uint8_t addr ) -> uint8_t = 0;

    virtual auto writeIO( uint8_t addr, uint8_t value ) -> void = 0;

    virtual auto clock(int cycles, int sampleCounter, int sampleLimit, bool audioOut) -> int = 0;

    virtual auto clock() -> void = 0;

    virtual auto clockSilent() -> void = 0;

    virtual auto getSample() -> float = 0;

    virtual auto serialize(Emulator::Serializer& s, bool light) -> void = 0;

    virtual auto useLeftChannel(bool state) -> void { leftChannel = state; }

    virtual auto useRightChannel(bool state) -> void { rightChannel = state; }

    virtual auto enableFilter( bool state ) -> void = 0;

    virtual auto filterEnabled() -> bool { return useFilter; }

    virtual auto adjustFilterCurve6581(int value) -> void = 0;

    virtual auto getFilterCurve6581() -> int { return curve6581; }

    virtual auto adjustFilterCurve8580(int value) -> void = 0;

    virtual auto getFilterCurve8580() -> int  { return curve8580; }

    virtual auto adjustFilterRange6581(int value) -> void = 0;

    virtual auto getFilterRange6581() -> int { return range6581;  }

    virtual auto setWaveformStrength(uint8_t value) -> void = 0;

    virtual auto getWaveformStrength() -> uint8_t { return waveStrength; }

    virtual auto updateSnapshot(DebuggerSnapshot& snap) -> void = 0;

    virtual auto setSampleRate(double sampleRate) -> void = 0;

    virtual auto clone(Sid* src, bool keepProps) -> void = 0;
};

}