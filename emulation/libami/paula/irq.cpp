
// todo: To determine the correct interrupt delay within Paula, it must be taken into account that the CPU only tests for interrupts in certain cycles.
// The CPU emulation synchronizes in multiples of DMA cycles (= 2 cycles). The limit of when an interrupt is detected or not happens after an odd number of CPU cycles.
// If the interrupt happens in the 2nd half of the DMA cycle, it is not recognized, although the CPU evaluates IPL in this DMA cycle.
// Synchronization after each CPU cycle slows down emulation. It is more performant to change the IPL pins one DMA cycle later in such situations.

namespace LIBAMI {

auto Paula::setIntena(uint16_t value) -> void {
    if (value & 0x8000)
        intena |= value & 0x7fff;
    else
        intena &= ~value;

    prepareIpl();
}

auto Paula::setIntreq(uint16_t value) -> void {
    if (value & 0x8000)
        intreq |= value & 0x7fff;
    else
        intreq &= ~value;

    // Note that as long as the interrupt signal enters Paula, the CIA lines cannot be reset when writing "intreq".
    if (int2Current) intreq |= 8;
    if (int6Current) intreq |= 0x2000;

    prepareIpl();
}

auto Paula::setInt2(bool state) -> void { // CIA 1
    // CIAs don't generate a pulse like blitter. Paula checks here on a flank.

    if (state && !int2Current) {
        intreq |= 8;
        prepareIpl();
    }

    int2Current = state;
}

auto Paula::setInt6(bool state) -> void { // CIA 2
    if (state && !int6Current) {
        intreq |= 0x2000;
        prepareIpl();
    }

    int6Current = state;
}

auto Paula::setVblInt() -> void {
    intreq |= 0x20;
    prepareIpl();
}

auto Paula::setDskSyncInt() -> void {
    intreq |= 0x1000;
    prepareIpl();
}

auto Paula::setDskBlkInt() -> void {
    intreq |= 2;
    prepareIpl();
}

#define UPD_INTREQ_EVENT \
    if (nextClock < agnus.eventClock[Agnus::EVENT_INTREQ]) \
        agnus.updateEventAbs<Agnus::EVENT_INTREQ>(nextClock);

template<uint8_t nr, bool dma> auto Paula::scheduleIntreqAud() -> void {
    int64_t nextClock = agnus.clock + (dma ? 4 : 3);

    if constexpr (nr == 0)
        intreqAud0Clock = nextClock;
    else if constexpr (nr == 1)
        intreqAud1Clock = nextClock;
    else if constexpr (nr == 2)
        intreqAud2Clock = nextClock;
    else if constexpr (nr == 3)
        intreqAud3Clock = nextClock;

    UPD_INTREQ_EVENT
}

auto Paula::scheduleIntreqBlt() -> void {
    int64_t nextClock = agnus.clock + 1;
    intreqBltClock = nextClock;
    UPD_INTREQ_EVENT
}

auto Paula::scheduleIntreqTbe() -> void {
    int64_t nextClock = agnus.clock + 1;
    intreqTbeClock = nextClock;
    UPD_INTREQ_EVENT
}

#define PROCESS_INTREQ(var, bit) \
    if (clock == var) { \
        intreq |= 1 << bit; \
        var = INT64_MAX; \
    } else { \
        if (var < nextClock) \
            nextClock = var; \
    }

auto Paula::intreqEvent() -> void {
    int64_t clock = agnus.clock;
    int64_t nextClock = INT64_MAX;

    PROCESS_INTREQ(intreqTbeClock, 0)
    PROCESS_INTREQ(intreqBltClock, 6)
    PROCESS_INTREQ(intreqAud0Clock, 7)
    PROCESS_INTREQ(intreqAud1Clock, 8)
    PROCESS_INTREQ(intreqAud2Clock, 9)
    PROCESS_INTREQ(intreqAud3Clock, 10)

    prepareIpl();
    agnus.updateEventAbs<Agnus::EVENT_INTREQ>(nextClock);
}

auto Paula::prepareIpl() -> void {
    int level = 0;
    uint16_t intMask = intreq & intena;

    if (intMask && (intena & 0x4000)) {
        if (intMask & (0x4000 | 0x2000))
            level = 6;
        else if (intMask & (0x1000 | 0x800))
            level = 5;
        else if (intMask & (0x400 | 0x200 | 0x100 | 0x80))
            level = 4;
        else if (intMask & (0x40 | 0x20 | 0x10))
            level = 3;
        else if (intMask & 8)
            level = 2;
        else if (intMask & (1 | 2 | 4))
            level = 1;
    }

    int lastIPL = ipl & 0x7;

    if (level != lastIPL) {
        ipl = (ipl & ~0xff) | level;

        if (((lastIPL & 1) && !(level & 1)) ||
            ((lastIPL & 2) && !(level & 2)) ||
            ((lastIPL & 4) && !(level & 4)) );
            // from WinUAE changelog it seems to matter if a bit is set or cleared.
            // CPU detect IRQs in the first half of DMA (CCK) Sample cycle (confimed with fx68k)
            // This means that a delay of only one CPU cycle is enough to miss this sample cycle and you have to wait for the sample point in the next opcode.
        else
            ipl = (ipl & ~0xff00) | (level << 8);

        iplCounter = 3;
    }
}

}
