
/**
 * blitter shifter behaviour has been reversed enginered from WinUAE
 *
 * UAE - The Un*x Amiga Emulator
 * (c) 2002 - 2021 Toni Wilen
 *
 * Programming is currently experimental and numerous borderline cases need to be cross-checked.
 *
 */

#include "blitter.h"

#define STAGE_A 1
#define STAGE_B 2
#define STAGE_X 4
#define STAGE_Y 8
#define BLT_A 0x80
#define BLT_B 0x40
#define BLT_C 0x20
#define BLT_D 0x10
#define FILL_IDLE 0x100
#define AT_LEAST_ONE_SHIFTOUT 0x200
#define LINE_MODE 0x400
#define STAGE_CHANGE 0x8000

namespace LIBAMI {

auto Blitter::stateMachine() -> void {
    if (!agnus.canBlitterUseBus())
        return;

    if (shiftOut) {
        shiftOut = false;

        if (bltcon1 & 1)
            bltDdat = doMinterm(bltcon0 & 0xff, bltADatShifted, (bltBDatShifted & 1) ? 0xffff : 0, bltCdat);
        else {
            bltDdat = doMinterm(bltcon0 & 0xff, bltADatShifted, bltBDatShifted, bltCdat);

            if (isFillMode()) {
                uint16_t resultLo = fill[(fillCarry << 9) | (isExclusiveFill << 4) | (bltDdat & 0xff)];
                fillCarry = resultLo >> 8;
                uint16_t resultHi = fill[(fillCarry << 9) | (isExclusiveFill << 4) | ((bltDdat >> 8) & 0xff)];

                bltDdat = (resultLo & 0xff) | ((resultHi & 0xff) << 8);

                if (curW == bltSizeW)
                    fillCarry = isFci();
                else
                    fillCarry = resultHi >> 8;
            }
        }
        if (bltDdat) zero = false;
    }

    if (shifter & LINE_MODE) {
        if (shifter & STAGE_A) {
            if (curW == bltSizeW) { // first
                if (shifter & AT_LEAST_ONE_SHIFTOUT) {
                    if (shifter & BLT_A) { // use A
                        if (agnus.getActiveEvent<Agnus::EVENT_DMA_POINTER>())
                            agnus.forceEvent<Agnus::EVENT_DMA_POINTER>();

                        if (bltcon1 & BLT_SIGN)
                            bltApt += (int16_t) bltBmod;
                        else
                            bltApt += (int16_t) bltAmod;
                    }
                    // needs this at least one shiftout before ?
                    if (0 > (int16_t) bltApt)   bltcon1 |= BLT_SIGN;
                    else                        bltcon1 &= ~BLT_SIGN;
                }

                bltADatShifted = (bltAdat & bltAfwm) >> SHIFTA;

                writeLineDot = !(bltcon1 & BLT_SING) || !oneDotPerLine;
                oneDotPerLine = true;
            } else if ((curW + 1) == bltSizeW) { // second only (means "last" by typical horizontal size), never when horizontal size is one
                if (shifter & BLT_C) {
                    if (agnus.getActiveEvent<Agnus::EVENT_DMA_POINTER>())
                        agnus.forceEvent<Agnus::EVENT_DMA_POINTER>();

                    if (bltcon1 & BLT_SUD) {
                        if (bltcon1 & BLT_AUL)  LINE_DECX
                        else                    LINE_INCX
                    } else {
                        if (!(bltcon1 & BLT_SIGN)) {
                            if (bltcon1 & BLT_SUL)  LINE_DECX
                            else                    LINE_INCX
                        }
                    }
                }
            }
        } else if (shifter & STAGE_B) { // B (optional) ... second bit in shifter can only be one, if channel B is in use, so no extra check needed
            // for a blit size of two there is one dummy access and so on
            agnus.fetchBlitterDmaNoBUSCheck<Agnus::PTR_BLT_B_H>(bltBpt, bltBdat);

            if (curW == 1) // last, or first if horizontal size is one
                bltBpt += bltBmod;

        } else if (shifter & STAGE_X) {
            if (curW == bltSizeW) { // first
                bltBDatShifted = ((bltBdat << 16) | bltBdat) >> SHIFTB;
                if ((bltcon1 & 0xf000) == 0) {
                    bltcon1 |= 0xf000;
                } else
                    bltcon1 -= 0x1000;

                if (shifter & BLT_C)
                    agnus.fetchBlitterDmaNoBUSCheck<Agnus::PTR_BLT_C_H>(bltCpt, bltCdat); // maybe happens each blit, expect last one ?

            } else if ((curW + 1) == bltSizeW) { // second only (means "last" by typical horizontal size), never when horizontal size is one
                if (shifter & BLT_C) {
                    if (writeLineDot)
                        agnus.writeBlitterDmaNoBUSCheck(bltDpt, doff ? agnus.dataBus : bltDdat); // second or last ?

                    if (agnus.getActiveEvent<Agnus::EVENT_DMA_POINTER>())
                        agnus.forceEvent<Agnus::EVENT_DMA_POINTER>();

                    if (!(bltcon1 & BLT_SUD)) {
                        if (bltcon1 & BLT_AUL)
                            bltCpt -= bltCmod;
                        else
                            bltCpt += bltCmod;

                        oneDotPerLine = false;
                    } else {
                        if (!(bltcon1 & BLT_SIGN)) {
                            if (bltcon1 & BLT_SUL)
                                bltCpt -= bltCmod;
                            else
                                bltCpt += bltCmod;

                            oneDotPerLine = false;
                        }
                    }
                }
                bltDpt = bltCpt;
            }
        }
    } else { // block mode
        if ((skipB && (shifter & STAGE_X)) || (shifter & STAGE_B)) {
            if (curW == bltSizeW)   bltAdat &= bltAfwm;
            if (curW == 1)          bltAdat &= bltAlwm;

            if (desc)
                bltADatShifted = ((bltAdat << 16) | bltADatOld) >> (16 - SHIFTA);
            else
                bltADatShifted = ((bltADatOld << 16) | bltAdat) >> SHIFTA;

            bltADatOld = bltAdat;
        }

        if ((shifter & BLT_B) && (shifter & STAGE_X)) {
            if (desc)
                bltBDatShifted = ((bltBdat << 16) | bltBDatOld) >> (16 - SHIFTB);
            else
                bltBDatShifted = ((bltBDatOld << 16) | bltBdat) >> SHIFTB;

            bltBDatOld = bltBdat;
        }

        uint8_t channel = channels[shifter & 0x3ff];

        if (channel == 1) {
            agnus.fetchBlitterDmaNoBUSCheck<Agnus::PTR_BLT_A_H>(bltApt, bltAdat);

            if (desc)  bltApt += -2;
            else       bltApt += +2;

            if (curW == 1) {
                if (desc)  bltApt += -bltAmod;
                else       bltApt += bltAmod;
            }
        } else if (channel == 2) {
            agnus.fetchBlitterDmaNoBUSCheck<Agnus::PTR_BLT_B_H>(bltBpt, bltBdat);

            if (desc)  bltBpt += -2;
            else       bltBpt += +2;

            if (curW == 1) {
                if (desc)  bltBpt += -bltBmod;
                else       bltBpt += bltBmod;
            }
        } else if (channel == 3) {
            agnus.fetchBlitterDmaNoBUSCheck<Agnus::PTR_BLT_C_H>(bltCpt, bltCdat);

            if  (desc)  bltCpt += -2;
            else        bltCpt += +2;

            if (curW == 1) {
                if (desc)  bltCpt += -bltCmod;
                else       bltCpt += bltCmod;
            }
        } else if (channel == 4) {
            agnus.writeBlitterDmaNoBUSCheck(bltDpt, doff ? agnus.dataBus : bltDdat);

            if (desc)  bltDpt += -2;
            else       bltDpt += 2;

            if (curW == 1) {
                if (desc)  bltDpt += -bltDmod;
                else       bltDpt += bltDmod;
            }
        }
    }

    if (curSkipB ^ skipB) {
        if (skipB)
            shifter &= ~(STAGE_A | STAGE_B);
        else
            // this is critical because more than one shifter bit can get into the pipeline, resulting in faster counting down of the horizontal counter.
            shifter = (shifter & ~(STAGE_B | STAGE_X)) | ((shifter & STAGE_A) << 1) | ((shifter & STAGE_A) << 2);

        curSkipB = skipB;
    }

    if (curSkipY ^ skipY) {
        if (skipY)
            shifter &= ~(STAGE_X | STAGE_Y);

        curSkipY = skipY;
    }

    if (skipY) {
        shiftOut = shifter & STAGE_X;
    } else {
        shiftOut = shifter & STAGE_Y;
        shifter = (shifter & ~STAGE_Y) | ((shifter & STAGE_X) << 1);
    }

    if (skipB) {
        shifter = (shifter & ~STAGE_X) | ((shifter & STAGE_A) << 2);
    } else {
        shifter = (shifter & ~STAGE_X) | ((shifter & STAGE_B) << 1);
        shifter = (shifter & ~STAGE_B) | ((shifter & STAGE_A) << 1);
    }

    if (shifter & STAGE_CHANGE) { // change channel usage
        shifter &= ~STAGE_CHANGE;

        shifter &= ~0x5f0;
        shifter |= ((bltcon0 >> 4) & 0xf0);
        if (hasFillmodeIdle())
            shifter |= FILL_IDLE;

        if (bltcon1 & 1) {
            shifter |= LINE_MODE;
            skipY = true;
        } else
            skipY = hasSkipY();

        skipB = hasSkipB();
    }

    if (shiftOut) {
        shifter |= AT_LEAST_ONE_SHIFTOUT | STAGE_A;
        if (!--curW) {
            curW = bltSizeW;

            if (!--curH) {

                if (bltcon1 & 1) {
                    agnus.actions &= ~Agnus::ACT_BLITTER;
                    // todo signal IRQ to Paula here, Paula has another 2-4? CPU cycle delay from external
                    busy = false;
                    copper.blitterBusyUpdate();
                } else {
                    flags = ((bltcon0 >> 4) & 0xf0) | (desc << 9) | (isFillMode() << 8) | 0xd; // 5 + shift out

                    //if (!agnus.aga() || !(bltcon0 & 0x100) ) { // OCS, ECS and AGA (only, if there is no D write)
                        // todo: signal IRQ to Paula here, Paula has another 2-4? CPU cycle delay from external
                        busy = false;
                        copper.blitterBusyUpdate();
                    //}
                }
            }
        }
    } else
        shifter &= ~STAGE_A;
}

// lookup table
auto Blitter::prepareChannel() -> void {
    for (unsigned i = 0; i < 1024; i++) {
        bool useA = i & BLT_A;
        bool useB = i & BLT_B;
        bool useC = i & BLT_C;
        bool useD = i & BLT_D;
        bool fillIdle = i & FILL_IDLE;
        bool atleastOneShiftoutHappened = i & AT_LEAST_ONE_SHIFTOUT;
        uint8_t _shifter = i & 0xf;

        if ((_shifter & 4) && useC)
            channels[i] = 3;
        else if ((_shifter & 8) && useC && (_shifter & 3))
            channels[i] = 3;
        else if ((_shifter & 1) && useA)
            channels[i] = 1;
        else if ((_shifter & 2) && useB)
            channels[i] = 2;
        else if (_shifter & 1)
            channels[i] = 0;
        else if (atleastOneShiftoutHappened) {
            if (fillIdle) {
                if (_shifter & 8)
                    channels[i] = 0;
                else if ((_shifter & 4) && useD)
                    channels[i] = 4;
                else
                    channels[i] = 0;
            } else {
                if ((_shifter & 4) && !useC && useD)
                    channels[i] = 4;
                else if ((_shifter & 8) && useC && useD)
                    channels[i] = 4;
                else
                    channels[i] = 0;
            }
        } else
            channels[i] = 0;
    }
}

}
