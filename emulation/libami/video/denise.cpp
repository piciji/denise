
#include "denise.h"
#include "../agnus/agnus.h"
#include "../system/system.h"

#define LINE_BUFFER_WIDTH 1024
#define LINE_BUFFER_HEIGHT 600
#define LINE_MAX_WIDTH 384
#define LINE_RENDER_OFFSET 5 // needed to avoid sanity checking for CRT emulation later on

namespace LIBAMI {

Denise::Denise(System* system, Agnus& agnus, Input& input) : system(system), agnus(agnus), input(input) {
    frameBuffer = new uint16_t[LINE_BUFFER_WIDTH * LINE_BUFFER_HEIGHT];
    lineCallback.use = false;
    lineCallback.line = 0;
}

Denise::~Denise() {
    delete[] frameBuffer;
}

auto Denise::joy0Dat() -> uint16_t {
    return input.readDenisePortA();
}

auto Denise::joy1Dat() -> uint16_t {
    return input.readDenisePortB();
}

auto Denise::power() -> void {
    hPos = 2;
    std::fill_n(colors, 64, 0);
    ready = 0;
    hires = false;
    ham = false;
    doublePlayfield = false;
    useInterlace = 0;
    activePlanes = 0;

    shifterA = 0;
    shifterB = 0;

    shifterAClxEna = 0;
    shifterBClxEna = 0;

    shifterAClxPolarity = 0;
    shifterBClxPolarity = 0;

    clxDat = 0;
    delayPf1 = 0;
    delayPf2 = 0;
    enableDisplay = false;

    linePtr = frameBuffer;
    lineVCounter = 0;
    linePos = 0;
    enableSequencer = true;

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
    endFrame = 0;
    vBlank = true;
    hBlank = true;
    hiresFrame = 0;

    crop.left = 0;
    crop.right = 0;
}

// Denise listen for addresses, puted on RGA BUS
auto Denise::setBpl1Dat(uint16_t value) -> void {
    bpl1dat = value;
    if (!hBlank) {
        enableDisplay = true;
        ready = 1 | 2;
        updateCropLeft();
    }
}

auto Denise::strhor() -> void {
    hPos = 2;
    if (vBlank) {
        lineVCounter = 0;
        vBlank = false;
    }
}

auto Denise::strequ() -> void {
    // no hpos reset
    if (!vBlank) {
        endFrame = model == OCS ? 2 : 1;
        vBlank = true;
    }
}

auto Denise::strvbl() -> void {
    // Denise has no vcounter
    hPos = 2;
    if (!vBlank) {
        endFrame = model == OCS ? 2 : 1;
        vBlank = true;
    }
}

auto Denise::startHblank() -> void {
    hBlank = true;

    if (endFrame) {
        endFrame--;
        if (!endFrame) {
            if (useInterlace)
                lineVCounter--;

            unsigned width = hiresFrame ? (LINE_MAX_WIDTH << 1) : LINE_MAX_WIDTH;

            system->videoRefresh(frameBuffer + LINE_RENDER_OFFSET, width, lineVCounter, LINE_BUFFER_WIDTH - width, useInterlace);
        }
    } else if (lineCallback.use && (lineVCounter == lineCallback.line)) {
        system->videoMidScreenCallback(useInterlace);
    }
}

auto Denise::endHblank() -> void {
    linePos = 0;

    if (!vBlank || endFrame) {
        hBlank = false;
        if (lineVCounter == 0) {
            crop.left = 0;
            crop.right = 0;
            hiresFrame = hires ? 1 : 0; // can switch to hires mid frame
            // Denise doesn't need to know if Interlace is active. However, in order to arrange the resulting image in memory,
            // we need this information here.
            if (agnus.lace()) {
                if (agnus.lof) {
                    useInterlace = 2;
                    lineVCounter = 1;
                } else
                    useInterlace = 1;
            } else
                useInterlace = 0;
        }

        if (lineVCounter >= LINE_BUFFER_HEIGHT) // could happen, if Agnus beam position has been changed
            lineVCounter = LINE_BUFFER_HEIGHT - 1;

        linePtr = frameBuffer + lineVCounter * LINE_BUFFER_WIDTH + LINE_RENDER_OFFSET;
        lineVCounter += (useInterlace & 0x80) ? 2 : 1;
    }
}

auto Denise::setDiwStrt(uint16_t value) -> void {
    hStart = value & 0xff;
}

auto Denise::setDiwStop(uint16_t value) -> void {
    hStop = (value & 0xff) | 0x100;
}

auto Denise::setColor( uint8_t pos, uint16_t value ) -> void {
    colors[pos] = value;

    if (model == OCS_A1000_NO_EHB)
        colors[ 32 + pos ] = value;
    else {
        //extra half brite
        uint8_t r = (value >> 8) & 0xf;
        uint8_t g = (value >> 4) & 0xf;
        uint8_t b = (value >> 0) & 0xf;
        colors[32 + pos] = ((r >> 1) << 8) | ((g >> 1) << 4) | (b >> 1);
    }
}

auto Denise::setClxCon(uint16_t value) -> void {
    shifterAClxEna = ((value & 0x40ULL) << 41) | ((value & 0x100) << 23) | ((value & 0x400) << 5);
    shifterBClxEna = ((value & 0x80ULL) << 40) | ((value & 0x200) << 22) | ((value & 0x800) << 4);

    shifterAClxPolarity = ((value & 1ULL) << 47) | ((value & 4) << 29) | ((value & 16) << 11);
    shifterBClxPolarity = ((value & 2ULL) << 46) | ((value & 8) << 28) | ((value & 32) << 10);

    sprClxMask = 0x55; // even sprites are always allowed
    for (unsigned i = 0; i < 4; i++ ) {
        if ( (value >> 12) & (1 << i))
            sprClxMask |= 1 << ( (i << 1) + 1 );
    }
}

auto Denise::setSprDatA( uint8_t nr, uint16_t value ) -> void {
    sprites[nr].datA = value;
    sprites[nr].armed = true;
}

auto Denise::setSprDatB( uint8_t nr, uint16_t value ) -> void {
    sprites[nr].datB = value;
}

auto Denise::setSprCtl( uint8_t nr, uint16_t value ) -> void {
    Sprite& spr = sprites[nr];
    spr.x &= ~1;
    spr.x |= value & 1;
    spr.attached = (nr & 1) && (value & 0x80);
    spr.armed = false;
}

auto Denise::setSprPos( uint8_t nr, uint16_t value ) -> void {
    Sprite& spr = sprites[nr];
    spr.x &= 1;
    spr.x |= (value & 0xff) << 1;
}

auto Denise::getClxDat() -> uint16_t {
    uint16_t out = clxDat | 0x8000;
    clxDat = 0;
    return out;
}

auto Denise::setBplCon0( uint16_t value ) -> void {
    hires = value & 0x8000;
    if (hires) {
        if (hiresFrame == 0)
            switchToHiresMidframe();

        hiresFrame = 1;
    } else {
        if (hiresFrame == 1)
            hiresFrame |= 0x80;
    }

    doublePlayfield = value & 0x400;
    ham = value & 0x800;
    activePlanes = (value >> 12) & 7;
}

auto Denise::setBplCon1( uint16_t value ) -> void {
    delayPf1 = value & 0xf;
    delayPf2 = (value >> 4) & 0xf;
}

auto Denise::setBplCon2( uint16_t value ) -> void {
    pf1Prio = value & 7;
    pf2Prio = (value >> 3) & 7;
    pf2PrioOverPf1 = (value >> 6) & 1;
    if (pf1Prio > 4) {
        pf1PrioIllegal = true;
        pf1Prio = 0;
    } else
        pf1PrioIllegal = false;

    if (pf2Prio > 4) {
        pf2PrioIllegal = true;
        pf2Prio = 0;
    } else
        pf2PrioIllegal = false;
}

inline auto Denise::processDelay() -> void {
    if (ready & 1) {
        if ((hPos & 0xf) == delayPf1) {
            if (activePlanes >= 5) {
                shifterA |= (unsigned long long)bpl1dat << 32;
                shifterA |= bpl3dat << 16;
                if (!hires)
                    shifterA |= bpl5dat;
            } else if (activePlanes >= 3) {
                shifterA |= (unsigned long long)bpl1dat << 32;
                shifterA |= bpl3dat << 16;
            } else if (activePlanes >= 1) {
                shifterA |= (unsigned long long)bpl1dat << 32;
            }
            ready &= ~1;
        }
    }
    if (ready & 2) {
        if ((hPos & 0xf) == delayPf2) {
            if (activePlanes >= 6) {
                shifterB |= (unsigned long long)bpl2dat << 32;
                shifterB |= bpl4dat << 16;
                if (!hires)
                    shifterB |= bpl6dat;
            } else if (activePlanes >= 4) {
                shifterB |= (unsigned long long)bpl2dat << 32;
                shifterB |= bpl4dat << 16;
            } else if (activePlanes >= 2) {
                shifterB |= (unsigned long long)bpl2dat << 32;
            }
            ready &= ~2;
        }
    }
}

template<bool useHires> auto Denise::processPixel() -> void {
    uint8_t sprGroup = 0;
    uint8_t sprPrio = 0;
    uint16_t sprData = 0;
    uint8_t colIndex = 0;
    uint8_t colIndex2 = 0;
    uint16_t color;
    bool _ham = ham;

    Sprite& spr0 = sprites[0];
    Sprite& spr1 = sprites[1];
    Sprite& spr2 = sprites[2];
    Sprite& spr3 = sprites[3];
    Sprite& spr4 = sprites[4];
    Sprite& spr5 = sprites[5];
    Sprite& spr6 = sprites[6];
    Sprite& spr7 = sprites[7];

    if constexpr (!useHires) { // two hires pixels in a row use same hpos, no need to compare same values again
        if (spr0.armed && (hPos == spr0.x)) spr0.shift = (spr0.datA << 16) | spr0.datB;
        if (spr1.armed && (hPos == spr1.x)) spr1.shift = (spr1.datA << 16) | spr1.datB;
        if (spr2.armed && (hPos == spr2.x)) spr2.shift = (spr2.datA << 16) | spr2.datB;
        if (spr3.armed && (hPos == spr3.x)) spr3.shift = (spr3.datA << 16) | spr3.datB;
        if (spr4.armed && (hPos == spr4.x)) spr4.shift = (spr4.datA << 16) | spr4.datB;
        if (spr5.armed && (hPos == spr5.x)) spr5.shift = (spr5.datA << 16) | spr5.datB;
        if (spr6.armed && (hPos == spr6.x)) spr6.shift = (spr6.datA << 16) | spr6.datB;
        if (spr7.armed && (hPos == spr7.x)) spr7.shift = (spr7.datA << 16) | spr7.datB;
    }

    if (spr0.shift) {
        sprData |= ((spr0.shift & 0x80000000) >> 31) | ((spr0.shift & 0x8000) >> 14);
        if ((sprData & 3) && (sprClxMask & 1) ) sprGroup |= 1;
        spr0.shift = (spr0.shift << 1) & ~(0x10000);
    }

    if (spr1.shift) {
        sprData |= ((spr1.shift & 0x80000000) >> 29) | ((spr1.shift & 0x8000) >> 12);
        if ((sprData & 0xc) && (sprClxMask & 2) ) sprGroup |= 1;
        spr1.shift = (spr1.shift << 1) & ~(0x10000);
    }

    if (spr2.shift) {
        sprData |= ((spr2.shift & 0x80000000) >> 27) | ((spr2.shift & 0x8000) >> 10);
        if ((sprData & 0x30) && (sprClxMask & 4) ) sprGroup |= 2;
        spr2.shift = (spr2.shift << 1) & ~(0x10000);
    }

    if (spr3.shift) {
        sprData |= ((spr3.shift & 0x80000000) >> 25) | ((spr3.shift & 0x8000) >> 8);
        if ((sprData & 0xc0) && (sprClxMask & 8) ) sprGroup |= 2;
        spr3.shift = (spr3.shift << 1) & ~(0x10000);
    }

    if (spr4.shift) {
        sprData |= ((spr4.shift & 0x80000000) >> 23) | ((spr4.shift & 0x8000) >> 6);
        if ((sprData & 0x300) && (sprClxMask & 0x10) ) sprGroup |= 4;
        spr4.shift = (spr4.shift << 1) & ~(0x10000);
    }

    if (spr5.shift) {
        sprData |= ((spr5.shift & 0x80000000) >> 21) | ((spr5.shift & 0x8000) >> 4);
        if ((sprData & 0xc00) && (sprClxMask & 0x20) ) sprGroup |= 4;
        spr5.shift = (spr5.shift << 1) & ~(0x10000);
    }

    if (spr6.shift) {
        sprData |= ((spr6.shift & 0x80000000) >> 19) | ((spr6.shift & 0x8000) >> 2);
        if ((sprData & 0x3000) && (sprClxMask & 0x40) ) sprGroup |= 8;
        spr6.shift = (spr6.shift << 1) & ~(0x10000);
    }

    if (spr7.shift) {
        sprData |= ((spr7.shift & 0x80000000) >> 17) | ((spr7.shift & 0x8000) >> 0);
        if ((sprData & 0xc000) && (sprClxMask & 0x80) ) sprGroup |= 8;
        spr7.shift = (spr7.shift << 1) & ~(0x10000);
    }

    // todo: influence illegal pf2 prio modes clxDat calculation
    bool pf1NoCol = shifterAClxEna & (shifterA ^ shifterAClxPolarity);
    bool pf2NoCol = shifterBClxEna & (shifterB ^ shifterBClxPolarity);

#define SPF(v1) \
    if (!pf2NoCol && !pf1NoCol)             clxDat |= (v1 << 5) | (v1 << 1) | 1; \
    else if (!pf2NoCol)                     clxDat |= (v1 << 5); \
    else if (!pf1NoCol && doublePlayfield)  clxDat |= (v1 << 1);

#define SPF2(v1, v2) \
    if (!pf2NoCol && !pf1NoCol)             clxDat |= (v1 << 5) | (v1 << 1) | (v2 << 9) | 1; \
    else if (!pf2NoCol)                     clxDat |= (v1 << 5) | (v2 << 9); \
    else if (!pf1NoCol && doublePlayfield)  clxDat |= (v1 << 1) | (v2 << 9);

    switch(sprGroup) {
        case 0: if (!pf2NoCol && !pf1NoCol) clxDat |= 1; break;
        case 1: SPF(1) break;
        case 2: SPF(2) break;
        case 3: SPF2(3, 1) break;
        case 4: SPF(4) break;
        case 5: SPF2(5, 2) break;
        case 6: SPF2(6, 8) break;
        case 7: SPF2(7, 11) break;
        case 8: SPF(8) break;
        case 9: SPF2(9, 4) break;
        case 10: SPF2(10, 13) break;
        case 11: SPF2(11, 21) break;
        case 12: SPF2(12, 14) break;
        case 13: SPF2(13, 38) break;
        case 14: SPF2(14, 56) break;
        case 15: SPF2(15, 63) break;
    }

    if (shifterA | shifterB) {
        if (doublePlayfield) {
            colIndex = (shifterA >> 47) | ((shifterA >> 30) & 2) | ((shifterA >> 13) & 4);
            colIndex2 = (shifterB >> 47) | ((shifterB >> 30) & 2) | ((shifterB >> 13) & 4);

            if (colIndex2)
                colIndex2 |= 8;

            if (_ham) {
                uint8_t colIndexHam = (colIndex2 && (!colIndex || pf2PrioOverPf1)) ? colIndex2 : colIndex;

                switch( ((colIndex2 & 4) >> 1) | ((colIndex & 4) >> 2) ) {
                    case 0: hamColor = colors[ colIndexHam ]; break;
                    case 1: hamColor = (hamColor & 0xff0) | colIndexHam; break;
                    case 2: hamColor = (hamColor & 0x0ff) | (colIndexHam << 8); break;
                    case 3: hamColor = (hamColor & 0xf0f) | (colIndexHam << 4); break;
                }
            }
        } else {
            colIndex = (shifterA >> 47) | ((shifterB >> 46) & 2) | ((shifterA >> 29) & 4)
                       | ((shifterB >> 28) & 8) | ((shifterA >> 11) & 16) | ((shifterB >> 10) & 32);

            if (_ham) {
                switch(colIndex & 0x30) {
                    case 0:     hamColor = colors[ colIndex & 0xf ]; break;
                    case 0x10:  hamColor = (hamColor & 0xff0) | (colIndex & 0xf); break;
                    case 0x20:  hamColor = (hamColor & 0x0ff) | ((colIndex & 0xf) << 8); break;
                    case 0x30:  hamColor = (hamColor & 0xf0f) | ((colIndex & 0xf) << 4); break;
                }
            } else if (pf2PrioIllegal && (colIndex & 0x10)) {
                colIndex &= 0x30;
            }
        }

        shifterA = (shifterA << 1) & ~(0x1000100010000);
        shifterB = (shifterB << 1) & ~(0x1000100010000);
    }

    if constexpr (!useHires) {
        if (!enableDisplay && (hPos == hStart)) {
            enableDisplay = true;
            if (!hBlank)
                updateCropLeft();
        }
    }

    if (enableDisplay) {
        if constexpr (!useHires) {
            if (hPos == hStop) {
                enableDisplay = false;
                if (!hBlank)
                    updateCropRight();
                colIndex = colIndex2 = 0;
                _ham = false;
                if (model & (OCS_A1000 | OCS_A1000_NO_EHB) );
                else
                    sprData = 0;
            }
        }

        if (!hBlank) {
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

                if (doublePlayfield) {
                    if (!colIndex && !colIndex2) // both playfields are transparent
                        color = colors[sprData];
                    else if (!colIndex) { // playfield 1 is transparent
                        if (sprPrio < pf2Prio)
                            color = colors[sprData];
                        else
                            color = colors[pf2PrioIllegal ? 0 : colIndex2];
                    } else if (!colIndex2) { // playfield 2 is transparent
                        if (sprPrio < pf1Prio)
                            color = colors[sprData];
                        else
                            color = colors[pf1PrioIllegal ? 0 : colIndex];
                    } else { // both playfields are non transparent
                        if ((sprPrio >= pf1Prio) && (sprPrio >= pf2Prio)) { // sprite behind playfields
                            if (pf2PrioOverPf1)
                                color = colors[pf2PrioIllegal ? 0 : colIndex2];
                            else
                                color = colors[pf1PrioIllegal ? 0 : colIndex];
                        } else if ((sprPrio < pf1Prio) && (sprPrio < pf2Prio)) { // sprite before playfields
                            color = colors[sprData];
                        } else { // sprite between playfields
                            if (pf1Prio > pf2Prio)
                                color = colors[pf2PrioIllegal ? 0 : colIndex2];
                            else
                                color = colors[pf1PrioIllegal ? 0 : colIndex];
                        }
                    }
                } else {
                    if (_ham) {
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
                if (doublePlayfield) {
                    if (colIndex2 && (!colIndex || pf2PrioOverPf1)) {
                        color = colors[pf2PrioIllegal ? 0 : colIndex2];
                    } else
                        color = colors[pf1PrioIllegal ? 0 : colIndex];
                } else {
                    color = _ham ? hamColor : colors[colIndex];
                }
            }
        } else
            color = 0; // blank
    } else
        color = hBlank ? 0 : colors[0]; // border

    *(linePtr + linePos++) = color;

    if constexpr(!useHires) {
        if (hiresFrame & 0x80) // lores frame with hires content
            *(linePtr + linePos++) = color;
    }

    // if agnus misses strobe
    linePos &= LINE_BUFFER_WIDTH - 1;
}

auto Denise::process() -> void {
    if (enableSequencer) {
        processPixel<false>();
        if (hires)
            processPixel<true>();

        if (ready)
            processDelay();

        hPos++;

        processPixel<false>();
        if (hires)
            processPixel<true>();

        if (ready)
            processDelay();

        hPos++;
    } else
        hPos += 2;

    hPos &= 0x1ff; // wrap around when strEqu

    // OCS Denise has no CSYNC input
    if (hBlank) {
        if (hPos == 96)
            endHblank();
    } else {
        if (hPos == 24)
            startHblank();
    }
}

auto Denise::switchToHiresMidframe() -> void {
    uint16_t* curLinePtr;
    unsigned xStart;

    for (int y = 0; y < lineVCounter; y++) {
        curLinePtr = frameBuffer + y * LINE_BUFFER_WIDTH;
        xStart = curLinePtr != linePtr ? LINE_MAX_WIDTH : linePos - 1;

        doubleLoresPixel( curLinePtr, xStart );
    }
}

inline auto Denise::doubleLoresPixel(uint16_t* _ptr, unsigned _xStart) -> void {
    for (int _x = _xStart; _x >= 0; _x--) {
        *( _ptr + (_x * 2 + 1) ) = *( _ptr + _x );
        *( _ptr + (_x * 2) ) = *( _ptr + _x );
    }
}

auto Denise::updateCropLeft() -> void {

    if (!crop.left || (crop.left > linePos))
        crop.left = hiresFrame ? linePos << 1 : linePos;
}

auto Denise::updateCropRight() -> void {
    unsigned diff = 0;
    unsigned limit = hiresFrame ? LINE_MAX_WIDTH << 1 : LINE_MAX_WIDTH;

    if (limit > linePos)
        diff = limit - linePos;

    if (!crop.right || (crop.right > diff))
        crop.right = diff;
}

auto Denise::serialize(Emulator::Serializer& s) -> void {
    s.integer((uint8_t&)model);
    s.integer(hPos);
    s.array( colors );
    s.integer(ready);
    s.integer(hires);
    s.integer(ham);
    s.integer(doublePlayfield);
    s.integer(useInterlace);
    s.integer(activePlanes);
    s.integer(hamColor);
    s.integer(bpl1dat);
    s.integer(bpl2dat);
    s.integer(bpl3dat);
    s.integer(bpl4dat);
    s.integer(bpl5dat);
    s.integer(bpl6dat);
    s.integer(shifterA);
    s.integer(shifterB);
    s.integer(shifterAClxEna);
    s.integer(shifterBClxEna);
    s.integer(shifterAClxPolarity);
    s.integer(shifterBClxPolarity);
    s.integer(clxDat);
    s.integer(delayPf1);
    s.integer(delayPf2);
    s.integer(enableDisplay);
    s.integer(linePos);
    s.integer(lineVCounter);
    s.integer(enableSequencer);

    for(unsigned i = 0; i < 8; i++) {
        Sprite& spr = sprites[i];
        s.integer(spr.datA);
        s.integer(spr.datB);
        s.integer(spr.shift);
        s.integer(spr.x);
        s.integer(spr.armed);
        s.integer(spr.attached);
    }

    s.integer(crop.left);
    s.integer(crop.right);
    s.integer(sprClxMask);
    s.integer(pf2PrioOverPf1);
    s.integer(pf1Prio);
    s.integer(pf2Prio);
    s.integer(pf1PrioIllegal);
    s.integer(pf2PrioIllegal);
    s.integer(hStart);
    s.integer(hStop);
    s.integer(endFrame);
    s.integer(vBlank);
    s.integer(hBlank);
    s.integer(hiresFrame);
}

}
