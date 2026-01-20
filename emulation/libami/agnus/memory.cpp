
#include "agnus.h"

#define eCyclePosition  10 - ((eClockCycle - clock) << 1)

namespace LIBAMI {

#define LOG_DMA(res) if constexpr (logDma) { \
    auto& dmaLogger = debugger.dma[hPos]; \
    dmaLogger.usage = BUS_USAGE_CPU; \
    dmaLogger.mapper = _map; \
    dmaLogger.address = dmaLogger.addrCpu = addrBus = adr; \
    dmaLogger.data = dmaLogger.dataCpu = res; \
    peekDmaWatcher(dmaLogger); \
}

#define LOG_DMA_CPU(res) if constexpr (logDma) { \
    auto& dmaLogger = debugger.dma[hPos]; \
    dmaLogger.mapper = _map; \
    dmaLogger.addrCpu = adr; \
    dmaLogger.dataCpu = res; \
}

auto Agnus::readByte(uint32_t adr) -> uint8_t {
    return debugger.dmaLog ? readByte<true>(adr) : readByte<false>(adr);
}

template<bool logDma> auto Agnus::readByte(uint32_t adr) -> uint8_t {
    adr &= 0xffffff;
    uint8_t _map = mapper[adr >> 16];
    uint8_t res;

    switch( _map ) {
        case CHIP_MEM:
            addWaitstatesToCPU<logDma>();
            res = *(chipMem + (adr & chipMemMask));
            dataBus = (res << 8) | res;
            LOG_DMA(res)
            break;
        case MMIO_CUSTOM:
            addWaitstatesToCPU<logDma>();
            if (adr & 1)
                res = (uint8_t)readCustom<true>(adr & 0x1fe);
            else
                res = (uint8_t)(readCustom<true>(adr & 0x1fe) >> 8);
            dataBus = (res << 8) | res;
            LOG_DMA(res)
            break;
        case MMIO_CIA: {
            sync( cpu.internalWaitCyclesBasedOnEClock<2>( eCyclePosition ) );
            uint8_t reg = (adr >> 8) & 0xf;
            switch(adr & 0x3000) {
                default:
                case 0x0000: res = (adr & 1) ? cia1.read( reg ) : cia2.read( reg ); break;
                case 0x1000: res = (adr & 1) ? (uint8_t)cpu.getIRC() : cia2.read( reg ); break;
                case 0x2000: res = (adr & 1) ? cia1.read( reg ) : (cpu.getIRC() >> 8); break;
                case 0x3000: res = (adr & 1) ? (uint8_t)cpu.getIRC() : (cpu.getIRC() >> 8); break;
            }
            LOG_DMA_CPU(res)
            break;
        }
        case SLOW_MEM:
            addWaitstatesToCPU<logDma>();
            res = *(slowMem + (adr - 0xc00000));
            dataBus = (res << 8) | res;
            LOG_DMA(res)
            break;
        case FAST_MEM:
            res = *(fastMem + (adr - fastMemExpansion.baseAdr));
            LOG_DMA_CPU(res)
            break;
        case KICK_ROM:
            res = *(kickRom + (adr & kickRomMask));
            LOG_DMA_CPU(res)
            break;
        case EXPANSION:
            res = expansionsConfigured[adr >> 16]->read(adr);
            LOG_DMA_CPU(res)
            break;
        case EXT_ROM:
            res = *(extRom + (adr & extRomMask));
            LOG_DMA_CPU(res)
            break;
        case WOM:
            res = *(wom + (adr & 0x3ffff));
            LOG_DMA_CPU(res)
            break;
        case MMIO_RTC:
            res = (adr & 1) ? rtc.read( (adr >> 2) & 0xf, system->isProcessFrame() ) : (cpu.getIRC() >> 8);
            LOG_DMA_CPU(res)
            break;
        case AUTO_CONF:
            res = readAutoConf(adr);
            LOG_DMA_CPU(res)
            break;
        default:
        case Unmapped:
            res = static_cast<uint8_t>(cpu.getIRC());
            LOG_DMA_CPU(res)
            break;
    }
    return res;
}

auto Agnus::peekWord(uint32_t adr) -> uint16_t {
    adr &= 0xffffff;

    switch( mapper[adr >> 16] ) {
        case CHIP_MEM:
            return _swapWord(*(uint16_t*)(chipMem + (adr & chipMemMask)));
        case MMIO_CUSTOM:
            return peekCustom(adr & 0x1fe);
        case MMIO_CIA: {
            uint8_t reg = (adr >> 8) & 0xf;
            switch(adr & 0x3000) {
                case 0x0000: return cia1.peek( reg ) | (cia2.peek( reg ) << 8);
                case 0x1000: return (cpu.getIRC() >> 8) | (cia2.peek( reg ) << 8);
                case 0x2000: return cia1.peek( reg ) | (cpu.getIRC() << 8);
            }
        } break;
        case SLOW_MEM:
            return _swapWord(*(uint16_t*)(slowMem + (adr - 0xc00000)));
        case FAST_MEM:
            return _swapWord(*(uint16_t*)(fastMem + (adr - fastMemExpansion.baseAdr)));
        case KICK_ROM:
            return _swapWord(*(uint16_t*)(kickRom + (adr & kickRomMask)));
        case EXPANSION:
            return expansionsConfigured[adr >> 16]->peekW(adr);
        case EXT_ROM:
            return _swapWord(*(uint16_t*)(extRom + (adr & extRomMask)));
        case WOM:
            return _swapWord(*(uint16_t*)(wom + (adr & 0x3ffff)));
        case MMIO_RTC:
            return (cpu.getIRC() & 0xff00) | rtc.read( (adr >> 2) & 0xf, true );
        case AUTO_CONF:
            return (readAutoConf(adr) << 8) | readAutoConf(adr + 1);
        case Unmapped:
            break;
    }

    return dataBus;
}

auto Agnus::memoryDump(uint8_t bank, uint16_t* dump) -> void {
    uint16_t _irc = cpu.getIRC();

    switch( mapper[bank] ) {
        case CHIP_MEM: {
            for (unsigned addr = bank << 16; addr < ((bank << 16) | 0xffff); addr += 2)
                *dump++ = *(uint16_t*)(chipMem + (addr & chipMemMask));
        } break;

        case MMIO_CUSTOM: {
            uint16_t temp[0x200];
            for (unsigned addr = 0; addr < 0x200; addr += 2)
                temp[addr] = peekCustom(addr & 0x1fe);

            for (unsigned addr = 0; addr < 0xffff; addr += 2)
                *dump++ = temp[addr & 0x1fe];
        } break;
        case MMIO_CIA: {
            uint8_t tempC1[16];
            uint8_t tempC2[16];
            for (uint8_t a = 0; a < 16; a++)
                tempC1[a] = cia1.peek( a );
            for (uint8_t a = 0; a < 16; a++)
                tempC2[a] = cia2.peek( a );
            for (unsigned addr = 0; addr < 0xffff; addr += 2) {
                uint8_t reg = (addr >> 8) & 0xf;
                switch(addr & 0x3000) {
                    case 0x0000: *dump++ = tempC1[reg] | (tempC2[reg] << 8); break;
                    case 0x1000: *dump++ = (_irc >> 8) | (tempC2[reg] << 8); break;
                    case 0x2000: *dump++ =  tempC1[reg] | (_irc << 8); break;
                    default: *dump++ = _irc; break;
                }
            }
        } break;
        case SLOW_MEM: {
            for (unsigned addr = bank << 16; addr < ((bank << 16) | 0xffff); addr += 2)
                *dump++ = *(uint16_t*)(slowMem + (addr - 0xc00000));
        } break;
        case FAST_MEM: {
            for (unsigned addr = bank << 16; addr < ((bank << 16) | 0xffff); addr += 2)
                *dump++ = *(uint16_t*)(fastMem + (addr - fastMemExpansion.baseAdr));
        } break;
        case KICK_ROM: {
            for (unsigned addr = bank << 16; addr < ((bank << 16) | 0xffff); addr += 2)
                *dump++ = *(uint16_t*)(kickRom + (addr & kickRomMask));
        } break;

        case EXPANSION:
            for (unsigned addr = bank << 16; addr < ((bank << 16) | 0xffff); addr += 2)
                *dump++ = expansionsConfigured[addr >> 16]->peekW(addr);
            break;
        case EXT_ROM: {
            for (unsigned addr = bank << 16; addr < ((bank << 16) | 0xffff); addr += 2)
                *dump++ = *(uint16_t*)(extRom + (addr & extRomMask));
        } break;
        case WOM: {
            for (unsigned addr = bank << 16; addr < ((bank << 16) | 0xffff); addr += 2)
                *dump++ = *(uint16_t*)(wom + (addr & 0x3ffff));
        } break;
        case MMIO_RTC: {
            uint8_t tempRTC[16];
            for (uint8_t a = 0; a < 16; a++)
                tempRTC[a] = rtc.read( a, true );
            for (unsigned addr = 0; addr < 0xffff; addr += 2)
                *dump++ = (_irc & 0xff00) | tempRTC[(addr >> 2) & 0xf];
        } break;
        case AUTO_CONF:
            for (unsigned addr = 0; addr < 0xffff; addr += 2)
                *dump++ = (readAutoConf(addr) << 8) | readAutoConf(addr + 1);
            break;
        default:
        case Unmapped:
            for (unsigned addr = 0; addr < 0xffff; addr += 2)
                *dump++ = _irc;
            break;
    }
}

auto Agnus::readWord(uint32_t adr) -> uint16_t {
    return debugger.dmaLog ? readWord<true>(adr) : readWord<false>(adr);
}

template<bool logDma> auto Agnus::readWord(uint32_t adr) -> uint16_t {
    adr &= 0xffffff;
    uint8_t _map = mapper[adr >> 16];
    // 68k is big endian, modern architecture is little endian
    switch( _map ) {
        case CHIP_MEM:
            addWaitstatesToCPU<logDma>();
            dataBus = _swapWord(*(uint16_t*)(chipMem + (adr & chipMemMask)));
            LOG_DMA(dataBus)
            break;
        case MMIO_CUSTOM:
            addWaitstatesToCPU<logDma>();
            dataBus = readCustom(adr & 0x1fe);
            LOG_DMA(dataBus)
            break;
        case MMIO_CIA: {
            // CIA CS (Chip Select) happens when A12/A13 and VMA (respond of VPA in 68k E-Mode) are active.
            // Gary assert VPA but don't see A12/A13. It only sees the upper address bits and knows when in general CIA area.
            // hence Gary asserts VPA, even if no CIA is selected at all ... "case 0x3000" in switch/case below.
            // same applies to CIA writes.
            sync( cpu.internalWaitCyclesBasedOnEClock<2>( eCyclePosition ) );
            uint8_t reg = (adr >> 8) & 0xf;
            uint16_t res;

            switch(adr & 0x3000) {
                default:
                case 0x0000: res = cia1.read( reg ) | (cia2.read( reg ) << 8); break;
                case 0x1000: res = (cpu.getIRC() >> 8) | (cia2.read( reg ) << 8); break;
                case 0x2000: res = cia1.read( reg ) | (cpu.getIRC() << 8); break;
                case 0x3000: res = cpu.getIRC(); break;
            }
            LOG_DMA_CPU( res )
            return res;
        };
        case SLOW_MEM:
            addWaitstatesToCPU<logDma>();
            dataBus = _swapWord(*(uint16_t*)(slowMem + (adr - 0xc00000)));
            LOG_DMA(dataBus)
            break;
        case FAST_MEM: {
            uint16_t res = _swapWord(*(uint16_t*)(fastMem + (adr - fastMemExpansion.baseAdr)));
            LOG_DMA_CPU( res )
            return res;
        }
        case KICK_ROM: {
            uint16_t res = _swapWord(*(uint16_t*)(kickRom + (adr & kickRomMask)));
            LOG_DMA_CPU( res )
            return res;
        }
        case EXPANSION: {
            uint16_t res = expansionsConfigured[adr >> 16]->readW(adr);
            LOG_DMA_CPU( res )
            return res;
        }
        case EXT_ROM: {
            uint16_t res = _swapWord(*(uint16_t*)(extRom + (adr & extRomMask)));
            LOG_DMA_CPU( res )
            return res;
        }
        case WOM: {
            uint16_t res = _swapWord(*(uint16_t*)(wom + (adr & 0x3ffff)));
            LOG_DMA_CPU( res )
            return res;
        }
        case MMIO_RTC: {
            uint16_t res = cpu.getIRC() & ~0xff;
            res |= rtc.read( (adr >> 2) & 0xf, system->isProcessFrame() );
            LOG_DMA_CPU(res)
            return res;
        }
        case AUTO_CONF: {
            uint16_t res = readAutoConf(adr) << 8;
            res |= readAutoConf(adr + 1);
            LOG_DMA_CPU(res)
            return res;
        }
        default:
        case Unmapped: {
            // floating BUS, don't return zero (Hollywood Poker Pro)
            uint16_t res = cpu.getIRC();
            LOG_DMA_CPU(res)
            return res;
        }
    }

    return dataBus;
}

auto Agnus::writeByte(uint32_t adr, uint8_t value) -> void {
    debugger.dmaLog ? writeByte<true>(adr, value) : writeByte<false>(adr, value);
}

template<bool logDma> auto Agnus::writeByte(uint32_t adr, uint8_t value) -> void {
    adr &= 0xffffff;
    uint8_t _map = mapper[adr >> 16];

    switch( _map ) {
        case CHIP_MEM:
            addWaitstatesToCPU<logDma>();
            adr &= chipMemMask;
            if (memState)
                memState->trackers[TRACKER_CHIP].remember(adr & ~1);

            *(chipMem + adr) = value;
            dataBus = (value << 8) | value;
            LOG_DMA(value)
            break;
        case MMIO_CUSTOM:
            addWaitstatesToCPU<logDma>();
            writeCustom( adr & 0x1fe, value | (value << 8) );
            dataBus = (value << 8) | value;
            LOG_DMA(value)
            break;
        case MMIO_CIA: {
            sync( cpu.internalWaitCyclesBasedOnEClock<2>( eCyclePosition ) );
            uint8_t reg = (adr >> 8) & 0xf;
            if ((adr & 0x1000) == 0)
                cia1.write( reg, value );
            if ((adr & 0x2000) == 0)
                cia2.write( reg, value );

            dataBus = (value << 8) | value;
            LOG_DMA_CPU( value )
        } break;
        case SLOW_MEM:
            addWaitstatesToCPU<logDma>();
            adr -= 0xc00000;
            if (memState)
                memState->trackers[TRACKER_SLOW].remember(adr & ~1);
            *(slowMem + adr) = value;
            dataBus = (value << 8) | value;
            LOG_DMA(value)
            break;
        case FAST_MEM:
            adr -= fastMemExpansion.baseAdr;
            if (memState)
                memState->trackers[TRACKER_FAST].remember(adr & ~1);
            *(fastMem + adr) = value;
            LOG_DMA_CPU( value )
            break;
        case KICK_ROM:
            if ( (model == OCS_A1000) && !womLock)
                lockWom();
            LOG_DMA_CPU( value )
            break;
        case EXPANSION:
            expansionsConfigured[adr >> 16]->write(adr, value);
            LOG_DMA_CPU( value )
            break;
        case EXT_ROM:
            LOG_DMA_CPU( value )
            break;
        case WOM:
            if (!womLock) *(wom + (adr & 0x3ffff)) = value;
            LOG_DMA_CPU( value )
            break;
        case MMIO_RTC:
            if ((adr & 1) && system->isProcessFrame())
                rtc.write( (adr >> 2) & 0xf, value );
            LOG_DMA_CPU( value )
            break;
        case AUTO_CONF:
            writeAutoConf(adr, value);
            LOG_DMA_CPU( value )
            break;
        case Unmapped:
            LOG_DMA_CPU( value )
            break;
    }
}

auto Agnus::writeWord(uint32_t adr, uint16_t value) -> void {
    debugger.dmaLog ? writeWord<true>(adr, value) : writeWord<false>(adr, value);
}

template<bool logDma> auto Agnus::writeWord(uint32_t adr, uint16_t value) -> void {
    adr &= 0xffffff;
    uint8_t _map = mapper[adr >> 16];

    switch( _map ) {
        case CHIP_MEM:
            addWaitstatesToCPU<logDma>();
            adr &= chipMemMask;
            if (memState)
                memState->trackers[TRACKER_CHIP].remember(adr);

            *(uint16_t*)(chipMem + adr) = _swapWord(value);
            dataBus = value;
            LOG_DMA(value)
            break;
        case MMIO_CUSTOM:
            addWaitstatesToCPU<logDma>();
            writeCustom( adr & 0x1fe, value );
            dataBus = value;
            LOG_DMA(value)
            break;
        case MMIO_CIA: {
            sync( cpu.internalWaitCyclesBasedOnEClock<2>( eCyclePosition ) );
            uint8_t reg = (adr >> 8) & 0xf;
            if ((adr & 0x1000) == 0)
                cia1.write( reg, (uint8_t)value );
            if ((adr & 0x2000) == 0)
                cia2.write( reg, (uint8_t)(value >> 8) );
            LOG_DMA_CPU( value )
        } break;
        case SLOW_MEM:
            addWaitstatesToCPU<logDma>();
            adr -= 0xc00000;
            if (memState)
                memState->trackers[TRACKER_SLOW].remember(adr);
            *(uint16_t*)(slowMem + adr) = _swapWord(value);
            dataBus = value;
            LOG_DMA(value)
            break;
        case FAST_MEM:
            adr -= fastMemExpansion.baseAdr;
            if (memState)
                memState->trackers[TRACKER_FAST].remember(adr);
            *(uint16_t*)(fastMem + adr) = _swapWord(value);
            LOG_DMA_CPU( value )
            break;
        case KICK_ROM:
            if ( (model == OCS_A1000) && !womLock)
                lockWom();
            LOG_DMA_CPU( value )
            break;
        case EXPANSION:
            expansionsConfigured[adr >> 16]->writeW(adr, value);
            LOG_DMA_CPU( value )
            break;
        case EXT_ROM:
            LOG_DMA_CPU( value )
            break;
        case WOM:
            if (!womLock) *(uint16_t*)(wom + (adr & 0x3ffff)) = _swapWord(value);
            LOG_DMA_CPU( value )
            break;
        case MMIO_RTC:
            if (system->isProcessFrame())
                rtc.write( (adr >> 2) & 0xf, value & 0xff );
            LOG_DMA_CPU( value )
            break;
        case AUTO_CONF:
            writeAutoConfWord(adr, value);
            LOG_DMA_CPU( value )
            break;
        case Unmapped:
            LOG_DMA_CPU( value )
            break;
    }
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

auto Agnus::updateMemorySnapshot(DebuggerSnapshot& snap) -> void {
    std::memcpy(snap.mapper, mapper, sizeof(mapper));
}


}
