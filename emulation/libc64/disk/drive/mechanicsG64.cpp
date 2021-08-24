
#include "drive.h"

namespace LIBC64 {

    auto Drive::rotateG64( ) -> void {
        unsigned todo;
        bool motorAdvance = motorRun() && loaded;

        // the g64 format contains user and structure data, simply all bits of a track
        // but hasn't information about bit cell length. not quite true but read on.

        // reading a g64 image:
        // the bit cell length is calculated by divding the amount of ref cycles
        // per revolution by the amount of bits per track.
        // this way each bit cell on a track has the exact same duration.
        // it's possible to master a disk with variable bit cell length.
        // therefore a speedzone area in gcr specs exists, where
        // each byte can be assigned by a different speedzone.
        // there is no g64 image out there using this feature.
        // a copy protection which relies on exact bit cell duration works only when
        // the track size matches the original track length. so a carefully prepared g64
        // image is needed.
        //
        // writing a g64 image:
        // when creating a blank g64 image there are used standard track sizes.
        // when writing it back later during emulation the original track size
        // isn't changed anymore, even when the bits will be written with a non standard
        // track speedzone. there is simply no reliable way to find out the new size when
        // not writing the complete track or if a complete track at once was written at all.
        // writing a non standard g64 image in emulation could possibly not readed correctly.
        // it seems the speedzone area of a g64 image can not fill this gap fully.
        // a real bit cell duration would be better instead of a speedzone value
        // per byte (not bit).
        // in practice this limitation could be a problem when duplicating copy protected
        // disks within emulation.
        // Note: a real 1541 can not duplicate each possible pattern 1:1

        // we know the amount of bit cells for any track and the amount of ref cycles for a
        // complete revolution. ref cycles / bit cells = ref cycles for a single bit cell.
        // the fraction of the division would be a problem, so we scale each ref cycle up
        // by the amount of bit cells for the current track. that way we sum up the scaled
        // ref cycles and compare this value with the total amount of ref cycles per revolution.
        // if exceeded we reach a new bit cell.

#define OVERFLOW_NOT_THIS_CYCLE \
        ((refCyclesInCpuCycle - refCycles + todo) > (refCyclesInCpuCycle >> 1))

        uint8_t refCycles = refCyclesInCpuCycle;
        unsigned delta;

        if (readMode) {
            do {
                todo = 1;

                if ( uf6aFlipFlop == comperatorFlipFlop ) {

                    if (motorAdvance) {
                        delta = refCyclesPerRevolution - accum;

                        if ((gcrTrack->bits << 1) <= delta) {
                            todo = delta / gcrTrack->bits;

                            if (refCycles < todo)
                                todo = refCycles;

                            if ((16 - ue7Counter) < todo)
                                todo = 16 - ue7Counter;

                            if (randCounter && (randCounter < todo))
                                todo = randCounter;
                        }
                    } else {
                        todo = refCycles;

                        if ((16 - ue7Counter) < todo)
                            todo = 16 - ue7Counter;

                        if (randCounter && (randCounter < todo))
                            todo = randCounter;
                    }

                    ue7Counter += todo;
                    randCounter -= todo;

                    if (!randCounter) {
                        if ( (type != Type::D1570) || (side == 0) ) {
                            // there is no second head for 1570, means there are no random flux reversals
                            ue7Counter = speedZone & 3;
                            uf4Counter = 0;

                            if (ue3Counter == 8)
                                byteFetched(OVERFLOW_NOT_THIS_CYCLE);
                        }
                        // freespin relies on that following random flux reversals don't prevent UE3 counter increments.
                        // so a random flux reversal mustn't happen too soon. freespin use this to find out if disk was removed
                        randCounter = ( (randomizer.xorShift() >> 16 ) % 202) + 198;
                    }

                } else {
                    uf6aFlipFlop = comperatorFlipFlop;
                    ue7Counter = speedZone & 3;
                    uf4Counter = 0;

                    if (ue3Counter == 8)
                        byteFetched( OVERFLOW_NOT_THIS_CYCLE );
                    // after an amount of time without a flux reversal
                    // the rule that a one is shifted in after 3 zeros in a row
                    // is violated by some randomness. means the counter registers
                    // will be reset after some time but that doesn't mean it can
                    // be more than 3 zeros in row shifted in but fewer.
                    // Rubicon timex protection relies on FIRST random flux reversal
                    randCounter = ( (randomizer.xorShift() >> 16 ) % 31) + 233; // 14.5 - 16.5
                }

                if (ue7Counter == 16) {
                    ue7Counter = speedZone & 3;

                    // uf4 is a 4 bit counter.
                    // every 16 ref cycles uf4 is incremented, at least for speedzone 0.
                    // when uf4 == 2 a one is shifted in.
                    // when uf4 == (6 or 10 or 14) a zero is shifted in.
                    // if there is no further flux reversal a one will be shiftd in each 3 zeros.
                    // because of magnetic mediums can not read too much zeros in row reliable.
                    uf4Counter = (uf4Counter + 1) & 0xf;

                    if ((uf4Counter & 3) == 2) {

                        readBuffer = ((readBuffer << 1) & 0x3fe) | (uf4Counter == 2 ? 1 : 0);

                        writeBuffer <<= 1;

                        if (readBuffer == 0x3ff)
                            ue3Counter = 0;
                        else
                            ue3Counter++;

                        if (!ca1Line)
                            via2->ca1In( ca1Line = true );
                    }
                        // uf4: 0,1,4,5,8,9,12,13
                    else if (((uf4Counter & 2) == 0) && (ue3Counter == 8)) {
                        // check if we count more than 6 drive cycles within this CPU cycle.
                        // comparison with 8 should be correct, see comments in drive1541.cpp
                        byteFetched( OVERFLOW_NOT_THIS_CYCLE );
                    }
                }

                if (motorAdvance) {
                    accum += gcrTrack->bits * todo;

                    if (accum >= refCyclesPerRevolution) {
                        accum -= refCyclesPerRevolution;

                        if (readBit())
                            // too short ( < 2.5 microseconds ) flux reversals will be removed by a filter
                            // not emulated, because variable bit cell length isn't emulated either but necessary for this
                            // NOTE: gcr images are almost clean already
                            comperatorFlipFlop ^= 1;
                    }
                }

                refCycles -= todo;
            } while ( refCycles );

        } else { // write
            do {
                if (motorAdvance) {
                    todo = 1;
                    delta = refCyclesPerRevolution - accum;

                    if ((gcrTrack->bits << 1) <= delta) {
                        todo = delta / gcrTrack->bits;

                        if (refCycles < todo)
                            todo = refCycles;

                        if ((16 - ue7Counter) < todo)
                            todo = 16 - ue7Counter;
                    }

                    accum += gcrTrack->bits * todo;

                    if (accum >= refCyclesPerRevolution)
                        accum -= refCyclesPerRevolution;

                } else {
                    todo = refCycles;

                    if ((16 - ue7Counter) < todo)
                        todo = 16 - ue7Counter;
                }

                // ue7 and uf4 works same like reading
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
                            via2->ca1In( ca1Line = true );
                    }
                        // uf4: 0,1,4,5,8,9,12,13
                    else if (((uf4Counter & 2) == 0) && (ue3Counter == 8)) {

                        ue3Counter = 0;
                        writeBuffer = writeValue;
                        bool overflowNotThisCycle = OVERFLOW_NOT_THIS_CYCLE;
                        if (byteReadyOverflow) {
                            cpu->triggerSO(overflowNotThisCycle ? 2 : 1);
                            byteReady = true;
                            via2->ca1In(ca1Line = false, overflowNotThisCycle);
                        }
                    }
                }

                refCycles -= todo;
            } while ( refCycles );
        }

#undef OVERFLOW_NOT_THIS_CYCLE
    }
}
