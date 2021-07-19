
#include "drive1541.h"

namespace LIBC64 {

    auto Drive1541::rotateP64(  ) -> void {
        unsigned todo;
        bool motorAdvance = motorRun() && loaded;

#define OVERFLOW_NOT_THIS_CYCLE \
        ((refCyclesInCpuCycle - refCycles + todo) > (refCyclesInCpuCycle >> 1))

        uint8_t refCycles = refCyclesInCpuCycle;

        if (readMode) {
            do {
                if (motorAdvance) {
                    todo = pulseDelta;

                    if (refCycles < todo)
                        todo = refCycles;
                } else
                    todo = refCycles;

                if ((16 - ue7Counter) < todo)
                    todo = 16 - ue7Counter;

                if ((pulseDuration < 40) && ((40 - pulseDuration) < todo))
                    todo = 40 - pulseDuration;

                if (randCounter && (randCounter < todo))
                    todo = randCounter;

                ue7Counter += todo;

                pulseDuration += todo;
                if ((pulseDuration == 40) && (uf6aFlipFlop != comperatorFlipFlop)) {
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
                    //randCounter = randomizer.rand(0, 31) + 189; // 12 - 14 micro
                    randCounter = ( (randomizer.xorShift() >> 16 ) % 31) + 194;
                } else {

                    randCounter -= todo;

                    if (!randCounter) {
                        ue7Counter = speedZone & 3;
                        uf4Counter = 0;

                        if (ue3Counter == 8)
                            byteFetched( OVERFLOW_NOT_THIS_CYCLE );

                       // randCounter = randomizer.rand(0, 367) + 33;  // 2 - 25 micro
                        randCounter = ( (randomizer.xorShift() >> 16 ) % 367) + 33;
                    }
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
                    }
                        // uf4: 0,1,4,5,8,9,12,13
                    else if (((uf4Counter & 2) == 0) && (ue3Counter == 8))
                        // check if we count more than 6 drive cycles within this CPU cycle.
                        // comparison with 8 should be correct, see comments in drive1541.cpp
                        byteFetched( OVERFLOW_NOT_THIS_CYCLE );
                }

                if (motorAdvance) {
                    pulseDelta -= todo;

                    if (!pulseDelta) {
                        Structure1541::Pulse& pulse = gcrTrack->pulses[pulseIndex];

                        pulseIndex = pulse.next;

                        if (pulseIndex >= 0)
                            pulseDelta = gcrTrack->pulses[pulseIndex].position - pulse.position;
                        else {
                            pulseIndex = gcrTrack->firstPulse;

                            pulseDelta = gcrTrack->pulses[pulseIndex].position
                                         + (CyclesPerRevolution300Rpm - pulse.position);
                        }

                        if ((pulse.strength == 0xffffffff) || (randomizer.rand() < pulse.strength)) {
                            comperatorFlipFlop ^= 1;
                            pulseDuration = 0;
                        }
                    }
                }
                refCycles -= todo;
            } while (refCycles);
        // write mode
        } else {
            bool flux;

            do {
                flux = false;

                if (motorAdvance) {
                    todo = pulseDelta;

                    if (refCycles < todo)
                        todo = refCycles;
                } else
                    todo = refCycles;

                if ((16 - ue7Counter) < todo)
                    todo = 16 - ue7Counter;

                ue7Counter += todo;
                if (ue7Counter == 16) {

                    ue7Counter = speedZone & 3;

                    uf4Counter = (uf4Counter + 1) & 0xf;

                    if ((uf4Counter & 3) == 2) {

                        readBuffer = ((readBuffer << 1) & 0x3fe) | (uf4Counter == 2 ? 1 : 0);

                        flux = (writeBuffer & 0x80) != 0;

                        writeBuffer <<= 1;

                        ue3Counter++;
                    }
                        // uf4: 0,1,4,5,8,9,12,13
                    else if (((uf4Counter & 2) == 0) && (ue3Counter == 8)) {

                        ue3Counter = 0;
                        writeBuffer = writeValue;
                        bool overflowNotThisCycle = OVERFLOW_NOT_THIS_CYCLE;

                        if (byteReadyOverflow) {
                            cpu->triggerSO(overflowNotThisCycle ? 2 : 1);
                            byteReady = true;
                            via2->ca1In(false, overflowNotThisCycle);
                        }
                    }
                }

                if (motorAdvance) {
                    pulseDelta -= todo;

                    if (pulseDelta) {

                        if (flux) {
                            if (!writeProtected) {
                                Structure1541::Pulse& pulse = gcrTrack->pulses[pulseIndex];
                                unsigned position;

                                if (pulseDelta >= pulse.position)
                                    position = CyclesPerRevolution300Rpm - (pulseDelta - pulse.position);
                                else
                                    position = pulse.position - pulseDelta;

                                structure1541.addPulse(gcrTrack, position, 0xffffffff);

                                if (!written)
                                    written = true;

                                gcrTrack->written |= 1;
                            }
                        }
                        // else
                        // no new flux at this position ... there is already no flux here ... nothing to do
                    } else {

                        Structure1541::Pulse& pulse = gcrTrack->pulses[pulseIndex];

                        if (!writeProtected) {
                            if (flux) {
                                if (pulse.strength != 0xffffffff)
                                    // 1541 always write strong pulses
                                    pulse.strength = 0xffffffff;

                            } else
                                structure1541.freePulse(gcrTrack, pulseIndex);

                            if (!written)
                                written = true;

                            gcrTrack->written |= 1;
                        }

                        pulseIndex = pulse.next;

                        if (pulseIndex >= 0)
                            pulseDelta = gcrTrack->pulses[pulseIndex].position - pulse.position;
                        else {
                            pulseIndex = gcrTrack->firstPulse;

                            pulseDelta = gcrTrack->pulses[pulseIndex].position
                                         + (CyclesPerRevolution300Rpm - pulse.position);
                        }
                    }

                }

                refCycles -= todo;
            } while (refCycles);
        }

#undef OVERFLOW_NOT_THIS_CYCLE
    }

}