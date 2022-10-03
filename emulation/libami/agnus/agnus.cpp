
#include "agnus.h"
#include "../../tools/sanitizer.h"
#include "../system/system.h"
#include "register.cpp"

#define ERSY (bplCon0 & 2)

namespace LIBAMI {

Agnus::Agnus(Cpu& cpu, Blitter& blitter, Copper& copper, Cia& cia1, Cia& cia2) : cpu(cpu), blitter(blitter), copper(copper), cia1(cia1), cia2(cia2) {

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
            case PTR_REF: setRefPtr(data); break;
        }
    };

    addEvent<Agnus::EVENT_DMA_POINTER>( &dmaPointerUpdate );

    regUpdate = [&](uint8_t job, uint16_t data) {

        switch (job) {
            case DMACON:
                if (data & 0x8000)
                    dmaCon |= data & 0x7ff;
                else
                    dmaCon &= ~data; // no masking needed, unused upper 5 bits will never be set
                break;
        }
    };

    addEvent<Agnus::EVENT_REG_UPDATE>( &regUpdate );

    leaveEmulation = [&](uint8_t job, uint16_t data) {
        // When a frame is fully calculated, control is given back to the user interface.
        // Frequent changes in position (VHPOSW) can cause this to never happen or only after a very long time. In order to keep the user interface responsive,
        // control must be returned in a timely manner.
        system->leaveEmulation = true;
    };

    addEvent<Agnus::EVENT_LEAVE_EMULATION>( &leaveEmulation );
}

auto Agnus::reset() -> void {
    eClockPosition = 2;
    hPos = 5; // 4 + 1 (ahead)
    lol = false;
    lolToggle = ntsc;
    lof = false;
    lofToggle = false;
    lines = ntsc ? 261 : 311;
    rDmaPtr = 0;

    blitter.reset();
    copper.reset();
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
        system->resetFromKeyboard = 0;
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

    switch(hPos) {
        case 1:
            if (ERSY) {
                hPos = 0; // need external sync to proceed
                
                if (++resyncCounter == 1000000) {
                    resyncCounter = 0;
                    system->leaveEmulation = true; // sync up user interface
                }
            }
            break;

        case 2:
            if (initVCounter) {
                initVCounter = false;
                vPos = 0;
            } else {
                vPos++;
                vPos &= ecsAndHigher() ? 0x7ff : 0x1ff; // register change of VPos could lead to a wrap around of 9-bit (OCS Agnus) counter.
            }
            actions |= ACT_COPPER;
            break;

        case 3: // adjust HRM DMA view by 4 cycles to Beam position
            if (vPos == 0) {
                copper.strobeCOPJMP(false, Trigger_Vsync);
            }
        case 5:
        case 7:
        case 9:
            // for AGA chipset, DRAM use internal refresh counter. AGA chipset only triggers Refresh but can't control the refresh position.
            // We don't need this pointer in an emulator to prevent memory locations from losing their charge.
            // However, Bitplane Fetch and Refresh Slot can overlap. In this case, the Refresh DMA pointer affects the current Bitplane DMA address.
            // Furthermore, by permanently overwriting the horizontal Agnus position, it can be prevented that memory cells are updated in time.
            if (!ecs()) {
                // if (someAdr & 0x201fe == rDmaPtr & 0x201fe )  // not interested in CAS, so we mask it out
                    // 1024 bytes to refresh for each rDmaPtr (same ptr is used for slow ram)

                // To emulate memory loss, a timer would have to run for each "rDmaPtr". That would be 512 parallel timers,
                // because "rDmaPtr" could be always updated by writing to "refPtr" register.
                // the timer for a specific "rDmaPtr" is reset at this point and should at best never expire. If this expires, some 1 bits must be set to 0
                // for 1024 memory addresses + 1024 more if slow mem is active. probably all bits of a memory address expire slightly offset.
                // emulating this would consume too much performance for nothing, because if this happens, the program can no longer work meaningfully.

                rDmaPtr += 2; // 16-bit memory access
                rDmaPtr &= chipMemMask;
            } else if (!aga()) {
                rDmaPtr += 0x200; // RAS and CAS were replaced. To increase RAS, the "adder" must be moved 8 bits
                rDmaPtr &= chipMemMask;
            }
            if ((getActiveEvent<EVENT_DMA_POINTER>() ) == PTR_REF)
                setEventInactive<EVENT_DMA_POINTER>(); // ignore it, because was updated last cycle

            busUsage[hPos] = BUS_USAGE_REFRESH;
            break;

        case 0xe1:
            break;

        case 0xe2:
            if (!lol) {
                if (vPos == (lines + lof) ) {
                    if (lofToggle) lof ^= 1;
                    initVCounter = true;
                }
            }
            break;
        case 0xe3:
            if (!lol) {// short line
                hPos = 0;
                if (lolToggle) lol ^= 1;
                shortLineBefore = true;
                actions &= ~ACT_COPPER; // "even" cycle 0 after a short line is not usable by Copper, otherwise Copper would progress 2 cycles in a row.
            } else {
                shortLineBefore = false;
                if (vPos == (lines + lof) ) {
                    if (lofToggle) lof ^= 1;
                    initVCounter = true;
                }
            }
            break;
        case 0xe4: // NTSC long line
            hPos = 0;
            if (lolToggle) lol ^= 1;
            break;
        // register change of HPos could lead to a wrap around of 8 bit counter.
    }

    if (actions) {
        uint32_t _actions = actions;

        if (_actions & ACT_COPPER) {
            if ((hPos & 1) == 0)
                copper.process();
        }

        if (_actions & ACT_BLITTER)
            blitter.process();
    }

    processEvents();
    hPos++; // reading V(H)POS get the already incremented (pointing to next cycle) position, Copper uses the current position for comparisons.
    // to avoid another switch/case we don't update hPos wraparounds or vPos incrementations now. if VPOS is readed, we temporarly do these steps.

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

auto Agnus::POSR(bool vhpos) -> uint16_t {
    auto h = hPos;
    auto v = vPos;
    auto _lol = lol;
    auto _lof = lof;

    // for performance reasons, following is calculated with beginning of next cycle.
    if (h == 1) {
        if (ERSY) h = 0;
    } else if (h == 2) {
        if (initVCounter) v = 0;
        else { v++; v &= ecsAndHigher() ? 0x7ff : 0x1ff; }
    } else if (h == 0xe2) {
        if (!_lol && lofToggle && (v == (lines + _lof) )) _lof ^= 1;
    } else if (h == 0xe3) {
        if (!_lol) {
            h = 0;
            if (lolToggle) _lol ^= 1;
        } else {
            if (lofToggle && (v == (lines + _lof) )) _lof ^= 1;
        }
    } else if (h == 0xe4) {
        h = 0;
        if (lolToggle) _lol ^= 1;
    }

    // VHPOSR
    if (vhpos)
        return ((v & 0xff) << 8) | h;

    // VPOSR
    if (ecsAndHigher())
        return ((v >> 8) & 7) | (_lof << 15) | (_lol << 7) | (ntsc << 12) | (1 << 13) | (aga() ? 3 : 0);

    return ((v >> 8) & 1) | (_lof << 15) | (ntsc << 12);
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
            if (adr & 1)
                dataBus = (uint8_t)readCustom<true>(adr & 0x1fe);
            else
                dataBus = (uint8_t)(readCustom<true>(adr) >> 8);
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

auto Agnus::canCopperUseBus() -> bool {
    if (busUsage[hPos] != BUS_FREE)
        return false; // a higher DMA

    if (!useCopperDMA())
        return false;

    return true;
};

auto Agnus::allocateCopper() -> bool {
    if (canCopperUseBus()) {
        busUsage[hPos] = BUS_USAGE_COPPER;
        return true;
    }
    return false;
}

auto Agnus::fetchCopperDma(uint32_t adr, uint16_t& result) -> bool {
    if(!canCopperUseBus())
        return false;

    busUsage[hPos] = BUS_USAGE_COPPER;

    result = _swapWord(*(uint16_t*)(chipMem + (adr & chipMemMask)));

    dataBus = result;

    return true;
}

auto Agnus::fetchCopperDmaNoBUSCheck(uint32_t adr, uint16_t& result) -> void {

    busUsage[hPos] = BUS_USAGE_COPPER;

    result = _swapWord(*(uint16_t*)(chipMem + (adr & chipMemMask)));

    dataBus = result;
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

    dataBus = result;

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

    dataBus = value;

    if ((getActiveEvent<EVENT_DMA_POINTER>() & ~1) == PTR_BLT_D_H)
        setEventInactive<EVENT_DMA_POINTER>();

    return true;
}

template<uint8_t ptrEvent> auto Agnus::fetchBlitterDmaNoBUSCheck(uint32_t adr, uint16_t& result) -> void {
    busUsage[hPos] = BUS_USAGE_BLITTER;

    result = _swapWord(*(uint16_t*)(chipMem + (adr & chipMemMask)));

    dataBus = result;

    if ((getActiveEvent<EVENT_DMA_POINTER>() & ~1) == ptrEvent)
        setEventInactive<EVENT_DMA_POINTER>();
}

auto Agnus::writeBlitterDmaNoBUSCheck(uint32_t adr, uint16_t value) -> void {
    busUsage[hPos] = BUS_USAGE_BLITTER;

    *(uint16_t*)(chipMem + (adr & chipMemMask)) = _swapWord(value);

    dataBus = value;

    if ((getActiveEvent<EVENT_DMA_POINTER>() & ~1) == PTR_BLT_D_H)
        setEventInactive<EVENT_DMA_POINTER>();
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

            if (size <= (512 * 1024))
                mode = Mode::OCS;
            else
                mode = Mode::ECS;

            unsigned mask = size - 1;

            if (mask == chipMemMask)
                break;

            if (chipMem)
                delete[] chipMem;

            chipMem = new uint8_t[size];
            chipMemMask = mask;
        } break;

        case 1: { // Slow mem
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

auto Agnus::setRefPtr(uint16_t value) -> void {
    // todo AGA
    // http://eab.abime.net/showthread.php?p=1542797
    rDmaPtr = value & 0xfffe;
    if (value & 1)
        rDmaPtr |= 0x10000;
    if (value & 2)
        rDmaPtr |= 0x20000;
    if (value & 4)
        rDmaPtr |= 0x40000;

    if (ecs()) {
        if (value & 8) // 1 MB ECS
            rDmaPtr |= 0x80000;

        if (chipMemMask == 0x1fffff) { // 2 MB ECS
            if (value & 16)
                rDmaPtr |= 0x100000;
        }
    }
}

template auto Agnus::fetchBlitterDmaNoBUSCheck<Agnus::PTR_BLT_A_H>(uint32_t adr, uint16_t& result) -> void;
template auto Agnus::fetchBlitterDmaNoBUSCheck<Agnus::PTR_BLT_B_H>(uint32_t adr, uint16_t& result) -> void;
template auto Agnus::fetchBlitterDmaNoBUSCheck<Agnus::PTR_BLT_C_H>(uint32_t adr, uint16_t& result) -> void;

template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_A_H>(uint32_t adr, uint16_t& result) -> bool;
template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_B_H>(uint32_t adr, uint16_t& result) -> bool;
template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_C_H>(uint32_t adr, uint16_t& result) -> bool;

}
