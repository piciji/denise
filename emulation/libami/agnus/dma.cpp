
#include "agnus.h"

namespace LIBAMI {

template<uint8_t nr> auto Agnus::setAudPtH(uint16_t value) -> void {
    AudioDmaChannel& cha = audioDmaChannels[nr];
    cha.ptrLatch &= 0xffff;
    cha.ptrLatch |= value << 16;
    cha.ptrLatch &= dmaChipMemMask;
}

template<uint8_t nr> auto Agnus::setAudPtL(uint16_t value) -> void {
    AudioDmaChannel& cha = audioDmaChannels[nr];
    cha.ptrLatch &= ~0xffff;
    cha.ptrLatch |= value & 0xfffe;
}

auto Agnus::setBpl1ptH(uint16_t value) -> void {
    bpl1pt &= 0xffff;
    bpl1pt |= value << 16;
    bpl1pt &= dmaChipMemMask;
}

auto Agnus::setBpl1ptL(uint16_t value) -> void {
    bpl1pt &= ~0xffff;
    bpl1pt |= value & 0xfffe;
}

auto Agnus::setBpl2ptH(uint16_t value) -> void {
    bpl2pt &= 0xffff;
    bpl2pt |= value << 16;
    bpl2pt &= dmaChipMemMask;
}

auto Agnus::setBpl2ptL(uint16_t value) -> void {
    bpl2pt &= ~0xffff;
    bpl2pt |= value & 0xfffe;
}

auto Agnus::setBpl3ptH(uint16_t value) -> void {
    bpl3pt &= 0xffff;
    bpl3pt |= value << 16;
    bpl3pt &= dmaChipMemMask;
}

auto Agnus::setBpl3ptL(uint16_t value) -> void {
    bpl3pt &= ~0xffff;
    bpl3pt |= value & 0xfffe;
}

auto Agnus::setBpl4ptH(uint16_t value) -> void {
    bpl4pt &= 0xffff;
    bpl4pt |= value << 16;
    bpl4pt &= dmaChipMemMask;
}

auto Agnus::setBpl4ptL(uint16_t value) -> void {
    bpl4pt &= ~0xffff;
    bpl4pt |= value & 0xfffe;
}

auto Agnus::setBpl5ptH(uint16_t value) -> void {
    bpl5pt &= 0xffff;
    bpl5pt |= value << 16;
    bpl5pt &= dmaChipMemMask;
}

auto Agnus::setBpl5ptL(uint16_t value) -> void {
    bpl5pt &= ~0xffff;
    bpl5pt |= value & 0xfffe;
}

auto Agnus::setBpl6ptH(uint16_t value) -> void {
    bpl6pt &= 0xffff;
    bpl6pt |= value << 16;
    bpl6pt &= dmaChipMemMask;
}

auto Agnus::setBpl6ptL(uint16_t value) -> void {
    bpl6pt &= ~0xffff;
    bpl6pt |= value & 0xfffe;
}

template<uint8_t num> auto Agnus::setSprptH(uint16_t value) -> void {
    sprites[num].ptr &= 0xffff;
    sprites[num].ptr |= value << 16;
    sprites[num].ptr &= dmaChipMemMask;
}

template<uint8_t num> auto Agnus::setSprptL(uint16_t value) -> void {
    sprites[num].ptr &= ~0xffff;
    sprites[num].ptr |= value & 0xfffe;
}

auto Agnus::setDskPtH(uint16_t value) -> void {
    dskpt &= 0xffff;
    dskpt |= value << 16;
    dskpt &= dmaChipMemMask;
}

auto Agnus::setDskPtL(uint16_t value) -> void {
    dskpt &= ~0xffff;
    dskpt |= value & 0xfffe;
}

template<uint8_t pos, bool addMod> auto Agnus::fetchPlane() -> void {
    if constexpr ( pos == 1) {
        dataBus = _swapWord(*(uint16_t*) (chipMem + (bpl1pt & dmaChipMemMask)));
        denise.setBpl1Dat( dataBus );
        bpl1pt += 2;
        if constexpr (addMod) bpl1pt += bpl1Mod;
    } else if constexpr ( pos == 2) {
        dataBus = _swapWord(*(uint16_t*) (chipMem + (bpl2pt & dmaChipMemMask)));
        denise.setBpl2Dat( dataBus );
        bpl2pt += 2;
        if constexpr (addMod) bpl2pt += bpl2Mod;
    } else if constexpr ( pos == 3) {
        dataBus = _swapWord(*(uint16_t*) (chipMem + (bpl3pt & dmaChipMemMask)));
        denise.setBpl3Dat( dataBus );
        bpl3pt += 2;
        if constexpr (addMod) bpl3pt += bpl1Mod;
    } else if constexpr ( pos == 4) {
        dataBus = _swapWord(*(uint16_t*) (chipMem + (bpl4pt & dmaChipMemMask)));
        denise.setBpl4Dat( dataBus );
        bpl4pt += 2;
        if constexpr (addMod) bpl4pt += bpl2Mod;
    } else if constexpr ( pos == 5) {
        dataBus = _swapWord(*(uint16_t*) (chipMem + (bpl5pt & dmaChipMemMask)));
        denise.setBpl5Dat( dataBus );
        bpl5pt += 2;
        if constexpr (addMod) bpl5pt += bpl1Mod;
    } else if constexpr ( pos == 6) {
        dataBus = _swapWord(*(uint16_t*) (chipMem + (bpl6pt & dmaChipMemMask)));
        denise.setBpl6Dat( dataBus );
        bpl6pt += 2;
        if constexpr (addMod) bpl6pt += bpl2Mod;
    }

    dmaClock = clock;
    busUsage = BUS_USAGE_BPL;
}

template<uint8_t pos, bool addMod, bool strobe> auto Agnus::fetchPlaneConflict() -> void {
    if constexpr ( pos == 1) {
        if constexpr (!strobe) {
            dataBus = 0xffff;
            denise.setBpl1Dat(dataBus);
        }
        bpl1pt |= rDmaPtr;
        if constexpr (addMod) bpl1pt += bpl1Mod | rasAdder;
        else bpl1pt += rasAdder;
        rDmaPtr = bpl1pt;
    } else if constexpr ( pos == 2) {
        if constexpr (!strobe) {
            dataBus = 0xffff;
            denise.setBpl2Dat(dataBus);
        }
        bpl2pt |= rDmaPtr;
        if constexpr (addMod) bpl2pt += bpl2Mod | rasAdder;
        else bpl2pt += rasAdder;
        rDmaPtr = bpl2pt;
    } else if constexpr ( pos == 3) {
        if constexpr (!strobe) {
            dataBus = 0xffff;
            denise.setBpl3Dat(dataBus);
        }
        bpl3pt |= rDmaPtr;
        if constexpr (addMod) bpl3pt += bpl1Mod | rasAdder;
        else bpl3pt += rasAdder;
        rDmaPtr = bpl3pt;
    } else if constexpr ( pos == 4) {
        if constexpr (!strobe) {
            dataBus = 0xffff;
            denise.setBpl4Dat(dataBus);
        }
        bpl4pt |= rDmaPtr;
        if constexpr (addMod) bpl4pt += bpl2Mod | rasAdder;
        else bpl4pt += rasAdder;
        rDmaPtr = bpl4pt;
    } else if constexpr ( pos == 5) {
        if constexpr (!strobe) {
            dataBus = 0xffff;
            denise.setBpl5Dat(dataBus);
        }
        bpl5pt |= rDmaPtr;
        if constexpr (addMod) bpl5pt += bpl1Mod | rasAdder;
        else bpl5pt += rasAdder;
        rDmaPtr = bpl5pt;
    } else if constexpr ( pos == 6) {
        if constexpr (!strobe) {
            dataBus = 0xffff;
            denise.setBpl6Dat(dataBus);
        }
        bpl6pt |= rDmaPtr;
        if constexpr (addMod) bpl6pt += bpl2Mod | rasAdder;
        else bpl6pt += rasAdder;
        rDmaPtr = bpl6pt;
    }

    dmaClock = clock;
    busUsage = BUS_USAGE_BPL;
}

auto Agnus::diskDma(uint8_t slot, bool writeMode) -> void {
    dmaClock = clock;
    bplQueue &= ~0xff; // no BPL fetch
    busUsage = BUS_USAGE_DMAL;

    if (writeMode) {
        dataBus = _swapWord(*(uint16_t*) (chipMem + dskpt));
        if (!paula.setDskDat(dataBus))
            return;
    } else {
        uint16_t value;
        if (!paula.dskDatR(slot, value))
            return;

        if (trackMemChanges)
            rememberChipMem(dskpt);

        *(uint16_t*)(chipMem + dskpt) = _swapWord(value);
        dataBus = value;
    }

    inactivateOneCycleEvent<true>(Agnus::PTR_DSK_H);

    dskpt += 2;
    dskpt &= dmaChipMemMask;
}

auto Agnus::fakeDiskDma(uint16_t& word) -> void {
    if (trackMemChanges)
        rememberChipMem(dskpt);

    *(uint16_t*)(chipMem + dskpt) = _swapWord(word);
    dskpt += 2;
    dskpt &= dmaChipMemMask;
}

auto Agnus::fakeDiskDmaNoTracking(uint16_t word) -> void {
    if (paula.dmaDisk) {
        *(uint16_t*) (chipMem + dskpt) = _swapWord(word);
        dskpt += 2;
        dskpt &= dmaChipMemMask;
    }
}

auto Agnus::fakeDiskDma() -> uint16_t {
    dataBus = _swapWord(*(uint16_t*) (chipMem + dskpt));
    dskpt += 2;
    dskpt &= dmaChipMemMask;
    return dataBus;
}

template<uint8_t nr> auto Agnus::fetchSample(bool reset) -> void {
    AudioDmaChannel& cha = audioDmaChannels[nr];

    dataBus = _swapWord(*(uint16_t*) (chipMem + cha.ptr));

    if (reset)
        cha.ptr = cha.ptrLatch;
    else
        cha.ptr += 2;

    cha.ptr &= dmaChipMemMask;

    addOneCycleEvent(AUD_DAT0 + nr, dataBus); // put on RGA BUS

    dmaClock = clock;
    bplQueue &= ~0xff; // no BPL fetch
    busUsage = BUS_USAGE_DMAL;
}

template<uint8_t nr, uint8_t options> inline auto Agnus::fetchSprite() -> void {
    Sprite& spr = sprites[nr];

    constexpr uint8_t target = options & 0xf;
    constexpr uint8_t control = (options >> 4) & 0xf;

    if constexpr (control == 0) {
        dataBus = _swapWord(*(uint16_t*) (chipMem + spr.ptr));

        if constexpr (target == 0) {
            addOneCycleEvent(SPR_DATA0 + nr, dataBus);
        } else if constexpr (target == 1) {
            addOneCycleEvent(SPR_DATB0 + nr, dataBus);
        } else if constexpr (target == 2) {
            setSprPos<nr>(dataBus);
        } else if constexpr (target == 3) {
            setSprCtl<nr>(dataBus);
            spr.fetchData = false;
            checkSpriteV<nr, false>();
        }

        dmaClock = clock;
        busUsage = BUS_USAGE_SPRITE;
        spr.ptr += 2;
        spr.ptr &= dmaChipMemMask;
    } else if constexpr (control == 1) {
        spr.ptr += 2;
        spr.ptr &= dmaChipMemMask;

    } else if constexpr (control == 2) {
        busUsage = BUS_USAGE_SPRITE;
    }

    if constexpr (!!(target & 2)) {
        if (vBlankEnd || vBlankEndNext) {
            spr.fetchData = false;
            spr.enable = false;
        }
    }
}

template<bool oddCycle1> inline auto Agnus::canCopperUseBus() -> bool {
    if constexpr (oddCycle1) {
        if (bplQueue & 0xff)
            return false; // a higher DMA
    } else {
        if (busUsage != BUS_FREE)
            return false; // a higher DMA
    }

    if (!useCopperDMAForQueue())
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
    dmaClock = clock;

    result = _swapWord(*(uint16_t*)(chipMem + (adr & dmaChipMemMask)));

    dataBus = result;

    return true;
}

auto Agnus::fetchCopperDmaNoBUSCheck(uint32_t adr, uint16_t& result) -> void {

    busUsage = BUS_USAGE_COPPER;
    dmaClock = clock;

    result = _swapWord(*(uint16_t*)(chipMem + (adr & dmaChipMemMask)));

    dataBus = result;
}

inline auto Agnus::canBlitterUseBus() -> bool {
    if (busUsage != BUS_FREE)
        return false; // a higher DMA

    if (!useBlitterDMAForQueue())
        return false; // blitter get stuck

    if (!blitterNasty() && (countWaitCycles > 2))
        return false; // if blitter has no priority over CPU all wait cycles matter, not only the cycles when blitter can proceed

    return true;
}

auto Agnus::canBlitterUseBusExt() -> bool {
    return canBlitterUseBus();
}

template<uint8_t ptrEvent, bool desc, bool add, bool mod, bool check>
auto Agnus::fetchBlitterDma(uint32_t& adr, uint16_t& result, const int16_t& modVal) -> bool {
    if constexpr (check) {
        if(!canBlitterUseBus())
            return false;
    }

    busUsage = BUS_USAGE_BLITTER;

    if (copper.state == Copper::Read1Buggy) {
        adr |= copper.copPtrBefore;

        result = _swapWord(*(uint16_t*)(chipMem + (adr & dmaChipMemMask)));

        adr = copper.copPtr;

        if constexpr (mod) {
            if constexpr (desc)  copper.copPtr += -modVal;
            else                 copper.copPtr += modVal;
        }
    } else {
        result = _swapWord(*(uint16_t*)(chipMem + (adr & dmaChipMemMask)));

        if constexpr (add) {
            if constexpr (desc)  adr += -2;
            else                 adr += +2;
        }
    }

    if constexpr (mod) {
        if constexpr (desc)  adr += -modVal;
        else                 adr += modVal;
    }

    dataBus = result;
    dmaClock = clock;

    inactivateOneCycleEvent<true>(ptrEvent);
    return true;
}

template<bool desc, bool add, bool mod, bool check>
auto Agnus::writeBlitterDma(uint32_t& adr, uint16_t& value, const int16_t& modVal) -> bool {
    if constexpr (check) {
        if(!canBlitterUseBus())
            return false;
    }

    busUsage = BUS_USAGE_BLITTER;

    adr &= dmaChipMemMask;
    if (trackMemChanges)
        rememberChipMem(adr);

    if (copper.state == Copper::Read1Buggy) {
        adr |= copper.copPtrBefore;

        *(uint16_t*)(chipMem + adr) = _swapWord(value);

        adr = copper.copPtr;

        if constexpr (mod) {
            if constexpr (desc)  copper.copPtr += -modVal;
            else                 copper.copPtr += modVal;
        }
    } else {
        *(uint16_t*)(chipMem + adr) = _swapWord(value);

        if constexpr (add) {
            if constexpr (desc)  adr += -2;
            else                 adr += +2;
        }
    }

    if constexpr (mod) {
        if constexpr (desc)  adr += -modVal;
        else                 adr += modVal;
    }

    dataBus = value;
    dmaClock = clock;

    inactivateOneCycleEvent<true>(PTR_BLT_D_H);
    return true;
}

auto Agnus::checkCopperBlitterConflict(uint32_t& ptr) -> bool {
    if (useCopperDMAForQueue() && (((bplQueue >> 16) & 0xff) == 0) ) { // DMAL, Refresh is out of way
        return true;
    }

    return false;
}

}
