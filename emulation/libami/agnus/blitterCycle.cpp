
#include "blitter.h"

#define BLT_SING 0x2
#define BLT_AUL 0x4
#define BLT_SUL 0x8
#define BLT_SUD 0x10
#define BLT_SIGN 0x40

#define SHIFTA (bltcon0 >> 12)
#define SHIFTB (bltcon1 >> 12)

namespace LIBAMI {

template<uint16_t jobs, uint8_t nextCycle> auto Blitter::blockMode() -> void {
    if constexpr ((jobs & BLT_IDLE) == 0) {
        if (!agnus.canBlitterUseBus())
            return;
    }

    if constexpr (!!(jobs & BLT_FETCH_A)) {
        if(curW == 1)
            agnus.fetchBlitterDma<Agnus::PTR_BLT_A_H, !!(jobs & BLT_DESC), true>(bltApt, bltAdat, bltAmod);
        else
            agnus.fetchBlitterDma<Agnus::PTR_BLT_A_H, !!(jobs & BLT_DESC), true>(bltApt, bltAdat);
        //
        // if constexpr (!!(jobs & BLT_DESC))  bltApt += -2;
        // else                                bltApt += +2;
        //
        // if (curW == 1) {
        //     if constexpr (!!(jobs & BLT_DESC))  bltApt += -bltAmod;
        //     else                                bltApt += bltAmod;
        // }
    } else if constexpr (!!(jobs & BLT_FETCH_B)) {
        if(curW == 1)
            agnus.fetchBlitterDma<Agnus::PTR_BLT_B_H, !!(jobs & BLT_DESC), true>(bltBpt, bltBdat, bltBmod);
        else
            agnus.fetchBlitterDma<Agnus::PTR_BLT_B_H, !!(jobs & BLT_DESC), true>(bltBpt, bltBdat);

        // if constexpr (!!(jobs & BLT_DESC))  bltBpt += -2;
        // else                                bltBpt += +2;
        //
        // if (curW == 1) {
        //     if constexpr (!!(jobs & BLT_DESC))  bltBpt += -bltBmod;
        //     else                                bltBpt += bltBmod;
        // }
    } else if constexpr (!!(jobs & BLT_FETCH_C)) {
        if(curW == 1)
            agnus.fetchBlitterDma<Agnus::PTR_BLT_C_H, !!(jobs & BLT_DESC), true>(bltCpt, bltCdat, bltCmod);
        else
            agnus.fetchBlitterDma<Agnus::PTR_BLT_C_H, !!(jobs & BLT_DESC), true>(bltCpt, bltCdat);

        // if constexpr (!!(jobs & BLT_DESC))  bltCpt += -2;
        // else                                bltCpt += +2;
        //
        // if (curW == 1) {
        //     if constexpr (!!(jobs & BLT_DESC))  bltCpt += -bltCmod;
        //     else                                bltCpt += bltCmod;
        // }
    } else if constexpr (!!( jobs & BLT_WRITE_D )) {
        if(curW == bltSizeW)
            agnus.writeBlitterDma<!!(jobs & BLT_DESC), true>(bltDpt, doff ? agnus.dataBus : bltDdat, bltDmod);
        else
            agnus.writeBlitterDma<!!(jobs & BLT_DESC), true>(bltDpt, doff ? agnus.dataBus : bltDdat);

        // if constexpr (!!(jobs & BLT_DESC))  bltDpt += -2;
        // else                                bltDpt += 2;
        //
        // if (curW == bltSizeW) {
        //     if constexpr (!!(jobs & BLT_DESC))  bltDpt += -bltDmod;
        //     else                                bltDpt += bltDmod;
        // }
    } /*else if constexpr ((jobs & BLT_IDLE) == 0) {
        if (!agnus.canBlitterUseBus())
            return;
    }*/

    if constexpr (!!(jobs & BLT_IDLE)) {
        if (!agnus.aga() || !nextCycle) {
            if (!restartTimer)
                busy = false;
            finish();
        }
    }

    if constexpr (!!(jobs & BLT_SHIFT_A)) {
        uint16_t mask = 0xffff;
        if (curW == bltSizeW)   mask &= bltAfwm;
        if (curW == 1)          mask &= bltAlwm;

        if constexpr (!!(jobs & BLT_DESC))
            bltADatShifted = (((bltAdat & mask) << 16) | bltADatOld) >> (16 - SHIFTA);
        else
            bltADatShifted = ((bltADatOld << 16) | (bltAdat & mask) ) >> SHIFTA;

        bltADatOld = bltAdat & mask;

    } else if constexpr (!!(jobs & BLT_SHIFT_B)) {
        if constexpr (!!(jobs & BLT_DESC))
            bltBDatShifted = ((bltBdat << 16) | bltBDatOld) >> (16 - SHIFTB);
        else
            bltBDatShifted = ((bltBDatOld << 16) | bltBdat) >> SHIFTB;

        bltBDatOld = bltBdat;

    } else if constexpr (!!(jobs & BLT_CALC)) {
        bltDdat = doMinterm(bltcon0 & 0xff, bltADatShifted, bltBDatShifted, bltCdat);

        if constexpr (!!(jobs & BLT_FILL)) {
            #define isExclusiveFill (bltcon1 & 0x10)

            uint16_t resultLo = fill[ (fillCarry << 9) | (isExclusiveFill << 4) | (bltDdat & 0xff)];
            fillCarry = resultLo >> 8;
            uint16_t resultHi = fill[ (fillCarry << 9) | (isExclusiveFill << 4) | ((bltDdat >> 8) & 0xff)];

            bltDdat = (resultLo & 0xff) | ((resultHi & 0xff) << 8);

            if (curW == bltSizeW)
                fillCarry = isFci();
            else
                fillCarry = resultHi >> 8;
        }

        if (bltDdat) zero = false;
    }

    if constexpr (!!(jobs & BLT_NEXT)) {
        if (!--curW) {
            curW = bltSizeW;

            if (!--curH) {
                if (!agnus.aga() && !restartTimer)
                    busy = false;
                flags = (flags & 0xfff0) | (8 | 5); // shift out and cycle 5
                return;
            }
        }

        flags = (flags & 0xfff0) | (8 | 1); // shift out and cycle 1
    } else {
        flags = (flags & 0xfff8) | nextCycle;

        if constexpr (nextCycle == 0) {
            if (agnus.aga()) {
                if (!restartTimer)
                    busy = false;
                finish();
            }
            flags = 0;
        }
    }
}

// line draw
// following is valid for a horizontal blit size of two, state of channel D in bltcon0 will not be evaluated.

//  A,B,C       first                       last
//  Cycle 1     BH*, ShiftA                 CalcD, LineX
//  Cycle 2     FetchB (dummy)              FetchB, B_MOD
//  Cycle 3     FetchC, ShiftB              WriteD, LineY, Next

//  A,C         first                       last
//  Cycle 1     BH*, ShiftA                 CalcD, LineX
//  Cycle 2     FetchC, ShiftB              WriteD, LineY, Next

//  A,B         first                       last
//  Cycle 1     BH*, ShiftA                 CalcD
//  Cycle 2     FetchB (dummy)              FetchB, B_MOD
//  Cycle 3     ShiftB                      Next

//  A           first                       last
//  Cycle 1     BH*, ShiftA                 CalcD
//  Cycle 2     ShiftB                      Next

// (*) happens at beginning of second horizontal blit
// if channel A is not in use, Bresenham logic in cycle 1 of first blit never happens. no other changes

template<uint16_t jobs, uint8_t nextCycle> auto Blitter::lineMode() -> void {
    if constexpr ((jobs & BLT_IDLE) == 0) {
        if (!agnus.canBlitterUseBus())
            return;
    }

    if constexpr (!!(jobs & BLT_FETCH_B)) {

        if constexpr (!!(jobs & BLT_B_MOD))
            agnus.fetchBlitterDma<Agnus::PTR_BLT_B_H, false, false>(bltBpt, bltBdat, bltBmod);
        else
            agnus.fetchBlitterDma<Agnus::PTR_BLT_B_H, false, false>(bltBpt, bltBdat);

        // if (!agnus.fetchBlitterDma<Agnus::PTR_BLT_B_H>(bltBpt, bltBdat))
        //     return;
        //
        // if constexpr (!!(jobs & BLT_B_MOD))
        //     bltBpt += bltBmod;

    } else if constexpr (!!(jobs & BLT_FETCH_C)) {
        agnus.fetchBlitterDma<Agnus::PTR_BLT_C_H, false, false>(bltCpt, bltCdat);
        //if (!agnus.fetchBlitterDma<Agnus::PTR_BLT_C_H>(bltCpt, bltCdat))
          //  return;

    } else if constexpr (!!(jobs & BLT_WRITE_D)) {
        if (writeLineDot) {
            agnus.writeBlitterDma<false, false>(bltDpt, doff ? agnus.dataBus : bltDdat);
            //if (!agnus.writeBlitterDma(bltDpt, doff ? agnus.dataBus : bltDdat))
              //  return;
        } //else if (!agnus.canBlitterUseBus())
            //return;

    } /*else if constexpr ((jobs & BLT_IDLE) == 0) {
        if (!agnus.canBlitterUseBus())
            return;
    }*/

    if constexpr (!!(jobs & BLT_SHIFT_A)) {
        bltADatShifted = (bltAdat & bltAfwm) >> SHIFTA;

        // should get his own job name
        writeLineDot = !(bltcon1 & BLT_SING) || !oneDotPerLine;
        oneDotPerLine = true;

    } else if constexpr (!!(jobs & BLT_SHIFT_B)) {
        bltBDatShifted = ((bltBdat << 16) | bltBdat) >> SHIFTB;

        if ((bltcon1 & 0xf000) == 0) {
            bltcon1 |= 0xf000;
        } else
            bltcon1 -= 0x1000;
    }

    if constexpr (!!(jobs & BLT_CALC)) {
        bltDdat = doMinterm(bltcon0 & 0xff, bltADatShifted, (bltBDatShifted & 1) ? 0xffff : 0, bltCdat);
        if (bltDdat) zero = false;
    }

    if constexpr (!!(jobs & BLT_LINE_X)) { // happens only if channel C is in use
        agnus.forceOneCycleEvent(Agnus::PTR_BLT_C_H);

#define LINE_DECX   { if ((bltcon0 & 0xf000) == 0) { \
                        bltCpt -= 2;    \
                        bltcon0 |= 0xf000;  \
                    } else bltcon0 -= 0x1000; }

#define LINE_INCX   { if ((bltcon0 & 0xf000) == 0xf000) { \
                        bltCpt += 2;    \
                        bltcon0 &= 0x0fff;  \
                    } else bltcon0 += 0x1000; }

        if (bltcon1 & BLT_SUD) {
            if (bltcon1 & BLT_AUL)  LINE_DECX
            else                    LINE_INCX
        } else {
            if (!(bltcon1 & BLT_SIGN)) {
                if (bltcon1 & BLT_SUL)  LINE_DECX
                else                    LINE_INCX
            }
        }
    } else if constexpr (!!(jobs & BLT_LINE_Y)) { // happens only if channel C is in use
        agnus.forceOneCycleEvent(Agnus::PTR_BLT_C_H);

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

    // following two jobs occur after first shift out, like the first D-Write in block mode
    if constexpr (!!(jobs & BLT_BH)) { // Bresenham slope logic, happens only if channel A is in use
        agnus.forceOneCycleEvent(Agnus::PTR_BLT_A_H);

        if (bltcon1 & BLT_SIGN)
            bltApt += (int16_t)bltBmod;
        else
            bltApt += (int16_t)bltAmod;
    }

    if constexpr (!!(jobs & BLT_UPDATE_SIGN)) { // don't need an active channel A (todo: probably happen in cycle 2)
        if (0 > (int16_t)bltApt)    bltcon1 |= BLT_SIGN;
        else                        bltcon1 &= ~BLT_SIGN;
    }

    if constexpr (!!(jobs & BLT_NEXT)) {
        bltDpt = bltCpt;

        if (!--curH) {
            if(!restartTimer)
                busy = false;
            flags = (flags & 0xfff8) | 7;
            return;
        }

        flags = (flags & 0xfff0) | (8 | 1); // shift out and cycle 1
    } else if constexpr ((jobs & BLT_IDLE) == 0) {
        flags = (flags & 0xfff8) | nextCycle;
    } else {
        finish();
        flags = 0;
    }
}

}
