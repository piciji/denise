
#define ERSY (bplCon0 & 2)

#define HARDDIS 0x4000
#define VARVBEN 0x1000
#define LOLDIS 0x800
#define VARVSYEN 0x200
#define VARHSYEN 0x100
#define VARBEAMEN 0x80
#define BEAM_PAL 0x20
#define VARCSYEN 0x10
#define BLANKEN 8
#define CSYTRUE 4

#define FREQUENCY_PAL   28375160
#define FREQUENCY_NTSC  28636360

#define LINE_MAX_WIDTH 384
#define LINE_RENDER_OFFSET 5 // needed to avoid costly sanity checking for CRT emulation later on
#define LINE_BUFFER_WIDTH (2048)
#define LINE_BUFFER_HEIGHT 600
#define LINE_CROP_TEST 150
//#define LOG_DMA_USAGE

#include "agnus.h"
#include "../../tools/sanitizer.h"
#include "../system/system.h"
#include "../interface.h"
#include "events.cpp"
#include "memory.cpp"
#include "dma.cpp"
#include "blanking.cpp"
#include "graphics.cpp"
#include "register.cpp"
#include "log.cpp"
#include "serialization.cpp"
#include <cmath>

namespace LIBAMI {

Agnus::Agnus(System* system, Cpu& cpu, Denise& denise, Paula& paula, Cia<MOS_8520>& cia1, Cia<MOS_8520>& cia2, Input& input, RTC& rtc)
:   system(system), cpu(cpu), denise(denise), paula(paula), cia1(cia1), cia2(cia2),
    input(input), rtc(rtc), blitter(*this), copper(*this), fastMemExpansion(*this) {

    this->interface = system->interface;

    expansions[0] = &fastMemExpansion;
    expansions[1] = new HDController(*this, system->hardDrives[0]);
    expansions[2] = new HDController(*this, system->hardDrives[1]);
    expansions[3] = new HDController(*this, system->hardDrives[2]);
    expansions[4] = new HDController(*this, system->hardDrives[3]);

    chipMemChangeSize = slowMemChangeSize = fastMemChangeSize = 10 * 1024;
    chipMemChange = new MemChange[chipMemChangeSize];
    slowMemChange = new MemChange[slowMemChangeSize];
    fastMemChange = new MemChange[fastMemChangeSize];

    wom = new uint8_t[256 * 1024];
    ntsc = false;

    frameBuffer = new uint16_t[LINE_BUFFER_WIDTH * LINE_BUFFER_HEIGHT + LINE_RENDER_OFFSET];
    lineCallback.use = false;
    lineCallback.called = true;
    lineCallback.line = 0;

    encryptedRom = nullptr;
    overclock.cycles = 0;
    overclock.speed = 0;

    resetFps();
}

Agnus::~Agnus() {
    delete[] chipMemChange;
    delete[] slowMemChange;
    delete[] fastMemChange;
    delete[] wom;

    if (chipMemMask)
        delete[] chipMem;

    if (fastMemSize)
        delete[] fastMem;

    if(encryptedRom)
        delete[] encryptedRom;

    for(auto expansion : expansions) {
        if (!dynamic_cast<FastMemExpansion*>(expansion))
            delete expansion;
    }

    delete[] frameBuffer;
}

auto Agnus::frequency() const -> unsigned {
    return (ntsc ? FREQUENCY_NTSC : FREQUENCY_PAL) >> 3;
}

auto Agnus::dmaControl(uint16_t data) -> void {
    if (data & 0x8000)
        dmaConImm |= data & 0x7ff;
    else
        dmaConImm &= ~data; // no masking needed, unused upper 5 bits will never be set

    updateDdfEnableCache();
}

auto Agnus::setRas() -> void {
    if (aga()) {
        rasAdder = 0;
    } else if (ecs()) {
        rasAdder = 0x200;
    } else {
        rasAdder = 0x2;
    }
}

auto Agnus::power(bool softReset, bool resetInstruction) -> void {
    overclock.cycles = 0;
    unsigned resetDelay = hasActiveEvent<EVENT_KBD>() ? getEventDelay<EVENT_KBD>() : 0;
    clearEvents();
    dmaClock = 0;
    checkForRomEncryption();

    if (!chipMem)
        setChipmem(512 * 1024);

    if (!softReset) {
        std::memset(chipMem, 0x0, chipMemMask + 1);
        if (slowMemSize)
            std::memset(slowMem, 0, slowMemSize);
        if (fastMemSize)
            std::memset(fastMem, 0, fastMemSize);
        if (model == OCS_A1000)
            std::memset(wom, 0, 256 * 1024);

        rDmaPtr = 0;
    }

    setRas();

    for(auto expansion : expansions)
        expansion->reset(softReset);

    for (auto& expansionConf : expansionsConfigured)
        expansionConf = nullptr;

    hardDrivesBusy = false;
    actions = 0;
    busUsage = BUS_FREE;
    hPos = 4;
    vPos = 0;
    hPosLocked = false;
    vPosLocked = 0;
    vStart = 0;
    vStop = 0;
    dmal = 0;
    frameClock = 0;
    fpsChange = 0;
    secureRA = false;

    hBlank = false;
    vBlankEnd = false;
    vBlankEndNext = false;
    vBlank = true;
    vBlankStart = false;
    sprInhibited = false;
    blitterConflict = false;

    vTotal = 0x7ff;
    vBStrt = 0;
    vBStop = 0;
    hTotal = 0xff;
    hTotalChanged = false;

    hsStrt = 0;
    hsStop = 0;
    hBStrt = 0;
    hBStop = 0;

    ntsc = system->ntsc;
    beamCon = (ntsc || !ecsAndHigher()) ? 0 : BEAM_PAL;
    lines = ntsc ? 261 : 311;

    rJob1.reset();
    rJob2.reset();
    rJob3.reset();

    for(auto& spr : sprites) {
        spr.ptr = 0;
        spr.pos = 0;
        spr.ctl = 0;
        spr.vStart = 0;
        spr.vStop = 0;
        spr.fetchData = false;
        spr.enable = false;
    }

    for(auto& cha : audioDmaChannels) {
        cha.ptr = 0;
        cha.ptrLatch = 0xffff;
    }

    dskpt = 0;
    chipMemChangePos = 0;
    slowMemChangePos = 0;
    fastMemChangePos = 0;
    trackMemChanges = false;

    ddfStart = 0;
    ddfStop = 0;

    bpl1pt = bpl2pt = bpl3pt = bpl4pt = bpl5pt = bpl6pt = 0;
    bpl1Mod = bpl2Mod = 0;

    dataBus = 0xffff;
    dmaCon = 0;
    dmaConImm = 0;
    dmaConCop = false;
    dmaConBlt = false;
    dmaConSpr = false;
    bplCon0 = 0;
    countWaitCycles = 1;

    lol = false;
    lof = true;
    lolToggle = ntsc;
    laceMode = 0;
    laceFrame = 0;

    initVCounter = false;
    shortLineBefore = true;
    stopFetching = false;
    bplCycle = 0;
    bplQueue = 0;
    sprQueue = 0;
    ddfStartMatch = 0;
    harddisH = false;
    harddisV = false;
    ddfEnableBefore = false;
    bplState = 0;
    hardStop = false;
    diwFlipFlop = false;

    if (!softReset || resetInstruction)
        womLock = false;

    if (!softReset) {
        resetFromKeyboard = 0;
    } else {
        if (resetFromKeyboard && resetDelay)
            input.keyboard.addEvent(Keyboard::KBD_Hardreset, resetDelay);
    }

    blitter.reset();
    copper.reset();
    mapMemory();
    mapRom();

    if (model == OCS_A1000) {
        powerSupply.init(frequency(), ntsc ? 60 : 50);
        updateEvent<EVENT_POWER_SUPPLY>(powerSupply.nextTickCount());
    } else
        setEventInactive<EVENT_POWER_SUPPLY>();

    initCiaClock();

    if (model == OCS_A1000)
        denise.model = ntsc ? Denise::Model::OCS_A1000_NO_EHB : Denise::Model::OCS_A1000;

    hTotalFirst = true;
    updateEvent<EVENT_HTOTAL>(0xe2 - hPos);
    denise.linePtr = frameBuffer;
    lineVCounter = 0;
    vBlankOffset = 0;
    crop.reset();
    updateDdfEnableCache();
}

auto Agnus::powerOff() -> void {
    ntsc = system->ntsc;
    resetFps();
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

auto Agnus::waitKeyboardReset() -> void {
    if ((resetFromKeyboard & 0x80) == 0) {
        system->power(true);
        resetFromKeyboard |= 0x80;
        interface->updateLedState(Emulator::Interface::LedId::CapsLock, ecsAndHigher() ? 3 : 1);
    }

    while(true) { // CPU and most chips on hold, Denise hasn't a reset line
        dmaCycle();

        if (input.externalKeyEvent())
            input.emergencyPoll();

        if (!resetFromKeyboard) {
            system->power(true);
            break;
        }

        if (system->leaveEmulation) {
            break;
        }
    }
}

auto Agnus::addWaitstatesToCPU() -> void {
    if (overclock.speed) {
        if (overclock.cycles)
            dmaCycle(); // set address on BUS (clock stretch to stock speed)

        // access BUS (clock stretch to stock speed)
        overclock.cycles = overclock.speed - 2;
    }

    while (busUsage != BUS_FREE) {
        dmaCycle();
        countWaitCycles++;

#ifdef LOG_DMA_USAGE
        logDmaUsage();
#endif
    }

    // in non nasty mode, CPU gets BUS after 3 wait cycles.
    // note: first wait cycle already happened when entering this function, because CPU executes "sync" one DMA cycle ahead
    // to find out if BUS is free for a possible read/write
    countWaitCycles = 1;
    busUsage = BUS_USAGE_CPU;

#ifdef LOG_DMA_USAGE
    logDmaUsage(true);
#endif
}

auto Agnus::updateVCounter() -> void {
    if (vBlankStart)
        vBlankStart = false;

    if (vBlankEnd) {
        vBlankEnd = false;
        vBlankEndNext = true;
    } else if (vBlankEndNext)
        vBlankEndNext = false;

    if (initVCounter) {
        if (system->isProcessFrame())
            observeFrameDuration();

        if (!harddisV) {
            diwFlipFlop = false;
            updateDdfEnableCache();
        }
        initVCounter = false;
        vPos = 0;
        if (model == OCS_A1000) {
            vBlank = true;
            vBlankStart = true;
        }
        copper.strobeCOPJMP(1, Trigger_Vsync);
    } else {
        vPos++;
        vPos &= ecsAndHigher() ? 0x7ff : 0x1ff; // register change of VPos could lead to a wrap around of 9-bit (OCS Agnus) counter.
    }

    if (beamCon & VARVBEN) {
        if (vPos == vBStrt) {
            vBlank = true;
            vBlankStart = true;
            if (vPos < (ntsc ? 19 : 24))
                vBlankOffset = (ntsc ? 19 : 24) - vPos;
        }
        if (vPos == vBStop) {
            vBlank = false;
            vBlankEnd = true;
        }
    } else {
        if (vPos == (ntsc ? 19 : 24)) {
            vBlank = false;
            vBlankEnd = true;
        }
        if ( (vPos == (lines + lof)) && (model != OCS_A1000) ) {
            vBlank = true;
            vBlankStart = true;
        }
    }
}

template<uint8_t slot> inline auto Agnus::refreshCycle() -> void {
    busUsage = BUS_USAGE_REFRESH;
    constexpr bool firstStrobe = slot == 0;
    bool secondStrobe = slot == 1 && lol;

    // For AGA chipset, DRAM use internal refresh counter.
    // AGA chipset only triggers Refresh but can't control the refresh position.
    // OCS: if (someAdr & 0x201fe == rDmaPtr & 0x201fe )  // not interested in CAS, so we mask it out
    //          1024 bytes to refresh for each rDmaPtr (same ptr is used for slow mem)

    // ECS: RAS and CAS were replaced. To increase RAS, the "adder" must be moved 8 bits

    // We don't need this pointer in an emulator to prevent memory locations from losing their charge.
    // However, Bitplane Fetch and Refresh Slot can overlap.
    // In this case, the Refresh DMA pointer affects the current Bitplane DMA address.
    if (bplQueue & 0xff) {
        if (firstStrobe || secondStrobe)
            bplQueue |= 0x20;

        bplQueue |= 0x40;
        return;
    }

    rDmaPtr += rasAdder;
    rDmaPtr &= dmaChipMemMask;

    if (firstStrobe) {
        addOneCycleEvent(STROBE);
    } else if (secondStrobe) {
        addOneCycleEvent(STROBE2);
    }
}

inline auto Agnus::dmaCycle() -> void {
    busUsage = BUS_FREE;

    // de-adjust HRM DMA view by 4 cycles to Beam position.
    switch(++hPos) {
        case 1:
            if (ERSY)
                hPosLocked = true;

            if (shortLineBefore)
                copper.cycle1();
            break;

        case 2:
            updateVCounter();
            updateVdiw();
            actions |= ACT_COPPER; // if Copper waits for next line
            break;

        case 3: refreshCycle<0>(); break;
        case 5: refreshCycle<1>(); break;
        case 7: refreshCycle<2>(); break;
        case 9: refreshCycle<3>(); break;

        case 0xb:
            if (dmal & 3)
                diskDma(0, dmal & 2);
            dmal >>= 2;
            break;
        case 0xc:
            startHblank();
            break;
        case 0xd:
            if (dmal & 3)
                diskDma(1, dmal & 2);
            bplQueue = 0;
            dmal >>= 2;
            if (hardDrivesBusy) // at least one
                checkHardDrives();
            break;
        case 0xf:
            if (dmal & 3)
                diskDma(2, dmal & 2);
            dmal >>= 2;
            break;

        case 0x11:
            if (dmal & 0x3)
                fetchSample<0>( dmal & 0x1 );
            dmal >>= 2;
            break;

        case 0x12:
            if (!vBlank) startHsync();
            break;

        case 0x13:
            if (dmal & 0x3)
                fetchSample<1>( dmal & 0x1 );
            dmal >>= 2;

            if (!lof && (ERSY == 0) && (vPos == (ntsc ? 6 : 5) ) ) {
                if (model != OCS_A1000) cia1.tod(eClockCycle & 3); // hardwired vsync end
            }
            break;
        case 0x15:
            if (dmal & 0x3)
                fetchSample<2>( dmal & 0x1 );
            dmal >>= 2;
            break;
        case 0x17:
            if (dmal & 0x3)
                fetchSample<3>( dmal & 0x1 );
            dmal >>= 2;

            if (!vBlank) spriteControl<0, true>();
            break;
        case 0x18:
            hardStop = false;
            updateDdfEnableCache();
            break;
        case 0x19:
            if (!vBlank) spriteControl<0, false>();
            break;
        case 0x1b: if (!vBlank) spriteControl<1, true>(); break;
        case 0x1d: if (!vBlank) spriteControl<1, false>(); break;
        case 0x1f: if (!vBlank) spriteControl<2, true>(); break;
        case 0x21: if (!vBlank) spriteControl<2, false>(); break;
        case 0x23: if (!vBlank) spriteControl<3, true>(); break;
        case 0x24: // hsync end
            if (ERSY == 0)
                cia2.tod(eClockCycle & 3);
            if (denise.extblanken && ecs() && ((beamCon & BLANKEN) == 0))
                denise.csync(csyncPolTrue(vBlank));
            break;
        case 0x25: if (!vBlank) spriteControl<3, false>(); break;
        case 0x27: if (!vBlank) spriteControl<4, true>(); break;
        case 0x29: if (!vBlank) spriteControl<4, false>(); break;
        case 0x2b: if (!vBlank) spriteControl<5, true>(); break;
        case 0x2d: if (!vBlank) spriteControl<5, false>(); break;
        case 0x2f:
            endHblank();
            if (!vBlank) spriteControl<6, true>(); break;
        case 0x31: if (!vBlank) spriteControl<6, false>(); break;
        case 0x33: if (!vBlank) spriteControl<7, true>(); break;
        case 0x35: if (!vBlank) spriteControl<7, false>(); break;

        case 0x38: actions &= ~ACT_SPRITE; break;

        case 0xd7:
            if (ecsAndHigher()) {
                hardStop = true;
                updateDdfEnableCache();
            }

            if (bplState && (bplState != 4)) {
                if (!ecsAndHigher() || !harddisH)
                    stopFetching = true;
            }
            break;

        case 0x85:
            if (lof && (ERSY == 0) && (vPos == (ntsc ? 6 : 5) ) ) {
                if (model != OCS_A1000) cia1.tod(eClockCycle & 3);
            }
            break;

        // register change of HPos could lead to a wrap around of counter.
    }

    if (++clock == nextClock)
        processEvents(clock);

    if (ecsAndHigher()) {
        if (hPos == ddfStart)
            bplControl<true, true>();
        else if (ddfStartMatch || bplState)
            bplControl<true, false>();
        else if (!(hPos & 1)) {
            ECS_BPL_START_CHECK
        }
    } else {
        if (bplState)
            bplControl<false, false>();
        else if (hPos == ddfStart)
            bplControl<false, true>();
    }

    if (actions) {
        int _actions = actions;

        if (_actions & ACT_IPLCOUNTER)
            paula.iplUpdate();

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

auto Agnus::blanken() -> void {
    int delayStrt, delayStop;

    if (beamCon & VARCSYEN) {
        if (hPos == hsStop) denise.csync(csyncPolTrue(false));
        else if (hPos == hsStrt) denise.csync(csyncPolTrue(true));

        delayStop = hsStop - hPos;
        if (delayStop <= 0)
            delayStop = ((beamCon & VARBEAMEN) ? (hTotal + lol) : (0xe2 + lol)) - hPos + hsStop;

        delayStrt = hsStrt - hPos;
        if (delayStrt <= 0)
            delayStrt = ((beamCon & VARBEAMEN) ? (hTotal + lol) : (0xe2 + lol)) - hPos + hsStrt;

    } else {
        if (hPos == hBStop) denise.csync(csyncPolTrue(false));
        else if (hPos == hBStrt) denise.csync(csyncPolTrue(true));

        delayStop = hBStop - hPos;
        if (delayStop <= 0)
            delayStop = ((beamCon & VARBEAMEN) ? (hTotal + lol) : (0xe2 + lol)) - hPos + hBStop;

        delayStrt = hBStrt - hPos;
        if (delayStrt <= 0)
            delayStrt = ((beamCon & VARBEAMEN) ? (hTotal + lol) : (0xe2 + lol)) - hPos + hBStrt;
    }

    updateEvent<EVENT_BLANKEN>(delayStop <= delayStrt ? delayStop : delayStrt);
}

auto Agnus::POSR(bool vhpos) -> uint16_t {
    uint8_t h = hPos + 1;
    auto v = vPos;
    auto _lol = lol;
    auto _lof = lof;
    uint8_t hCompare = (beamCon & VARBEAMEN) ? ((hTotal - 1) & 0xff + _lol) : (226 + _lol);

    // reading V(H)POS get the already incremented (pointing to next cycle) position, Copper uses the current position for comparisons.
    // for performance reasons following is calculated with beginning of next cycle, so we do it here temporarily
    if (h == 1) {
        if (ERSY) h = 0;
    } else if (h == 2) {
        if (initVCounter) v = 0;
        else { v++; v &= ecsAndHigher() ? 0x7ff : 0x1ff; }
    }

    if (h == hCompare) {
        if (lace() && (v == (lines + _lof) )) _lof ^= 1;

    } else if (h == (hCompare + 1) ) {
        h = 0;
        if (lolToggle) _lol ^= 1;
    }

    if (ERSY) {
        v = vPosLocked;
        if (hPosLocked) h = 0;
    }

    // VHPOSR
    if (vhpos)
        return ((v & 0xff) << 8) | h;

    // VPOSR
    if (ecsAndHigher())
        return ((v >> 8) & 7) | (_lof << 15) | (_lol << 7) | (ntsc << 12) | (1 << 13) | (aga() ? 3 : 0);

    return ((v >> 8) & 1) | (_lof << 15) | (ntsc << 12);
}

auto Agnus::sync(unsigned cycles) -> void {
    if (overclock.speed) {
        overclock.cycles += cycles;

        while (overclock.cycles >= overclock.speed) {
            dmaCycle();
            overclock.cycles -= overclock.speed;
        }

    } else {
        // triggers DMA cycle following current CPU micro cycle.
        // means it is always a DMA cycle ahead of CPU to find out if next cycle is usable for CPU
        while( cycles ) {
            dmaCycle();
            cycles -= 2;
    #ifdef LOG_DMA_USAGE
            logDmaUsage();
    #endif
        }
    }
}

auto Agnus::iackCycle(uint8_t level, uint8_t& vector) -> int {
    vector = 24 + level;
    return 0;
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
    harddisH = (beamCon & VARBEAMEN) || (beamCon & HARDDIS) || (bplCon0 & 0xc0);
    harddisV = (beamCon & VARBEAMEN) || (beamCon & HARDDIS) || (beamCon & VARVBEN);
    updateDdfEnableCache();
}

auto Agnus::isEquLine() -> bool {
    if (!vPos && (model == OCS_A1000 || model == OCS))
        return false;

    if (model != OCS_A1000) {
        if (ntsc)
            return vPos <= 10;

        return vPos < (lof ? 9 : 8);
    }

    if (ntsc)
        return vPos <= 11;

    return vPos < (lof ? 10 : 9);
}

auto Agnus::setDiwStrt(uint16_t value) -> void {
    vStart = (value >> 8) & 0xff;
    updateVdiw<1>();
}

auto Agnus::setDiwStop(uint16_t value, bool triggerCopper) -> void {
    vStop = (value >> 8) & 0xff;

    if ((vStop & 0x80) == 0)
        vStop |= 0x100;

    triggerCopper ? updateVdiw<2 | 8>() : updateVdiw<2>();
}

auto Agnus::setDiwHigh(uint16_t value, bool triggerCopper) -> void {
    if (!ecsAndHigher())
        return;

    vStart &= 0xff;
    vStart |= (value & 15) << 8;
    vStop &= 0xff;
    vStop |= ((value >> 8) & 15) << 8;
    triggerCopper ? updateVdiw<4 | 8>() : updateVdiw<4>();
}

template<int mode> auto Agnus::updateVdiw() -> void {
    constexpr bool diwStart = mode & 1;
    constexpr bool diwStop = mode & 2;
    constexpr bool diwHigh = mode & 4;
    constexpr bool triggerCopper = mode & 8;

    bool hardLimit = vBlankStart && !harddisV;

    if ((vPos == vStart) && !hardLimit) {
        if (!diwFlipFlop) {
            diwFlipFlop = true;
            updateCropTop();
            updateDdfEnableCache();
        }
        if (diwStart || diwHigh)
            bplCycle &= ~BPL_ADD_MOD;
    }

    if ((vPos == vStop) || hardLimit) {
        if (diwStop || diwHigh) {
            addOneCycleEvent(UPD_V_DIW, 0,3);
            return;
        } else if (diwFlipFlop) {
            diwFlipFlop = false;
            updateCropBottom();
            updateDdfEnableCache();
        }
    }

    if (triggerCopper && (diwStop || diwHigh) && diwFlipFlop) {
        ECS_BPL_START_CHECK
    }
}

auto Agnus::vposw(uint16_t value) -> void {
    bool lolBefore = lol;
    bool lofBefore = lof;
    uint16_t vPosBefore = vPos;

    lof = value & 0x8000; // could result in a wrap around of vPos
    vPos &= 0xff;
    if (ecsAndHigher()) {
        vPos |= (value & 7) << 8;
        lol = 0;
    } else {
        vPos |= (value & 1) << 8;
    }

    if (lolBefore != lol || lofBefore != lof)
        fpsChange |= 1;
    if (vPosBefore != vPos)
        fpsChange |= 2;

    //if (fpsChange && !hasActiveEvent<EVENT_LEAVE_EMULATION>())
      //  updateEvent<EVENT_LEAVE_EMULATION>(150000);
}

auto Agnus::vhposw(uint16_t value) -> void {
    uint16_t vPosBefore = vPos;
    uint8_t hPosBefore = hPos + 1;

    hPos = value & 0xff;
    vPos &= 0x300;
    vPos |= value >> 8;

    if (vPosBefore != vPos) {
        fpsChange |= 2;
    }

    if (hPosBefore != hPos ) {
        fpsChange |= 2;

        int delay = 1;
        int limit = (beamCon & VARBEAMEN) ? hTotal : 0xe2;

        if (hPos > (limit + lol) ) // miss comparator -> wrap around
            delay += (0xff - hPos) + (limit + lol);
        else
            delay += limit - hPos + lol;

        if (!hTotalFirst) {
            hTotalFirst = true;

            if (bplState || bplQueue)
                actions |= ACT_BPL;

        } else {
            int diff = hPosBefore - hPos - 1;

            if (hPosBefore > 0x2f && (diff > 0) ) {
                denise.process(-1);
                denise.linePos -= diff << 1;
            }
        }

        updateEvent<EVENT_HTOTAL>(delay);

//        interface->log("vhpos");
//        interface->log(vPos,0);
//        interface->log(hPosBefore,0,1);
//        interface->log(hPos,0,1);

        // For ease of use, the emulator increases the position at the beginning of the cycle.
        // However, when writing the position manually, this is a problem.
        hPos = hPos ? (hPos - 1) : 0xff;
    }

   // if (fpsChange && !hasActiveEvent<EVENT_LEAVE_EMULATION>())
     //   updateEvent<EVENT_LEAVE_EMULATION>(150000);
}

auto Agnus::observeFrameDuration() -> void {
    // caution: extreme deviations lead to faulty synchronization.

    if (!fpsChange)
        return;

    double fpsOld = fps;

    if (fpsChange & 2) { // Beam position altered
        double elapsedCycles = (double)fallBackCycles( frameClock );
        fps = (double)frequency() / elapsedCycles;

        if (fps < 1.0) fps = 1.0;

        fpsChange = 1; // reset to "typical" in next frame, if the beam position has not been changed again.

        system->updateStats();
        if (std::fabs(fpsOld - fps) > 0.03) {
            //interface->log("fps change 2");
            //interface->log(fps, 0);
            interface->fpsChanged();
        }

    } else if (fpsChange & 1) { // typical, e.g. lace change
        double linesPerField;
        double cyclesPerLine;

        if (laceMode & 3)
            linesPerField = lines + 1.5;
        else
            linesPerField = lines + (lof ? 2.0 : 1.0);

        if (lolToggle)
            cyclesPerLine = ((beamCon & VARBEAMEN) ? ((double)hTotal + 1.0) : 227.0) + 0.5;
        else {
            cyclesPerLine = (beamCon & VARBEAMEN) ? ((double)hTotal + 1.0) : 227.0;
            if (lol)
                cyclesPerLine += 1.0;
        }

        fps = (double)frequency() / (cyclesPerLine * linesPerField);
        if (fps < 1.0) fps = 1.0;
        fpsChange = 0;

        system->updateStats();
        if (std::fabs(fpsOld - fps) > 0.03) {
            //interface->log("fps change 1");
            //interface->log(fps, 0);
            interface->fpsChanged();
        }
    }

    frameClock = clock;
}

auto Agnus::resetFps() -> void {
    if (ntsc)
        fps = (double)frequency() / (227.5 * 263.0); // start with lol toggling
    else
        fps = (double)frequency() / (227.0 * 313.0);
}

inline auto Agnus::setLines() -> void {
    lines = (beamCon & VARBEAMEN) ? vTotal : (ntsc ? 261 : 311);
}

auto Agnus::setBltConflictThisCycle() -> void {
    blitterConflict = true;
    addOneCycleEvent(Agnus::END_BLT_CONFLICT, sprQueue & 0xff, 1);
}

auto Agnus::checkHardDrives() -> void {
    hardDrivesBusy = false;
    
    for(auto& hardDrive : system->hardDrives)
        hardDrivesBusy |= hardDrive.process();
}

inline auto Agnus::csyncPolTrue(bool state) -> bool {
    return (beamCon & CSYTRUE) ? !state : state;
}

template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_A_H,false,true,false,true>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;
template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_A_H,true,true,false,true>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;
template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_A_H,false,true,true,true>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;
template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_A_H,true,true,true,true>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;

template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_B_H,false,true,false,true>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;
template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_B_H,true,true,false,true>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;
template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_B_H,false,true,true,true>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;
template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_B_H,true,true,true,true>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;

template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_C_H,false,true,false,true>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;
template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_C_H,true,true,false,true>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;
template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_C_H,false,true,true,true>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;
template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_C_H,true,true,true,true>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;

template auto Agnus::writeBlitterDma<false,true,false,true>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;
template auto Agnus::writeBlitterDma<true,true,false,true>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;
template auto Agnus::writeBlitterDma<false,true,true,true>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;
template auto Agnus::writeBlitterDma<true,true,true,true>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;


template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_A_H,false,true,false,false>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;
template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_A_H,true,true,false,false>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;
template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_A_H,false,true,true,false>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;
template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_A_H,true,true,true,false>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;

template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_B_H,false,true,false,false>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;
template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_B_H,true,true,false,false>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;
template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_B_H,false,true,true,false>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;
template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_B_H,true,true,true,false>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;

template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_C_H,false,true,false,false>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;
template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_C_H,true,true,false,false>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;
template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_C_H,false,true,true,false>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;
template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_C_H,true,true,true,false>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;

template auto Agnus::writeBlitterDma<false,true,false,false>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;
template auto Agnus::writeBlitterDma<true,true,false,false>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;
template auto Agnus::writeBlitterDma<false,true,true,false>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;
template auto Agnus::writeBlitterDma<true,true,true,false>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;

// line mode
template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_B_H,false,false,false,true>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;
template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_B_H,false,false,true,true>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;

template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_C_H,false,false,false,true>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;
template auto Agnus::writeBlitterDma<false,false,false,true>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;

template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_B_H,false,false,false,false>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;
template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_B_H,false,false,true,false>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;

template auto Agnus::fetchBlitterDma<Agnus::PTR_BLT_C_H,false,false,false,false>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;
template auto Agnus::writeBlitterDma<false,false,false,false>(uint32_t& adr, uint16_t& result, const int16_t& mod) -> bool;


template auto Agnus::setEventInactive<Agnus::EVENT_KBD>() -> void;
template auto Agnus::updateEvent<Agnus::EVENT_KBD>(int delay) -> void;

template auto Agnus::setEventInactive<Agnus::EVENT_AUDIO_STATE>() -> void;
template auto Agnus::updateEvent<Agnus::EVENT_AUDIO_STATE>(int delay) -> void;

template auto Agnus::updateEvent<Agnus::EVENT_ONE_CYCLE_DELAY>( int delay ) -> void;
template auto Agnus::updateEventAbs<Agnus::EVENT_AUDIO_STATE>(int64_t absClock) -> void;

template auto Agnus::updateEvent<Agnus::EVENT_SERIAL>( int delay ) -> void;
template auto Agnus::updateEventAbs<Agnus::EVENT_SERIAL>( int64_t absClock ) -> void;

template auto Agnus::updateEventAbs<Agnus::EVENT_INTREQ>(int64_t absClock) -> void;

template auto Agnus::updateEvent<Agnus::EVENT_FLOPPY>( int delay ) -> void;

template auto Agnus::updateEvent<Agnus::EVENT_LEAVE_EMULATION>(int delay) -> void;

template auto Agnus::getEventDelay<Agnus::EVENT_FLOPPY>() -> unsigned;

template auto Agnus::allocateCopper<false>() -> bool;
template auto Agnus::allocateCopper<true>() -> bool;

template auto Agnus::canCopperUseBus<false>() -> bool;
template auto Agnus::canCopperUseBus<true>() -> bool;

template auto Agnus::doubleResMidframe<false>(bool fromHires) -> void;
template auto Agnus::doubleResMidframe<true>(bool fromHires) -> void;

}
