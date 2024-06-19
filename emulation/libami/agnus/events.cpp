
namespace LIBAMI {
// inspired by vAmiga, a 64 bit event counter is used.
// By means of 8 byte counters no overflow handling is necessary. Theoretically, this could run into an overflow
// with constant use of save states. It has been thousands of years. The use of signed variables is faster,
// because the compiler does not incorporate overflow handling from itself.
// 32bit architectures have a disadvantage here, as additional operations are necessary.

template<uint8_t Channel>
auto Agnus::updateEvent(int delay) -> void {
    updateEventAbs<Channel>(clock + delay);
}

template<uint8_t Channel>
inline auto Agnus::updateEventAbs(int64_t absClock) -> void {
    eventClock[Channel] = absClock;
    if (absClock < nextClock)
        nextClock = absClock;
}

auto Agnus::processEvents(int64_t curClock) -> void {

    if (curClock == eventClock[EVENT_KBD])
        input.keyboard.processEvent();

    if (curClock == eventClock[EVENT_HTOTAL])
        HTotalEvent();

    if (curClock == eventClock[EVENT_ONE_CYCLE_DELAY]) {
        if (curClock == rapidJobs[0].clock)
            processOneCycleEvent(rapidJobs[0]);
        if (curClock == rapidJobs[1].clock)
            processOneCycleEvent(rapidJobs[1]);
    }

    if (curClock == eventClock[EVENT_POWER_SUPPLY])
        powerSupplyEvent();

    if (curClock == eventClock[EVENT_LEAVE_EMULATION])
        leaveEmulationEvent();

    if (curClock == eventClock[EVENT_AUDIO_STATE])
        paula.audioEvent();

    if (curClock == eventClock[EVENT_SERIAL])
        paula.serialEvent();

    if (curClock == eventClock[EVENT_INTREQ])
        paula.intreqEvent();

    if (curClock == eventClock[EVENT_FLOPPY])
        paula.diskEvent();

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
    if (eventClock[EVENT_SERIAL] < next)
        next = eventClock[EVENT_SERIAL];
    if (eventClock[EVENT_INTREQ] < next)
        next = eventClock[EVENT_INTREQ];
    if (eventClock[EVENT_FLOPPY] < next)
        next = eventClock[EVENT_FLOPPY];

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

auto Agnus::addOneCycleEvent(int job, uint16_t data, int delay) -> void {
    RapidJob& rJob1 = rapidJobs[0];
    RapidJob& rJob2 = rapidJobs[1];

    if (rJob1.clock == INT64_MAX) {
        rJob1.job = job;
        rJob1.data = data;
        int64_t useClock = clock + delay;

        if (useClock < eventClock[EVENT_ONE_CYCLE_DELAY])
            updateEventAbs<EVENT_ONE_CYCLE_DELAY>(useClock);

        rJob1.clock = useClock;
    } else if (rJob2.clock == INT64_MAX) {
        rJob2.job = job;
        rJob2.data = data;
        int64_t useClock = clock + delay;

        if (useClock < eventClock[EVENT_ONE_CYCLE_DELAY])
            updateEventAbs<EVENT_ONE_CYCLE_DELAY>(useClock);

        rJob2.clock = useClock;
    } else {
        // interface->log("check one cycle event");
        // We should never end up here. If it does, this does not mean that it necessarily leads to an error situation
        // if the event is executed early.
        if (rJob1.clock <= rJob2.clock) {
        //    interface->log(rJob1.job,0);
            processOneCycleEvent<true>(rJob1);
        } else {
          //  interface->log(rJob2.job,0);
            processOneCycleEvent<true>(rJob2);
        }
        addOneCycleEvent(job, data, delay);
    }
}

auto Agnus::forceOneCycleEvent(int job) -> void {
    if (hasActiveEvent<EVENT_ONE_CYCLE_DELAY>()) {
        if ((rapidJobs[0].job & ~1) == job) {
            processOneCycleEvent(rapidJobs[0]);
        } else if ((rapidJobs[1].job & ~1) == job) {
            processOneCycleEvent(rapidJobs[1]);
        }
    }
}

template<bool isPtr> auto Agnus::inactivateOneCycleEvent(int job) -> void {
    if (hasActiveEvent<EVENT_ONE_CYCLE_DELAY>()) {
        int mask = isPtr ? ~1 : ~0;

        if ((rapidJobs[0].job & mask) == job) {
            rapidJobs[0].clock = INT64_MAX;
            updateEventAbs<Agnus::EVENT_ONE_CYCLE_DELAY>(rapidJobs[0].sibling->clock);
        } else if ((rapidJobs[1].job & mask) == job) {
            rapidJobs[1].clock = INT64_MAX;
            updateEventAbs<Agnus::EVENT_ONE_CYCLE_DELAY>(rapidJobs[1].sibling->clock);
        }
    }
}

template<bool tooSoon> auto Agnus::processOneCycleEvent(RapidJob& rJob) -> void {
    uint16_t data = rJob.data;
    int job = rJob.job;

    rJob.clock = INT64_MAX;
    rJob.job = -1;
    updateEventAbs<Agnus::EVENT_ONE_CYCLE_DELAY>(rJob.sibling->clock);

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
        case DMACON_1: {
            bool dmaConSprNew = useSpriteDMA();
            bool dmaConSprOld = (dmaCon & 0x220) == 0x220;

            if (dmaConSprNew && !dmaConSprOld) {
                if (sprQueue & 0x00ff0000) {
                    sprQueue |= 0x10 << 16;
                }
            } else if (!dmaConSprNew && dmaConSprOld) {
                if (sprQueue & 0x00ff0000) {
                    sprQueue |= 0x08 << 16;
                }
            }

            dmaConSpr = dmaConSprNew;
            addOneCycleEvent(DMACON, data, 1);
        } break;

        case DMACON: {
            if ((dmaCon ^ dmaConImm) & 0x21f)
                paula.dmaCon(dmaConImm);

            if ((dmaCon ^ dmaConImm) & 0x400)
                countWaitCycles = 1;

            dmaCon = dmaConImm;

            bool dmaConCopNew = useCopperDMA();
            bool dmaConBltNew = useBlitterDMA();
            bool aCycleMore = false;

            if (dmaConBlt != dmaConBltNew) {
                if (data) // data = Copper Access
                    dmaConBlt = useBlitterDMA();
                else
                    aCycleMore = true;
            }

            if (dmaConCop != dmaConCopNew) {
                // vAmigaTS/Agnus/Copper/CopDma/dmatoggle1
                if (dmaConCopNew && (copper.state == Copper::Read1 || copper.state == Copper::Read2)) {
                    if (hPos & 1) {
                        copper.skipped = true;
                    } else {
                        copper.prevState = copper.state;
                        copper.state = ((clock - dmaClock) == 1) ? Copper::WARMUP : Copper::WARMUP2;
                    }
                }
                aCycleMore = true;
            }

            if (aCycleMore)
                addOneCycleEvent(DMACON_3, data, 1);
        } break;
        case DMACON_3:
            dmaConCop = useCopperDMA();
            dmaConBlt = useBlitterDMA();
            break;

        case BLT_INIT: blitter.initBlit(); break;

        case SPR_DATA0: denise.setSprDatA(0, data); break;
        case SPR_DATA1: denise.setSprDatA(1, data); break;
        case SPR_DATA2: denise.setSprDatA(2, data); break;
        case SPR_DATA3: denise.setSprDatA(3, data); break;
        case SPR_DATA4: denise.setSprDatA(4, data); break;
        case SPR_DATA5: denise.setSprDatA(5, data); break;
        case SPR_DATA6: denise.setSprDatA(6, data); break;
        case SPR_DATA7: denise.setSprDatA(7, data); break;

        case SPR_DATB0: denise.setSprDatB(0, data); break;
        case SPR_DATB1: denise.setSprDatB(1, data); break;
        case SPR_DATB2: denise.setSprDatB(2, data); break;
        case SPR_DATB3: denise.setSprDatB(3, data); break;
        case SPR_DATB4: denise.setSprDatB(4, data); break;
        case SPR_DATB5: denise.setSprDatB(5, data); break;
        case SPR_DATB6: denise.setSprDatB(6, data); break;
        case SPR_DATB7: denise.setSprDatB(7, data); break;

        case SPR_CTL0: denise.setSprCtl(0, data); break;
        case SPR_CTL1: denise.setSprCtl(1, data); break;
        case SPR_CTL2: denise.setSprCtl(2, data); break;
        case SPR_CTL3: denise.setSprCtl(3, data); break;
        case SPR_CTL4: denise.setSprCtl(4, data); break;
        case SPR_CTL5: denise.setSprCtl(5, data); break;
        case SPR_CTL6: denise.setSprCtl(6, data); break;
        case SPR_CTL7: denise.setSprCtl(7, data); break;

        case SPR_POS0: denise.setSprPos(0, data); break;
        case SPR_POS1: denise.setSprPos(1, data); break;
        case SPR_POS2: denise.setSprPos(2, data); break;
        case SPR_POS3: denise.setSprPos(3, data); break;
        case SPR_POS4: denise.setSprPos(4, data); break;
        case SPR_POS5: denise.setSprPos(5, data); break;
        case SPR_POS6: denise.setSprPos(6, data); break;
        case SPR_POS7: denise.setSprPos(7, data); break;

        case BPL_CON0: denise.setBplCon0(data); break;
        case BPL_CON1: denise.setBplCon1(data); break;
        case BPL_CON2: denise.setBplCon2(data); break;
        case INTREQ: paula.setIntreq(data); break;
        case INTENA: paula.setIntena(data); break;

        case BPL_MOD1: bpl1Mod = (int16_t)(data & 0xfffe); break;
        case BPL_MOD2: bpl2Mod = (int16_t)(data & 0xfffe); break;

        case AUD_LEN0: paula.audxLen<0>(data); break;
        case AUD_LEN1: paula.audxLen<1>(data); break;
        case AUD_LEN2: paula.audxLen<2>(data); break;
        case AUD_LEN3: paula.audxLen<3>(data); break;

        case AUD_PER0: paula.audxPer<0>(data); break;
        case AUD_PER1: paula.audxPer<1>(data); break;
        case AUD_PER2: paula.audxPer<2>(data); break;
        case AUD_PER3: paula.audxPer<3>(data); break;

        case AUD_DAT0: paula.audxDat<0>(data); break;
        case AUD_DAT1: paula.audxDat<1>(data); break;
        case AUD_DAT2: paula.audxDat<2>(data); break;
        case AUD_DAT3: paula.audxDat<3>(data); break;
        case SER_DAT: paula.setSerdat(data); break;
        case DIW_START: denise.setDiwStrt(data); break;
        case DIW_STOP: denise.setDiwStop(data); break;
        case UPD_V_DIW:
            if (diwFlipFlop) {
                diwFlipFlop = false;
                updateCropBottom();
            }
            break;

        case BLT_MODA: blitter.setBltAMod(data); break;
        case BLT_MODB: blitter.setBltBMod(data); break;
        case BLT_MODC: blitter.setBltCMod(data); break;
        case BLT_MODD: blitter.setBltDMod(data); break;

        case VPOSW: vposw(data); break;
        case VHPOSW:
            vhposw(data);
            addOneCycleEvent(UPD_DENISE_VHPOS, (data & 0xff) << 1);
            break;
        case UPD_DENISE_VHPOS:
            denise.process(tooSoon ? 0 : -1);
            denise.hPos = data;
            break;
        case STROBE:
            if (isEquLine()) {
                if (!paula.vBlankIntr)
                    paula.setVblInt();

                if (!denise.vBlank)
                    denise.strequ();
            } else if (vBlank && !vBlankStart ) {
                denise.strvbl();
                if (!paula.vBlankIntr)
                    paula.setVblInt();
            } else {
                if (paula.vBlankIntr)
                    paula.vBlankIntr = false;

                denise.strhor();
            }

            if (paula.pot.running)
                paula.progressPot();
            // DMAL is fetched serial bit by bit (14 cycles).
            dmal = paula.dmal();
            break;
    }
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
    // interface->log("frame is too long, force back to UI");
    system->leaveEmulation = true;
    system->runAhead.pos = 0;
    setEventInactive<Agnus::EVENT_LEAVE_EMULATION>();
}

auto Agnus::HTotalEvent() -> void {
    if (hTotalFirst) {
        if (vPos == (lines + lof) ) {
            if (lace()) {
                lof ^= 1;
                laceMode = lof ? 1 : 2;
            } else
                laceMode = 0;
            initVCounter = true;
        }
        updateEvent<EVENT_HTOTAL>(1);
        hTotalFirst = false;
        if (!lol) {
            if (actions & ACT_BPL) {
                fetchPlanes<true>();
                actions &= ~ACT_BPL;
            }
        }

    } else {
        if (!lol) {
            actions &= ~ACT_COPPER; // "even" cycle 0 after a short line is not usable by Copper, otherwise Copper would progress 2 cycles in a row.

            shortLineBefore = true;

            if (bplState || bplQueue)
                actions |= ACT_BPL;
        } else
            shortLineBefore = false;

        hPos = 0;
        if (lolToggle) lol ^= 1;
        updateEvent<EVENT_HTOTAL>((beamCon & VARBEAMEN) ? (hTotal + lol) : (0xe2 + lol));
        hTotalFirst = true;

        if (((vPos & 3) == 0) && input.externalKeyEvent() && system->isProcessFrame())
            input.emergencyPoll();
    }
}

}
