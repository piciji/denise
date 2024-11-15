
#include "drive.h"

namespace LIBC64 {

#define OVERFLOW_NOT_THIS_CYCLE \
    ((refCyclesInCpuCycle - refCycles + todo) > (refCyclesInCpuCycle >> 1))

    template<bool withWd1770> auto Drive::rotateEncoded( ) -> void {
        unsigned _refCyclesPerRevolution = refCyclesPerRevolution;
        unsigned refCycles = refCyclesInCpuCycle;
        unsigned delta;
        unsigned todo;

        if (!mechanics.motorDelay) {
            // MFM in G71 is supported. However, MFM is only saved decoded to prevent G71 from having to be enlarged later
            // or created with an atypical size. G81, on the other hand, contains encoded data.
            if constexpr (withWd1770)
                wd1770.rotateDecoded(motorOn ? _refCyclesPerRevolution : 0);

            if (!motorOn)
                return;
        } else {
            if constexpr (withWd1770) {
                if(!motorRun(_refCyclesPerRevolution))
                    _refCyclesPerRevolution = 0;

                wd1770.rotateDecoded( _refCyclesPerRevolution );
                if (!_refCyclesPerRevolution)
                    return;
            } else {
                if(!motorRun(_refCyclesPerRevolution))
                    return;
            }
        }

        if (readMode) {
            do {
                todo = 1;

                delta = _refCyclesPerRevolution - accum;

                if ((gcrTrack->bits << 1) <= delta) {
                    todo = delta / gcrTrack->bits;

                    if (refCycles < todo)
                        todo = refCycles;

                    if ((16 - ue7Counter) < todo)
                        todo = 16 - ue7Counter;

                    if (randCounter && (randCounter < todo))
                        todo = randCounter;
                }

                if ( uf6aFlipFlop == comperatorFlipFlop ) {
                    randCounter -= todo;

                    if (!randCounter) {
                        if ( (type != Type::D1570) || (side == 0) ) {
                            // there is no second head for 1570, means there are no random flux reversals
                            ue7Counter = speedZone & 3;
                            uf4Counter = 0;

                            if (ue3Counter == 8)
                                byteFetched(OVERFLOW_NOT_THIS_CYCLE);
                        }
                        randCounter = ( (randomizer.xorShift() >> 16 ) % 367) + 33; // continuous oscillation happens faster
                    }

                } else {
                    uf6aFlipFlop = comperatorFlipFlop;
                    ue7Counter = speedZone & 3;
                    uf4Counter = 0;
                    todo = 1;

                    if (ue3Counter == 8)
                        byteFetched( OVERFLOW_NOT_THIS_CYCLE );

                    randCounter = ( (randomizer.xorShift() >> 16 ) % 31) + 233; // 14.5 - 16.5
                }

                ue7Counter += todo;
                if (ue7Counter == 16) {
                    ue7Counter = speedZone & 3;

                    uf4Counter = (uf4Counter + 1) & 0xf;

                    if ((uf4Counter & 3) == 2) {

                        readBuffer = ((readBuffer << 1) & 0x3fe) | (uf4Counter == 2 ? 1 : 0);

                        writeBuffer <<= 1;

                        if (readBuffer == 0x3ff)
                            ue3Counter = 0;
                        else
                            ue3Counter++;

                        if (!ca1Line)
                            via2.ca1In( ca1Line = true );
                    }
                        // uf4: 0,1,4,5,8,9,12,13
                    else if (((uf4Counter & 2) == 0) && (ue3Counter == 8)) {
                        // check if we count more than 6 drive cycles within this CPU cycle.
                        // comparison with 8 should be correct, see comments in drive1541.cpp
                        byteFetched( OVERFLOW_NOT_THIS_CYCLE );
                    }
                }

                accum += gcrTrack->bits * todo;

                if (accum >= _refCyclesPerRevolution) {
                    accum -= _refCyclesPerRevolution;

                    if (readBit())
                        // too short (< 2.5 microseconds) flux reversals will be removed by a filter
                        // not emulated, because variable bit cell length isn't emulated either but necessary for this
                        // NOTE: gcr images are almost clean already
                        comperatorFlipFlop ^= 1;
                }

                refCycles -= todo;
            } while ( refCycles );

        } else { // write
            // todo: set standard bit length depending on drive speed
            do {
                todo = 1;
                delta = _refCyclesPerRevolution - accum;

                if ((gcrTrack->bits << 1) <= delta) {
                    todo = delta / gcrTrack->bits;

                    if (refCycles < todo)
                        todo = refCycles;

                    if ((16 - ue7Counter) < todo)
                        todo = 16 - ue7Counter;
                }

                accum += gcrTrack->bits * todo;

                if (accum >= _refCyclesPerRevolution)
                    accum -= _refCyclesPerRevolution;

                // ue7 and uf4 work same like reading
                ue7Counter += todo;
                if (ue7Counter == 16) {

                    ue7Counter = speedZone & 3;

                    uf4Counter = (uf4Counter + 1) & 0xf;

                    if ((uf4Counter & 3) == 2) {

                        readBuffer = ((readBuffer << 1) & 0x3fe) | (uf4Counter == 2 ? 1 : 0);

                        writeBit( (writeBuffer & 0x80) != 0 );

                        writeBuffer <<= 1;

                        accum = gcrTrack->bits << 1;

                        ue3Counter++;

                        if (!ca1Line)
                            via2.ca1In( ca1Line = true );
                    }
                        // uf4: 0,1,4,5,8,9,12,13
                    else if (((uf4Counter & 2) == 0) && (ue3Counter == 8)) {
                        byteWritten(OVERFLOW_NOT_THIS_CYCLE);
                    }
                }

                refCycles -= todo;
            } while ( refCycles );
        }

#undef OVERFLOW_NOT_THIS_CYCLE
    }
}
