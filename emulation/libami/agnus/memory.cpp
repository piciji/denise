
#include "agnus.h"

#define eCyclePosition  10 - ((eClockCycle - clock) << 1)

namespace LIBAMI {

auto Agnus::readByte(uint32_t adr) -> uint8_t {
    adr &= 0xffffff;

    switch( mapper[adr >> 16] ) {
        case CHIP_MEM:
            addWaitstatesToCPU();
            dataBus = *(chipMem + (adr & chipMemMask));
            break;
        case MMIO_CUSTOM:
            addWaitstatesToCPU();
            if (adr & 1)
                dataBus = (uint8_t)readCustom<true>(adr & 0x1fe);
            else
                dataBus = (uint8_t)(readCustom<true>(adr & 0x1fe) >> 8);
            break;
        case MMIO_CIA: {
            sync( cpu.internalWaitCyclesBasedOnEClock<2>( eCyclePosition ) );
            uint8_t reg = (adr >> 8) & 0xf;
            switch(adr & 0x3000) {
                case 0x0000: dataBus = (adr & 1) ? cia1.read( reg ) : cia2.read( reg ); break;
                case 0x1000: dataBus = (adr & 1) ? (uint8_t)dataBus : cia2.read( reg ); break;
                case 0x2000: dataBus = (adr & 1) ? cia1.read( reg ) : (dataBus >> 8); break;
                case 0x3000: dataBus = (adr & 1) ? (uint8_t)dataBus : (dataBus >> 8); break;
            }
        } break;
        case SLOW_MEM:
            addWaitstatesToCPU();
            dataBus = *(slowMem + (adr - 0xc00000));
            break;
        case FAST_MEM:
            dataBus = *(fastMem + (adr - fastMemExpansion.baseAdr));
            break;
        case KICK_ROM:
            dataBus = *(kickRom + (adr & kickRomMask));
            break;
        case EXPANSION:
            dataBus = expansionsConfigured[adr >> 16]->read(adr);
            break;
        case EXT_ROM:
            dataBus = *(extRom + (adr & extRomMask));
            break;
        case WOM:
            dataBus = *(wom + (adr & 0x3ffff));
            break;
        case MMIO_RTC:
            dataBus = (adr & 1) ? rtc.read( (adr >> 2) & 0xf, system->isProcessFrame() ) : (dataBus >> 8);
            break;
        case AUTO_CONF:
            dataBus = readAutoConf(adr);
            break;
        case Unmapped:
            break;
    }

    dataBus = (dataBus << 8) | dataBus;
    return (uint8_t)dataBus;
}

auto Agnus::readWord(uint32_t adr) -> uint16_t {
    adr &= 0xffffff;
    // 68k is big endian, modern architecture is little endian
    switch( mapper[adr >> 16] ) {
        case CHIP_MEM:
            addWaitstatesToCPU();
            dataBus = _swapWord(*(uint16_t*)(chipMem + (adr & chipMemMask)));
            break;
        case MMIO_CUSTOM:
            addWaitstatesToCPU();
            dataBus = readCustom(adr & 0x1fe);
            break;
        case MMIO_CIA: {
            // CIA CS (Chip Select) happens when A12/A13 and VMA (respond of VPA in 68k E-Mode) are active.
            // Gary assert VPA but don't see A12/A13. It only sees the upper address bits and knows when in general CIA area.
            // hence Gary asserts VPA, even if no CIA is selected at all ... "case 0x3000" in switch/case below.
            // same applies to CIA writes.
            sync( cpu.internalWaitCyclesBasedOnEClock<2>( eCyclePosition ) );
            uint8_t reg = (adr >> 8) & 0xf;
            switch(adr & 0x3000) {
                case 0x0000: dataBus = cia1.read( reg ) | (cia2.read( reg ) << 8); break;
                case 0x1000: dataBus = (dataBus >> 8) | (cia2.read( reg ) << 8); break;
                case 0x2000: dataBus = cia1.read( reg ) | (dataBus << 8); break;
                    // case 0x3000: break; get last BUS value, should be IRC in most cases
                    // E-Clock is used too
            }
        } break;
        case SLOW_MEM:
            addWaitstatesToCPU();
            dataBus = _swapWord(*(uint16_t*)(slowMem + (adr - 0xc00000)));
            break;
        case FAST_MEM:
            dataBus = _swapWord(*(uint16_t*)(fastMem + (adr - fastMemExpansion.baseAdr)));
            break;
        case KICK_ROM:
            // firmware access needs sanity checking
            dataBus = _swapWord(*(uint16_t*)(kickRom + (adr & kickRomMask)));
            break;
        case EXPANSION:
            dataBus = expansionsConfigured[adr >> 16]->readW(adr);
            break;
        case EXT_ROM:
            dataBus = _swapWord(*(uint16_t*)(extRom + (adr & extRomMask)));
            break;
        case WOM:
            dataBus = _swapWord(*(uint16_t*)(wom + (adr & 0x3ffff)));
            break;
        case MMIO_RTC:
            dataBus &= ~0xff;
            dataBus |= rtc.read( (adr >> 2) & 0xf, system->isProcessFrame() );
            break;
        case AUTO_CONF:
            dataBus = readAutoConf(adr) << 8;
            dataBus |= readAutoConf(adr + 1);
            break;
        case Unmapped: // floating BUS, don't return zero (Hollywood Poker Pro)
            break;
    }

    return dataBus;
}

auto Agnus::writeByte(uint32_t adr, uint8_t value) -> void {
    adr &= 0xffffff;

    switch( mapper[adr >> 16] ) {
        case CHIP_MEM:
            addWaitstatesToCPU();
            adr &= chipMemMask;
            if (memState)
                memState->trackers[TRACKER_CHIP].remember(adr & ~1);

            *(chipMem + adr) = value;
            break;
        case MMIO_CUSTOM:
            addWaitstatesToCPU();
            writeCustom( adr & 0x1fe, value | (value << 8) );
            break;
        case MMIO_CIA: {
            sync( cpu.internalWaitCyclesBasedOnEClock<2>( eCyclePosition ) );
            uint8_t reg = (adr >> 8) & 0xf;
            if ((adr & 0x1000) == 0)
                cia1.write( reg, value );
            if ((adr & 0x2000) == 0)
                cia2.write( reg, value );
        } break;
        case SLOW_MEM:
            addWaitstatesToCPU();
            adr -= 0xc00000;
            if (memState)
                memState->trackers[TRACKER_SLOW].remember(adr & ~1);
            *(slowMem + adr) = value;
            break;
        case FAST_MEM:
            adr -= fastMemExpansion.baseAdr;
            if (memState)
                memState->trackers[TRACKER_FAST].remember(adr & ~1);
            *(fastMem + adr) = value;
            break;
        case KICK_ROM:
            if ( (model == OCS_A1000) && !womLock)
                lockWom();
            break;
        case EXPANSION:
            expansionsConfigured[adr >> 16]->write(adr, value);
            break;
        case EXT_ROM:
            break;
        case WOM:
            if (!womLock) *(wom + (adr & 0x3ffff)) = value;
            break;
        case MMIO_RTC:
            if ((adr & 1) && system->isProcessFrame())
                rtc.write( (adr >> 2) & 0xf, value );
            break;
        case AUTO_CONF:
            writeAutoConf(adr, value);
            break;
        case Unmapped:
            break;
    }

    dataBus = (value << 8) | value;
}

auto Agnus::writeWord(uint32_t adr, uint16_t value) -> void {
    adr &= 0xffffff;

    switch( mapper[adr >> 16] ) {
        case CHIP_MEM:
            addWaitstatesToCPU();
            adr &= chipMemMask;
            if (memState)
                memState->trackers[TRACKER_CHIP].remember(adr);

            *(uint16_t*)(chipMem + adr) = _swapWord(value);
            break;
        case MMIO_CUSTOM:
            addWaitstatesToCPU();
            writeCustom( adr & 0x1fe, value );
            break;
        case MMIO_CIA: {
            sync( cpu.internalWaitCyclesBasedOnEClock<2>( eCyclePosition ) );
            uint8_t reg = (adr >> 8) & 0xf;
            if ((adr & 0x1000) == 0)
                cia1.write( reg, (uint8_t)value );
            if ((adr & 0x2000) == 0)
                cia2.write( reg, (uint8_t)(value >> 8) );
        } break;
        case SLOW_MEM:
            addWaitstatesToCPU();
            adr -= 0xc00000;
            if (memState)
                memState->trackers[TRACKER_SLOW].remember(adr);
            *(uint16_t*)(slowMem + adr) = _swapWord(value);
            break;
        case FAST_MEM:
            adr -= fastMemExpansion.baseAdr;
            if (memState)
                memState->trackers[TRACKER_FAST].remember(adr);
            *(uint16_t*)(fastMem + adr) = _swapWord(value);
            break;
        case KICK_ROM:
            if ( (model == OCS_A1000) && !womLock)
                lockWom();
            break;
        case EXPANSION:
            expansionsConfigured[adr >> 16]->writeW(adr, value);
            break;
        case EXT_ROM:
            break;
        case WOM:
            if (!womLock) *(uint16_t*)(wom + (adr & 0x3ffff)) = _swapWord(value);
            break;
        case MMIO_RTC:
            if (system->isProcessFrame())
                rtc.write( (adr >> 2) & 0xf, value & 0xff );
            break;
        case AUTO_CONF:
            writeAutoConfWord(adr, value);
            break;
        case Unmapped:
            break;
    }
    dataBus = value;
}

auto Agnus::fakeWriteByte(uint32_t adr, uint8_t value) -> void {
    adr &= 0xffffff;

    switch (mapper[adr >> 16]) {
        case CHIP_MEM:
            adr &= chipMemMask;
            if (memState)
                memState->trackers[TRACKER_CHIP].remember(adr & ~1);

            *(chipMem + adr) = value;
            break;
        case SLOW_MEM:
            adr -= 0xc00000;
            if (memState)
                memState->trackers[TRACKER_SLOW].remember(adr & ~1);
            *(slowMem + adr) = value;
            break;
        case FAST_MEM:
            adr -= fastMemExpansion.baseAdr;
            if (memState)
                memState->trackers[TRACKER_FAST].remember(adr & ~1);
            *(fastMem + adr) = value;
            break;
        case WOM:
            if (!womLock) *(wom + (adr & 0x3ffff)) = value;
            break;
    }
}

auto Agnus::fakeWriteByte(uint32_t adr, uint8_t* data, unsigned len) -> void {
    for(unsigned i = 0; i < len; i++)
        fakeWriteByte(adr + i, *data++);
}

auto Agnus::fakeWriteWord(uint32_t adr, uint16_t value) -> void {
    fakeWriteByte(adr, value >> 8);
    fakeWriteByte(adr + 1, value & 0xff);
}

auto Agnus::fakeWriteLongWord(uint32_t adr, uint32_t value) -> void {
    fakeWriteWord(adr, value >> 16);
    fakeWriteWord(adr + 2, value & 0xffff);
}

auto Agnus::fakeReadWord(uint32_t adr) -> uint16_t {
    adr &= 0xffffff;
    switch (mapper[adr >> 16]) {
        case CHIP_MEM:  return _swapWord(*(uint16_t*)(chipMem + (adr & chipMemMask)));
        case SLOW_MEM:  return _swapWord(*(uint16_t*)(slowMem + (adr - 0xc00000)));
        case FAST_MEM:  return _swapWord(*(uint16_t*)(fastMem + (adr - fastMemExpansion.baseAdr)));
        case KICK_ROM:  return _swapWord(*(uint16_t*)(kickRom + (adr & kickRomMask)));
        case EXT_ROM:   return _swapWord(*(uint16_t*)(extRom + (adr & extRomMask)));
        case WOM:       return _swapWord(*(uint16_t*)(wom + (adr & 0x3ffff)));
    }
    return 0;
}

auto Agnus::fakeReadByte(uint32_t adr) -> uint8_t {
    adr &= 0xffffff;
    switch (mapper[adr >> 16]) {
        case CHIP_MEM:  return *(chipMem + (adr & chipMemMask));
        case SLOW_MEM:  return *(slowMem + (adr - 0xc00000));
        case FAST_MEM:  return *(fastMem + (adr - fastMemExpansion.baseAdr));
        case KICK_ROM:  return *(kickRom + (adr & kickRomMask));
        case EXT_ROM:   return *(extRom + (adr & extRomMask));
        case WOM:       return *(wom + (adr & 0x3ffff));
    }
    return 0;
}

auto Agnus::fakeRead(uint32_t adr, uint8_t* buf, unsigned len) -> void {
    for (unsigned i = 0; i < len; i++)
        buf[i] = fakeReadByte(adr + i);
}

auto Agnus::setChipmem(unsigned size) -> void {
    if (size == 0)
        size = 512 * 1024;
    else if (size > (2 * 1024 * 1024))
        size = 2 * 1024 * 1024;
    else
        size = Emulator::powerOfTwo(size);

    unsigned mask = size - 1;

    if (mask == chipMemMask)
        return;

    createChipSlowMem(size, slowMemSize);
}

auto Agnus::setSlowmem(unsigned size) -> void {
    if (size > (1792 * 1024))
        size = 1792 * 1024;

    if (size == slowMemSize)
        return;

    createChipSlowMem(chipMemMask + 1, size);
}

auto Agnus::createChipSlowMem(unsigned sizeChip, unsigned sizeSlow) -> void {
    if (chipMem)
        delete[] chipMem;

    chipMem = new uint8_t[sizeChip + sizeSlow];
    chipMemMask = sizeChip - 1;
    slowMem = nullptr;
    if (sizeSlow)
        slowMem = chipMem + sizeChip;
    slowMemSize = sizeSlow;
}

auto Agnus::setFastmem(unsigned size) -> void {
    if (size > (8192 * 1024))
        size = 8192 * 1024;

    if (size == fastMemSize)
        return;

    if (fastMem) {
        delete[] fastMem;
        fastMem = nullptr;
    }

    if (size)
        fastMem = new uint8_t[size];
    fastMemSize = size;
}

auto Agnus::mapRom(bool init ) -> void {
    uint8_t romAssignment = kickRom ? KICK_ROM : Unmapped;
    uint8_t romOrwomAssignment = (model == OCS_A1000) ? WOM : romAssignment;

    if (extRom) { // AROS
        for (int i = 0xe0; i <= 0xe7; i++)
            mapper[i] = EXT_ROM;
    } else {
        for (int i = 0xe0; i <= 0xe7; i++)
            mapper[i] = romAssignment; // mirror
    }

    for (int i = 0xf8; i <= 0xff; i++)
        mapper[i] = romOrwomAssignment;

    if ((model == OCS_A1000) && !womLock) {
        for (int i = 0xf8; i <= 0xfb; i++)
            mapper[i] = romAssignment;
    }

    if (init || (mapper[0] != CHIP_MEM))
        setOVL(true);
}

auto Agnus::mapMemory() -> void {

    if (ecs() && (chipMemMask == 0x7ffff) && (slowMemSize >= (512 * 1024))) {
        dmaChipMemMask = 0xfffff;
    } else
        dmaChipMemMask = chipMemMask;

    for(int i = 8; i <= 0x1f; i++) // max 2 MB (mirrored)
        mapper[i] = CHIP_MEM;

    for(int i = 0x20; i <= 0x9f; i++)
        mapper[i] = Unmapped; // auto config (e.g. fast ram)

    for(int i = 0xa0; i <= 0xbe; i++)
        mapper[i] = MMIO_CIA; // CIA mirror or overmap for Zorro 2 IO expansion

    mapper[0xbf] = MMIO_CIA;

    for (int i = 0xc0; i <= 0xd7; i++)
        mapper[i] = MMIO_CUSTOM; // mirror

    // todo: gayle (A600, A1200): unmapped for 0xc0 - 0xd8

    if (model == OCS_A1000) {
        for (int i = 0xd8; i <= 0xdb; i++)
            mapper[i] = MMIO_CUSTOM; // mirror
    } else {
        for (int i = 0xd8; i <= 0xdb; i++)
            mapper[i] = Unmapped;
    }

    if (slowMem) { // overmap slow mem (max. 1.75 MB, not mirrored)
        uint8_t page = slowMemSize / (64 * 1024);

        for(int i = 0xc0; i < (0xc0 + page); i++) // max: 0xdb
            mapper[i] = SLOW_MEM;
    }

    if (useRTC) {
        if (model == OCS_A1000) {
            for (int i = 0xd8; i <= 0xdb; i++)
                mapper[i] = MMIO_RTC;

            mapper[0xdc] = MMIO_CUSTOM;
        } else
            mapper[0xdc] = MMIO_RTC;
    } else {
        mapper[0xdc] = (model == OCS_A1000) ? MMIO_CUSTOM : Unmapped;
    }

    mapper[0xdd] = (model == OCS_A1000) ? MMIO_CUSTOM : Unmapped;
    mapper[0xde] = MMIO_CUSTOM;
    mapper[0xdf] = MMIO_CUSTOM;

    mapper[0xe8] = Unmapped;
    for(auto expansion : expansions) {
        if (expansion->boardState != ExpansionPort::BoardState::ShutUp) {
            mapper[0xe8] = AUTO_CONF;
            break;
        }
    }

    for(int i = 0xe9; i <= 0xef; i++)
        mapper[i] = Unmapped;

    for(int i = 0xf0; i <= 0xf7; i++)
        mapper[i] = Unmapped; // extended ROM, CD32
}

auto Agnus::setOVL(bool state) -> void {
    if (state) {
        for (int i = 0x0; i < 0x8; i++)
            mapper[i] = mapper[0xf8 + i];
    } else {
        for (int i = 0x0; i < 0x8; i++)
            mapper[i] = CHIP_MEM;
    }
}

auto Agnus::lockWom() -> void {
    for (int i = 0xf8; i <= 0xfb; i++)
        mapper[i] = WOM;

    if (mapper[0] != CHIP_MEM)
        setOVL(true);

    womLock = true;
}

auto Agnus::readAutoConf(uint32_t addr) -> uint8_t {
    for (auto expansion : expansions) {
        if (expansion->boardState == ExpansionPort::BoardState::AutoConf)
            return expansion->readAutoConf(addr);
    }
    return 0xff;
}

auto Agnus::writeAutoConf(uint32_t addr, uint8_t value) -> void {
    for (auto expansion : expansions) {
        if (expansion->boardState == ExpansionPort::BoardState::AutoConf)
            return expansion->writeAutoConf(addr, value);
    }
}

auto Agnus::writeAutoConfWord(uint32_t addr, uint16_t value) -> void {
    for (auto expansion : expansions) {
        if (expansion->boardState == ExpansionPort::BoardState::AutoConf)
            return expansion->writeAutoConfW(addr, value);
    }
}

auto Agnus::checkForRomEncryption() -> void {
    int i, k;

    if (!extRom || !extRomSize || !kickRom || !kickRomSize)
        return;

    if ((kickRomSize < 12) || (std::memcmp(kickRom, "AMIROMTYPE1", 11)))
        return;

    auto encSize = kickRomSize - 11;
    if (!encryptedRom || system->firmwareChanged) {
        if (encryptedRom)
            delete[] encryptedRom;

        encryptedRom = new uint8_t[encSize];
        std::memcpy(encryptedRom, kickRom + 11, encSize);

        for (i = k = 0; i < encSize; i++, k = (k + 1) % extRomSize)
            encryptedRom[i] ^= extRom[k];
    }

    kickRom = encryptedRom;
    kickRomMask = Emulator::powerOfTwo( encSize ) - 1;

    extRom = nullptr;
    extRomSize = 0;
    extRomMask = 0;
    system->firmwareChanged = false;
}

auto Agnus::isChipMem(uint32_t addr) -> bool {
    if (addr > 0xffffff)
        return false;

    return mapper[addr >> 16] == CHIP_MEM;
}

auto Agnus::isSlowMem(uint32_t addr) -> bool {
    if (addr > 0xffffff)
        return false;

    return mapper[addr >> 16] == SLOW_MEM;
}

auto Agnus::isFastMem(uint32_t addr) -> bool {
    if (addr > 0xffffff)
        return false;

    return mapper[addr >> 16] == FAST_MEM;
}

auto Agnus::isMem(uint32_t addr) -> bool {
    if (addr > 0xffffff)
        return false;

    auto _m = mapper[addr >> 16];
    return _m == FAST_MEM || _m == CHIP_MEM || _m == SLOW_MEM;
}


}
