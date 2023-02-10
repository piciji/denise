
namespace LIBAMI {

// By means of 8 byte counters no overflow handling is necessary. Theoretically, this could run into an overflow
// with constant use of savestates. It has been thousands of years. The use of signed variables is faster,
// because the compiler does not incorporate overflow handling from itself.
// 32 bit architectures have a disadvantage here, as additional operations are necessary.

template<uint8_t Channel, bool executeCurEvent>
auto Agnus::updateEvent(int delay) -> void {
    updateEventAbs<Channel, executeCurEvent>(clock + delay);
}

template<uint8_t Channel, bool executeCurEvent>
inline auto Agnus::updateEventAbs(int64_t absClock) -> void {
    if constexpr (executeCurEvent) {
        if (hasActiveEvent<Channel>()) {
            if constexpr (Channel == EVENT_ONE_CYCLE_DELAY)
                processOneCycleEvent(oneCycleJob, oneCycleData);
        }
    }

    eventClock[Channel] = absClock;
    if (absClock < nextClock)
        nextClock = absClock;
}

auto Agnus::processEvents(int64_t curClock) -> void {

    if (curClock == eventClock[EVENT_KBD])
        input.keyboard.processEvent();

    if (curClock == eventClock[EVENT_ONE_CYCLE_DELAY])
        processOneCycleEvent(oneCycleJob, oneCycleData);

    if (curClock == eventClock[EVENT_POWER_SUPPLY])
        powerSupplyEvent();

    if (curClock == eventClock[EVENT_LEAVE_EMULATION])
        leaveEmulationEvent();

    if (curClock == eventClock[EVENT_HTOTAL])
        HTotalEvent();

    if (curClock == eventClock[EVENT_AUDIO_STATE])
        paula.audioEvent();

    int64_t next = eventClock[EVENT_KBD];
    if (eventClock[EVENT_ONE_CYCLE_DELAY] < next)
        next = eventClock[EVENT_ONE_CYCLE_DELAY];
    if (eventClock[EVENT_LEAVE_EMULATION] < next)
        next = eventClock[EVENT_LEAVE_EMULATION];
    if (eventClock[EVENT_POWER_SUPPLY] < next)
        next = eventClock[EVENT_POWER_SUPPLY];
    if (eventClock[EVENT_AUDIO_STATE] < next)
        next = eventClock[EVENT_AUDIO_STATE];
    if (eventClock[EVENT_HTOTAL] < next)
        next = eventClock[EVENT_HTOTAL];

    nextClock = next;
}

auto Agnus::clearEvents() -> void {
    std::fill_n(eventClock, EVENT_CHANNELS, INT64_MAX);

    clock = 0;
    nextClock = INT64_MAX;
}

template<uint8_t Channel>
auto Agnus::getEventDelay() -> unsigned {
    int64_t c = eventClock[Channel];
    if (c > clock)
        return c - clock;

    return 0;
}

auto Agnus::addOneCycleEvent(uint8_t job, uint16_t data, int delay) -> void {
    updateEvent<EVENT_ONE_CYCLE_DELAY, true>( delay );
    oneCycleJob = job;
    oneCycleData = data;
}

auto Agnus::forceOneCycleEvent(uint8_t job) -> void {
    if (hasActiveEvent<EVENT_ONE_CYCLE_DELAY>()) {
        if ((oneCycleJob & ~1) == job)
            processOneCycleEvent(job, oneCycleData);
    }
}

auto Agnus::inactivateOneCycleEvent(uint8_t job) -> void {
    if (hasActiveEvent<EVENT_ONE_CYCLE_DELAY>()) {
        if ((oneCycleJob & ~1) == job)
            setEventInactive<EVENT_ONE_CYCLE_DELAY>();
    }
}

auto Agnus::processOneCycleEvent(uint8_t job, uint16_t data) -> void {
    switch (job) {
        case PTR_BLT_A_H: blitter.setBltAptH(data); break;
        case PTR_BLT_A_L: blitter.setBltAptL(data); break;
        case PTR_BLT_B_H: blitter.setBltBptH(data); break;
        case PTR_BLT_B_L: blitter.setBltBptL(data); break;
        case PTR_BLT_C_H: blitter.setBltCptH(data); break;
        case PTR_BLT_C_L: blitter.setBltCptL(data); break;
        case PTR_BLT_D_H: blitter.setBltDptH(data); break;
        case PTR_BLT_D_L: blitter.setBltDptL(data); break;
        case PTR_DSK_H: setDskPtH(data); break;
        case PTR_DSK_L: setDskPtL(data); break;
        case DMACON: dmaCon = dmaConImm; break;
        case BLT_INIT: blitter.initBlit(); break;
        case BLT_BUSY_DELAY: break;
    }
    setEventInactive<EVENT_ONE_CYCLE_DELAY>();
}

auto Agnus::powerSupplyEvent() -> void {
    cia1.tod( );
    updateEvent<EVENT_POWER_SUPPLY>(powerSupply.nextTickCount());
}

auto Agnus::leaveEmulationEvent() -> void {
    // When a frame is fully calculated, control is given back to the user interface.
    // Frequent changes in position (VHPOSW) can cause this to never happen or only after a very long time. In order to keep the user interface responsive,
    // control must be returned in a timely manner.
    // todo: blank screen in such cases
    system->leaveEmulation = true;
    setEventInactive<Agnus::EVENT_LEAVE_EMULATION>();
}

auto Agnus::HTotalEvent() -> void {
    if (hTotalFirst) {
        if (vPos == (lines + lof) ) {
            if (lace()) lof ^= 1;
            initVCounter = true;
            lines = (beamCon & VARBEAMEN) ? vTotal : (ntsc ? 261 : 311);
        }
        updateEvent<EVENT_HTOTAL>(1);
        hTotalFirst = false;
    } else {
        if (!lol) {
            actions &= ~ACT_COPPER; // "even" cycle 0 after a short line is not usable by Copper, otherwise Copper would progress 2 cycles in a row.
            if (actions & ACT_BPL) {
                fetchPlanes<true>();
                actions &= ~ACT_BPL;
            }

            shortLineBefore = true;
        } else
            shortLineBefore = false;

        hPos = 0;
        if (lolToggle) lol ^= 1;
        if(ERSY == 0)
            updateEvent<EVENT_HTOTAL>((beamCon & VARBEAMEN) ? (hTotal + lol) : (0xe2 + lol) );
        else
            setEventInactive<EVENT_HTOTAL>();
        hTotalFirst = true;
    }
}

}
