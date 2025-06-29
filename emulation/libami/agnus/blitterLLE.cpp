
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

#define LINE_DECX2   { if ((bltcon0 & 0xf000) == 0) { \
                        bltcon0 |= 0xf000;  \
                    } else bltcon0 -= 0x1000; }

#define LINE_INCX2   { if ((bltcon0 & 0xf000) == 0xf000) { \
                        bltcon0 &= 0x0fff;  \
                    } else bltcon0 += 0x1000; }

#define BLITTER_DMA_FETCH(useMod, Cha)  \
    if(useMod) {    \
        if (desc)   \
            agnus.fetchBlitterDma<Agnus::PTR_BLT_##Cha##_H, true, true, true, false>(blt##Cha##pt, blt##Cha##dat, blt##Cha##mod);    \
        else    \
            agnus.fetchBlitterDma<Agnus::PTR_BLT_##Cha##_H, false, true, true, false>(blt##Cha##pt, blt##Cha##dat, blt##Cha##mod);   \
    } else {    \
        if (desc)   \
            agnus.fetchBlitterDma<Agnus::PTR_BLT_##Cha##_H, true, true, false, false>(blt##Cha##pt, blt##Cha##dat);     \
        else    \
            agnus.fetchBlitterDma<Agnus::PTR_BLT_##Cha##_H, false, true, false, false>(blt##Cha##pt, blt##Cha##dat);    \
    }

#define BLITTER_DMA_WRITE(useMod)  \
    if(useMod) {    \
        if (desc)   \
            agnus.writeBlitterDma<true, true, true, false>(bltDpt, doff ? agnus.dataBus : bltDdat, bltDmod);    \
        else    \
            agnus.writeBlitterDma<false, true, true, false>(bltDpt, doff ? agnus.dataBus : bltDdat, bltDmod);   \
    } else {    \
        if (desc)   \
            agnus.writeBlitterDma<true, true, false, false>(bltDpt, doff ? agnus.dataBus : bltDdat);     \
        else    \
            agnus.writeBlitterDma<false, true, false, false>(bltDpt, doff ? agnus.dataBus : bltDdat);    \
    }

auto Blitter::stateMachine() -> void {
    if (!agnus.canBlitterUseBusExt())
        return;

    if (shiftOut) {
        shiftOut = false;

        if (bltcon1 & 1) {
            bltDdat = doMinterm(bltcon0 & 0xff, bltADatShifted, (bltBDatShifted & 1) ? 0xffff : 0, bltCdat);
        } else {
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

            if (curW == bltSizeW) { // first, or last if horizontal size is one
                writeLineDot = !(bltcon1 & BLT_SING) || !oneDotPerLine;
                oneDotPerLine = true;

                if (shifter & AT_LEAST_ONE_SHIFTOUT) {
                    if (shifter & BLT_A) { // use A
                        agnus.forceOneCycleEvent(Agnus::PTR_BLT_A_H);

                        if (bltSizeW != 1) {
                            if (bltcon1 & BLT_SIGN)
                                bltApt += (int16_t) bltBmod;
                            else
                                bltApt += (int16_t) bltAmod;

                            if (0 > (int16_t) bltApt) bltcon1 |= BLT_SIGN;
                            else bltcon1 &= ~BLT_SIGN;
                        }
                    }
                }

                int _shiftA = SHIFTA;
                shifter &= ~0xf0000;
                shifter |= _shiftA << 16;

                if (bltSizeW == 1) {
                    // /vAmigaTS/Agnus/Blitter/line/line13
                    if ((bltcon1 & (BLT_SUD | BLT_SUL | BLT_AUL)) != (BLT_SUD | BLT_SUL))
                        bltADatShifted = (bltAdat & bltAfwm & bltAlwm) >> _shiftA;
                } else
                    bltADatShifted = (bltAdat & bltAfwm) >> _shiftA;
            } else if (curW != 1) { // > 2,  not first and not last, no masking needed
                bltADatShifted = bltAdat >> ((shifter >> 16) & 0xf);
            }

        } else if (shifter & STAGE_B) { // B (optional)
            if (curW == 1) // last, or first if horizontal size is one
                agnus.fetchBlitterDma<Agnus::PTR_BLT_B_H, false, false, true, false>(bltBpt, bltBdat, bltBmod);
            else
                agnus.fetchBlitterDma<Agnus::PTR_BLT_B_H, false, false, false, false>(bltBpt, bltBdat);

        } else if (shifter & STAGE_X) {
            if (curW == 1) { // last, or first if horizontal size is one
                if (shifter & BLT_C) {
                    if (writeLineDot)
                        agnus.writeBlitterDma<false, false, false, false>(bltDpt, doff ? agnus.dataBus : bltDdat);

                    agnus.forceOneCycleEvent(Agnus::PTR_BLT_C_H);

                    if (!(shifter & 0x8000'0000)) {
                        if ((bltSizeW != 1) || ((curH != bltSizeH) || ((bltcon1 & (BLT_SUD | BLT_SUL | BLT_AUL)) != 0))  ) {
                            if (!(bltcon1 & BLT_SUD)) {
                                if (bltcon1 & BLT_AUL) bltCpt -= bltCmod;
                                else bltCpt += bltCmod;

                                oneDotPerLine = false;
                            } else {
                                if (!(bltcon1 & BLT_SIGN)) {
                                    if (bltcon1 & BLT_SUL) bltCpt -= bltCmod;
                                    else bltCpt += bltCmod;

                                    oneDotPerLine = false;
                                }
                            }
                        }
                    } else
                        shifter &= ~0x8000'0000;

                    if (bltSizeW == 1 ) {
                        // /vAmigaTS/Agnus/Blitter/line/line12
                        // uncomment next condition to show test results after reset, keep it to show result after cold start
                      //  if (((bltcon1 & (BLT_SUD | BLT_SUL | BLT_AUL)) == 0) && (curH == 0x30 ));
                            // really ... we need a lot more tests here, hopefully we don't need a transistor level emulation to get this right
                        //else {
                            if (!(bltcon1 & BLT_SUD)) {
                                if (bltcon1 & BLT_AUL) LINE_DECX2
                                else LINE_INCX2
                            } else {
                                if (!(bltcon1 & BLT_SIGN)) {
                                    if (bltcon1 & BLT_SUL) LINE_DECX2
                                    else LINE_INCX2
                                }
                            }
                        //}
                    }
                }

                bltDpt = bltCpt;
            } else {
                bltBDatShifted = ((bltBdat << 16) | bltBdat) >> SHIFTB;
                if ((bltcon1 & 0xf000) == 0) {
                    bltcon1 |= 0xf000;
                } else
                    bltcon1 -= 0x1000;

                if (shifter & BLT_C) {
                    // /vAmigaTS/Agnus/Blitter/line/line12
                    if ((curW == bltSizeW) || ((bltcon1 & (BLT_SUD | BLT_SUL | BLT_AUL)) != (BLT_SUL)))
                        agnus.fetchBlitterDma<Agnus::PTR_BLT_C_H, false, false, false, false>(bltCpt, bltCdat);

                    if (curW == bltSizeW) {
                        if (bltcon1 & BLT_SUD) {
                            if (bltcon1 & BLT_AUL) LINE_DECX
                            else LINE_INCX
                        } else {
                            if (!(bltcon1 & BLT_SIGN)) {
                                if (bltcon1 & BLT_SUL) LINE_DECX
                                else LINE_INCX
                            }
                        }
                    } else { // not first, not last
                        if ( !(shifter & 0x8000'0000) && !(bltcon1 & BLT_SUD)) {

                            // /vAmigaTS/Agnus/Blitter/line/line12
                            // comment next condition to show test results after reset, keep it to show result after cold start
                            if (((bltcon1 & (BLT_SUD | BLT_SUL | BLT_AUL)) == BLT_SUL) && (curH == 0x2e ));
                            else {
                                if (bltcon1 & BLT_AUL) bltCpt -= bltCmod;
                                else bltCpt += bltCmod;
                            }

                            oneDotPerLine = false;
                            shifter |= 0x8000'0000;
                        }
                    }
                }
            }
        }
    } else { // block mode
        if ((skipB && (shifter & STAGE_X)) || (shifter & STAGE_B)) {
            uint16_t mask = 0xffff;
            if (curW == bltSizeW)   mask &= bltAfwm;
            if (curW == 1)          mask &= bltAlwm;

            if (desc)
                bltADatShifted = (((bltAdat & mask) << 16) | bltADatOld) >> (16 - SHIFTA);
            else
                bltADatShifted = ((bltADatOld << 16) | (bltAdat & mask)) >> SHIFTA;

            bltADatOld = bltAdat & mask;
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
            BLITTER_DMA_FETCH(curW == 1, A)
        } else if (channel == 2) {
            BLITTER_DMA_FETCH(curW == 1, B)
        } else if (channel == 3) {
            BLITTER_DMA_FETCH(curW == 1, C)
        } else if (channel == 4) {
            BLITTER_DMA_WRITE(curW == bltSizeW)
        }
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


    if (shiftOut) {
        shifter |= AT_LEAST_ONE_SHIFTOUT | STAGE_A;
        if (!--curW) {
            curW = bltSizeW;

            if (!--curH) {
                if (shifter & (STAGE_B | STAGE_X | STAGE_Y)) {
                    // TLC Lotus 2 or TLC CRTrJimPower have more than one shifter bit at the end
                    curW = curH = 1; // one more round

                    if (shifter & (STAGE_X | STAGE_Y)) {
                        shifter |= STAGE_X;
                        shifter &= ~STAGE_A;
                    }

                    shifter &= ~STAGE_B;
                    shifter &= ~STAGE_Y;

                } else {
                    if (bltcon1 & 1) {
                        if(!restartTimer)
                            busy = false;
                        flags = ((bltcon0 >> 4) & 0xf0) | LINE_MODE | 0xf;
                    } else {
                        if (!agnus.aga() && !restartTimer)
                            busy = false;

                        flags = ((bltcon0 >> 4) & 0xf0) | (desc << 9) | (isFillMode() << 8) | 0xd; // 5 + shift out
                    }
                }
            }
        }
    } else
        shifter &= ~STAGE_A;


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

        if (curSkipY ^ skipY) {
            if (skipY) {
                shifter &= ~(STAGE_X | STAGE_Y);
                shiftOut = false;
            } else {
                shifter = (shifter & ~STAGE_Y) | ((shifter & STAGE_X) << 1);
                shiftOut = shifter & STAGE_Y;
            }

            curSkipY = skipY;
        }

        if (curSkipB ^ skipB) {
            if (skipB)
                shifter &= ~(STAGE_A | STAGE_B);
            else {
                // this is critical because more than one shifter bits can get into the pipeline, resulting in faster counting down.
                shifter = (shifter & ~(STAGE_B )) | ((shifter & STAGE_A) << 1);
            }

            curSkipB = skipB;
        }
    }
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

#undef BLITTER_DMA_FETCH
#undef BLITTER_DMA_WRITE