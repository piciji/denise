
#include "drive1541.h"

namespace LIBC64 {

    auto Drive1541::rotateG64( bool irqNextCycle ) -> void {

        if (!motorRun())
            return;

        // the g64 format contains user and structure data, simply all bits of a track
        // but hasn't information about bit cell length. not quite true but read on.

        // reading a g64 image:
        // the bit cell length is calculated by divding the amount of ref cycles
        // per revolution by the amount of bits per track.
        // this way each bit cell on a track has the exact same duration.
        // it's possible to master a disk with variable bit cell length.
        // therefore a speedzone area in gcr specs exists, where
        // each byte can be assigned by a different speedzone.
        // I have read there is no g64 image out there using this feature.
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

        uint8_t refCycles = 8;
        if (readMode) {

            while ( refCycles-- ) {

                if ( filter != lastFilter ) {
                    lastFilter = filter;
                    ue7Counter = speedZone & 3;
                    uf4Counter = 0;
                    // after an amount of time without a flux reversal
                    // the rule that a one is shifted in after 3 zeros in a row
                    // is violated by some randomness. means the counter registers
                    // will be reset after some time but that doesn't mean it can
                    // be more than 3 zeros in row shifted in but fewer.
                    randCounter = ( (randomizer.xorShift() >> 16 ) % 31) + 289;
                } else {

                    if (randCounter)
                        randCounter--;

                    if (!randCounter) {
                        ue7Counter = speedZone & 3;
                        uf4Counter = 0;

                        randCounter = ( (randomizer.xorShift() >> 16 ) % 367) + 33;
                    }
                }

                ue7Counter++;

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

                        readBuffer = ((readBuffer << 1) & 0x3fe ) | ( uf4Counter == 2 ? 1 : 0 );

                        //writeBuffer <<= 1;

                        if ( readBuffer == 0x3ff )
                            ue3Counter = 0;

                        else {

                            if (++ue3Counter == 8) {
                                ue3Counter = 0;
                                writeBuffer = readBuffer & 0xff;

                                if (byteReadyOverflow) {
                                    // cpu low cycle should progress only 6 reference cycles instead of 8.
                                    // because SO is detected by cpu at ~400 ns cycle time, otherwise it needs
                                    // another cpu cycle to be recognized.
                                    // NOTE: cpu code handles recognition and execution time of v flag change.
                                    cpu->setSo( true );
                                }
                                via2->ca1In( !byteReadyOverflow, irqNextCycle );
                            } else {
                                // SO is a edge transition like nmi, don't know when real 1541 reset line.
                                // but it have to keep active at least one whole cpu cycle to be recognized safely.
                                // it's not that important how many time passes exactly because a new trigger can only
                                // happen when off state switches to on. of course it should be happen before next byte
                                // is ready.
                                cpu->setSo( false );
                                // same like Cpu SO flag the via input is edge transition
                                via2->ca1In( true, irqNextCycle );
                            }
                        }
                    }
                }

                accum += gcrTrack->bits;

                if ( accum >= refCyclesPerRevolution ) {
                    accum -= refCyclesPerRevolution;

                    if ( readBit() )
                        // too short ( < 2.5 microseconds ) flux reversals will be removed by a filter
                        // not emulated, because variable bit cell length isn't emulated either but necessary for this
                        // NOTE: gcr images are almost clean already
                        filter ^= 1;
                }
            }

        } else { // write
            while (refCycles--) {

                accum += gcrTrack->bits;

                if (accum >= refCyclesPerRevolution)
                    accum -= refCyclesPerRevolution;

                // ue7 and uf4 works same like reading
                if (++ue7Counter == 16) {

                    ue7Counter = speedZone & 3;

                    uf4Counter = (uf4Counter + 1) & 0xf;

                    if ((uf4Counter & 3) == 2) {

                        readBuffer = ((readBuffer << 1) & 0x3fe) | (uf4Counter == 2);

                        writeBit( (writeBuffer & 0x80) != 0 );

                        writeBuffer <<= 1;

                        accum = gcrTrack->bits << 1;

                        if (++ue3Counter == 8) {
                            ue3Counter = 0;

                            writeBuffer = writeValue; // fetch next byte to buffer

                            if (byteReadyOverflow)
                                cpu->setSo(true);

                            via2->ca1In( !byteReadyOverflow, irqNextCycle);
                        } else {
                            cpu->setSo(false);
                            via2->ca1In( true, irqNextCycle );
                        }
                    }
                }
            }
        }
    }
}
