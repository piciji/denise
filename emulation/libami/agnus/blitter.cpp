
//#define START_WITH_STATE_MACHINE

#include "../interface.h"
#include "blitter.h"
#include "agnus.h"
#include "copper.h"
#include "../../tools/serializer.h"
#include "minterm.cpp"
#include "blitterCycle.cpp"
#include "blitterSequencer.cpp"
#include "blitterLLE.cpp"
#include "../paula/paula.h"

namespace LIBAMI {

Blitter::Blitter(Agnus& agnus) : agnus(agnus), copper(agnus.copper), paula(agnus.paula) {
    preFill();
    prepareChannel();
}

auto Blitter::initBlit() -> void {
    bltADatOld = 0;
    bltBDatOld = 0;
    curW = bltSizeW;
    curH = bltSizeH;

    if (flags & LLE) // LLE flag is removed when the final cycles begin.
        flags = 0;
    else if (flags & LINE_MODE) {
        // allow final slope calculation
        if((flags & 7) != 7)
            flags = 0;
    } else {
        // when the cycle after writing to blit size is calculation of "Final D" or later, then last D will be written (if allowed)
        if((flags & 7) < 5)
            flags = 0;
    }

    restartTimer = 2;
    agnus.actions |= Agnus::ACT_BLITTER;
}

auto Blitter::startBlit() -> void {
    if (restartTimer) {
        if ( (agnus.busUsage >= Agnus::BUS_USAGE_BLITTER_A && agnus.busUsage <= Agnus::BUS_USAGE_BLITTER_CONFLICT_SPRITE)
            || agnus.canBlitterUseBusExt() ) {
            if (!busy && (agnus.model == Agnus::OCS_A1000) ) // if A1000 Blitter get first cycle
                busy = true;

            if (--restartTimer == 0) {
                fillCarry = isFci();
                oneDotPerLine = false;

                if (bltcon1 & 1) { // line draw
#ifndef START_WITH_STATE_MACHINE
                    if (bltSizeW == 2)
                        flags = ((bltcon0 >> 4) & 0xf0) | LINE_MODE | 1;
                    else // start with LLE
#endif
                    {
                        shifter = 1 | LINE_MODE;
                        shifter |= ((bltcon0 >> 4) & 0xf0);
                        shiftOut = false;
                        skipB = curSkipB = hasSkipB();
                        skipY = curSkipY = true; // always skipped for line draw
                        flags = LLE;
                    }
                } else {
#ifdef START_WITH_STATE_MACHINE
                    shifter = 1;
                    shifter |= ((bltcon0 >> 4) & 0xf0);
                    if (hasFillmodeIdle())
                        shifter |= FILL_IDLE;
                    shiftOut = false;
                    skipB = curSkipB = hasSkipB();
                    skipY = curSkipY = hasSkipY();
                    flags = LLE;
#else
                    flags = ((bltcon0 >> 4) & 0xf0) | (desc << 9) | (isFillMode() << 8) | 1;
#endif
                }

                agnus.actions |= Agnus::ACT_BLITTER;
            }
        }
    } else if (!flags)
        agnus.actions &= ~Agnus::ACT_BLITTER;
}

auto Blitter::preFill() -> void {
    uint16_t mask;
    bool carry;

    uint8_t inclusive;
    uint8_t exclusive;
    uint8_t inclusiveFCI;
    uint8_t exclusiveFCI;

    for(unsigned data = 0; data <= 0xff; data++ ) {
        inclusive = data;
        exclusive = data;
        inclusiveFCI = data;
        exclusiveFCI = data;
        carry = false;

        for (mask = 1; mask != 0x100; mask <<= 1) {
            if (carry) {
                inclusive |= mask; // fills inside area, "inclusive" only change 0 -> 1 ... means border will never remove
                exclusive ^= mask; // exclusive mode toggles between 0 <> 1 ... means left border will be deleted, because carry switch happens afterward
            } else {
                inclusiveFCI |= mask; // fills outside area, "inclusive" only change 0 -> 1 ... means border will never remove
                exclusiveFCI ^= mask; // exclusive mode toggles between 0 <> 1 ... means right border will be deleted, because carry switch happens afterward
            }

            if (data & mask) carry ^= 1;
        }

        fill[      data] = (carry << 8) | inclusive;
        fill[256 + data] = (carry << 8) | exclusive;
        carry ^= 1;
        fill[512 + data] = (carry << 8) | inclusiveFCI;
        fill[768 + data] = (carry << 8) | exclusiveFCI;
    }
}

auto Blitter::hasFillmodeIdle() -> bool {
    return isFillMode() && ((bltcon0 & 0x300) == 0x100);
}

auto Blitter::hasSkipB() -> bool {
    return (bltcon0 & 0x400) == 0;
}

auto Blitter::hasSkipY() -> bool {
    bool skip = (bltcon0 & 0x300) != 0x300;
    if (hasFillmodeIdle()) skip = false;
    return skip;
}

auto Blitter::activateLLEWhenNeeded(uint8_t bltRegister, uint16_t value) -> void {
    if (flags == LLE)
        return; // already activated

    if (!(agnus.actions & Agnus::ACT_BLITTER) || !flags)
        return; // LLE is only needed when channel/line/fill mode is changed while Blitter is running

    uint8_t cycle = flags & 7;

    if (bltRegister == 0x40) { // bltcon0
        if ((value & 0xf00) == (bltcon0 & 0xf00))
            return;

        if (flags & LINE_MODE) {
            if (cycle == 7)
                return;
        } else {
            if (cycle >= 5) // final D calc or final D write
                return;
        }

        skipB = curSkipB = hasSkipB();
        skipY = curSkipY = (bltcon1 & 1) ? true : hasSkipY();

    } else if (bltRegister == 0x42) { // bltcon1
        curSkipY = (bltcon1 & 1) ? true : hasSkipY();

        if (((bltcon1 ^ value) & 1) == 0) { // no toggling of block <> line
            if ((value & 1) == 0) { // keeps block mode. check for possible change of "fill" which could cause a cycle change
                if (cycle >= 5)
                    return;

                uint16_t tmp = bltcon1;
                bltcon1 = value;
                skipY = hasSkipY();
                bltcon1 = tmp;

                if (skipY == curSkipY)
                    return;
            } else
                return; // keep line mode

        } else { // toggling of block <> line
            if (value & 1) { // switch block to line
                if (cycle >= 5) {
                    if (cycle == 5)
                        flags = 0;
                    return;
                }
            } else { // switch line to block
                if (cycle == 7) {
                    return;
                }
            }
        }

        skipY = curSkipY;
        skipB = curSkipB = hasSkipB();

    } else // no critical registers are changed
        return;

    // now we have to create the initial state for the shifter logic, as if it was already running from the beginning.
    // it is important that the old register values for Bltcon 0/1 are used, as the changed values are only adopted at a certain point.
    if (bltcon1 & 1) {
        if (skipB && cycle > 2) {
            curW = 1;
            cycle -= 2;
        } else if (!skipB && cycle > 3) {
            curW = 1;
            cycle -= 3;
        } else
            curW = 2;
    }

    if (skipB && (cycle > 1))
        shifter = 1 << cycle;
    else
        shifter = 1 << (cycle - 1);

    shifter |= ((bltcon0 >> 4) & 0xf0);

    if (((bltcon1 & 1) == 0) && hasFillmodeIdle())
        shifter |= 0x100;

    if (flags & 8)
        shifter |= 0x200; // first shift out already happened, means a "one" was shifted out at least one time.

    shiftOut = cycle == 1;

    flags = LLE; // activate LLE
    // agnus.interface->log("LLE");
}

auto Blitter::finish() -> void {
    copper.blitterBusyUpdate();
    paula.scheduleIntreqBlt(); // Agnus generates one DMA cycle pulse
}

auto Blitter::reset() -> void {
    bltcon0 = 0;
    bltcon1 = 0;
    zero = true;
    busy = false;
    bltADatOld = 0;
    bltBDatOld = 0;
    desc = false;
    doff = false;
    flags = 0;
    restartTimer = 0;

    bltApt = 0;
    bltBpt = 0;
    bltCpt = 0;
    bltDpt = 0;

    bltAdat = 0;
    bltBdat = 0;
    bltCdat = 0;
    bltDdat = 0;

    bltADatShifted = 0;
    bltBDatShifted = 0;

    bltAfwm = 0;
    bltAlwm = 0;

    bltAmod = 0;
    bltBmod = 0;
    bltCmod = 0;
    bltDmod = 0;
}

auto Blitter::setBltCon0(uint16_t value) -> void {
    activateLLEWhenNeeded( 0x40, value );
    bltcon0 = value;

    if (flags == LLE)
        shifter |= STAGE_CHANGE;
    else {
        if ((flags & 7) == ((flags & LINE_MODE) ? 7 : 5)) {
            flags &= ~0xf0;
            flags |= ((bltcon0 >> 4) & 0xf0);
        }
    }
}

auto Blitter::setBltCon0L(uint16_t value) -> void {
    bltcon0 &= ~0xff;
    bltcon0 |= value & 0xff;
}

auto Blitter::setBltCon1(uint16_t value) -> void {
    activateLLEWhenNeeded( 0x42, value );
    bltcon1 = value;
    desc = value & 2;
    doff = agnus.ecsAndHigher() && (bltcon1 & 0x80);

    if (flags == LLE)
        shifter |= STAGE_CHANGE;
    else if (flags) {
        flags &= ~0x300;
        if ((flags & LINE_MODE) == 0) {
            if (desc)           flags |= 0x200;
            if (isFillMode())   flags |= 0x100;
        }
    }
}

auto Blitter::setBltAfwm(uint16_t value) -> void {
    bltAfwm = value;
}

auto Blitter::setBltAlwm(uint16_t value) -> void {
    bltAlwm = value;
}

auto Blitter::setBltCptH(uint16_t value) -> void {
    bltCpt &= 0xffff;
    bltCpt |= value << 16;
    bltCpt &= agnus.dmaChipMemMask;
}

auto Blitter::setBltCptL(uint16_t value) -> void {
    bltCpt &= ~0xffff;
    bltCpt |= value & 0xfffe;
}

auto Blitter::setBltBptH(uint16_t value) -> void {
    bltBpt &= 0xffff;
    bltBpt |= value << 16;
    bltBpt &= agnus.dmaChipMemMask;
}

auto Blitter::setBltBptL(uint16_t value) -> void {
    bltBpt &= ~0xffff;
    bltBpt |= value & 0xfffe;
}

auto Blitter::setBltAptH(uint16_t value) -> void {
    bltApt &= 0xffff;
    bltApt |= value << 16;
    bltApt &= agnus.dmaChipMemMask;
}

auto Blitter::setBltAptL(uint16_t value) -> void {
    bltApt &= ~0xffff;
    bltApt |= value & 0xfffe;
}

auto Blitter::setBltDptH(uint16_t value) -> void {
    bltDpt &= 0xffff;
    bltDpt |= value << 16;
    bltDpt &= agnus.dmaChipMemMask;
}

auto Blitter::setBltDptL(uint16_t value) -> void {
    bltDpt &= ~0xffff;
    bltDpt |= value & 0xfffe;
}

auto Blitter::setBltSize(uint16_t value) -> void {
    bltSizeH = value >> 6;
    bltSizeW = value & 0x3F;

    if (!bltSizeH) bltSizeH = 1024;
    if (!bltSizeW) bltSizeW = 64;

    zero = true;
    busy = (agnus.model != Agnus::OCS_A1000) || agnus.useBlitterDMAForQueue();
}

auto Blitter::setBltSizeV(uint16_t value) -> void {
    bltSizeH = value & 0x7fff;
    if (!bltSizeH) bltSizeH = 0x8000;
}

auto Blitter::setBltSizeH(uint16_t value) -> void {
    bltSizeW = value & 0x7ff;
    if (!bltSizeW) bltSizeW = 0x800;

    zero = true;
    busy = (agnus.model != Agnus::OCS_A1000) || agnus.useBlitterDMAForQueue();
}

auto Blitter::setBltCMod(uint16_t value) -> void {
    bltCmod = int16_t(value & 0xfffe);
}

auto Blitter::setBltBMod(uint16_t value) -> void {
    bltBmod = int16_t(value & 0xfffe);
}

auto Blitter::setBltAMod(uint16_t value) -> void {
    bltAmod = int16_t(value & 0xfffe);
}

auto Blitter::setBltDMod(uint16_t value) -> void {
    bltDmod = int16_t(value & 0xfffe);
}

auto Blitter::setBltCDat(uint16_t value) -> void {
    bltCdat = value;
}

auto Blitter::setBltBDat(uint16_t value) -> void {
    bltBdat = value;

    if (desc)
        bltBDatShifted = ((bltBdat << 16) | bltBDatOld) >> (16 - SHIFTB);
    else
        bltBDatShifted = ((bltBDatOld << 16) | bltBdat) >> SHIFTB;

    bltBDatOld = bltBdat;
}

auto Blitter::setBltADat(uint16_t value) -> void {
    bltAdat = value;
}

auto Blitter::serialize(Emulator::Serializer& s) -> void {
    s.integer(bltcon0);
    s.integer(bltcon1);
    s.integer(bltAdat);
    s.integer(bltBdat);
    s.integer(bltCdat);
    s.integer(bltDdat);
    s.integer(bltADatOld);
    s.integer(bltBDatOld);
    s.integer(bltADatShifted);
    s.integer(bltBDatShifted);
    s.integer(bltAfwm);
    s.integer(bltAlwm);
    s.integer(bltApt);
    s.integer(bltBpt);
    s.integer(bltCpt);
    s.integer(bltDpt);
    s.integer(bltAmod);
    s.integer(bltBmod);
    s.integer(bltCmod);
    s.integer(bltDmod);
    s.integer(bltSizeW);
    s.integer(bltSizeH);
    s.integer(curW);
    s.integer(curH);
    s.integer(fillCarry);
    s.integer(busy);
    s.integer(zero);
    s.integer(desc);
    s.integer(flags);
    s.integer(restartTimer);
    s.integer(writeLineDot);
    s.integer(oneDotPerLine);
    s.integer(skipB);
    s.integer(skipY);
    s.integer(curSkipB);
    s.integer(curSkipY);
    s.integer(shifter);
    s.integer(shiftOut);
    s.integer(doff);
}

}
