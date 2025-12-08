
#include "pia.h"

namespace Emulator {

Pia::Pia() {
    ca2Out = [this](bool direction) { };
    cb2Out = [this](bool direction) { };

    readPort = [this]( Port port ) {
        return port == Port::A ? ioa : iob;
    };

    peekPort = [this]( Port port ) {
        return port == Port::A ? ioa : iob;
    };

    writePort = []( Port, uint8_t data ) {};
    irqCall = [](bool state) {};
}

auto Pia::ca1In( bool direction ) -> void {

    if ( (direction ? 2 : 0) != (cra & 2) )
        return;

    if ((cra & 0x38) == 0x20) { // read strobe
        if (!ca2) {
            ca2Out( ca2 = true );
        }
    }

    cra |= 0x80;

    if (cra & 1)
        irqCall(true);
}

auto Pia::cb1In( bool direction ) -> void {

    if ( (direction ? 2 : 0) != (crb & 2) )
        return;

    if ((crb & 0x38) == 0x20) { // write strobe
        if (!cb2) {
            cb2Out( cb2 = true );
        }
    }

    crb |= 0x80;

    if (crb & 1)
        irqCall(true);
}

auto Pia::ca2In( bool direction ) -> void {
    if (cra & 0x20) // output
        return;

    if ( (direction ? 0x10 : 0) != (cra & 0x10) )
        return;

    cra |= 0x40;

    if (cra & 8)
        irqCall(true);
}

auto Pia::cb2In( bool direction ) -> void {
    if (crb & 0x20) // output
        return;

    if ( (direction ? 0x10 : 0) != (crb & 0x10) )
        return;

    crb |= 0x40;

    if (crb & 8)
        irqCall(true);
}

auto Pia::read(uint8_t adr) -> uint8_t {
    if (adr & 2) { // Port B
        if (adr & 1) {
            return crb;
        }

        if (crb & 4) {
            uint8_t out = readPort(Port::B);

            out = (out & ~ddrB) | (dataB & ddrB);

            crb &= ~0xc0;

            irqCall(false);

            return out;
        }

        return ddrB;
    }

    // Port A
    if (adr & 1) {
        return cra;
    }

    if (cra & 4) {
        bool strobe = (cra & 0x30) == 0x20;

        if (strobe) { // strobe
            if (ca2) {
                ca2Out( ca2 = false );
            }
        }

        uint8_t out = readPort( Port::A );

        out = ( out & ~ddrA ) | (dataA & ddrA);

        if (strobe && (cra & 8)) {
            ca2Out( ca2 = true );
        }

        cra &= ~0xc0;

        irqCall(false);

        return out;
    }

    return ddrA;
}

auto Pia::peek(uint8_t adr) -> uint8_t {
    if (adr & 2) { // Port B
        if (adr & 1) {
            return crb;
        }

        if (crb & 4) {
            uint8_t out = peekPort(Port::B);

            out = (out & ~ddrB) | (dataB & ddrB);

            return out;
        }

        return ddrB;
    }

    // Port A
    if (adr & 1) {
        return cra;
    }

    if (cra & 4) {
        uint8_t out = peekPort( Port::A );

        out = ( out & ~ddrA ) | (dataA & ddrA);

        return out;
    }

    return ddrA;
}

auto Pia::write(uint8_t adr, uint8_t value) -> void {
    if (adr & 2) {  // Port B
        if (adr & 1) {  // control
            crb &= ~0x3f;
            crb |= value & 0x3f;

            if (value & 0x20) { // output
                if ((value & 0x18) == 0x18) {
                    if (!cb2) {
                        cb2Out( cb2 = true );
                    }
                } else if ((value & 0x18) == 0x10) {
                    if (cb2) {
                        cb2Out( cb2 = false );
                    }
                }
            }
        } else {    // data
            bool strobe = false;

            if (crb & 4) {
                dataB = value;
                strobe = (crb & 0x30) == 0x20;

                if (strobe) { // strobe
                    if (cb2) {
                        cb2Out( cb2 = false );
                    }
                }
            } else {
                ddrB = value;
            }

            iob = dataB | ~ddrB;

            writePort( Port::B, iob );

            if (strobe && (crb & 8)) {
                // timing is not quite correct, happens next E transition
                cb2Out( cb2 = true );
            }
        }
    } else {    // Port A
        if (adr & 1) {  // control
            cra &= ~0x3f;
            cra |= value & 0x3f;

            if (value & 0x20) { // output
                if ((value & 0x18) == 0x18) {
                    if (!ca2) {
                        ca2Out( ca2 = true );
                    }
                } else if ((value & 0x18) == 0x10) {
                    if (ca2) {
                        ca2Out( ca2 = false );
                    }
                }
            }
        } else {    // data
            if (cra & 4) {
                dataA = value;
            } else {
                ddrA = value;
            }

            ioa = dataA | ~ddrA;

            writePort( Port::A, ioa );
        }
    }
}

auto Pia::reset() -> void {
    cra = crb = 0;
    dataA = dataB = 0;
    ddrA = ddrB = 0;

    ca2 = cb2 = true;
    ioa = iob = 0xff;
}

auto Pia::serialize(Emulator::Serializer &s) -> void {
    s.integer( cra );
    s.integer( crb );
    s.integer( dataA );
    s.integer( dataB );
    s.integer( ddrA );
    s.integer( ddrB );
    s.integer( ca2 );
    s.integer( cb2 );
    s.integer( ioa );
    s.integer( iob );
}

}
