
#pragma once

#include <cstdint>

namespace Emulator {
    struct Serializer;
}

namespace LIBAMI {

struct System;
struct Agnus;
struct Input;

// A1000 + OCS Denise, todo: ECS Denise
struct Denise {
    Denise(System* system, Agnus& agnus, Input& input);
    ~Denise();

    enum Model : uint8_t { OCS_A1000_NO_EHB = 1, OCS_A1000 = 2, OCS = 4 } model = OCS;
    enum { PF1_SHIFT = 1, PF2_SHIFT = 2, INIT_HPOS = 4, RESET_HPOS = 8, BPL1_WRITTEN = 0x10 };

    System* system;
    Agnus& agnus;
    Input& input;

    uint16_t hPos; // 9 bit counter
    uint16_t colors[64];
    bool hires;
    bool ham;
    bool doublePlayfield;
    uint8_t activePlanes;
    uint16_t hamColor;

    uint16_t bpl1dat;
    uint16_t bpl2dat;
    uint16_t bpl3dat;
    uint16_t bpl4dat;
    uint16_t bpl5dat;
    uint16_t bpl6dat;

    uint16_t dat1;
    uint16_t dat2;
    uint16_t dat3;
    uint16_t dat4;
    uint16_t dat5;
    uint16_t dat6;

    int64_t shifterA; // playfield 1: planes 1, 3, 5
    int64_t shifterB; // playfield 2: planes 2, 4, 6

    int64_t shifterAClxEna;
    int64_t shifterBClxEna;

    int64_t shifterAClxPolarity;
    int64_t shifterBClxPolarity;

    uint16_t clxDat;

    int actions; // Bits 0: PF1 shift, 1: PF2 shift, 2: init hpos reset, 3: reset hpos, 4: bpl1dat written
    uint8_t delayPf1;
    uint8_t delayPf2;
    uint8_t bplCon1;
    bool enableDisplay;
    bool borderFlipFlop;

    uint16_t* linePtr;
    int linePos;
    bool enableSequencer = true;

    struct Sprite {
        uint16_t datA;
        uint16_t datB;
        uint32_t shift;
        uint16_t x;
        bool armed;
        bool attached;
    } sprites[8];

    uint8_t sprClxMask;
    bool pf2PrioOverPf1;
    uint8_t pf1Prio;
    uint8_t pf2Prio;
    bool pf1PrioIllegal;
    bool pf2PrioIllegal;

    uint16_t hStart;
    uint16_t hStop;

    bool hBlank;
    bool hiresFrame;

    auto strhor() -> void;
    auto strequ() -> void;
    auto strvbl() -> void;
    auto process() -> void;
    auto power() -> void;
    auto disableSequencer(bool state) -> void { enableSequencer = !state; }
    inline auto useSequencer() -> bool { return enableSequencer; }
    auto serialize(Emulator::Serializer& s) -> void;

    auto joy0Dat() -> uint16_t;
    auto joy1Dat() -> uint16_t;
    auto joyTest(uint16_t data) -> void;

    auto setDiwStrt(uint16_t value) -> void;
    auto setDiwStop(uint16_t value) -> void;
    auto setClxCon(uint64_t value) -> void;
    auto getClxDat() -> uint16_t;
    auto setColor(uint8_t pos, uint16_t value ) -> void;
    auto setBplCon0(uint16_t value ) -> void;
    auto setBplCon1(uint16_t value ) -> void;
    auto setBplCon2(uint16_t value ) -> void;
    auto setSprDatA(uint8_t nr, uint16_t value, bool force = true ) -> bool;
    auto setSprDatB(uint8_t nr, uint16_t value, bool force = true ) -> bool;
    auto setSprCtl(uint8_t nr, uint16_t value ) -> void;
    auto setSprPos(uint8_t nr, uint16_t value ) -> void;
    auto setBpl1Dat(uint16_t value) -> void;
    auto setBpl2Dat(uint16_t value) -> void { bpl2dat = value; }
    auto setBpl3Dat(uint16_t value) -> void { bpl3dat = value; }
    auto setBpl4Dat(uint16_t value) -> void { bpl4dat = value; }
    auto setBpl5Dat(uint16_t value) -> void { bpl5dat = value; }
    auto setBpl6Dat(uint16_t value) -> void { bpl6dat = value; }

    template<bool useHires> auto processDelayPf1() -> void;
    template<bool useHires> auto processDelayPf2() -> void;
    template<bool useHires> auto processPixel() -> void;
    auto updateBplDelay() -> void;
};

}
