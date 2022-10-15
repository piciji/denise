
#include "agnus.h"

namespace LIBAMI {

auto Agnus::setBpl1ptH(uint16_t value) -> void {
    bpl1pt &= 0xffff;
    bpl1pt |= value << 16;
    bpl1pt &= chipMemMask;
}

auto Agnus::setBpl1ptL(uint16_t value) -> void {
    bpl1pt &= ~0xffff;
    bpl1pt |= value & 0xfffe;
}

auto Agnus::setBpl2ptH(uint16_t value) -> void {
    bpl2pt &= 0xffff;
    bpl2pt |= value << 16;
    bpl2pt &= chipMemMask;
}

auto Agnus::setBpl2ptL(uint16_t value) -> void {
    bpl2pt &= ~0xffff;
    bpl2pt |= value & 0xfffe;
}

auto Agnus::setBpl3ptH(uint16_t value) -> void {
    bpl3pt &= 0xffff;
    bpl3pt |= value << 16;
    bpl3pt &= chipMemMask;
}

auto Agnus::setBpl3ptL(uint16_t value) -> void {
    bpl3pt &= ~0xffff;
    bpl3pt |= value & 0xfffe;
}

auto Agnus::setBpl4ptH(uint16_t value) -> void {
    bpl4pt &= 0xffff;
    bpl4pt |= value << 16;
    bpl4pt &= chipMemMask;
}

auto Agnus::setBpl4ptL(uint16_t value) -> void {
    bpl4pt &= ~0xffff;
    bpl4pt |= value & 0xfffe;
}

auto Agnus::setBpl5ptH(uint16_t value) -> void {
    bpl5pt &= 0xffff;
    bpl5pt |= value << 16;
    bpl5pt &= chipMemMask;
}

auto Agnus::setBpl5ptL(uint16_t value) -> void {
    bpl5pt &= ~0xffff;
    bpl5pt |= value & 0xfffe;
}

auto Agnus::setBpl6ptH(uint16_t value) -> void {
    bpl6pt &= 0xffff;
    bpl6pt |= value << 16;
    bpl6pt &= chipMemMask;
}

auto Agnus::setBpl6ptL(uint16_t value) -> void {
    bpl6pt &= ~0xffff;
    bpl6pt |= value & 0xfffe;
}

template<uint8_t pos, bool addMod> auto Agnus::fetchPlane() -> void {
    if constexpr ( pos == 1) {
        plane1dat = _swapWord(*(uint16_t*) (chipMem + bpl1pt));
        bpl1pt += 2;
        if constexpr (addMod) bpl1pt += bpl1Mod;
        bpl1pt &= chipMemMask;
        if ((getActiveEvent<EVENT_ONE_CYCLE_DELAY>() & ~1) == PTR_BPL_1_H)
            setEventInactive<EVENT_ONE_CYCLE_DELAY>();
    } else if constexpr ( pos == 2) {
        plane2dat = _swapWord(*(uint16_t*) (chipMem + bpl2pt));
        bpl2pt += 2;
        if constexpr (addMod) bpl2pt += bpl2Mod;
        bpl2pt &= chipMemMask;
        if ((getActiveEvent<EVENT_ONE_CYCLE_DELAY>() & ~1) == PTR_BPL_2_H)
            setEventInactive<EVENT_ONE_CYCLE_DELAY>();
    } else if constexpr ( pos == 3) {
        plane3dat = _swapWord(*(uint16_t*) (chipMem + bpl3pt));
        bpl3pt += 2;
        if constexpr (addMod) bpl3pt += bpl1Mod;
        bpl3pt &= chipMemMask;
        if ((getActiveEvent<EVENT_ONE_CYCLE_DELAY>() & ~1) == PTR_BPL_3_H)
            setEventInactive<EVENT_ONE_CYCLE_DELAY>();
    } else if constexpr ( pos == 4) {
        plane4dat = _swapWord(*(uint16_t*) (chipMem + bpl4pt));
        bpl4pt += 2;
        if constexpr (addMod) bpl4pt += bpl2Mod;
        bpl4pt &= chipMemMask;
        if ((getActiveEvent<EVENT_ONE_CYCLE_DELAY>() & ~1) == PTR_BPL_4_H)
            setEventInactive<EVENT_ONE_CYCLE_DELAY>();
    } else if constexpr ( pos == 5) {
        plane5dat = _swapWord(*(uint16_t*) (chipMem + bpl5pt));
        bpl5pt += 2;
        if constexpr (addMod) bpl5pt += bpl1Mod;
        bpl5pt &= chipMemMask;
        if ((getActiveEvent<EVENT_ONE_CYCLE_DELAY>() & ~1) == PTR_BPL_5_H)
            setEventInactive<EVENT_ONE_CYCLE_DELAY>();
    } else if constexpr ( pos == 6) {
        plane6dat = _swapWord(*(uint16_t*) (chipMem + bpl6pt));
        bpl6pt += 2;
        if constexpr (addMod) bpl6pt += bpl2Mod;
        bpl6pt &= chipMemMask;
        if ((getActiveEvent<EVENT_ONE_CYCLE_DELAY>() & ~1) == PTR_BPL_6_H)
            setEventInactive<EVENT_ONE_CYCLE_DELAY>();
    }

    busUsage = BUS_USAGE_BPL;
}

#define UseBpl0(c)  case c | 0x8000:
#define UseBpl1(c)  case c | 0x8100:
#define UseBpl2(c)  case c | 0x8200:
#define UseBpl3(c)  case c | 0x8300:
#define UseBpl4(c)  case c | 0x8400:
#define UseBpl5(c)  case c | 0x8500:
#define UseBpl6(c)  case c | 0x8600:
#define UseBpl7(c)  case c | 0x8700:

#define UseBpl0Mod(c) UseBpl0(c | 0x40)
#define UseBpl1Mod(c) UseBpl1(c | 0x40)
#define UseBpl2Mod(c) UseBpl2(c | 0x40)
#define UseBpl3Mod(c) UseBpl3(c | 0x40)
#define UseBpl4Mod(c) UseBpl4(c | 0x40)
#define UseBpl5Mod(c) UseBpl5(c | 0x40)
#define UseBpl6Mod(c) UseBpl6(c | 0x40)
#define UseBpl7Mod(c) UseBpl7(c | 0x40)

#define ALL_BPL(c) UseBpl0(c) UseBpl1(c) UseBpl2(c) UseBpl3(c) UseBpl4(c) UseBpl5(c) UseBpl6(c) UseBpl7(c)
#define ALL_BPL_MOD(c) ALL_BPL(c | 0x40)

// Bits 0 - 2: cycle, Bit 3: overflow, Bit 4: lores/hires, Bit 5: Shires(reserved), Bit 6: add mod,
// Bit 7: reserved, Bits 8 - 10: useBpl

template<bool onlyProgressQueue> auto Agnus::fetchPlanes() -> void {

    if constexpr (!onlyProgressQueue) {
        switch(bplCycle++) {
            case 0xfff0:
            case 0xfff1:
            case 0xfff2: break;
            case 0xfff3:
                actions &= ~ACT_BPL;
                return;

            ALL_BPL(0) ALL_BPL_MOD(0)
                break;

            UseBpl4(1) UseBpl5(1) UseBpl6(1) UseBpl7(1)
                bplQueue |= 4 << 24; break;

            UseBpl4Mod(1) UseBpl5Mod(1) UseBpl6Mod(1) UseBpl7Mod(1)
                bplQueue |= 0x84 << 24; break;

            UseBpl6(2)
                bplQueue |= 6 << 24; break;

            UseBpl6Mod(2)
                bplQueue |= 0x86 << 24; break;

            UseBpl2(3) UseBpl3(3) UseBpl4(3) UseBpl5(3) UseBpl6(3) UseBpl7(3)
                bplQueue |= 2 << 24; break;

            UseBpl2Mod(3) UseBpl3Mod(3) UseBpl4Mod(3) UseBpl5Mod(3) UseBpl6Mod(3) UseBpl7Mod(3)
                bplQueue |= 0x82 << 24; break;

            ALL_BPL(4) ALL_BPL_MOD(4)
                break;

            UseBpl3(5) UseBpl4(5) UseBpl5(5) UseBpl6(5) UseBpl7(5)
                bplQueue |= 3 << 24; break;

            UseBpl3Mod(5) UseBpl4Mod(5) UseBpl5Mod(5) UseBpl6Mod(5) UseBpl7Mod(5)
                bplQueue |= 0x83 << 24; break;

            UseBpl5(6) UseBpl6(6)
                bplQueue |= 5 << 24; break;

            UseBpl5Mod(6) UseBpl6Mod(6)
                bplQueue |= 0x85 << 24; break;

            UseBpl1(7) UseBpl2(7) UseBpl3(7) UseBpl4(7) UseBpl5(7) UseBpl6(7) UseBpl7(7)
                bplQueue |= 1 << 24;
                bplCycle &= ~15;
                if (stopFetching)
                    bplCycle |= 0x40;
                break;

            UseBpl1Mod(7) UseBpl2Mod(7) UseBpl3Mod(7) UseBpl4Mod(7) UseBpl5Mod(7) UseBpl6Mod(7) UseBpl7Mod(7)
                bplQueue |= 0x81 << 24;
                stopFetching = false;
                bplCycle = 0xfff0;
                bplActive = false;
                bplFetchPossible = false;
                break;

            UseBpl0(7)
                bplCycle &= ~15;
                if (stopFetching)
                    bplCycle |= 0x40;
                break;

            UseBpl0Mod(7)
                stopFetching = false;
                bplCycle = 0xfff0;
                bplActive = false;
                bplFetchPossible = false;
                break;
        }
    }

    switch(bplQueue & 0xff) {
        case 0: break;
        case 1: fetchPlane<1, false>(); break;
        case 2: fetchPlane<2, false>(); break;
        case 3: fetchPlane<3, false>(); break;
        case 4: fetchPlane<4, false>(); break;
        case 5: fetchPlane<5, false>(); break;
        case 6: fetchPlane<6, false>(); break;

        case 0x81: fetchPlane<1, true>(); break;
        case 0x82: fetchPlane<2, true>(); break;
        case 0x83: fetchPlane<3, true>(); break;
        case 0x84: fetchPlane<4, true>(); break;
        case 0x85: fetchPlane<5, true>(); break;
        case 0x86: fetchPlane<6, true>(); break;
    }

    bplQueue >>= 8;
}

auto Agnus::updateDdfEvent(uint8_t hComp) -> void {
    if ((ddfStart > hComp) && (ddfStop > hComp)) {
        if ( ddfStop < ddfStart)
            updateEvent<EVENT_BPL>(~0, ddfStop - hComp);
        else
            updateEvent<EVENT_BPL>(~0, ddfStart - hComp);

    } else if (ddfStart > hComp)
        updateEvent<EVENT_BPL>(~0, ddfStart - hComp);

    else if (ddfStop > hComp)
        updateEvent<EVENT_BPL>(~0, ddfStop - hComp);
}

}