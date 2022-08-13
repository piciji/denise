
#include "cia.h"

template<> auto Cia::read<MOS_6526>( unsigned pos ) -> uint8_t {

    switch (pos & 0xf) {
        case 0:
            return readPort( PORTA, &lines );

        case 1: {
            uint8_t out = readPort( PORTB, &lines );

            adjustBit6And7( out );

            return out;
        }
        case 2:
            return lines.ddra;

        case 3:
            return lines.ddrb;

        case 4:
            return readCounter<T_A>() & 0xff;

        case 5:
            return readCounter<T_A>() >> 8;

        case 6:
            return readCounter<T_B>() & 0xff;

        case 7:
            return readCounter<T_B>() >> 8;

        case 8:
            if(!todLatched)
                todLatch = todc;

            todLatched = false;

            return (todLatch >> 0) & 0xff;

        case 9:
            if(!todLatched)
                todLatch = todc;

            return (todLatch >> 8) & 0xff;

        case 0xa:
            if(!todLatched)
                todLatch = todc;

            return (todLatch >> 16) & 0xff;

        case 0xb:
            if(!todLatched)
                todLatch = todc;

            todLatched = true;

            return (todLatch >> 24) & 0xff;

        case 0xc:
            return sdr;

        case 0xd:
            delay |= CIA_ACK0;
            return icr;

        case 0xe:
            return timerA.control & 0xef;

        case 0xf:
            return timerB.control & 0xef;
    }
    _unreachable
}

template<> auto Cia::read<MOS_8520>( unsigned pos ) -> uint8_t {

    switch (pos & 0xf) {
        case 0:
            return readPort( PORTA, &lines );

        case 1: {
            uint8_t out = readPort( PORTB, &lines );

            adjustBit6And7( out );

            return out;
        }
        case 2:
            return lines.ddra;

        case 3:
            return lines.ddrb;

        case 4:
            return readCounter<T_A>() & 0xff;

        case 5:
            return readCounter<T_A>() >> 8;

        case 6:
            return readCounter<T_B>() & 0xff;

        case 7:
            return readCounter<T_B>() >> 8;

        case 8:
            if (todLatched) {
                todLatched = false;
                return todLatch & 0xff;
            }
            return todc & 0xff;

        case 9:
            if (todLatched) {
                return (todLatch >> 8) & 0xff;
            }
            return (todc >> 8) & 0xff;

        case 0xa:
            if (!todLatched) {
                if ( !(timerB.control & 0x80) ) todLatched = true;
                todLatch = todc;
            }
            return (todLatch >> 16) & 0xff;

        case 0xb:
            return 0xff;

        case 0xc:
            return sdr;

        case 0xd:
            delay |= CIA_ACK0;
            return icr;

        case 0xe:
            return timerA.control & 0xef;

        case 0xf:
            return timerB.control & 0xef;
    }
    _unreachable
}

template<> auto Cia::write<MOS_6526>( unsigned pos, uint8_t value ) -> void {
    pos &= 0xf;

    switch (pos) {
        case 8:
        case 9:
        case 0xa:
        case 0xb: {
            uint8_t shifter = (pos - 8) << 3;

            if (pos == 8)
                value &= 0x0f;
            else if (pos == 9)
                value &= 0x7f;
            else if (pos == 0xa)
                value &= 0x7f;
            else { // 0xb
                value &= 0x9f;

                if ( ( ( value & 0x1f ) == 0x12 ) && !( timerB.control & 0x80 ) )
                    value ^= 0x80; //flip AM / PM
            }

            bool changed;

            if (timerB.control & 0x80) {

                changed = value != ( ( alarm >> shifter ) & 0xff);

                alarm = (alarm & ~(0xff << shifter) ) | (value << shifter);

            } else {
                if (pos == 8) {
                    if (!todActive)
                        tickCounter = 0;

                    todActive = true;

                } else if (pos == 0xb)
                    todActive = false;

                changed = value != ( ( todc >> shifter ) & 0xff);

                todc = (todc & ~(0xff << shifter) ) | (value << shifter);
            }

            if (changed)
                if ( todc == alarm )
                    intIncomming |= 4;

            return;
        }
    }

    writeGeneric<MOS_6526>( pos, value );
}

template<> auto Cia::write<MOS_8520>( unsigned pos, uint8_t value ) -> void {
    pos &= 0xf;

    switch (pos) {
        case 8:
        case 9:
        case 0xa: {
            uint8_t shifter = (pos - 8) << 3;
            bool changed;

            if (timerB.control & 0x80) {
                changed = value != ( ( alarm >> shifter ) & 0xff);

                alarm = (alarm & ~(0xff << shifter) ) | (value << shifter);

            } else {
                if (pos == 8)
                    todActive = true;

                else if (pos == 0xa)
                    todActive = false;

                changed = value != ((todc >> shifter) & 0xff);

                todc = (todc & ~(0xff << shifter)) | (value << shifter);
            }

            if ( changed && (todc == alarm ))
                intIncomming |= 4;

            return;
        }

        case 0xb: //unused
            return;

        case 0xe:
            value &= 0x7f;
            break;
    }

    writeGeneric<MOS_8520>( pos, value );
}

template<uint8_t model> auto Cia::writeGeneric( unsigned pos, uint8_t value ) -> void {

    switch( pos ) {

        case 0:
        case 2: {
            if (pos == 0) {
                lines.pra = value;
                lines.praChange = true;
            } else {
                lines.ddra = value;
                lines.praChange = false;
            }
            // without external influence, lines show "pra" in output mode 
            // and will be pulled up in input mode, so there is no need to "and" pra with ddra
            uint8_t ioa = lines.pra | ~lines.ddra;

            // show the basic state of the lines
            lines.ioa = ioa;

            writePort( PORTA, &lines );

            lines.ioaOld = ioa;

            break;
        }
        case 1:
        case 3: {
            if (pos == 1) {
                lines.prb = value;
                lines.prbChange = true;
            } else {
                lines.ddrb = value;
                lines.prbChange = false;
            }

            uint8_t iob = lines.prb | ~lines.ddrb;

            adjustBit6And7( iob );

            lines.iob = iob;

            writePort( PORTB, &lines );

            lines.iobOld = iob;

            break;
        }
        case 4:
            timerA.latch = (timerA.latch & 0xff00) | value;

            if (delay & CIA_FL_TA2 )
                timerA.counter = timerA.latch;

            break;
        case 6:
            timerB.latch = (timerB.latch & 0xff00) | value;

            if (delay & CIA_FL_TB2)
                timerB.counter = timerB.latch;

            break;
        case 5:
            timerA.latch = (timerA.latch & 0xff) | (value << 8);

            if (delay & CIA_FL_TA2)
                timerA.counter = timerA.latch;
            else if (!(timerA.control & 1))
                delay |= CIA_FL_TA0;

            if constexpr (model == MOS_8520) {
                if (timerA.control & 8) { // oneshot mode
                    if (!(timerA.control & 0x20)) {
                        if ((delay & CIA_START_TA1) == 0)
                            delay |= CIA_START_TA0;

                        delay |= CIA_FL_TA0;
                    }
                    if (!(timerA.control & 1))
                        timerA.toggle = true;

                    timerA.control |= 1;
                }
            }
            break;

        case 7:
            timerB.latch = (timerB.latch & 0xff) | (value << 8);

            if (delay & CIA_FL_TB2 )
                timerB.counter = timerB.latch;
            else if ( !(timerB.control & 1))
                delay |= CIA_FL_TB0;

            if constexpr (model == MOS_8520) {
                if (timerB.control & 8) { // oneshot mode
                    if ( !(timerB.control & 0x60) ) {
                        if ((delay & CIA_START_TB1) == 0)
                            delay |= CIA_START_TB0;

                        delay |= CIA_FL_TB0;
                    }
                    if (!(timerB.control & 1))
                        timerB.toggle = true;

                    timerB.control |= 1;
                }
            }
            break;

        case 0xc:
            sdr = value;
            delay &= ~CIA_START_SDR1;
            delay |= CIA_START_SDR0;
            break;

        case 0xd:
            if (value & 0x80)
                icrmask |= value & 0x7F;
            else
                icrmask &= ~value;

            delay |= CIA_MASK_WRITE0;

            if ((delay & CIA_ACK1) == 0)
                handleInterrupt( 0 );

            break;

        case 0xe: {
            if ((value ^ timerA.control) & 0x40)
                switchSerialDirection((value & 0x40) == 0);

            // to do: oneshot stops the timer, is this recognized for 'toggle' reset
            // or have to write 'stop' to control register first
            if ((value & 1) && !(timerA.control & 1))
                timerA.toggle = true;

            // one shot
            if (value & 8) { // active
                timerA.oneshot = true; // no delay
                delay &= ~(CIA_DIS_OS_TA0 | CIA_DIS_OS_TA1);
            } else {
                // disabling one shot has one cycle delay
                if ((delay & CIA_DIS_OS_TA1) == 0)
                    delay |= CIA_DIS_OS_TA0;
            }
            // force load (one time)
            if ( value & 0x10 ) {
                // delayed one cycle
                // when fired, it can not be disabled next cycle
                delay |= CIA_FL_TA0;
            }

            // start + phase in
            if ( (value & 1) && !(value & 0x20) ) {
                if ((delay & CIA_START_TA1) == 0)
                    delay |= CIA_START_TA0;

            } else {
                if ((delay & CIA_STOP_TA1) == 0)
                    delay |= CIA_STOP_TA0;
            }
            timerA.control = value;
        } break;

        case 0xf: {
            if ((value & 1) && !(timerB.control & 1))
                timerB.toggle = true;

            // one shot
            if (value & 8) { // active
                timerB.oneshot = true; // no delay
                delay &= ~(CIA_DIS_OS_TB0 | CIA_DIS_OS_TB1);
            } else {
                if ((delay & CIA_DIS_OS_TB1) == 0)
                    delay |= CIA_DIS_OS_TB0;
            }
            // force load (one time)
            if ( value & 0x10 ) {
                // delayed one cycle
                // when fired, it can not be disabled next cycle
                delay |= CIA_FL_TB0;
            }
            if (value & 0x40) {
                if ((delay & CIA_STOP_TB1) == 0)
                    delay |= CIA_STOP_TB0;

            } else {
                // start + phase in
                if ( (value & 1) && !(value & 0x20) ) {
                    if ((delay & CIA_START_TB1) == 0)
                        delay |= CIA_START_TB0;

                } else {
                    // stop the timer is delayed by one cycle
                    if ((delay & CIA_STOP_TB1) == 0)
                        delay |= CIA_STOP_TB0;
                }
            }

            timerB.control = value;
        } break;
    }
}

auto Cia::adjustBit6And7( uint8_t& inOut ) -> void {
    if (timerB.control & 2) {
        inOut &= ~0x80;
        if ( ( (timerB.control & 4) ? timerB.toggle : (delay & CIA_UF_TB1) ) )
            inOut |= 0x80;
    }

    if (timerA.control & 2) {
        inOut &= ~0x40;
        if ( ( (timerA.control & 4) ? timerA.toggle : (delay & CIA_UF_TA1) ) )
            inOut |= 0x40;
    }
}
