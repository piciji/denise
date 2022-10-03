
#pragma once

#include <cstdint>
#include <functional>

namespace Emulator {
    struct Serializer;
}

namespace LIBAMI {

struct Agnus;
struct Blitter;

struct Copper {

    Copper(Agnus& agnus);

    // 0x80 allocate Copper if BUS is free
    // 0x40 allocate Copper if BUS is free and no long gap
    enum State {
        Off                         = 0,
        Strobe0                     = 1 | 0x80,
        Strobe0Self                 = 2 | 0x80,
        Strobe1                     = 3,
        Strobe2                     = 4 | 0x40,
        Strobe3                     = 5 | 0x80,
        Strobe2Vsync                = 6 | 0x80,
        Strobe3Vsync                = 7 | 0x80,
        Strobe4Vsync                = 8 | 0x80,
        Strobe5Vsync                = 9 | 0x80,
        Strobe1Unaligned            = 10,
        Strobe2Unaligned            = 11,
        Strobe3Unaligned            = 12 | 0x80,
        Read1                       = 13 | 0x80,
        Read1AfterSkip              = 14 | 0x80,
        Read2                       = 15 | 0x80,
        Skip1                       = 16,
        Skip2                       = 17,
        Wait1                       = 18,
        Wait2                       = 19,
        Wait3                       = 20,
        Wait4                       = 21
    } state, prevState;

    Agnus& agnus;
    Blitter& blitter;

    uint16_t cdang;
    bool useCop1;
    uint32_t cop1lc;
    uint32_t cop2lc;
    uint32_t copPtr;

    uint16_t ir1;
    uint16_t ir2;

    struct {
        uint8_t hPos;
        uint16_t vPos;
        uint8_t hMask;
        uint16_t vMask;
    } comp;

    bool skipped;

    auto setCopCon( uint16_t value ) -> void;

    auto process() -> void;
    template<bool wait = true> auto compare() -> bool;

    auto setCOP1LCH(uint16_t value) -> void;
    auto setCOP1LCL(uint16_t value) -> void;
    auto setCOP2LCH(uint16_t value) -> void;
    auto setCOP2LCL(uint16_t value) -> void;

    auto strobeCOPJMP(bool firstLocation, uint8_t triggeredBy ) -> void;

    auto blitterBusyUpdate() -> void;
    auto allocationCycle() -> bool;
    auto reset() -> void;
    auto serialize(Emulator::Serializer& s) -> void;

};

}
