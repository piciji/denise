
#include "copper.h"
#include "agnus.h"

namespace LIBAMI {

Copper::Copper(Agnus& agnus) : agnus(agnus), blitter(agnus.blitter) {}

inline auto Copper::allocationCycle() -> bool {
    if (prevState & (0x40 | 0x80))
        return ((prevState & 0x80) != 0) || !agnus.copperLongGap();

    return false;
}

auto Copper::process() -> void {

    switch(state) {
        case Off:
            break;
        case Strobe0: // CPU writes to CopJmp
            // when CPU writes to CopJmp an already pipelined DMA access consumes a cycle, but do nothing
            if (allocationCycle())
                agnus.allocateCopper();

            state = Strobe1;
            break;
        case Strobe0Self: // Copper writes to CopJmp
            if (agnus.canCopperUseBus()) {
                // when Copper writes to register the next cycle would be Read1, if BUS is free. a Copper Write to CopJmp can't prevent a possible pipelined Read1 cycle
                agnus.fetchCopperDmaNoBUSCheck(copPtr, ir1);
                copPtr += 2;
                state = Strobe3;
            } else
                state = Strobe2;
            break;
        case Strobe1: // Copper or CPU write to CopJmp when Copper is waiting
            state = Strobe2;
            break;
        case Strobe2:
            if (agnus.canCopperUseBus()) {
                if (agnus.copperLongGap()) { // two wait cycles in a row between Strobe1 and Strobe2, instead of one
                    if (useCop1)    copPtr = cop1lc;
                    else            copPtr = cop2lc;
                    state = Read1;
                } else {
                    agnus.fetchCopperDmaNoBUSCheck(copPtr, ir1);
                    copPtr += 2;
                    state = Strobe3;
                }
            } else
                state = Strobe3;
            break;
        case Strobe2Vsync: // Vsync triggers CopJmp
            state = Strobe3;
            if (!allocationCycle())
                break;
            // fallthrough
        case Strobe3:
            if (agnus.fetchCopperDma(copPtr, ir2)) {
                if (useCop1)    copPtr = cop1lc;
                else            copPtr = cop2lc;
                state = Read1;
            }
            break;

        case Strobe3Vsync: // Vsync triggers CopJmp when Copper read instructions
            state = Strobe4Vsync;
            if (!allocationCycle())
                break;
            // fallthrough
        case Strobe4Vsync:
            if (agnus.fetchCopperDma(copPtr, ir2)) {
                if (useCop1)    copPtr = cop1lc;
                else            copPtr = cop2lc;
                state = Strobe5Vsync;
            }
            break;

        case Strobe5Vsync:
            if (agnus.allocateCopper())
                state = Read1;
            break;

        case Strobe1Unaligned: // CPU write to CopJmp when Copper is waiting (the write happens between the Copper cycles)
            state = Strobe2Unaligned;
            break;

        case Strobe2Unaligned:
            if (agnus.canCopperUseBus())
                state = Strobe3Unaligned;
            else
                state = Strobe3;
            break;

        case Strobe3Unaligned:
            if (agnus.allocateCopper()) {
                if (useCop1)    copPtr = cop1lc;
                else            copPtr = cop2lc;
                state = Read1;
            }
            break;

        case Read1:
        case Read1AfterSkip:
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
                        state = Strobe2;
                    } else if (reg == 0x8a) {
                        useCop1 = false;
                        state = Strobe2;
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
                skipped = !compare<false>();
                state = Read1AfterSkip;
            }
            break;
        case Wait1:
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

    // it seems a Blitter Busy change is detected in non Copper cycle too.
    agnus.actions |= Agnus::ACT_COPPER;

    if (state == Read1AfterSkip) {
        skipped = !compare<false>();
    } else if (state == Wait2) {
        if (compare())
            state = Wait3;
    } else if (state == Wait3) {
        if (!compare())
            state = Wait2;
    }
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

    if (hPos >= comp.hPos)
        return true;

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
        if (state == Read1 || state == Read2 || state == Read1AfterSkip)
            state = Strobe3Vsync;
        else
            state = Strobe2Vsync;
    } else if ((state == Wait1 || state == Wait2 || state == Wait4) && agnus.useCopperDMA()) {
        if (triggeredBy == Agnus::Trigger_Copper)
            state = Strobe1;
        else
            state = (agnus.hPos & 1) ? Strobe1Unaligned : Strobe1;
    } else if (triggeredBy == Agnus::Trigger_CPU) {
        prevState = state;
        state = Strobe0;
    } else /*if (triggeredBy == Agnus::Trigger_Copper) */ {
        state = Strobe0Self;
    }

    useCop1 = firstLocation;
    agnus.actions |= Agnus::ACT_COPPER;
}

auto Copper::reset() -> void {
    state = Off;
    prevState = Off;
    skipped = false;
    cdang = 0x80;
    useCop1 = false;
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
    s.integer(ir1);
    s.integer(ir2);
    s.integer(comp.hPos);
    s.integer(comp.vPos);
    s.integer(comp.hMask);
    s.integer(comp.vMask);
    s.integer(skipped);
}

}
