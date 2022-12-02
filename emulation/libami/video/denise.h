
#pragma once

#include <cstdint>

namespace Emulator {
    struct Serializer;
}

namespace LIBAMI {

struct Agnus;
struct Input;

// A1000 + OCS Denise, todo: ECS Denise
struct Denise {
    Denise(Agnus& agnus, Input& input);
    ~Denise();

    enum Model : uint8_t { OCS_A1000_NO_EHB = 1, OCS_A1000 = 2, OCS = 4 } model = OCS;
    Agnus& agnus;
    Input& input;

    uint16_t hPos; // 9 bit counter
    uint16_t colors[64];
    uint8_t ready;
    bool hires;
    bool ham;
    bool doublePlayfield;
    uint8_t useInterlace;
    uint8_t activePlanes;
    uint16_t hamColor;

    uint16_t bpl1dat;
    uint16_t bpl2dat;
    uint16_t bpl3dat;
    uint16_t bpl4dat;
    uint16_t bpl5dat;
    uint16_t bpl6dat;

    uint64_t shifterA; // playfield 1: planes 1, 3, 5
    uint64_t shifterB; // playfield 2: planes 2, 4, 6

    uint64_t shifterAClxEna;
    uint64_t shifterBClxEna;

    uint64_t shifterAClxPolarity;
    uint64_t shifterBClxPolarity;

    uint16_t clxDat;

    uint8_t delayPf1;
    uint8_t delayPf2;
    bool enableDisplay;

    uint16_t* frameBuffer;
    uint16_t* linePtr;
    unsigned linePos;
    uint16_t lineVCounter;
    bool enableSequencer;

    struct Sprite {
        uint16_t datA;
        uint16_t datB;
        uint32_t shift;
        uint16_t x;
        bool armed;
        bool attached;
    } sprites[8];

    struct {
        unsigned left;
        unsigned right;
    } crop;

    struct {
        bool use;
        unsigned line;
    } lineCallback;

    uint8_t sprClxMask;
    bool pf2PrioOverPf1;
    uint8_t pf1Prio;
    uint8_t pf2Prio;
    bool pf1PrioIllegal;
    bool pf2PrioIllegal;

    uint16_t hStart;
    uint16_t hStop;

    uint8_t endFrame;
    bool vBlank;
    bool hBlank;
    uint8_t hiresFrame;

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

    auto setDiwStrt(uint16_t value) -> void;
    auto setDiwStop(uint16_t value) -> void;
    auto setClxCon(uint16_t value) -> void;
    auto getClxDat() -> uint16_t;
    auto setColor(uint8_t pos, uint16_t value ) -> void;
    auto setBplCon0(uint16_t value ) -> void;
    auto setBplCon1(uint16_t value ) -> void;
    auto setBplCon2(uint16_t value ) -> void;
    auto setSprDatA(uint8_t nr, uint16_t value ) -> void;
    auto setSprDatB(uint8_t nr, uint16_t value ) -> void;
    auto setSprCtl(uint8_t nr, uint16_t value ) -> void;
    auto setSprPos(uint8_t nr, uint16_t value ) -> void;
    auto setBpl1Dat(uint16_t value) -> void;
    auto setBpl2Dat(uint16_t value) -> void { bpl2dat = value; }
    auto setBpl3Dat(uint16_t value) -> void { bpl3dat = value; }
    auto setBpl4Dat(uint16_t value) -> void { bpl4dat = value; }
    auto setBpl5Dat(uint16_t value) -> void { bpl5dat = value; }
    auto setBpl6Dat(uint16_t value) -> void { bpl6dat = value; }

    auto processDelay() -> void;
    template<bool useHires> auto processPixel() -> void;
    auto startHblank() -> void;
    auto endHblank() -> void;
    auto switchToHiresMidframe() -> void;
    inline auto doubleLoresPixel(uint16_t* _ptr, unsigned _xStart) -> void;
    auto updateCropLeft() -> void;
    auto updateCropRight() -> void;
};

}
