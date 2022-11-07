
#include "agnus.h"
#include "../../tools/sanitizer.h"
#include "../system/system.h"
#include "register.cpp"
#include "dma.cpp"

#define ERSY (bplCon0 & 2)
#define eCyclePosition  10 - (((eClockCycle - clock) & 0xffffffff) << 1)

namespace LIBAMI {

Agnus::Agnus(Cpu& cpu, Denise& denise, Blitter& blitter, Copper& copper, Cia<MOS_8520>& cia1, Cia<MOS_8520>& cia2, Input& input)
: cpu(cpu), denise(denise), blitter(blitter), copper(copper), cia1(cia1), cia2(cia2), input(input) {

    oneCycleDelay = [&](uint8_t job, uint16_t data) {

        switch (job) {
            case PTR_BLT_A_H: blitter.setBltAptH(data); break;
            case PTR_BLT_A_L: blitter.setBltAptL(data); break;
            case PTR_BLT_B_H: blitter.setBltBptH(data); break;
            case PTR_BLT_B_L: blitter.setBltBptL(data); break;
            case PTR_BLT_C_H: blitter.setBltCptH(data); break;
            case PTR_BLT_C_L: blitter.setBltCptL(data); break;
            case PTR_BLT_D_H: blitter.setBltDptH(data); break;
            case PTR_BLT_D_L: blitter.setBltDptL(data); break;
            case DMACON: dmaCon = dmaConImm; break;
            case BLT_INIT: blitter.initBlit(); break;
            case BLT_BUSY_DELAY: break;
        }
    };

    addEvent<Agnus::EVENT_ONE_CYCLE_DELAY>( &oneCycleDelay );

    leaveEmulation = [&](uint8_t job, uint16_t data) {
        // When a frame is fully calculated, control is given back to the user interface.
        // Frequent changes in position (VHPOSW) can cause this to never happen or only after a very long time. In order to keep the user interface responsive,
        // control must be returned in a timely manner.
        system->leaveEmulation = true;
    };

    addEvent<Agnus::EVENT_LEAVE_EMULATION>( &leaveEmulation );

    countDownPowerSupply = [&](uint8_t job, uint16_t data) {
        cia1.tod( );

        updateEvent<EVENT_POWER_SUPPLY>(~0, powerSupply.nextTickCount());
    };

    addEvent<Agnus::EVENT_POWER_SUPPLY>( &countDownPowerSupply );

    wom = new uint8_t[256 * 1024];
}

auto Agnus::dmaControl(uint16_t data) -> void {
    if (data & 0x8000)
        dmaConImm |= data & 0x7ff;
    else
        dmaConImm &= ~data; // no masking needed, unused upper 5 bits will never be set
}

auto Agnus::reset(bool softReset) -> void {
    auto resetDelay = getEventDelay<EVENT_KBD>();
    clearEvents();

    hPos = 4;
    vPos = 0;
    lol = false;
    lolToggle = ntsc;
    lof = false;
    lofToggle = false;
    lines = ntsc ? 261 : 311;
    rDmaPtr = 0;
    if (!softReset) {
        resetFromKeyboard = 0;
        womLock = false;
        std::memset(wom, 0, 256 * 1024);
    } else {
        if (resetFromKeyboard && resetDelay)
            updateEvent<EVENT_KBD>(Keyboard::KBD_Hardreset, resetDelay);
    }

    denise.power();
    blitter.reset();
    copper.reset();
    mapMemory();
    setOVL(true);

    if (model == A1000) {
        powerSupply.init((ntsc ? FREQUENCY_NTSC : FREQUENCY_PAL) >> 3, ntsc ? 60 : 50);
        updateEvent<EVENT_POWER_SUPPLY>(~0, powerSupply.nextTickCount());
    } else
        setEventInactive<EVENT_POWER_SUPPLY>();

    if (!softReset || !resetFromKeyboard)
        initCiaClock();

}

auto Agnus::initCiaClock() -> void {
    eClockCycle = clock + 4; // begin a DMA cycle later
}

auto Agnus::resetOut() -> void {
    system->power( true, true );
}

auto Agnus::pullResetLine(bool state) -> void {
    if (state) {
        system->leaveEmulation = true;
        resetFromKeyboard = 1;
    } else {
        resetFromKeyboard = 0;
    }
}

auto Agnus::mapMemory() -> void {
    uint8_t romAssignment = kickRom ? KICK_ROM : Unmapped;
    uint8_t romOrwomAssignment = (model == A1000) ? WOM : romAssignment;

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
            mapper[i] = romAssignment; // mirror
    }

    for(unsigned i = 0xe8; i <= 0xef; i++)
        mapper[i] = Unmapped; // auto config

    for(unsigned i = 0xf0; i <= 0xf7; i++)
        mapper[i] = Unmapped; // extended ROM, CD32

    for (unsigned i = 0xf8; i <= 0xff; i++)
        mapper[i] = romOrwomAssignment;

    if ((model == A1000) && !womLock) {
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

auto Agnus::waitKeyboardReset() -> void {
    if ((resetFromKeyboard & 0x80) == 0) {
        system->power(true);
        resetFromKeyboard |= 0x80;
    }

    updateEvent<EVENT_LEAVE_EMULATION>(~0, 150000);

    while(true) { // CPU and most chips on hold, Denise hasn't a reset line
        input.checkForEmergencyPoll(); // wait for releasing reset key combination
        processEvents();

        if (leaveEmulation)
            break;

        if (!resetFromKeyboard) {
            initCiaClock();
            setEventInactive<Agnus::EVENT_LEAVE_EMULATION>();
            break;
        }
    }
}

auto Agnus::addWaitstatesToCPU() -> void {

    while (busUsage != BUS_FREE) {
        dmaCycle();
        countWaitCycles++;
    }

    countWaitCycles = 0;
    busUsage = BUS_USAGE_CPU;
}

inline auto Agnus::dmaCycle() -> void {
    busUsage = BUS_FREE;

    switch(++hPos) {
        case 1:
            if (ERSY) {
                hPos = 0; // need external sync to proceed

                if (!getActiveEvent<EVENT_LEAVE_EMULATION>())
                    updateEvent<EVENT_LEAVE_EMULATION>(~0, 150000);

            } else {
                if (bplState || bplQueue)
                    actions |= ACT_BPL;
            }
            break;

        case 2:
            if (vBlankStart)
                vBlankStart = false;

            if (initVCounter) {
                diwFlipFlop = false;
                initVCounter = false;
                vPos = 0;
                if (model == A1000) {
                    vBlank = true;
                    vBlankStart = true;
                }

            } else {
                vPos++;
                vPos &= ecsAndHigher() ? 0x7ff : 0x1ff; // register change of VPos could lead to a wrap around of 9-bit (OCS Agnus) counter.
                if (vBlankEnd) {
                    vBlankEnd = false;
                    vBlankEndNext = true;
                } else {
                    vBlankEnd = vPos == (ntsc ? 19 : 24);
                    if (vBlankEnd)
                        vBlank = false;
                    vBlankEndNext = false;
                }

                if ( (vPos == (lines + lof)) && (model != A1000) ) {
                    vBlank = true;
                    vBlankStart = true;
                }
            }

            updateVdiw();
            actions |= ACT_COPPER;
            break;

        case 3: // de-adjust HRM DMA view by 4 (-1 + 4) cycles to Beam position
            if (vPos == 0) {
                copper.strobeCOPJMP(false, Trigger_Vsync);
            }

            if (isEquLine()) denise.strequ();
            else if (vBlank) denise.strvbl();
            else denise.strhor();

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

            busUsage = BUS_USAGE_REFRESH;
            break;

        case 0xd:
            bplQueue = 0;
            break; // hsync start

        case 0x16: if (!vBlank) spriteControl<0, true>(); break;
        case 0x18:
            hardStop = false;
            if (!vBlank) spriteControl<0, false>();
            break;
        case 0x1a: if (!vBlank) spriteControl<1, true>(); break;
        case 0x1c: if (!vBlank) spriteControl<1, false>(); break;
        case 0x1e: if (!vBlank) spriteControl<2, true>(); break;
        case 0x20: if (!vBlank) spriteControl<2, false>(); break;
        case 0x22: if (!vBlank) spriteControl<3, true>(); break;
        case 0x24: // hsync end
            cia2.tod();
            if (!vBlank) spriteControl<3, false>();
            break;
        case 0x26: if (!vBlank) spriteControl<4, true>(); break;
        case 0x28: if (!vBlank) spriteControl<4, false>(); break;
        case 0x2a: if (!vBlank) spriteControl<5, true>(); break;
        case 0x2c: if (!vBlank) spriteControl<5, false>(); break;
        case 0x2e: if (!vBlank) spriteControl<6, true>(); break;
        case 0x30: if (!vBlank) spriteControl<6, false>(); break;
        case 0x32: if (!vBlank) spriteControl<7, true>(); break;
        case 0x34: if (!vBlank) spriteControl<7, false>(); break;
        case 0x35:
            break;

        case 0x38: actions &= ~ACT_SPRITE; break;

        case 0x13:
            if (!lof && vPos == (ntsc ? 6 : 5) ) {
                if (model != A1000) cia1.tod();
            }

        case 0xd7:
            if (ecsAndHigher())
                hardStop = true;

            if (bplState && (bplState != 4)) {
                if (!ecsAndHigher() || !harddis)
                    stopFetching = true;
            }
            break;

        case 0x85:
            if (lof && vPos == (ntsc ? 6 : 5) ) {
                if (model != A1000) cia1.tod();
            }

        case 0xe2:
            if (!lol) {
                if (vPos == (lines + lof) ) {
                    if (lofToggle) lof ^= 1;
                    initVCounter = true;
                }
            }
            break;
        case 0xe3:
            if (!lol) { // short line
                hPos = 0;
                if (lolToggle) lol ^= 1;
                shortLineBefore = true;
                actions &= ~ACT_COPPER; // "even" cycle 0 after a short line is not usable by Copper, otherwise Copper would progress 2 cycles in a row.

                if (actions & ACT_BPL) {
                    fetchPlanes<true>();
                    actions &= ~ACT_BPL;
                }

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
        // register change of HPos could lead to a wrap around of counter.
    }

    processEvents();
    bplStartStop();
    denise.process();

    if (actions) {
        uint8_t _actions = actions;

        if (_actions & ACT_BPL)
            fetchPlanes();

        if (_actions & ACT_SPRITE)
            fetchSprites();

        if (_actions & ACT_COPPER) {
            if ((hPos & 1) == 0)
                copper.process();
        }

        // there is no problem of execution order if Copper writes to Blitter register, because Blitter can not proceed in that cycle.
        // exception cycle 5: (Final D calculation) before Final D write. this Blitter cycle don't need to be free.

        if (_actions & ACT_BLITTER)
            blitter.process();
    }

    if (eClockCycle == clock) {
        // CIA accesses must be tuned to E-Clock. One CIA BUS cycle corresponds to 2 + [6,8,10,12,14] + 2 CPU cycles.
        // The programming is coordinated in such a way that the CIA is first driven forward and then the register access takes place.
        // Tests, that evaluate CIA timers are difficult because while waiting for access, the CIA internally progresses 1 or 2 cycles.
        // To make matters worse, the E-Clock phase can change with each cold start.
        eClockCycle = clock + 5;
        cia1.clock();
        cia2.clock();
    }
}

auto Agnus::POSR(bool vhpos) -> uint16_t {
    auto h = hPos + 1;
    auto v = vPos;
    auto _lol = lol;
    auto _lof = lof;

    // reading V(H)POS get the already incremented (pointing to next cycle) position, Copper uses the current position for comparisons.
    // for performance reasons following is calculated with beginning of next cycle, so we do it here temporarly
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
        case Unmapped:
            break;
    }
    return dataBus;
}

auto Agnus::writeByte(uint32_t adr, uint8_t value) -> void {
    switch( mapper[adr >> 16] ) {
        case CHIP_MEM:
            addWaitstatesToCPU();
            *(chipMem + (adr & chipMemMask)) = value;
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
            *(slowMem + (adr - 0xc00000)) = value;
            break;
        case KICK_ROM:
            if (model == A1000 && !womLock) {
                womLock = true;
                for (unsigned i = 0xf8; i <= 0xfb; i++) mapper[i] = WOM;
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
            *(uint16_t*)(chipMem + (adr & chipMemMask)) = _swapWord(value);
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
            *(uint16_t*)(slowMem + (adr - 0xc00000)) = _swapWord(value);
            break;
        case KICK_ROM:
            if (model == A1000 && !womLock) {
                womLock = true;
                for (unsigned i = 0xf8; i <= 0xfb; i++) mapper[i] = WOM;
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

auto Agnus::canCopperUseBus() -> bool {
    if (busUsage != BUS_FREE)
        return false; // a higher DMA

    if (!useCopperDMA())
        return false;

    return true;
};

auto Agnus::allocateCopper() -> bool {
    if (canCopperUseBus()) {
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
    if ((getActiveEvent<EVENT_ONE_CYCLE_DELAY>() & ~1) == ptrEvent)
        setEventInactive<EVENT_ONE_CYCLE_DELAY>();

    return true;
}

auto Agnus::writeBlitterDma(uint32_t adr, uint16_t value) -> bool {
    if(!canBlitterUseBus())
        return false;

    busUsage = BUS_USAGE_BLITTER;

    *(uint16_t*)(chipMem + (adr & chipMemMask)) = _swapWord(value);

    dataBus = value;

    if ((getActiveEvent<EVENT_ONE_CYCLE_DELAY>() & ~1) == PTR_BLT_D_H)
        setEventInactive<EVENT_ONE_CYCLE_DELAY>();

    return true;
}

template<uint8_t ptrEvent> auto Agnus::fetchBlitterDmaNoBUSCheck(uint32_t adr, uint16_t& result) -> void {
    busUsage = BUS_USAGE_BLITTER;

    result = _swapWord(*(uint16_t*)(chipMem + (adr & chipMemMask)));

    dataBus = result;

    if ((getActiveEvent<EVENT_ONE_CYCLE_DELAY>() & ~1) == ptrEvent)
        setEventInactive<EVENT_ONE_CYCLE_DELAY>();
}

auto Agnus::writeBlitterDmaNoBUSCheck(uint32_t adr, uint16_t value) -> void {
    busUsage = BUS_USAGE_BLITTER;

    *(uint16_t*)(chipMem + (adr & chipMemMask)) = _swapWord(value);

    dataBus = value;

    if ((getActiveEvent<EVENT_ONE_CYCLE_DELAY>() & ~1) == PTR_BLT_D_H)
        setEventInactive<EVENT_ONE_CYCLE_DELAY>();
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

auto Agnus::updateHarddis() -> void {

    harddis = (beamCon & 0x80) || (beamCon & 0x4000) || (bplCon0 & 0xc0);
}

auto Agnus::isEquLine() -> bool {
    if (ntsc)
        return vPos <= 10;

    return vPos <= (lof ? 9 : 8);
}

auto Agnus::setDiwStrt(uint16_t value) -> void {
    vStart = value & 0xff;
    updateVdiw();
}

auto Agnus::setDiwStop(uint16_t value) -> void {
    vStop = value & 0xff;
    if ((value & 0x80) == 0)
        vStop |= 0x100;
    updateVdiw();
}

auto Agnus::updateVdiw() -> void {
    if ((vPos == vStart) && !vBlankStart) {
        if (!diwFlipFlop) {
            diwFlipFlop = true;
        }
    }

    if ((vPos == vStop) || vBlankStart) {
        if (diwFlipFlop) {
            diwFlipFlop = false;
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
