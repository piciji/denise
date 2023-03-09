
#include "copper.h"
#include "agnus.h"

namespace LIBAMI {

Copper::Copper(Agnus& agnus) : agnus(agnus), blitter(agnus.blitter) {}

inline auto Copper::allocationCycle() -> bool {
    // if strobe interrupts Copper, an already requested DMA access will be used and saves a Copper cycle
    if (prevState & (0x40 | 0x80))
        return ((prevState & 0x80) != 0) || !agnus.copperLongGap();

    return false;
}

auto Copper::cycle1() -> void { // only gets here, if short line before
    // Copper requests BUS each second cycle, so we end here in a NON Copper cycle
    // if DMA is requested, it does nothing, but prevents Blitter und CPU to use it (waste cycle)
    // following Copper cycle 2 handles the request.
    if(state == Strobe_CPU_6) {
        if (prevState & 0x80)
            agnus.allocateCopper<true>();
    } else if (state & 0x80) {
        agnus.allocateCopper<true>();
    }
}

auto Copper::process() -> void {

    switch(state) {
        case Off:
            break;
// Strobe Vblank
        case Strobe_VBL_1:
            state = Strobe_VBL_2;
            if (!allocationCycle())
                break;
            // take over already requested DMA
            // fallthrough
        case Strobe_VBL_2:
            if (agnus.fetchCopperDma(copPtr, ir2)) {
                if (useCop1)    copPtr = cop1lc;
                else            copPtr = cop2lc;
                state = Read1;
            }
            break;
        case Strobe_VBL_3:
            state = Strobe_VBL_4;
            if (!allocationCycle())
                break;
        case Strobe_VBL_4:
            if (agnus.fetchCopperDma(copPtr, ir1)) {
                copPtr += 2;
                if (useCop1)    copPtr = cop1lc;
                else            copPtr = cop2lc;
                state = Strobe_VBL_7;
            }
            break;
        case Strobe_VBL_5:
            state = Strobe_VBL_6;
            if (!allocationCycle())
                break;
        case Strobe_VBL_6:
            if (agnus.fetchCopperDma(copPtr, ir2)) {
                if (useCop1)    copPtr = cop1lc;
                else            copPtr = cop2lc;
                state = Strobe_VBL_7;
            }
            break;
        case Strobe_VBL_7:
            if (agnus.allocateCopper())
                state = Read1;
            break;
// Strobe Wait
        case Strobe_CPU_1:
            state = Strobe_CPU_2;
            break;
        case Strobe_CPU_2:
            if (agnus.canCopperUseBus()) {
                if (agnus.copperLongGap()) {
                    if (useCop1)    copPtr = cop1lc;
                    else            copPtr = cop2lc;
                    state = Read1;
                } else {
                    agnus.fetchCopperDmaNoBUSCheck(copPtr, ir1); // can not happen in cycle 1 (short lines only)
                    copPtr += 2;
                    state = Strobe_VBL_2;
                }
            } else
                state = Strobe_VBL_2;
            break;
// Strobe Wait Unaligned
        case Strobe_CPU_3:
            state = Strobe_CPU_4;
            break;
        case Strobe_CPU_4:
            if (!agnus.canCopperUseBus()) {
                state = Strobe_VBL_2;
                break;
            }
            state = Strobe_CPU_5;
            // fallthrough
        case Strobe_CPU_5:
            if (agnus.allocateCopper()) {
                if (useCop1)    copPtr = cop1lc;
                else            copPtr = cop2lc;
                state = Read1;
            }
            break;
// Strobe Read
        case Strobe_CPU_6:
            if (allocationCycle())
                agnus.allocateCopper();

            state = Strobe_CPU_1;
            break;

// normal operation
        case Read1:
            if (agnus.fetchCopperDma(copPtr, ir1)) {
                copPtr += 2;
                state = Read2;
            }
            break;
        case Read2:
            if (agnus.fetchCopperDma(copPtr, ir2)) {
                copPtr += 2;

                if (ir1 & 1) { // wait or skip
                    comp.vMask = (ir2 | 0x8000) >> 8; // highest bit is not used for masking
                    comp.hMask = ir2 & 0xfe;

                    comp.vPos = (ir1 & (ir2 | 0x8000)) >> 8;
                    comp.hPos = ir1 & ir2 & 0xfe;

                    state = (ir2 & 1) ? Skip1 : Wait1;
                } else { // move
                    uint16_t reg = ir1 & 0x1fe;

                    if (reg < cdang) {
                        state = Off;
                        agnus.actions &= ~Agnus::ACT_COPPER;
                        break;
                    }

                    if (skipped) {
                        skipped = false;
                        reg = 0x1fe;
                    }

                    if (reg == 0x88) {
                        useCop1 = true;
                        state = Strobe_CPU_2;
                    } else if (reg == 0x8a) {
                        useCop1 = false;
                        state = Strobe_CPU_2;
                    } else {
                        agnus.writeCustom(reg, ir2, Agnus::Trigger_Copper);
                        state = Read1;
                    }
                }
            }
            break;
        case Skip1:
            if (agnus.canCopperUseBus())
                state = Skip2;
            break;
        case Skip2:
            if (agnus.canCopperUseBus()) {
                skipped = compare<false>();
                state = Read1;
            }
            break;
        case Wait1:
            // needs a free cycle before wait (not allocated)
            if (agnus.canCopperUseBus()) {
                if (compare()) {
                    state = Wait3;
                    break;
                }

                if (ir1 == 0xffff && ir2 == 0xfffe) {
                    agnus.actions &= ~Agnus::ACT_COPPER;
                    state = Wait4;
                    break;
                }

                state = Wait2;
            }
            break;
        case Wait2:
            if (compare()) {
                state = Wait3;
                break;
            }
            break;
        case Wait3:
            // needs a free cycle after wait (not allocated)
            if (agnus.canCopperUseBus())
                state = Read1;
            break;
        case Wait4: // would never match
            agnus.actions &= ~Agnus::ACT_COPPER;
            break;
    }
}

auto Copper::blitterBusyUpdate() -> void {
    if (ir2 & 0x8000)
        return; // don't wait for Blitter

    agnus.actions |= Agnus::ACT_COPPER; // wake up Copper
}

template<bool wait> auto Copper::compare() -> bool {
    uint8_t hPos = agnus.hPos & comp.hMask;
    uint16_t vPos = agnus.vPos & comp.vMask;

    if (vPos < comp.vPos) {
        if constexpr (wait)
            agnus.actions &= ~Agnus::ACT_COPPER;

        return false;
    }

    if ((ir2 & 0x8000) == 0) {
        if (blitter.busy) {
            if constexpr (wait)
                agnus.actions &= ~Agnus::ACT_COPPER;

            return false;
        }
    }

    if (vPos > comp.vPos)
        return true;

    if (hPos >= comp.hPos) {
        if (comp.hPos && !agnus.lol && (agnus.hPos == 0xe2))
            // copper compares with the not yet increased horizontal counter, except in the last cycle of a short line. The counter has already jumped to 0 here.
            return false;

        return true;
    }

    return false;
}

auto Copper::setCopCon( uint16_t value ) -> void {
    cdang = (value & 2) ? (agnus.ecsAndHigher() ? 0 : 0x40) : 0x80;
}

auto Copper::setCOP1LCH(uint16_t value) -> void {
    // no need to check if DMA is used next cycle
    cop1lc = (cop1lc & 0xffff) | (value << 16);
}

auto Copper::setCOP1LCL(uint16_t value) -> void {
    cop1lc = (cop1lc & ~0xffff) | (value & 0xfffe);
}

auto Copper::setCOP2LCH(uint16_t value) -> void {
    cop2lc = (cop2lc & 0xffff) | (value << 16);
}

auto Copper::setCOP2LCL(uint16_t value) -> void {
    cop2lc = (cop2lc & ~0xffff) | (value & 0xfffe);
}

auto Copper::strobeCOPJMP(bool firstLocation, uint8_t triggeredBy) -> void {

    if (triggeredBy == Agnus::Trigger_Vsync) {
        prevState = state;

        if (state == Read1) state = Strobe_VBL_3;
        else if (state == Read2) state = Strobe_VBL_5;
        else state = Strobe_VBL_1;

    } else { // from CPU

        if ((state == Wait1 || state == Wait2 || state == Wait4) && agnus.useCopperDMA()) {
            state = (agnus.hPos & 1) ? Strobe_CPU_3 : Strobe_CPU_1;
        } else {
            prevState = state;
            state = Strobe_CPU_6;
        }
    }
    // access from Copper will be handled in "Read2" state

    useCop1 = firstLocation;
    skipped = false;
    agnus.actions |= Agnus::ACT_COPPER;
}

auto Copper::reset() -> void {
    state = Off;
    prevState = Off;
    skipped = false;
    cdang = 0x80;
    useCop1 = true;
    ir1 = ir2 = 0;
    cop1lc = cop2lc = 0;
    copPtr = 0;
}

auto Copper::serialize(Emulator::Serializer& s) -> void {
    s.integer((uint8_t&)state);
    s.integer((uint8_t&)prevState);
    s.integer(cdang);
    s.integer(useCop1);
    s.integer(cop1lc);
    s.integer(cop2lc);
    s.integer(copPtr);
    s.integer(ir1);
    s.integer(ir2);
    s.integer(comp.hPos);
    s.integer(comp.vPos);
    s.integer(comp.hMask);
    s.integer(comp.vMask);
    s.integer(skipped);
}

}
