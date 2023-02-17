
#include "agnus.h"

namespace LIBAMI {

template<uint8_t nr> auto Agnus::setAudPtH(uint16_t value) -> void {
    AudioDmaChannel& cha = audioDmaChannels[nr];
    cha.ptrLatch &= 0xffff;
    cha.ptrLatch |= value << 16;
    cha.ptrLatch &= chipMemMask;
}

template<uint8_t nr> auto Agnus::setAudPtL(uint16_t value) -> void {
    AudioDmaChannel& cha = audioDmaChannels[nr];
    cha.ptrLatch &= ~0xffff;
    cha.ptrLatch |= value & 0xfffe;
}

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

template<uint8_t num> auto Agnus::setSprptH(uint16_t value) -> void {
    sprites[num].ptr &= 0xffff;
    sprites[num].ptr |= value << 16;
    sprites[num].ptr &= chipMemMask;
}

template<uint8_t num> auto Agnus::setSprptL(uint16_t value) -> void {
    sprites[num].ptr &= ~0xffff;
    sprites[num].ptr |= value & 0xfffe;
}

auto Agnus::setDskPtH(uint16_t value) -> void {
    dskpt &= 0xffff;
    dskpt |= value << 16;
    dskpt &= chipMemMask;
}

auto Agnus::setDskPtL(uint16_t value) -> void {
    dskpt &= ~0xffff;
    dskpt |= value & 0xfffe;
}

template<uint8_t pos, bool addMod> auto Agnus::fetchPlane() -> void {
    if constexpr ( pos == 1) {
        dataBus = _swapWord(*(uint16_t*) (chipMem + bpl1pt));
        denise.setBpl1Dat( dataBus );
        bpl1pt += 2;
        if constexpr (addMod) bpl1pt += bpl1Mod;
        bpl1pt &= chipMemMask;
    } else if constexpr ( pos == 2) {
        dataBus = _swapWord(*(uint16_t*) (chipMem + bpl2pt));
        denise.setBpl2Dat( dataBus );
        bpl2pt += 2;
        if constexpr (addMod) bpl2pt += bpl2Mod;
        bpl2pt &= chipMemMask;
    } else if constexpr ( pos == 3) {
        dataBus = _swapWord(*(uint16_t*) (chipMem + bpl3pt));
        denise.setBpl3Dat( dataBus );
        bpl3pt += 2;
        if constexpr (addMod) bpl3pt += bpl1Mod;
        bpl3pt &= chipMemMask;
    } else if constexpr ( pos == 4) {
        dataBus = _swapWord(*(uint16_t*) (chipMem + bpl4pt));
        denise.setBpl4Dat( dataBus );
        bpl4pt += 2;
        if constexpr (addMod) bpl4pt += bpl2Mod;
        bpl4pt &= chipMemMask;
    } else if constexpr ( pos == 5) {
        dataBus = _swapWord(*(uint16_t*) (chipMem + bpl5pt));
        denise.setBpl5Dat( dataBus );
        bpl5pt += 2;
        if constexpr (addMod) bpl5pt += bpl1Mod;
        bpl5pt &= chipMemMask;
    } else if constexpr ( pos == 6) {
        dataBus = _swapWord(*(uint16_t*) (chipMem + bpl6pt));
        denise.setBpl6Dat( dataBus );
        bpl6pt += 2;
        if constexpr (addMod) bpl6pt += bpl2Mod;
        bpl6pt &= chipMemMask;
    }

    busUsage = BUS_USAGE_BPL;
}

auto Agnus::diskDma(bool writeMode) -> void {
    if (writeMode) {
        dataBus = _swapWord(*(uint16_t*) (chipMem + dskpt));
        paula.setDskDat(dataBus);
    } else {
        uint16_t value = paula.dskDatR();
        if (trackMemChanges)
            rememberChipMem(dskpt);

        *(uint16_t*)(chipMem + dskpt) = _swapWord(value);
        dataBus = value;
    }

    inactivateOneCycleEvent(Agnus::PTR_DSK_H);

    dskpt += 2;
    dskpt &= chipMemMask;
    busUsage = BUS_USAGE_DMAL;
}

auto Agnus::fakeDiskDma(uint16_t word) -> void {
    if (trackMemChanges)
        rememberChipMem(dskpt);

    *(uint16_t*)(chipMem + dskpt) = _swapWord(word);
    dskpt += 2;
    dskpt &= chipMemMask;
}

auto Agnus::fakeDiskDmaNoTracking(uint16_t word) -> void {
    if (paula.dmaDisk) {
        *(uint16_t*) (chipMem + dskpt) = _swapWord(word);
        dskpt += 2;
        dskpt &= chipMemMask;
    }
}

auto Agnus::fakeDiskDma() -> uint16_t {
    dataBus = _swapWord(*(uint16_t*) (chipMem + dskpt));
    dskpt += 2;
    dskpt &= chipMemMask;
    return dataBus;
}

template<uint8_t nr> auto Agnus::fetchSample(bool reset) -> void {
    AudioDmaChannel& cha = audioDmaChannels[nr];

    dataBus = _swapWord(*(uint16_t*) (chipMem + cha.ptr));

    if (reset)
        cha.ptr = cha.ptrLatch;
    else
        cha.ptr += 2;

    cha.ptr &= chipMemMask;

    paula.audxDat<nr, true>( dataBus ); // put on RGA BUS

    busUsage = BUS_USAGE_DMAL;
}

template<uint8_t nr, uint8_t target> inline auto Agnus::fetchSprite() -> void {
    dataBus = _swapWord(*(uint16_t*) (chipMem + sprites[nr].ptr));

    if constexpr (target == 0) {
        denise.setSprDatA(nr, dataBus );
    } else if constexpr (target == 1) {
        denise.setSprDatB(nr, dataBus );
    } else if constexpr (target == 2) {
        sprites[nr].pos = dataBus;
        updateSpriteV<nr>();
        denise.setSprPos( nr, sprites[nr].pos );
    } else {
        sprites[nr].ctl = dataBus;
        updateSpriteV<nr>();
        denise.setSprCtl( nr, sprites[nr].ctl );
    }

    sprites[nr].ptr += 2;
    sprites[nr].ptr &= chipMemMask;
    busUsage = BUS_USAGE_SPRITE;
}

template<bool oddCycle1> auto Agnus::canCopperUseBus() -> bool {
    if constexpr (oddCycle1) {
        if (bplQueue & 0xff)
            return false; // a higher DMA
    } else {
        if (busUsage != BUS_FREE)
            return false; // a higher DMA
    }

    if (!useCopperDMA())
        return false;

    return true;
};

template<bool oddCycle1> auto Agnus::allocateCopper() -> bool {
    if (canCopperUseBus<oddCycle1>()) {
        busUsage = BUS_USAGE_COPPER;
        return true;
    }
    return false;
}

auto Agnus::fetchCopperDma(uint32_t adr, uint16_t& result) -> bool {
    if(!canCopperUseBus())
        return false;

    busUsage = BUS_USAGE_COPPER;

    result = _swapWord(*(uint16_t*)(chipMem + (adr & chipMemMask)));

    dataBus = result;

    return true;
}

auto Agnus::fetchCopperDmaNoBUSCheck(uint32_t adr, uint16_t& result) -> void {

    busUsage = BUS_USAGE_COPPER;

    result = _swapWord(*(uint16_t*)(chipMem + (adr & chipMemMask)));

    dataBus = result;
}

auto Agnus::canBlitterUseBus() -> bool {
    if (busUsage != BUS_FREE)
        return false; // a higher DMA

    if (!useBlitterDMA())
        return false; // blitter get stuck

    if (!blitterNasty() && (countWaitCycles >= 3))
        return false; // if blitter has no priority over CPU all wait cycles matter, not only the cycles when blitter can proceed

    return true;
};

template<uint8_t ptrEvent> auto Agnus::fetchBlitterDma(uint32_t adr, uint16_t& result) -> bool {
    if(!canBlitterUseBus())
        return false;

    busUsage = BUS_USAGE_BLITTER;

    result = _swapWord(*(uint16_t*)(chipMem + (adr & chipMemMask)));

    dataBus = result;

    // if a modified pointer is used in the next cycle, the change is ignored.
    inactivateOneCycleEvent(ptrEvent);

    return true;
}

auto Agnus::writeBlitterDma(uint32_t adr, uint16_t value) -> bool {
    if(!canBlitterUseBus())
        return false;

    busUsage = BUS_USAGE_BLITTER;

    adr &= chipMemMask;
    if (trackMemChanges)
        rememberChipMem(adr);

    *(uint16_t*)(chipMem + adr) = _swapWord(value);

    dataBus = value;

    inactivateOneCycleEvent(PTR_BLT_D_H);

    return true;
}

template<uint8_t ptrEvent> auto Agnus::fetchBlitterDmaNoBUSCheck(uint32_t adr, uint16_t& result) -> void {
    busUsage = BUS_USAGE_BLITTER;

    result = _swapWord(*(uint16_t*)(chipMem + (adr & chipMemMask)));

    dataBus = result;

    inactivateOneCycleEvent(ptrEvent);
}

auto Agnus::writeBlitterDmaNoBUSCheck(uint32_t adr, uint16_t value) -> void {
    busUsage = BUS_USAGE_BLITTER;

    adr &= chipMemMask;
    if (trackMemChanges)
        rememberChipMem(adr);

    *(uint16_t*)(chipMem + adr) = _swapWord(value);

    dataBus = value;

    inactivateOneCycleEvent(PTR_BLT_D_H);
}

}
