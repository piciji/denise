
#pragma once

#include <cstdint>

namespace Emulator {
    struct Serializer;
}

namespace LIBC64 {

struct System;
struct SidManager;
struct DebuggerSnapshot;

struct Sid {
    enum Type { MOS_6581 = 0, MOS_8580 = 1 } type;

    unsigned nr;
    System* system;
    SidManager& sidManager;

    bool leftChannel = true;
    bool rightChannel = true;
    uint8_t ioPos;
    uint16_t ioMask;

    double sampleRate;

    Sid(unsigned nr, System* system, SidManager& sidManager, Type type)
    : system( system ), sidManager( sidManager ) {
        this->type = type;
        this->nr = nr;
    }

    virtual ~Sid() = default;

    virtual auto reset() -> void = 0;

    virtual auto setType( Type type ) -> void = 0;

    virtual auto setDigiBoost( bool state ) -> void = 0;

    virtual auto hasDigiBoost() -> bool = 0;

    virtual auto readIO( uint8_t addr ) -> uint8_t = 0;

    virtual auto peekIO( uint8_t addr ) -> uint8_t = 0;

    virtual auto writeIO( uint8_t addr, uint8_t value ) -> void = 0;

    virtual auto clock(int cycles, int sampleCounter, int sampleLimit, bool audioOut) -> int = 0;

    virtual auto clock() -> void = 0;

    virtual auto clockSilent() -> void = 0;

    virtual auto getSample() -> float = 0;

    virtual auto serialize(Emulator::Serializer& s, bool light) -> void = 0;

    virtual auto setIoMask(uint8_t pos) -> void = 0;

    virtual auto useLeftChannel(bool state) -> void = 0;

    virtual auto useRightChannel(bool state) -> void = 0;

    virtual auto enableFilter( bool state ) -> void = 0;

    virtual auto filterEnabled() -> bool = 0;

    virtual auto adjustFilterBias6581(int value) -> void = 0;

    virtual auto getFilterBias6581() -> int = 0;

    virtual auto adjustFilterBias8580(int value) -> void = 0;

    virtual auto getFilterBias8580() -> int = 0;

    virtual auto updateSnapshot(DebuggerSnapshot& snap) -> void = 0;

    virtual auto setSampleRate(double sampleRate) -> void = 0;

    virtual auto clone(Sid* src, bool keepProps) -> void = 0;
};

}