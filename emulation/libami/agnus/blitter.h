
#pragma once

#include <cstdint>
#include <functional>
#include <vector>

namespace Emulator {
    struct Serializer;
}

namespace LIBAMI {

struct Agnus;
struct Copper;
struct Paula;

struct Blitter {

    enum { BLT_NONE = 0, BLT_FETCH_A = 1, BLT_FETCH_B = 2, BLT_FETCH_C = 4, BLT_WRITE_D = 8,
            BLT_SHIFT_A = 0x10, BLT_SHIFT_B = 0x20, BLT_CALC = 0x40, BLT_FILL = 0x80,
            BLT_NEXT = 0x100, BLT_DESC = 0x200,
            BLT_BH = 0x400, BLT_LINE_X = 0x800, BLT_LINE_Y = 0x1000,
            BLT_B_MOD = 0x2000, BLT_UPDATE_SIGN = 0x4000, BLT_IDLE = 0x8000 };

    Blitter(Agnus& agnus);

    Agnus& agnus;
    Copper& copper;
    Paula& paula;

    uint16_t fill[1024];
    uint8_t channels[1024];

    uint16_t bltcon0;
    uint16_t bltcon1;

    uint16_t bltAdat;
    uint16_t bltBdat;
    uint16_t bltCdat;
    uint16_t bltDdat;

    uint16_t bltADatOld;
    uint16_t bltBDatOld;
    uint16_t bltADatShifted;
    uint16_t bltBDatShifted;

    uint16_t bltAfwm;
    uint16_t bltAlwm;

    uint32_t bltApt;
    uint32_t bltBpt;
    uint32_t bltCpt;
    uint32_t bltDpt;

    int16_t bltAmod;
    int16_t bltBmod;
    int16_t bltCmod;
    int16_t bltDmod;

    uint16_t bltSizeW;
    uint16_t bltSizeH;

    uint16_t curW;
    uint16_t curH;

    bool fillCarry;

    bool busy;
    bool zero;

    bool desc;

    uint16_t flags;

    uint8_t restartTimer;

    bool writeLineDot;
    bool oneDotPerLine;

    bool skipB;
    bool skipY;
    bool curSkipB;
    bool curSkipY;
    int shifter;
    bool shiftOut;

    bool doff;

    auto doMinterm(uint8_t operation, uint16_t a, uint16_t b, uint16_t c) const -> uint16_t;

    auto stateMachine() -> void;
    auto prepareChannel() -> void;
    auto process() -> void;
    auto reset() -> void;
    auto initBlit() -> void;
    auto startBlit() -> void;
    auto finish() -> void;
    template<uint16_t jobs, uint8_t nextCycle = 0> auto blockMode() -> void;
    template<uint16_t jobs, uint8_t nextCycle = 0> auto lineMode() -> void;

    auto preFill() -> void;

    bool isFillMode() { return bltcon1 & 0x18; }
    bool isInclusiveFill() { return (bltcon1 & 0x18) == 0x8; } // inclusive and exclusive fill together use exclusive fill
    bool isExclusiveFill() { return bltcon1 & 0x10; }
    bool isFci() { return bltcon1 & 4; }

    auto hasFillmodeIdle() -> bool;
    auto hasSkipB() -> bool;
    auto hasSkipY() -> bool;

    auto activateLLEWhenNeeded(uint8_t bltRegister, uint16_t value) -> void;
    auto serialize(Emulator::Serializer& s) -> void;

    auto setBltCon0(uint16_t value) -> void;
    auto setBltCon0L(uint16_t value) -> void;
    auto setBltCon1(uint16_t value) -> void;
    auto setBltAfwm(uint16_t value) -> void;
    auto setBltAlwm(uint16_t value) -> void;
    auto setBltCptH(uint16_t value) -> void;
    auto setBltCptL(uint16_t value) -> void;
    auto setBltBptH(uint16_t value) -> void;
    auto setBltBptL(uint16_t value) -> void;
    auto setBltAptH(uint16_t value) -> void;
    auto setBltAptL(uint16_t value) -> void;
    auto setBltDptH(uint16_t value) -> void;
    auto setBltDptL(uint16_t value) -> void;

    auto setBltSize(uint16_t value) -> void;
    auto setBltSizeV(uint16_t value) -> void;
    auto setBltSizeH(uint16_t value) -> void;
    auto setBltCMod(uint16_t value) -> void;
    auto setBltBMod(uint16_t value) -> void;
    auto setBltAMod(uint16_t value) -> void;
    auto setBltDMod(uint16_t value) -> void;

    auto setBltCDat(uint16_t value) -> void;
    auto setBltBDat(uint16_t value) -> void;
    auto setBltADat(uint16_t value) -> void;
};

}
