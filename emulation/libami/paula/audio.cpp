
#define AUDxIR(nr) scheduleIntreqAud<nr>();
#define AUDxIP(nr) ((intUpdClock == agnus.clock ? intreqLast : intreq) & (0x80 << nr))
#define volcntrld() cha.vol = (int8_t)cha.volLatch;
#define lencntrld() cha.len = cha.lenLatch;
#define dmasen() if (cha.len == 1) cha.dsr = true; \
                 else cha.dr = true;
#define notLenfin() (cha.len != 1)
#define lencount() cha.len--;
#define penhi() addSample<nr>( cha.buffer >> 8 );
#define penlo() addSample<nr>( cha.buffer & 0xff );

namespace LIBAMI {

auto Paula::audioEvent() -> void {
    Channel& cha0 = channels[0];
    Channel& cha1 = channels[1];
    Channel& cha2 = channels[2];
    Channel& cha3 = channels[3];
    int64_t clock = agnus.clock;

    if (cha0.percount == clock) {
        cha0.percount = INT64_MAX;
        perfin<0>();
    }
    if (cha1.percount == clock) {
        cha1.percount = INT64_MAX;
        perfin<1>();
    }
    if (cha2.percount == clock) {
        cha2.percount = INT64_MAX;
        perfin<2>();
    }
    if (cha3.percount == clock) {
        cha3.percount = INT64_MAX;
        perfin<3>();
    }

    int64_t nextClock = cha0.percount;
    if (cha1.percount < nextClock)
        nextClock = cha1.percount;

    if (cha2.percount < nextClock)
        nextClock = cha2.percount;

    if (cha3.percount < nextClock)
        nextClock = cha3.percount;

    agnus.updateEventAbs<Agnus::EVENT_AUDIO_STATE>(nextClock);
}

template<uint8_t nr> auto Paula::toggleAudioDMA( ) -> void {
    Channel& cha = channels[nr];

    if (cha.AUDxON) {
        if (cha.state == 0) {
            if (cha.intreq2) { // vAmigaTS/Paula/Audio/timing/dmatim2
                AUDxIR(nr);
                cha.intreq2 = false;
            }

            lencntrld()
            // if state 0 is not left or changed to state 1, "percntrld" is constantly reloaded. For performance reasons,
            // we do not do this. This should not be a problem because "percntrld" is reloaded before reaching state 2.
            cha.dsr = true; // schematics: except for this case, dmasen is true only when LENFIN = 1 ... means always true
            cha.dr = true;
            cha.state = 1;
        }
    } else {
        if (cha.state == 1 || cha.state == 5)
            cha.state = 0;
    }
    // state 2 and 3 do not react immediately to a DMA state change, only when "perfin"
}

auto Paula::updateModulation() -> void {
    for( int nr = 0; nr < 4; nr++) {
        Channel& cha = channels[nr];
        cha.audav = adkcon & (0x01 << nr);
        cha.audap = adkcon & (0x10 << nr);
        cha.napnav = !cha.audap || cha.audav;
    }
}

template<uint8_t nr> auto Paula::audxDat(uint16_t value) -> void {
    Channel& cha = channels[nr];
    cha.dat = value;

    if (cha.AUDxON) {
        if (cha.state == 2 || cha.state == 3) {
            if (notLenfin()) {
                lencount()
            } else {
                lencntrld()
                cha.intreq2 = true;
            }

        } else if (cha.state == 1) {
            AUDxIR(nr)
            dmasen()

            if (notLenfin())
                lencount()

            cha.state = 5;
        } else if (cha.state == 5) {
            percntrld<nr, true>();
            volcntrld()
            pbufld1<nr>();

            if (cha.napnav) {
                dmasen()
            }

            penhi()
            cha.state = 2;
        }

    } else {
        if (cha.state == 0) {
            if (!AUDxIP(nr)) {
                percntrld<nr, true>();
                volcntrld()
                pbufld1<nr>();
                AUDxIR(nr)
                penhi()
                cha.state = 2;
            }
        }
    }
}

template<uint8_t nr> inline auto Paula::addSample( uint8_t sample ) -> void {
    Channel& cha = channels[nr];
    cha.sample = (int8_t)sample * cha.vol;
}

template<uint8_t nr> auto Paula::audxLen(uint16_t value) -> void {
    Channel& cha = channels[nr];
    cha.lenLatch = value;
}

template<uint8_t nr> auto Paula::audxPer(uint16_t value) -> void {
    Channel& cha = channels[nr];
    cha.perLatch = value;
}

template<uint8_t nr> auto Paula::audxVol(uint16_t value) -> void {
    Channel& cha = channels[nr];
    value &= 127;
    if (value & 0x40) value = 0x40;
    cha.volLatch = value;
}

template<uint8_t nr> auto Paula::pbufld1() -> void {
    Channel& cha = channels[nr];

    if (cha.audav) {
        if constexpr (nr == 3) return;
        audxVol<nr + 1>(cha.dat);
    } else
        cha.buffer = cha.dat;
}

template<uint8_t nr> auto Paula::pbufld2() -> void {
    if constexpr (nr == 3) return;
    Channel& cha = channels[nr];
    audxPer<nr + 1>(cha.dat);
}

template<uint8_t nr> auto Paula::perfin() -> void {
    Channel& cha = channels[nr];

    if (cha.state == 2) {
        percntrld<nr, false>();
        if (cha.audap) {
            pbufld2<nr>();
            if (cha.AUDxON) {
                dmasen()

                if (cha.intreq2) {
                    AUDxIR(nr)
                    cha.intreq2 = false;
                }
            } else
                AUDxIR(nr)
        }
        penlo()
        cha.state = 3;

    } else if (cha.state == 3) {
        if (cha.AUDxON || !AUDxIP(nr)) {
            percntrld<nr, false>();
            volcntrld()
            pbufld1<nr>();

            if (cha.napnav) {
                if (cha.AUDxON) {
                    dmasen()

                    if (cha.intreq2) {
                        AUDxIR(nr)
                        cha.intreq2 = false;
                    }
                } else
                    AUDxIR(nr)
            }
            penhi()
            cha.state = 2;
        } else {
            cha.state = 0;
       //     cha.intreq2 = false; // don't disable it: vAmigaTS/Paula/Audio/timing/dmatim2
            cha.percount = INT64_MAX;
        }
    }
}

template<uint8_t nr, bool updEvent> auto Paula::percntrld() -> void {
    Channel& cha = channels[nr];
    cha.percount = agnus.clock + (int64_t)(cha.perLatch ? cha.perLatch : 0x10000);

    if constexpr (updEvent) {
        if (cha.percount < agnus.eventClock[Agnus::EVENT_AUDIO_STATE])
            agnus.updateEventAbs<Agnus::EVENT_AUDIO_STATE>(cha.percount);
    }
}

auto Paula::setFilterMode( int val ) -> void {
    filterMode = val;
    informPowerLED();
}

auto Paula::setResampleQuality( int val ) -> void {
    uint8_t sampleLimitBefore = sampleLimit;

    switch(val) {
        case 0: sampleLimit = 8; break;
        case 1: sampleLimit = 16; break;
        case 2: sampleLimit = 24; break;
        case 3: sampleLimit = 32; break;
        case 4: sampleLimit = 40; break;
        case 5: sampleLimit = 48; break;
        case 6: sampleLimit = 56; break;
        default:
        case 7: sampleLimit = 64; break;
        case 8: sampleLimit = 80; break;
        case 9: sampleLimit = 96; break;
        case 10: sampleLimit = 112; break;
        case 11: sampleLimit = 128; break;
    }

    if (sampleLimitBefore != sampleLimit)
        setFilter();
}

auto Paula::getResampleQuality( ) -> int {
    switch(sampleLimit) {
        case 8: return 0;
        case 16: return 1;
        case 24: return 2;
        case 32: return 3;
        case 40: return 4;
        case 48: return 5;
        case 56: return 6;
        default:
        case 64: return 7;
        case 80: return 8;
        case 96: return 9;
        case 112: return 10;
        case 128: return 11;
    }

    _unreachable
}

template auto Paula::audxDat<0>(uint16_t value) -> void;
template auto Paula::audxDat<1>(uint16_t value) -> void;
template auto Paula::audxDat<2>(uint16_t value) -> void;
template auto Paula::audxDat<3>(uint16_t value) -> void;

template auto Paula::audxLen<0>(uint16_t value) -> void;
template auto Paula::audxLen<1>(uint16_t value) -> void;
template auto Paula::audxLen<2>(uint16_t value) -> void;
template auto Paula::audxLen<3>(uint16_t value) -> void;

template auto Paula::audxPer<0>(uint16_t value) -> void;
template auto Paula::audxPer<1>(uint16_t value) -> void;
template auto Paula::audxPer<2>(uint16_t value) -> void;
template auto Paula::audxPer<3>(uint16_t value) -> void;

template auto Paula::audxVol<0>(uint16_t value) -> void;
template auto Paula::audxVol<1>(uint16_t value) -> void;
template auto Paula::audxVol<2>(uint16_t value) -> void;
template auto Paula::audxVol<3>(uint16_t value) -> void;

}
