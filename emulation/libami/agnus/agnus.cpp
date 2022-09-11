
#include "agnus.h"
#include "../../tools/sanitizer.h"
#include "../system/system.h"
#include "register.cpp"

namespace LIBAMI {

Agnus::Agnus(Cpu& cpu, Blitter& blitter, Cia& cia1, Cia& cia2) : cpu(cpu), blitter(blitter), cia1(cia1), cia2(cia2) {

    dmaPointerUpdate = [&](uint8_t job, uint16_t data) {

        switch (job) {
            case PTR_BLT_A_H: blitter.setBltAptH(data); break;
            case PTR_BLT_A_L: blitter.setBltAptL(data); break;
            case PTR_BLT_B_H: blitter.setBltBptH(data); break;
            case PTR_BLT_B_L: blitter.setBltBptL(data); break;
            case PTR_BLT_C_H: blitter.setBltCptH(data); break;
            case PTR_BLT_C_L: blitter.setBltCptL(data); break;
            case PTR_BLT_D_H: blitter.setBltDptH(data); break;
            case PTR_BLT_D_L: blitter.setBltDptL(data); break;
        }
    };

    addEvent<Agnus::EVENT_DMA_POINTER>( &dmaPointerUpdate );
}

auto Agnus::reset() -> void {
    eClockPosition = 2;
    mapMemory();
    setOVL(true);
    clearEvents( {EVENT_KBD} ); // don't clear possible running Keyboard event because of hard reset timer
}

auto Agnus::resetOut() -> void {
    system->power( true, true );
}

auto Agnus::pullResetLine(bool state) -> void {
    if (state) {
        system->leaveEmulation = true;
        system->resetFromKeyboard = 1;
    } else {
        system->resetFromKeyboard = false;
    }
}

auto Agnus::mapMemory() -> void {
    uint8_t kickAssignment = kickRom ? KICK_ROM : Unmapped;

    for(unsigned i = 0; i <= 0x1f; i++) // max 2 MB (mirrored)
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
            mapper[i] = kickAssignment; // mirror
    }

    for(unsigned i = 0xe8; i <= 0xef; i++)
        mapper[i] = Unmapped; // auto config

    for(unsigned i = 0xf0; i <= 0xf7; i++)
        mapper[i] = Unmapped; // extended ROM CD32

    for (unsigned i = 0xf8; i <= 0xff; i++)
        mapper[i] = kickAssignment;
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

auto Agnus::addWaitstatesToCPU() -> void {

    countWaitCycles = 0;
    while (busUsage[hPos] != BUS_FREE) {
        dmaCycle();
        countWaitCycles++;
    }

    busUsage[hPos] = BUS_USAGE_CPU;
}

inline auto Agnus::dmaCycle() -> void {
    hPos++;

    if (actions) {
        uint32_t _actions = actions;

        if (_actions & ACT_BLITTER)
            blitter.process();
    }

    processEvents();

    eClockPosition += 2;
    if (eClockPosition == 10) {
        // CIA accesses must be tuned to E-Clock. One CIA BUS cycle corresponds to 2 + [6,8,10,12,14] + 2 CPU cycles.
        // The programming is coordinated in such a way that the CIA is first driven forward and then the register access takes place.
        // Tests, that evaluate CIA timers are difficult because while waiting for access, the CIA internally progresses 1 or 2 cycles.
        // To make matters worse, the E-Clock phase can change with each cold start.
        eClockPosition = 0;
        cia1.clock();
        cia2.clock();
    }
}

auto Agnus::sync(uint16_t cycles) -> void {
    // triggers DMA cycle following current CPU micro cycle.
    // means it is always a DMA cycle ahead of CPU to find out if next cycle is usable for CPU, otherwise we have to take back
    // "sync" of second micro of current BUS cycle to apply right amount of cycles

    // register accesses happen after DMA for that cycle, because a written value can't be used this cycle.
    // a register read is more problematic, because it could wrongly read back changes of this cycle. handle this manually
    while( cycles ) {
        dmaCycle();
        cycles -= 2;
    }
}

auto Agnus::iackCycle(uint8_t level, uint8_t& vector) -> uint8_t {
    vector = 24 + level;
    return M68FAMILY::M68000::USER_VECTOR;
}

auto Agnus::readByte(uint32_t adr) -> uint8_t {
    switch( mapper[adr >> 16] ) {
        case CHIP_MEM:
            addWaitstatesToCPU();
            dataBus = *(chipMem + (adr & chipMemMask));
            break;
        case MMIO_CUSTOM:
            addWaitstatesToCPU();
            break;
        case MMIO_CIA: {
            sync( cpu.internalWaitCyclesBasedOnEClock<2>( eClockPosition ) );
            uint8_t reg = (adr >> 8) & 0xf;
            switch(adr & 0x3000) {
                case 0x0000: dataBus = (adr & 1) ? cia1.read<MOS_8520>( reg ) : cia2.read<MOS_8520>( reg ); break;
                case 0x1000: dataBus = (adr & 1) ? (uint8_t)dataBus : cia2.read<MOS_8520>( reg ); break;
                case 0x2000: dataBus = (adr & 1) ? cia1.read<MOS_8520>( reg ) : (dataBus >> 8); break;
                case 0x3000: dataBus = (adr & 1) ? (uint8_t)dataBus : (dataBus >> 8); break;
            }
        } break;
        case SLOW_MEM:
            addWaitstatesToCPU();
            dataBus = *(slowMem + (adr - 0xc00000));
            break;
        case KICK_ROM:
            dataBus = *(kickRom + (adr & kickRomMask));
            break;
        case EXT_ROM:
            dataBus = *(extRom + (adr & extRomMask));
            break;
        case MMIO_RTC:
            break;
        case Unmapped:
            break;
    }
    return (uint8_t)dataBus;
}

auto Agnus::canBlitterUseBus() -> bool {
    if (busUsage[hPos] != BUS_FREE)
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

    busUsage[hPos] = BUS_USAGE_BLITTER;

    result = _swapWord(*(uint16_t*)(chipMem + (adr & chipMemMask)));

    // if a modified pointer is used in the next cycle, the change is ignored.
    if ((getActiveEvent<EVENT_DMA_POINTER>() & ~1) == ptrEvent)
        setEventInactive<EVENT_DMA_POINTER>();

    return true;
}

auto Agnus::writeBlitterDma(uint32_t adr, uint16_t value) -> bool {
    if(!canBlitterUseBus())
        return false;

    busUsage[hPos] = BUS_USAGE_BLITTER;

    *(uint16_t*)(chipMem + (adr & chipMemMask)) = _swapWord(value);

    if ((getActiveEvent<EVENT_DMA_POINTER>() & ~1) == 8)
        setEventInactive<EVENT_DMA_POINTER>();

    return true;
}

template<uint8_t ptrEvent> auto Agnus::fetchBlitterDmaNoBUSCheck(uint32_t adr, uint16_t& result) -> void {
    busUsage[hPos] = BUS_USAGE_BLITTER;

    result = _swapWord(*(uint16_t*)(chipMem + (adr & chipMemMask)));

    if ((getActiveEvent<EVENT_DMA_POINTER>() & ~1) == ptrEvent)
        setEventInactive<EVENT_DMA_POINTER>();
}

auto Agnus::writeBlitterDmaNoBUSCheck(uint32_t adr, uint16_t value) -> void {
    busUsage[hPos] = BUS_USAGE_BLITTER;

    *(uint16_t*)(chipMem + (adr & chipMemMask)) = _swapWord(value);

    if ((getActiveEvent<EVENT_DMA_POINTER>() & ~1) == 8)
        setEventInactive<EVENT_DMA_POINTER>();
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
            break;
        case MMIO_CIA: {
            // always leads to the time for the next CIA cycle, after which the register is accessed.

            // CIA CS (Chip Select) happens when A12/A13 and VMA (respond of VPA in 68k E-Mode) are active.
            // Gary assert VPA but don't see A12/A13. It only see the upper address bits and knows when in general CIA area.
            // hence Gary asserts VPA, even if no CIA is selected at all ... "case 0x3000" in switch/case below.
            // same applies to CIA writes.
            sync( cpu.internalWaitCyclesBasedOnEClock<2>( eClockPosition ) );
            uint8_t reg = (adr >> 8) & 0xf;
            switch(adr & 0x3000) {
                case 0x0000: dataBus = cia1.read<MOS_8520>( reg ) | (cia2.read<MOS_8520>( reg ) << 8); break;
                case 0x1000: dataBus = (dataBus >> 8) | (cia2.read<MOS_8520>( reg ) << 8); break;
                case 0x2000: dataBus = cia1.read<MOS_8520>( reg ) | (dataBus << 8); break;
                // case 0x3000: break; get last BUS value, should be IRC in most cases
                // E-Clock is used too
            }
        } break;
        case SLOW_MEM:
            addWaitstatesToCPU();
            dataBus = _swapWord(*(uint16_t*)(slowMem + (adr - 0xc00000)));
            break;
        case KICK_ROM:
            dataBus = _swapWord(*(uint16_t*)(kickRom + (adr & kickRomMask)));
            break;
        case EXT_ROM:
            dataBus = _swapWord(*(uint16_t*)(extRom + (adr & extRomMask)));
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
            dataBus = *(chipMem + (adr & chipMemMask)) = value;
            break;
        case MMIO_CUSTOM:
            addWaitstatesToCPU();
            writeCustom( adr & 0x1fe, value | (value << 8) );
            break;
        case MMIO_CIA: {
            sync( cpu.internalWaitCyclesBasedOnEClock<2>( eClockPosition ) );
            uint8_t reg = (adr >> 8) & 0xf;
            if ((adr & 0x1000) == 0)
                cia1.write<MOS_8520>( reg, value );
            if ((adr & 0x2000) == 0)
                cia2.write<MOS_8520>( reg, value );
        } break;
        case SLOW_MEM:
            addWaitstatesToCPU();
            dataBus = *(slowMem + (adr - 0xc00000)) = value;
            break;
        case KICK_ROM:
            break;
        case EXT_ROM:
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
            *(uint16_t*)(chipMem + (adr & chipMemMask)) = _swapWord(value);
            break;
        case MMIO_CUSTOM:
            addWaitstatesToCPU();
            writeCustom( adr & 0x1fe, value );
            break;
        case MMIO_CIA: {
            sync( cpu.internalWaitCyclesBasedOnEClock<2>( eClockPosition ) );
            uint8_t reg = (adr >> 8) & 0xf;
            if ((adr & 0x1000) == 0)
                cia1.write<MOS_8520>( reg, (uint8_t)value );
            if ((adr & 0x2000) == 0)
                cia2.write<MOS_8520>( reg, (uint8_t)(value >> 8) );
        } break;
        case SLOW_MEM:
            addWaitstatesToCPU();
            *(uint16_t*)(slowMem + (adr - 0xc00000)) = _swapWord(value);
            break;
        case KICK_ROM:
        case EXT_ROM:
            break;
        case MMIO_RTC:
            break;
        case Unmapped:
            break;
    }
    dataBus = value;
}

auto Agnus::setMemory(unsigned typeId, unsigned size) -> void {
    switch (typeId) {
        case 0: // Chip mem
        default: {
            if (size == 0)
                size = 512 * 1024;
            else if (size > (2 * 1024 * 1024))
                size = 2 * 1024 * 1024;
            else
                size = Emulator::powerOfTwo(size);

            unsigned mask = size - 1;

            if (mask == chipMemMask)
                break;

            if (chipMem)
                delete[] chipMem;

            chipMem = new uint8_t[size];
            chipMemMask = mask;
        } break;

        case 1: { // slow mem
            if (size > (1792 * 1024))
                size = 1792 * 1024;

            if (size == slowMemSize)
                break;

            if (slowMem)
                delete[] slowMem;

            slowMem = nullptr;
            if (size)
                slowMem = new uint8_t[size];
            slowMemSize = size;
        } break;
    }
}


}
