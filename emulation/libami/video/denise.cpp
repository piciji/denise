
#include "denise.h"
#include "../agnus/agnus.h"
#include "../system/system.h"
#include "../interface.h"

namespace LIBAMI {

uint16_t Denise::ehbCols[4096];

Denise::Denise(System* system, Agnus& agnus, Input& input) : system(system), agnus(agnus), input(input) {
    static bool initialized = false;

    if (!initialized) {
        for(int c = 0; c < 4096; c++) {
            uint8_t r = (c >> 8) & 0xf;
            uint8_t g = (c >> 4) & 0xf;
            uint8_t b = (c >> 0) & 0xf;
            ehbCols[c] = ((r >> 1) << 8) | ((g >> 1) << 4) | (b >> 1);
        }

        initialized = true;
    }
}

Denise::~Denise() {}

auto Denise::joy0Dat() -> uint16_t {
    return input.readDenisePortA();
}

auto Denise::joy1Dat() -> uint16_t {
    return input.readDenisePortB();
}

auto Denise::joyTest(uint16_t data) -> void {
    input.writeDeniseJoytest(data);
}

auto Denise::power(bool softReset) -> void {
    deniseClock = 0;
    for (int i = 0; i < 256; i++)
        bplUpdate[i].actions = 0;

    if (!softReset) {
        hPos = 2;
        std::fill_n(colors, 64, 0);
        hires = false;
        activePlanes = 0;

        shifterA = 0;
        shifterB = 0;
        bpl1dat = bpl2dat = bpl3dat = bpl4dat = bpl5dat = bpl6dat = 0;
        dat1 = dat2 = dat3 = dat4 = dat5 = dat6 = 0;

        shifterAClxEna = 0;
        shifterBClxEna = 0;

        shifterAClxPolarity = 0;
        shifterBClxPolarity = 0;

        clxDat = 0;
        pfShift = 0;
        delayPf1 = 0;
        delayPf2 = 0;
        bplCon0 = 0;
        bplCon1 = 0;
        enableDisplay = false;

        linePos = 0;

        for (unsigned i = 0; i < 8; i++) {
            sprites[i].datA = 0;
            sprites[i].datB = 0;
            sprites[i].shift = 0;
            sprites[i].x = 0;
            sprites[i].armed = false;
            sprites[i].attached = false;
        }

        sprClxMask = 0;
        pf2PrioOverPf1 = false;
        pf2Prio = 0;
        pf1Prio = 0;
        hamColor = 0;
        hStart = 0;
        hStop = 0;
        pf1PrioIllegal = false;
        pf2PrioIllegal = false;
        hBlank = true;
        vBlank = true;
        borderFlipFlop = true;
        hiresFrame = false;
    }
}

// Denise listen for addresses, put on RGA BUS
auto Denise::setBpl1Dat(uint16_t value) -> void {
    bpl1dat = value;
    if (disableSequencer != 1) {
        int cycle = agnus.fallBackCycles(deniseClock);
        BplUpdate& upd = bplUpdate[cycle & 0xff];
        upd.dat1 = bpl1dat;
        upd.dat2 = bpl2dat;
        upd.dat3 = bpl3dat;
        upd.dat4 = bpl4dat;
        upd.dat5 = bpl5dat;
        upd.dat6 = bpl6dat;
        upd.actions |= BPL1_WRITTEN;
    }
}

auto Denise::strequ() -> void {
    process(-1);
    hBlank = true;
    vBlank = true;
    enableDisplay = false;
}

auto Denise::strhor() -> void {
    if (disableSequencer != 1) {
        if (vBlank) {
            process(-1);
            vBlank = false;
        }
        int cycle = agnus.fallBackCycles(deniseClock);
        BplUpdate& upd = bplUpdate[cycle & 0xff];
        upd.actions |= RESET_HPOS;
    }
}

auto Denise::strvbl() -> void {
    // Denise has no vcounter
    if (disableSequencer != 1) {
        int cycle = agnus.fallBackCycles(deniseClock);
        BplUpdate& upd = bplUpdate[cycle & 0xff];
        upd.actions |= RESET_HPOS;
    }
}

auto Denise::setDiwStrt(uint16_t value) -> void {
    process(-1);
    hStart = value & 0xff;
}

auto Denise::setDiwStop(uint16_t value) -> void {
    process(-1);
    hStop = (value & 0xff) | 0x100;
}

auto Denise::setColor( uint8_t pos, uint16_t value ) -> void {
    process();
//    int cycle = agnus.fallBackCycles(deniseClock);
//    BplUpdate& upd = bplUpdate[cycle - 0];
//    upd.actions |= UPD_COLOR;
//    upd.dat1 = value;
//    upd.dat2 = pos;
//    return;

    colors[pos] = value;

    if (model == OCS_A1000_NO_EHB)
        colors[ 32 + pos ] = value;
    else {
        //extra half brite
//        uint8_t r = (value >> 8) & 0xf;
//        uint8_t g = (value >> 4) & 0xf;
//        uint8_t b = (value >> 0) & 0xf;
//        colors[32 + pos] = ((r >> 1) << 8) | ((g >> 1) << 4) | (b >> 1);
        colors[32 + pos] = ehbCols[value & 0xfff];
    }
}

auto Denise::setClxCon(uint64_t value) -> void {
    process();
    shifterAClxEna = ((value & 0x40) << 41) | ((value & 0x100) << 23) | ((value & 0x400) << 5);
    shifterBClxEna = ((value & 0x80) << 40) | ((value & 0x200) << 22) | ((value & 0x800) << 4);

    shifterAClxPolarity = ((value & 1) << 47) | ((value & 4) << 29) | ((value & 16) << 11);
    shifterBClxPolarity = ((value & 2) << 46) | ((value & 8) << 28) | ((value & 32) << 10);

    sprClxMask = 0x55; // even sprites are always allowed
    for (unsigned i = 0; i < 4; i++ ) {
        if ( (value >> 12) & (1 << i))
            sprClxMask |= 1 << ( (i << 1) + 1 );
    }
}

auto Denise::setSprDatA( uint8_t nr, uint16_t value, bool force ) -> bool {
    process(force ? -1 : 0);
    Sprite& spr = sprites[nr];

    if (!force) {
        if (spr.armed && ((spr.x & 1) == 0) && (hPos == spr.x))
            return false;
    }

    spr.datA = value;
    spr.armed = true;
    return true;
}

auto Denise::setSprDatB( uint8_t nr, uint16_t value, bool force ) -> bool {
    process(force ? -1 : 0);
    Sprite& spr = sprites[nr];

    if (!force) {
        if (spr.armed && ((spr.x & 1) == 0) && (hPos == spr.x))
            return false;
    }

    spr.datB = value;
    return true;
}

auto Denise::setSprCtl( uint8_t nr, uint16_t value ) -> void {
    process(-1);
    Sprite& spr = sprites[nr];
    spr.x &= ~1;
    spr.x |= value & 1;
    spr.attached = (nr & 1) && (value & 0x80);
    spr.armed = false;
}

auto Denise::setSprPos( uint8_t nr, uint16_t value ) -> void {
    process(-1);
    Sprite& spr = sprites[nr];
    spr.x &= 1;
    spr.x |= (value & 0xff) << 1;
}

auto Denise::getClxDat() -> uint16_t {
    process();
    uint16_t out = clxDat | 0x8000;
    clxDat = 0;
    return out;
}

auto Denise::setBplCon0( uint16_t value ) -> void {
    process(-1);
    hires = value & 0x8000;
    if (hires) {
        if (!hiresFrame)
            agnus.switchToHiresMidframe();

        hiresFrame = true;
    }

    activePlanes = (value >> 12) & 7;
    bplCon0 = value;

    updateBplDelay();
}

auto Denise::setBplCon1( uint16_t value ) -> void {
    process(-1);
    bplCon1 = value;

    updateBplDelay();
}

auto Denise::updateBplDelay() -> void {
    delayPf1 = bplCon1 & 0xf;
    delayPf2 = (bplCon1 >> 4) & 0xf;
    if (hires) {
        delayPf1 &= 7;
        delayPf2 &= 7;
    }
}

auto Denise::setBplCon2( uint16_t value ) -> void {
    process(-1);
    pf1Prio = value & 7;
    pf2Prio = (value >> 3) & 7;
    pf2PrioOverPf1 = (value >> 6) & 1;
    pf1PrioIllegal = pf1Prio > 4;
    pf2PrioIllegal = pf2Prio > 4;
}


auto Denise::process(int offset) -> void {
    // offset -1: inited by event system, which is already incremented a cycle
    int cycles = ((int)(agnus.clock - deniseClock) + offset) & 0xff;
    deniseClock = agnus.clock + offset;

    if (disableSequencer) {
        if (disableSequencer & 1)
            return;

        switch(bplCon0 & (0x8000 | 0x400)) { // hires, dpf  ... HAM is not relevant
            case 0: process<false, false, false, false>(cycles, activePlanes); break;     // lores
            case 0x400: process<false, false, true, false>(cycles, activePlanes); break; // dpf

            case 0x8000: process<true, false, false, false>(cycles, activePlanes); break;  // hires
            case 0x8000 | 0x400: process<true, false, true, false>(cycles, activePlanes); break; // hires dpf
        }

    } else {
        switch(bplCon0 & (0x8000 | 0x800 | 0x400)) { // hires, ham, dpf
            case 0: process<false, false, false>(cycles, activePlanes); break;     // lores
            case 0x400: process<false, false, true>(cycles, activePlanes); break; // dpf

            case 0x8000: process<true, false, false>(cycles, activePlanes); break;  // hires
            case 0x800: process<false, true, false>(cycles, activePlanes); break; // ham

            case 0x8000 | 0x400: process<true, false, true>(cycles, activePlanes); break; // hires dpf
            case 0x8000 | 0x800: process<true, true, false>(cycles, activePlanes); break; // hires ham

            case 0x800 | 0x400: process<false, true, true>(cycles, activePlanes); break;
            case 0x8000 | 0x800 | 0x400: process<true, true, true>(cycles, activePlanes); break;
        }
    }

    BplUpdate& upd = bplUpdate[cycles];
    if (upd.actions) {
        // actions are scheduled a cycle later, so it could happen that synchronization misses this
        bplUpdate[0] = upd;
        upd.actions = 0; // disable for this position
    }
}

#define SPF(v1) \
    if (!clxNoCol)                                          clxDat |= (v1 << 5) | (v1 << 1) | 1; \
    else if ((clxNoCol & 2) == 0)                           clxDat |= (v1 << 5); \
    else if (((clxNoCol & 1) == 0) && _doublePlayfield)     clxDat |= (v1 << 1);

#define SPF2(v1, v2) \
    if (!clxNoCol)                                          clxDat |= (v1 << 5) | (v1 << 1) | (v2 << 9) | 1; \
    else if ((clxNoCol & 2) == 0)                           clxDat |= (v1 << 5) | (v2 << 9); \
    else if (((clxNoCol & 1) == 0) && _doublePlayfield)     clxDat |= (v1 << 1) | (v2 << 9); \
    else                                                    clxDat |= (v2 << 9);

template<bool _hires, bool _ham, bool _doublePlayfield, bool _display> inline auto Denise::process(const int cycles, const int _activePlanes) -> void {
    uint8_t sprGroup;
    uint8_t sprPrio;
    uint16_t sprData;
    uint8_t colIndex;
    uint8_t colIndex2;
    uint16_t color;
    int clxNoCol;
    uint8_t colIndexHam;
    uint8_t cMask1 = pf1PrioIllegal ? 0 : 0xf;
    uint8_t cMask2 = pf2PrioIllegal ? 0 : 0xf;
    bool _hBlank = hBlank;
    bool _vBlank = vBlank;

    Sprite& spr0 = sprites[0];
    Sprite& spr1 = sprites[1];
    Sprite& spr2 = sprites[2];
    Sprite& spr3 = sprites[3];
    Sprite& spr4 = sprites[4];
    Sprite& spr5 = sprites[5];
    Sprite& spr6 = sprites[6];
    Sprite& spr7 = sprites[7];

    for (int c = 0; c < cycles; c++) {
        BplUpdate& upd = bplUpdate[c];

        for(int p = 0; p < 2; p++) {
            sprGroup = 0;
            sprData = 0;
            sprPrio = 0;

            if (spr0.shift) {
                sprData |= ((spr0.shift & 0x80000000) >> 31) | ((spr0.shift & 0x8000) >> 14);
                if ((sprData & 3)  ) sprGroup |= 1;
                spr0.shift = (spr0.shift << 1) & ~(0x10000);
            }

            if (spr1.shift) {
                sprData |= ((spr1.shift & 0x80000000) >> 29) | ((spr1.shift & 0x8000) >> 12);
                if ((sprData & 0xc) && (sprClxMask & 2) ) sprGroup |= 1;
                spr1.shift = (spr1.shift << 1) & ~(0x10000);
            }

            if (spr2.shift) {
                sprData |= ((spr2.shift & 0x80000000) >> 27) | ((spr2.shift & 0x8000) >> 10);
                if ((sprData & 0x30) ) sprGroup |= 2;
                spr2.shift = (spr2.shift << 1) & ~(0x10000);
            }

            if (spr3.shift) {
                sprData |= ((spr3.shift & 0x80000000) >> 25) | ((spr3.shift & 0x8000) >> 8);
                if ((sprData & 0xc0) && (sprClxMask & 8) ) sprGroup |= 2;
                spr3.shift = (spr3.shift << 1) & ~(0x10000);
            }

            if (spr4.shift) {
                sprData |= ((spr4.shift & 0x80000000) >> 23) | ((spr4.shift & 0x8000) >> 6);
                if ((sprData & 0x300) ) sprGroup |= 4;
                spr4.shift = (spr4.shift << 1) & ~(0x10000);
            }

            if (spr5.shift) {
                sprData |= ((spr5.shift & 0x80000000) >> 21) | ((spr5.shift & 0x8000) >> 4);
                if ((sprData & 0xc00) && (sprClxMask & 0x20) ) sprGroup |= 4;
                spr5.shift = (spr5.shift << 1) & ~(0x10000);
            }

            if (spr6.shift) {
                sprData |= ((spr6.shift & 0x80000000) >> 19) | ((spr6.shift & 0x8000) >> 2);
                if ((sprData & 0x3000) ) sprGroup |= 8;
                spr6.shift = (spr6.shift << 1) & ~(0x10000);
            }

            if (spr7.shift) {
                sprData |= ((spr7.shift & 0x80000000) >> 17) | ((spr7.shift & 0x8000) >> 0);
                if ((sprData & 0xc000) && (sprClxMask & 0x80) ) sprGroup |= 8;
                spr7.shift = (spr7.shift << 1) & ~(0x10000);
            }

            if (spr0.armed && (hPos == spr0.x)) spr0.shift = (spr0.datA << 16) | spr0.datB;
            if (spr1.armed && (hPos == spr1.x)) spr1.shift = (spr1.datA << 16) | spr1.datB;
            if (spr2.armed && (hPos == spr2.x)) spr2.shift = (spr2.datA << 16) | spr2.datB;
            if (spr3.armed && (hPos == spr3.x)) spr3.shift = (spr3.datA << 16) | spr3.datB;
            if (spr4.armed && (hPos == spr4.x)) spr4.shift = (spr4.datA << 16) | spr4.datB;
            if (spr5.armed && (hPos == spr5.x)) spr5.shift = (spr5.datA << 16) | spr5.datB;
            if (spr6.armed && (hPos == spr6.x)) spr6.shift = (spr6.datA << 16) | spr6.datB;
            if (spr7.armed && (hPos == spr7.x)) spr7.shift = (spr7.datA << 16) | spr7.datB;

            if constexpr (_display) {
                if (sprData) {
                    if (sprData & 0xf) { // Spr 0/1
                        if (spr1.attached) {
                            sprData = (sprData & 0xf) + 16;
                        } else {
                            if (sprData & 3)
                                sprData = (sprData & 3) + 16;
                            else
                                sprData = ((sprData >> 2) & 3) + 16;
                        }
                    } else if (sprData & 0xf0) { // Spr 2/3
                        if (spr3.attached) {
                            sprData = ((sprData & 0xf0) >> 4) + 16;
                        } else {
                            if (sprData & 0x30)
                                sprData = ((sprData >> 4) & 3) + 20;
                            else
                                sprData = ((sprData >> 6) & 3) + 20;
                        }
                        sprPrio = 1;
                    } else if (sprData & 0xf00) { // Spr 4/5
                        if (spr5.attached) {
                            sprData = ((sprData & 0xf00) >> 8) + 16;
                        } else {
                            if (sprData & 0x300)
                                sprData = ((sprData >> 8) & 3) + 24;
                            else
                                sprData = ((sprData >> 10) & 3) + 24;
                        }
                        sprPrio = 2;
                    } else { // Spr 6/7
                        if (spr7.attached) {
                            sprData = ((sprData & 0xf000) >> 12) + 16;
                        } else {
                            if (sprData & 0x3000)
                                sprData = ((sprData >> 12) & 3) + 28;
                            else
                                sprData = ((sprData >> 14) & 3) + 28;
                        }
                        sprPrio = 3;
                    }
                }
            }

            if constexpr (_display) {
                if (!borderFlipFlop) {
                    if (hPos == hStop) {
                        borderFlipFlop = true;
                        if (!_hBlank && !agnus.crop.right)
                            agnus.updateCropRight(linePos);
                    }
                } else {
                    if (hPos == hStart) {
                        borderFlipFlop = false;
                        if (!agnus.crop.left)
                            agnus.updateCropLeft(linePos);
                    }
                }
            }

            for(int h = 0; h < (_hires ? 2 : 1); h++) {

                clxNoCol = ((shifterAClxEna & (shifterA ^ shifterAClxPolarity)) != 0)
                           | ((shifterBClxEna & (shifterB ^ shifterBClxPolarity)) != 0) << 1;

                switch(sprGroup) {
                    case 0: if (!clxNoCol) clxDat |= 1; break;
                    case 1: SPF(1) break;
                    case 2: SPF(2) break;
                    case 3: SPF2(3, 1) break;
                    case 4: SPF(4) break;
                    case 5: SPF2(5, 2) break;
                    case 6: SPF2(6, 8) break;
                    case 7: SPF2(7, 11) break;
                    case 8: SPF(8) break;
                    case 9: SPF2(9, 4) break;
                    case 10: SPF2(10, 16) break;
                    case 11: SPF2(11, 21) break;
                    case 12: SPF2(12, 32) break;
                    case 13: SPF2(13, 38) break;
                    case 14: SPF2(14, 56) break;
                    case 15: SPF2(15, 63) break;
                }

                if constexpr (_display) {
                    colIndex = 0;
                    colIndex2 = 0;

                    if (shifterA | shifterB) {
                        if constexpr (_doublePlayfield) {
                            colIndex = (shifterA >> 47) | ((shifterA >> 30) & 2) | ((shifterA >> 13) & 4);
                            colIndex2 = (shifterB >> 47) | ((shifterB >> 30) & 2) | ((shifterB >> 13) & 4);

                            if (colIndex2)
                                colIndex2 |= 8;

                            if constexpr (_ham) {
                                colIndexHam = (colIndex2 && (!colIndex || pf2PrioOverPf1)) ? (colIndex2 & cMask2) : (
                                        colIndex & cMask1);

                                switch (((colIndex2 & 4) >> 1) | ((colIndex & 4) >> 2)) {
                                    case 0:
                                        hamColor = colors[colIndexHam];
                                        break;
                                    case 1:
                                        hamColor = (hamColor & 0xff0) | colIndexHam;
                                        break;
                                    case 2:
                                        hamColor = (hamColor & 0x0ff) | (colIndexHam << 8);
                                        break;
                                    case 3:
                                        hamColor = (hamColor & 0xf0f) | (colIndexHam << 4);
                                        break;
                                }
                            }
                        } else {
                            colIndex = (shifterA >> 47) | ((shifterB >> 46) & 2) | ((shifterA >> 29) & 4)
                                       | ((shifterB >> 28) & 8) | ((shifterA >> 11) & 16) | ((shifterB >> 10) & 32);

                            if constexpr (_ham) {
                                switch (colIndex & 0x30) {
                                    case 0:
                                        hamColor = colors[colIndex & 0xf];
                                        break;
                                    case 0x10:
                                        hamColor = (hamColor & 0xff0) | (colIndex & 0xf);
                                        break;
                                    case 0x20:
                                        hamColor = (hamColor & 0x0ff) | ((colIndex & 0xf) << 8);
                                        break;
                                    case 0x30:
                                        hamColor = (hamColor & 0xf0f) | ((colIndex & 0xf) << 4);
                                        break;
                                }
                            } else if (pf2PrioIllegal && (colIndex & 0x10)) {
                                colIndex &= 0x30;
                            }
                        }

                        shifterA = (shifterA << 1) & ~(0x1000100010000);
                        shifterB = (shifterB << 1) & ~(0x1000100010000);
                    } else if constexpr (_ham)
                        hamColor = colors[0];

                    if (!borderFlipFlop && enableDisplay) {
                        if (sprData) {
                            if constexpr (_doublePlayfield) {
                                if (!colIndex && !colIndex2) // both playfields are transparent
                                    color = colors[sprData];
                                else if (!colIndex) { // playfield 1 is transparent
                                    if (sprPrio < pf2Prio)
                                        color = colors[sprData];
                                    else if constexpr (_ham)
                                        color = hamColor;
                                    else
                                        color = colors[colIndex2 & cMask2];
                                } else if (!colIndex2) { // playfield 2 is transparent
                                    if (sprPrio < pf1Prio)
                                        color = colors[sprData];
                                    else if constexpr (_ham)
                                        color = hamColor;
                                    else
                                        color = colors[colIndex & cMask1];
                                } else { // both playfields are non transparent
                                    if (pf2PrioOverPf1) {
                                        if (sprPrio < pf2Prio)
                                            color = colors[sprData];
                                        else if constexpr (_ham)
                                            color = hamColor;
                                        else
                                            color = colors[colIndex2 & cMask2];
                                    } else {
                                        if (sprPrio < pf1Prio)
                                            color = colors[sprData];
                                        else if constexpr (_ham)
                                            color = hamColor;
                                        else
                                            color = colors[colIndex & cMask1];
                                    }
                                }
                            } else {
                                if constexpr (_ham) {
                                    if (sprPrio < pf2Prio)
                                        color = colors[sprData];
                                    else
                                        color = hamColor;
                                } else {
                                    if (!colIndex || (sprPrio < pf2Prio))
                                        color = colors[sprData];
                                    else
                                        color = colors[colIndex];
                                }
                            }

                        } else { // no sprite data
                            if constexpr (_doublePlayfield) {
                                if constexpr (_ham)
                                    color = hamColor;
                                else {
                                    if (colIndex2 && (!colIndex || pf2PrioOverPf1)) {
                                        color = colors[colIndex2 & cMask2];
                                    } else
                                        color = colors[colIndex & cMask1];
                                }
                            } else if constexpr (_ham)
                                color = hamColor;
                            else
                                color = colors[colIndex];
                        }
                    } else
                        color = _hBlank ? 0 : colors[0];

                    *(linePtr + linePos++) = color;


                    if constexpr (!_hires) {
                        if (hiresFrame)
                            *(linePtr + linePos++) = color;
                    }

                    // if Agnus misses sync
                    linePos &= 0x3ff;
                } else {
                    if (shifterA | shifterB) {
                        shifterA = (shifterA << 1) & ~(0x1000100010000);
                        shifterB = (shifterB << 1) & ~(0x1000100010000);
                    }
                }
            }

            if (pfShift) {
                if (pfShift & PF1_SHIFT) {
                    if ((hPos & (_hires ? 7 : 15)) == delayPf1) {
                        pfShift &= ~PF1_SHIFT;

                        if (_activePlanes >= 5) {
                            shifterA = (uint64_t) dat1 << 32;
                            shifterA |= (uint64_t) dat3 << 16;
                            shifterA |= (uint64_t) dat5;
                        } else if (_activePlanes >= 3) {
                            shifterA = (uint64_t) dat1 << 32;
                            shifterA |= (uint64_t) dat3 << 16;
                        } else if (_activePlanes >= 1) {
                            shifterA = (uint64_t) dat1 << 32;
                        }
                    }
                }

                if (pfShift & PF2_SHIFT) {
                    if ((hPos & (_hires ? 7 : 15)) == delayPf2) {
                        pfShift &= ~PF2_SHIFT;

                        if (_activePlanes >= 6) {
                            shifterB = (uint64_t) dat2 << 32;
                            shifterB |= (uint64_t) dat4 << 16;
                            shifterB |= (uint64_t) dat6;
                        } else if (_activePlanes >= 4) {
                            shifterB = (uint64_t) dat2 << 32;
                            shifterB |= (uint64_t) dat4 << 16;
                        } else if (_activePlanes >= 2) {
                            shifterB = (uint64_t) dat2 << 32;
                        }
                    }
                }
            }

            hPos++;
        }

        if (upd.actions) {
            if (upd.actions & BPL1_WRITTEN) {
                if (!_hBlank) {
                    dat1 = upd.dat1;
                    dat2 = upd.dat2;
                    dat3 = upd.dat3;
                    dat4 = upd.dat4;
                    dat5 = upd.dat5;
                    dat6 = upd.dat6;
                    enableDisplay = true;
                    pfShift = PF1_SHIFT | PF2_SHIFT;
                }
                upd.actions &= ~BPL1_WRITTEN;
            }

            if (upd.actions & RESET_HPOS) {
                hPos = 2;
                upd.actions &= ~RESET_HPOS;
            }

//            if (upd.actions & UPD_COLOR) {
//                colors[upd.dat2] = upd.dat1;
//                if (model == OCS_A1000_NO_EHB)
//                    colors[upd.dat2 + 32] = upd.dat1;
//                else
//                    colors[upd.dat2 + 32] = ehbCols[upd.dat1 & 0xfff];
//                upd.actions &= ~UPD_COLOR;
//            }
        }

        hPos &= 0x1ff;
        if (_hBlank) {
            if ((hPos == 84) && !_vBlank)     // 456 - 2 - 384 (CRT Monitor) = 70 blanking pixel
                hBlank = _hBlank = false;
        } else {
            if (hPos == 14) {
                hBlank = _hBlank = true;
                enableDisplay = false;
            }
        }
    }
}

auto Denise::serialize(Emulator::Serializer& s) -> void {
    s.integer((uint8_t&)model);
    s.integer(hPos);
    s.array( colors );
    s.integer(hires);
    s.integer(activePlanes);
    s.integer(hamColor);
    s.integer(bpl1dat);
    s.integer(bpl2dat);
    s.integer(bpl3dat);
    s.integer(bpl4dat);
    s.integer(bpl5dat);
    s.integer(bpl6dat);
    s.integer(dat1);
    s.integer(dat2);
    s.integer(dat3);
    s.integer(dat4);
    s.integer(dat5);
    s.integer(dat6);
    s.integer(shifterA);
    s.integer(shifterB);
    s.integer(shifterAClxEna);
    s.integer(shifterBClxEna);
    s.integer(shifterAClxPolarity);
    s.integer(shifterBClxPolarity);
    s.integer(clxDat);
    s.integer(pfShift);
    s.integer(delayPf1);
    s.integer(delayPf2);
    s.integer(bplCon0);
    s.integer(bplCon1);
    s.integer(enableDisplay);
    s.integer(borderFlipFlop);
    s.integer(linePos);
    s.integer(deniseClock);

    for(int i = 0; i < 8; i++) {
        Sprite& spr = sprites[i];
        s.integer(spr.datA);
        s.integer(spr.datB);
        s.integer(spr.shift);
        s.integer(spr.x);
        s.integer(spr.armed);
        s.integer(spr.attached);
    }

    s.integer(sprClxMask);
    s.integer(pf2PrioOverPf1);
    s.integer(pf1Prio);
    s.integer(pf2Prio);
    s.integer(pf1PrioIllegal);
    s.integer(pf2PrioIllegal);
    s.integer(hStart);
    s.integer(hStop);
    s.integer(hBlank);
    s.integer(vBlank);
    s.integer(hiresFrame);
}

}
