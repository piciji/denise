
#include "agnus.h"

#define eCyclePosition  10 - (((eClockCycle - clock) & 0xffffffff) << 1)

namespace LIBAMI {

auto Agnus::readByte(uint32_t adr) -> uint8_t {
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
                dataBus = (uint8_t)(readCustom<true>(adr) >> 8);
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
        case KICK_ROM:
            dataBus = kickRom ? *(kickRom + (adr & kickRomMask)) : 0;
            break;
        case EXT_ROM:
            dataBus = extRom ? *(extRom + (adr & extRomMask)) : 0;
            break;
        case WOM:
            dataBus = *(wom + (adr & 0x3ffff));
            break;
        case MMIO_RTC:
            break;
        case Unmapped:
            break;
    }
    return (uint8_t)dataBus;
}

auto Agnus::readWord(uint32_t adr) -> uint16_t {
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
            // always leads to the time for the next CIA cycle, after which the register is accessed.

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
        case KICK_ROM:
            // firmware access needs sanity checking
            dataBus = kickRom ? _swapWord(*(uint16_t*)(kickRom + (adr & kickRomMask))) : 0;
            break;
        case EXT_ROM:
            dataBus = extRom ? _swapWord(*(uint16_t*)(extRom + (adr & extRomMask))) : 0;
            break;
        case WOM:
            dataBus = _swapWord(*(uint16_t*)(wom + (adr & 0x3ffff)));
            break;
        case MMIO_RTC:
            break;
        case Unmapped:
            break;
    }
    return dataBus;
}

auto Agnus::writeByte(uint32_t adr, uint8_t value) -> void {
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
        case KICK_ROM:
            if (model == OCS_A1000 && !womLock) {
                womLock = true;
                for (unsigned i = 0xf8; i <= 0xfb; i++)
                    mapper[i] = WOM;
            }
            break;
        case EXT_ROM:
            break;
        case WOM:
            if (!womLock) *(wom + (adr & 0x3ffff)) = value;
            break;
        case MMIO_RTC:
            break;
        case Unmapped:
            break;
    }
    dataBus = value;
}

auto Agnus::writeWord(uint32_t adr, uint16_t value) -> void {
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
        case KICK_ROM:
            if (model == OCS_A1000 && !womLock) {
                womLock = true;
                for (unsigned i = 0xf8; i <= 0xfb; i++)
                    mapper[i] = WOM;
            }
            break;
        case EXT_ROM:
            break;
        case WOM:
            if (!womLock) *(uint16_t*)(wom + (adr & 0x3ffff)) = _swapWord(value);
            break;
        case MMIO_RTC:
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

auto Agnus::increaseTrackMemStorage(MemChange*& memChange, unsigned& size) -> void {
    MemChange* memChangeTemp = new MemChange[size << 1];
    std::memcpy(memChangeTemp, memChange, sizeof(MemChange) * size );
    size <<= 1;
    delete[] memChange;
    memChange = memChangeTemp;
}

auto Agnus::mapMemory() -> void {
    uint8_t romAssignment = kickRom ? KICK_ROM : Unmapped;
    uint8_t romOrwomAssignment = (model == OCS_A1000) ? WOM : romAssignment;

    for (unsigned i = 0x0; i < 0x8; i++)
        mapper[i] = KICK_ROM; // OVL

    for(unsigned i = 8; i <= 0x1f; i++) // max 2 MB (mirrored)
        mapper[i] = CHIP_MEM;

    for(unsigned i = 0x20; i <= 0x9f; i++)
        mapper[i] = Unmapped; // auto config (e.g. fast ram)

    for(unsigned i = 0xa0; i <= 0xbe; i++)
        mapper[i] = MMIO_CIA; // CIA mirror or overmap for Zorro 2 IO expansion

    mapper[0xbf] = MMIO_CIA;

    for(unsigned i = 0xc0; i <= 0xdb; i++)
        mapper[i] = MMIO_CUSTOM; // mirror

    if (slowMem) { // overmap slow mem (max. 1.75 MB, not mirrored)
        uint8_t page = slowMemSize / (64 * 1024);

        for(unsigned i = 0xc0; i < (0xc0 + page); i++)
            mapper[i] = SLOW_MEM;
    }

    mapper[0xdc] = useRTC ? MMIO_RTC : MMIO_CUSTOM;

    mapper[0xdd] = Unmapped;

    for (unsigned i = 0xde; i <= 0xdf; i++)
        mapper[i] = MMIO_CUSTOM;

    if (extRom) { // AROS
        for (unsigned i = 0xe0; i <= 0xe7; i++)
            mapper[i] = EXT_ROM;
    } else {
        for (unsigned i = 0xe0; i <= 0xe7; i++)
            mapper[i] = romAssignment; // mirror
    }

    for(unsigned i = 0xe8; i <= 0xef; i++)
        mapper[i] = Unmapped; // auto config

    for(unsigned i = 0xf0; i <= 0xf7; i++)
        mapper[i] = Unmapped; // extended ROM, CD32

    for (unsigned i = 0xf8; i <= 0xff; i++)
        mapper[i] = romOrwomAssignment;

    if ((model == OCS_A1000) && !womLock) {
        for (unsigned i = 0xf8; i <= 0xfb; i++)
            mapper[i] = romAssignment;
    }
}

auto Agnus::setOVL(bool state) -> void {
    if (state) {
        for (unsigned i = 0x0; i < 0x8; i++)
            mapper[i] = KICK_ROM;
    } else {
        for (unsigned i = 0x0; i < 0x8; i++)
            mapper[i] = CHIP_MEM;
    }
}

}
