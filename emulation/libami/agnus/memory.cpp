
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
            dataBus = *(fastMem + (adr - zorroBaseAdr));
            break;
        case KICK_ROM:
            dataBus = *(kickRom + (adr & kickRomMask));
            break;
        case EXT_ROM:
            dataBus = *(extRom + (adr & extRomMask));
            break;
        case WOM:
            dataBus = *(wom + (adr & 0x3ffff));
            break;
        case MMIO_RTC:
            break;
        case AUTO_CONF:
            dataBus = readZorro(adr);
            break;
        case Unmapped:
            break;
    }

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
            // Gary assert VPA but don't see A12/A13. It only see the upper address bits and knows when in general CIA area.
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
            dataBus = _swapWord(*(uint16_t*)(fastMem + (adr - zorroBaseAdr)));
            break;
        case KICK_ROM:
            // firmware access needs sanity checking
            dataBus = _swapWord(*(uint16_t*)(kickRom + (adr & kickRomMask)));
            break;
        case EXT_ROM:
            dataBus = _swapWord(*(uint16_t*)(extRom + (adr & extRomMask)));
            break;
        case WOM:
            dataBus = _swapWord(*(uint16_t*)(wom + (adr & 0x3ffff)));
            break;
        case MMIO_RTC:
            break;
        case AUTO_CONF:
            dataBus = readZorro(adr) << 8;
            dataBus |= readZorro(adr + 1);
            break;
        case Unmapped:
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
            if (trackMemChanges)
                rememberChipMem(adr & ~1);

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
            if (trackMemChanges)
                rememberSlowMem(adr & ~1);
            *(slowMem + adr) = value;
            break;
        case FAST_MEM:
            adr -= zorroBaseAdr;
            if (trackMemChanges)
                rememberFastMem(adr & ~1);
            *(fastMem + adr) = value;
            break;
        case KICK_ROM:
            if ( (model == OCS_A1000) && !womLock)
                lockWom();
            break;
        case EXT_ROM:
            break;
        case WOM:
            if (!womLock) *(wom + (adr & 0x3ffff)) = value;
            break;
        case MMIO_RTC:
            break;
        case AUTO_CONF:
            writeZorro(adr, value);
            break;
        case Unmapped:
            break;
    }

    dataBus =  (value << 8) | value;
}

auto Agnus::writeWord(uint32_t adr, uint16_t value) -> void {
    adr &= 0xffffff;

    switch( mapper[adr >> 16] ) {
        case CHIP_MEM:
            addWaitstatesToCPU();
            adr &= chipMemMask;
            if (trackMemChanges)
                rememberChipMem(adr);

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
            if (trackMemChanges)
                rememberSlowMem(adr);
            *(uint16_t*)(slowMem + adr) = _swapWord(value);
            break;
        case FAST_MEM:
            adr -= zorroBaseAdr;
            if (trackMemChanges)
                rememberFastMem(adr);
            *(uint16_t*)(fastMem + adr) = _swapWord(value);
            break;
        case KICK_ROM:
            if ( (model == OCS_A1000) && !womLock)
                lockWom();
            break;
        case EXT_ROM:
            break;
        case WOM:
            if (!womLock) *(uint16_t*)(wom + (adr & 0x3ffff)) = _swapWord(value);
            break;
        case MMIO_RTC:
            break;
        case AUTO_CONF:
            writeZorro(adr, value >> 8);
            writeZorro(adr + 1, value & 0xff);
            break;
        case Unmapped:
            break;
    }
    dataBus = value;
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

    if (chipMem)
        delete[] chipMem;

    chipMem = new uint8_t[size];
    chipMemMask = mask;
}

auto Agnus::setSlowmem(unsigned size) -> void {
    if (size > (1792 * 1024))
        size = 1792 * 1024;

    if (size == slowMemSize)
        return;

    if (slowMem) {
        delete[] slowMem;
        slowMem = nullptr;
    }

    if (size)
        slowMem = new uint8_t[size];
    slowMemSize = size;
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

auto Agnus::rememberChipMem(uint32_t adr) -> void {
    MemChange* memChange = &chipMemChange[chipMemChangePos++];
    memChange->address = adr;
    memChange->value = *(uint16_t*)(chipMem + adr);

    if (chipMemChangePos == chipMemChangeSize)
        increaseTrackMemStorage(chipMemChange, chipMemChangeSize);
}

auto Agnus::rememberSlowMem(uint32_t adr) -> void {
    MemChange* memChange = &slowMemChange[slowMemChangePos++];
    memChange->address = adr;
    memChange->value = *(uint16_t*)(slowMem + adr);

    if (slowMemChangePos == slowMemChangeSize)
        increaseTrackMemStorage(slowMemChange, slowMemChangeSize);
}

auto Agnus::rememberFastMem(uint32_t adr) -> void {
    MemChange* memChange = &fastMemChange[fastMemChangePos++];
    memChange->address = adr;
    memChange->value = *(uint16_t*)(fastMem + adr);

    if (fastMemChangePos == fastMemChangeSize)
        increaseTrackMemStorage(fastMemChange, fastMemChangeSize);
}

auto Agnus::increaseTrackMemStorage(MemChange*& memChange, unsigned& size) -> void {
    MemChange* memChangeTemp = new MemChange[size << 1];
    std::memcpy(memChangeTemp, memChange, sizeof(MemChange) * size );
    size <<= 1;
    delete[] memChange;
    memChange = memChangeTemp;
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
    for(int i = 8; i <= 0x1f; i++) // max 2 MB (mirrored)
        mapper[i] = CHIP_MEM;

    for(int i = 0x20; i <= 0x9f; i++)
        mapper[i] = Unmapped; // auto config (e.g. fast ram)

    for(int i = 0xa0; i <= 0xbe; i++)
        mapper[i] = MMIO_CIA; // CIA mirror or overmap for Zorro 2 IO expansion

    mapper[0xbf] = MMIO_CIA;

    for(int i = 0xc0; i <= 0xdb; i++)
        mapper[i] = MMIO_CUSTOM; // mirror

    if (slowMem) { // overmap slow mem (max. 1.75 MB, not mirrored)
        uint8_t page = slowMemSize / (64 * 1024);

        for(int i = 0xc0; i < (0xc0 + page); i++)
            mapper[i] = SLOW_MEM;
    }

    mapper[0xdc] = useRTC ? MMIO_RTC : MMIO_CUSTOM;

    mapper[0xdd] = Unmapped;

    for (int i = 0xde; i <= 0xdf; i++)
        mapper[i] = MMIO_CUSTOM;

    mapper[0xe8] = fastMem ? AUTO_CONF : Unmapped;
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

// dirty fastmem integration
auto Agnus::readZorro(uint32_t adr) -> uint8_t {
    uint8_t out = 0xff;
    uint8_t offset = adr & 0xff;

    if (zorroBaseAdr)
        return out;

    if (((offset & 1) == 0) && (offset < 0x40)) {
        switch (offset >> 2) {
            case 0x0: out = 0x20 | 0xc0 | getZorroSize(); break;
            case 0x1: out = 0x51; break; // product ID
            case 0x2: out = 0x80; break;
            case 0x3: out = 0; break;
            case 0x4: out = 0x02; break; // manufactorer ID: 3-State
            case 0x5: out = 0x00; break;

            case 0x6: out = 0; break; // serial number
            case 0x7: out = 0; break;
            case 0x8: out = 0; break;
            case 0x9: out = 1; break;

            default: out = 0; break;
        }

        out = (offset & 2) ? (out & 0xf) : ((out >> 4) & 0xf);
        out = (offset < 4) ? (out << 4) : ~(out << 4);

    } else if (offset == 0x40 || offset == 0x42)
        out = 0;

    return out;
}

auto Agnus::writeZorro(uint32_t adr, uint8_t data) -> void {
    if (zorroBaseAdr)
        return;

    switch (adr & 0xffff) {
        case 0x48: {
            zorroBaseAdr |= (data & 0xf0) << 16;
            int startPage = zorroBaseAdr >> 16;

            int pages = fastMemSize / (64 * 1024);
            for (int i = startPage; i < (startPage + pages); i++)
                mapper[i] = FAST_MEM;
        } break;

        case 0x4a:
            zorroBaseAdr |= (data & 0xf0) << 12;
            break;
    }
}

auto Agnus::getZorroSize() -> uint8_t {
    int pages = fastMemSize / (64 * 1024);
    switch(pages) {
        case 1: return 1;
        case 2: return 2;
        case 4: return 3;
        case 8: return 4;
        case 16: return 5;
        case 32: return 6;
        case 64: return 7;
        case 128: return 0;
    }
    return ~0; // should never happen
}

}
