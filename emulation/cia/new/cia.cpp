
#include "cia.h"
#include "defines.h"
#include "register.cpp"
#include "tod.cpp"

template<uint8_t model>
Cia<model>::Cia( uint8_t ident ) {
    this->ident = ident;

    readPort = []( Port port, Lines* plines ) {
        // basic mode, when lines not modified from external
        return port == PORTA ? plines->ioa : plines->iob;
    };

    writePort = []( Port, Lines* ) {};
    irqCall = [](bool state) {};
    serialOut = [](bool spLine, bool cntLine) {};

    newVersion = true;
}

template<uint8_t model>
auto Cia<model>::reset() -> void {
    lines.pra = lines.prb = 0;
    lines.ddra = lines.ddrb = 0;
    lines.ioa = lines.iob = 0xff;
    lines.ioaOld = lines.iobOld = 0xff;
    lines.praChange = lines.prbChange = 0;

    icr = icrmask = 0;
    sdr = sdrShift = 0;
    sdrShiftCount = 0;

    sdrPending = false;
    sdrLoaded = false;
    cnt = CIA_CNT0;
    sdrForceFinish = false;

    delay = 0;
    icrTemp = 0;
    intIncomming = 0;

    timerA.run = 0;
    timerA.oneshot = 0;
    timerA.latch = timerA.counter = 0xffff;
    timerA.control = 0;
    timerA.toggle = true;

    timerB.run = 0;
    timerB.oneshot = 0;
    timerB.latch = timerB.counter = 0xffff;
    timerB.control = 0;
    timerB.toggle = true;

    todLatched = false;
    todActive = false;

    todLatch = todc = 1 << 24;
    alarm = 0;
    tickCounter = 0;
}

template<uint8_t model>
auto Cia<model>::clock() -> void {
    const uint64_t _delay = delay;

    if (_delay & CIA_TASKS) {
        if (_delay & CIA_START_TA1) timerA.run |= 1;
        else if (_delay & CIA_STOP_TA1) timerA.run &= ~1;

        if (_delay & CIA_START_TB1) timerB.run |= 1;
        else if (_delay & CIA_STOP_TB1) timerB.run &= ~1;

        if (_delay & CIA_DIS_OS_TA1) timerA.oneshot = 0;
        if (_delay & CIA_DIS_OS_TB1) timerB.oneshot = 0;

        if (_delay & CIA_UPD_ICR_IRQ1) {
            icr = icrTemp;
            if (!(_delay & CIA_ACK0))
                irqCall( true );
        }
        if (_delay & CIA_UPD_ICR_ONLY1) icr = icrTemp;

        if (_delay & CIA_TASKS_UNLIKELY) {
            
            if constexpr (model == MOS_8520) { // todo for 6526
                if (_delay & CIA_TOD3)
                    todIncrement();
            }

            if (_delay & CIA_STEPOUT_TA1) timerA.run &= ~2;
            if (_delay & CIA_STEPOUT_TB1) timerB.run &= ~2;

            if (_delay & CIA_STEP_TA1) {
                if ( timerA.control & 1 ) {
                    timerA.run |= 2;
                    delay |= CIA_STEPOUT_TA0;
                }
            }
            if (_delay & CIA_STEP_TB1) {
                if ( timerB.control & 1 ) {
                    timerB.run |= 2;
                    delay |= CIA_STEPOUT_TB0;
                }
            }

            if (_delay & CIA_START_SDR1) {
                if (!sdrLoaded) {
                    sdrShift = sdr;
                    sdrLoaded = true;
                } else
                    sdrPending = true;
            }

            if (_delay & CIA_FLIP_CNT2) flipCnt();
            if (_delay & CIA_FINISH_SDR2) intIncomming |= 8;
        }
    }

    // collect all incomming interrupt sources of this cycle
    icrTemp = 0;

    updateState<T_B>();
    updateState<T_A>();

    if (unlikely(intIncomming)) {
        handleInterrupt( intIncomming );
        intIncomming = 0;
    }

    if (delay & (CIA_INT1 | CIA_ACK0)) {
        if constexpr (model == MOS_8520)
            interruptControl();
        else
            newVersion ? interruptControl() : interruptControlOld();
    }

    delay = ((delay << 1) & CIA_MASK) | cnt;
}

template<uint8_t model>
inline auto Cia<model>::interruptControl() -> void {
    // for new cia models
    if (delay & CIA_INT1) {
        // interrupt is incomming and allowed by icr mask, a write in mask register
        // before can cause this situation too
        icrTemp |= 0x80;
        icr |= 0x80;

        if (delay & CIA_ACK0) { // we have both at same time, interrupt and acknowledge cycle
            irqCall( false );
            // interrupt is scheduled for next cycle, so cpu can not recognize it this cycle.
            // icr is reseted next cycle too with zero or the interrupts incomming this cycle
            delay |= CIA_UPD_ICR_IRQ0;
        } else
            // normal interrupt behaviour
            irqCall( true );

    } else /*if (delay & CIA_ACK0)*/ {
        // interrupt is not incomming or is not allowed by icr mask and
        // this is an acknowledge cycle
        irqCall( false );
        // we schedule to update icr next cycle, so a possible second read in a row
        // gets the non reseted state of icr.
        // in next cycle icr will be reseted with zero or the interrupts incomming this cycle
        delay |= CIA_UPD_ICR_ONLY0;
    }
}

template<uint8_t model>
inline auto Cia<model>::interruptControlOld() -> void {
    // for old cia models, interrupt is incomming one cycle later
    if (delay & CIA_INT1) {

        if (delay & CIA_ACK0) {
            // interrupt and acknowledge cycle at the same time.
            // msb is seted but there is no interrupt sended to cpu like new cia
            icr = 0x80;
            irqCall( false );
            // icr is reseted next cycle with zero or the interrupts incomming this cycle
            delay |= CIA_UPD_ICR_ONLY0;
        } else {
            // normal interrupt behaviour
            icr |= 0x80;
            irqCall( true );
        }

    } else /*if (delay & CIA_ACK0)*/ {
        // same behaviour like new cia, but all interrupt sources will be reseted
        // but not the msb of icr, matters when a second read in register 0d happens
        icr &= ~0x7f;
        irqCall( false );
        delay |= CIA_UPD_ICR_ONLY0;
    }
}

template<uint8_t model>
auto Cia<model>::handleInterrupt( uint8_t number ) -> void {
    icr |= number;
    icrTemp |= number;

    if (( (number ? number : icr) & icrmask) == 0 ) {
        // for old cias interrupts are delayed by one cycle.
        // if an underflow happens a cycle sooner followed by a second write
        // in a row to 0xd, which disables the mask, then
        // a scheduled interrupt is discarded.
        // can not happen for new cias
        if (number == 0) // write in icr mask register
            if(delay & CIA_MASK_WRITE1) // second write in mask register in a row
                delay &= ~CIA_INT;

        return;
    }

    if constexpr (model == MOS_8520)
        delay |= CIA_INT1;
    else
        delay |= newVersion ? CIA_INT1 : CIA_INT0;
}

template<uint8_t model>
template<uint8_t timerId> inline auto Cia<model>::updateState( ) -> void {
    Timer& rTimer = timerId == T_A ? timerA : timerB;

    if ( rTimer.run ) {
        if (rTimer.counter == 0) {
            delay |= timerId == T_A ? (CIA_UF_TA0 | CIA_FL_TA1) : (CIA_UF_TB0 | CIA_FL_TB1);

            timerId == T_A ? timerAUnderflow() : timerBUnderflow();

            if (rTimer.oneshot) {

                rTimer.control &= ~1;

                delay &= timerId == T_A ? ~(CIA_START_TA0 | CIA_START_TA1) : ~(CIA_START_TB0 | CIA_START_TB1);

                rTimer.run = 0;
            }

            rTimer.counter = rTimer.latch;
        }
        else if ( delay & ( timerId == T_A ? CIA_FL_TA1 : CIA_FL_TB1 ) )
            rTimer.counter = rTimer.latch;
        else
            rTimer.counter--;

    } else if ( delay & ( timerId == T_A ? CIA_FL_TA1 : CIA_FL_TB1 ) )
        rTimer.counter = rTimer.latch;
}

template<uint8_t model>
template<uint8_t timerId> inline auto Cia<model>::readCounter( ) -> uint16_t {
    Timer& rTimer = timerId == T_A ? timerA : timerB;

    if (rTimer.run && !(delay & ( timerId == T_A ? CIA_FL_TA2 : CIA_FL_TB2 )) )
        return (rTimer.counter + 1);

    return rTimer.counter;
}

template<uint8_t model>
auto Cia<model>::timerAUnderflow() -> void {
    if (timerA.control & 0x40)
        shiftOut();

    timerA.toggle ^= 1;

    if ( (timerB.control & 0x61) == 0x41 )
        delay |= CIA_STEP_TB0;

    else if ( (delay & CIA_CNT1) && ((timerB.control & 0x61) == 0x61 ))
        delay |= CIA_STEP_TB0;

    handleInterrupt( 1 );
}

template<uint8_t model>
auto Cia<model>::timerBUnderflow() -> void {
    timerB.toggle ^= 1;

    handleInterrupt( 2 );

    // timer B bug for old cias
    // if timer B underflows in acknowledge cycle, it triggers an interrupt
    // next cycle like expected but the second bit in icr is not seted
    // note: acknowledge cycle is not the cycle when the read happened but the
    // cycle after

    if constexpr (model == MOS_6526) {
        if ((delay & CIA_ACK0) && !newVersion) {
            icrTemp &= ~2;
            icr &= ~2;
        }
    }
}

template<uint8_t model>
auto Cia<model>::setFlag() -> void {
    intIncomming |= 0x10;
}

template<uint8_t model>
auto Cia<model>::setNewVersion( bool state ) -> void {
    newVersion = state;
}

template<uint8_t model>
auto Cia<model>::isNewVersion() -> bool {
    return newVersion;
}

template<uint8_t model>
auto Cia<model>::shiftOut() -> void {
    //timer A defines speed for this

    if ( sdrLoaded && !sdrShiftCount)
        sdrShiftCount = 16;

    if (!sdrShiftCount)
        return;

    if (delay & (CIA_FLIP_DUMMY1 | CIA_FLIP_CNT1)) {
        delay |= CIA_FLIP_DUMMY0;
    } else {
        delay |= CIA_FLIP_CNT0;
    }
}

// calling this function from extern assumes positive edge of CNT line.
// this saves you the need to reset the CNT pin to trigger another transition.
template<uint8_t model>
auto Cia<model>::serialIn(bool spLine) -> void {
    cnt = CIA_CNT0;
    delay |= CIA_CNT0;
    positiveCntTransition();

    if (timerA.control & 0x40) // serial is defined as output
        return;

    sdrShift <<= 1;
    sdrShift |= spLine;

    if ( ++sdrShiftCount == 8 ) {
        sdrShiftCount = 0;
        sdr = sdrShift;
        delay &= ~(CIA_FINISH_SDR1 | CIA_FINISH_SDR2);
        delay |= CIA_FINISH_SDR0;
    }
}

template<uint8_t model>
auto Cia<model>::setCNTAndSP(bool cntLine, bool spLine) -> void {
    if (cntLine == (!!cnt))
        // only by positive edge transition, state of SP line is recognized and timer steps (if activated)
        return;

    if (cntLine)
        serialIn( spLine );
    else
        cnt = 0;
}

template<uint8_t model>
auto Cia<model>::positiveCntTransition( ) -> void {
    if ((timerA.control & 0x21) == 0x21) {//timer A is driven by cnt pin transition
        delay &= ~CIA_STEP_TA1;
        delay |= CIA_STEP_TA0;
    }

    if ( ( timerB.control & 0x61) == 0x21) {//timer B is driven by cnt pin transition
        delay &= ~CIA_STEP_TB1;
        delay |= CIA_STEP_TB0;
    }
}

template<uint8_t model>
auto Cia<model>::switchSerialDirection(bool input) -> void {

    if ((sdrShiftCount > 1 && sdrShiftCount < 15) || (sdrShiftCount == 15 && !(delay & CIA_CNT2))) {
        delay &= ~(CIA_FINISH_SDR0 | CIA_FINISH_SDR2);
        delay |= CIA_FINISH_SDR1;
    }

    if (input) {
      //  if ((model == MOS_8520) || newVersion)
            sdrForceFinish = (delay & CIA_CNT_NEW) != CIA_CNT_NEW;
        //else
          //  sdrForceFinish = (delay & CIA_CNT) != CIA_CNT;

        if (!sdrForceFinish) {
            if (sdrShiftCount != 2 && (delay & CIA_FLIP_CNT2 )  )
                sdrForceFinish = true;
        }

    } else {
        if (!cnt && sdrShiftCount)
            sdrShift <<= 1;

        if (sdrForceFinish) {
            delay &= ~(CIA_FINISH_SDR0 | CIA_FINISH_SDR2);
            delay |= CIA_FINISH_SDR1;
            sdrForceFinish = false;
        }
    }

    if (!cnt)
        positiveCntTransition();

    cnt = CIA_CNT0;
    delay |= CIA_CNT0;
    serialOut(input, true);

    delay &= ~(CIA_FLIP_CNT0 | CIA_FLIP_CNT1 | CIA_FLIP_CNT2 | CIA_FLIP_DUMMY0 | CIA_FLIP_DUMMY1);
    sdrShiftCount = 0;
    sdrPending = false;
    sdrLoaded = false;
}

template<uint8_t model>
auto Cia<model>::flipCnt() -> void {
    if (!sdrShiftCount)
        return;

    if (!cnt)
        positiveCntTransition();

    cnt ^= CIA_CNT0;

    if (cnt)
        sdrShift <<= 1;
    else
        serialOut( (sdrShift & 0x80) != 0, false ); // SP data is valid on falling edge

    if (--sdrShiftCount == 1) {
        delay &= ~(CIA_FINISH_SDR1 | CIA_FINISH_SDR2);
        delay |= CIA_FINISH_SDR0;

        if (sdrPending) {
            sdrPending = false;
            sdrShift = sdr;
            sdrLoaded = true;
        } else
            sdrLoaded = false;
    }
};

template<uint8_t model>
auto Cia<model>::serialize(Emulator::Serializer& s) -> void {
    s.integer( lines.pra );
    s.integer( lines.prb );
    s.integer( lines.ddra );
    s.integer( lines.ddrb );
    s.integer( lines.ioa );
    s.integer( lines.iob );
    s.integer( lines.ioaOld );
    s.integer( lines.iobOld );
    s.integer( lines.praChange );
    s.integer( lines.prbChange );

    s.integer( timerA.run );
    s.integer( timerA.oneshot );
    s.integer( timerA.latch );
    s.integer( timerA.counter );
    s.integer( timerA.control );
    s.integer( timerA.toggle );

    s.integer( timerB.run );
    s.integer( timerB.oneshot );
    s.integer( timerB.latch );
    s.integer( timerB.counter );
    s.integer( timerB.control );
    s.integer( timerB.toggle );

    s.integer( delay );
    s.integer( newVersion );
    s.integer( icrTemp );
    s.integer( sdr );
    s.integer( sdrLoaded );
    s.integer( sdrPending );
    s.integer( cnt );
    s.integer( sdrShift );
    s.integer( sdrShiftCount );
    s.integer( sdrForceFinish );
    s.integer( icrmask );
    s.integer( icr );
    s.integer( intIncomming );

    s.integer( todLatched );
    s.integer( todActive );
    s.integer( todLatch );
    s.integer( alarm );
    s.integer( todc );
    s.integer( tickCounter );
}

template Cia<MOS_6526>::Cia( uint8_t ident);
template Cia<MOS_8520>::Cia( uint8_t ident);

template auto Cia<MOS_6526>::reset() -> void;
template auto Cia<MOS_8520>::reset() -> void;
template auto Cia<MOS_6526>::clock() -> void;
template auto Cia<MOS_8520>::clock() -> void;
template auto Cia<MOS_6526>::read(unsigned pos) -> uint8_t;
template auto Cia<MOS_8520>::read(unsigned pos) -> uint8_t;
template auto Cia<MOS_6526>::write(unsigned pos, uint8_t value) -> void;
template auto Cia<MOS_8520>::write(unsigned pos, uint8_t value) -> void;
template auto Cia<MOS_6526>::tod(unsigned clockAllignment) -> void;
template auto Cia<MOS_8520>::tod(unsigned clockAllignment) -> void;

template auto Cia<MOS_6526>::serialIn(bool spLine) -> void;
template auto Cia<MOS_8520>::serialIn(bool spLine) -> void;
template auto Cia<MOS_6526>::setCNTAndSP(bool cntLine, bool spLine) -> void;
template auto Cia<MOS_8520>::setCNTAndSP(bool cntLine, bool spLine) -> void;

template auto Cia<MOS_6526>::setFlag() -> void;
template auto Cia<MOS_8520>::setFlag() -> void;

template auto Cia<MOS_6526>::serialize(Emulator::Serializer& s) -> void;
template auto Cia<MOS_8520>::serialize(Emulator::Serializer& s) -> void;

template auto Cia<MOS_6526>::setNewVersion( bool newVersion ) -> void;
template auto Cia<MOS_6526>::isNewVersion() -> bool;