
#include "drive1541.h"

namespace LIBC64 {

    auto Drive1541::rotateP64(bool irqNextCycle) -> void {
        unsigned todo;
        bool motorAdvance = motorRun() && loaded;

        uint8_t refCycles = 8;

        if (readMode) {

            while (refCycles) {

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

                pulseDuration += todo;
                if ((pulseDuration == 40) && (uf6aFlipFlop != comperatorFlipFlop)) {
                    uf6aFlipFlop = comperatorFlipFlop;
                    ue7Counter = speedZone & 3;
                    uf4Counter = 0;
                    // after an amount of time without a flux reversal
                    // the rule that a one is shifted in after 3 zeros in a row
                    // is violated by some randomness. means the counter registers
                    // will be reset after some time but that doesn't mean it can
                    // be more than 3 zeros in row shifted in but fewer.
                    randCounter = randomizer.rand(0, 31) + 289; // 18 - 20 micro
                } else {

                    randCounter -= todo;

                    if (!randCounter) {
                        ue7Counter = speedZone & 3;
                        uf4Counter = 0;

                        randCounter = randomizer.rand(0, 367) + 33;  // 2 - 25 micro
                    }
                }

                ue7Counter += todo;
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
                        else {
                            ue3Counter++;
                        }
                        // SO is a edge transition like nmi, don't know when real 1541 reset line.
                        // but it have to keep active at least one whole cpu cycle to be recognized safely.
                        // it's not that important how many time passes exactly because a new trigger can only
                        // happen when off state switches to on. of course it should be happen before next byte
                        // is ready.
                        cpu->setSo(false);
                        // same like SO flag, the VIA input is edge transition
                        via2->ca1In(true, irqNextCycle);
                    }
                    // uf4: 0,1,4,5,8,9,12,13
                } else if (((uf4Counter & 2) == 0) && (ue3Counter == 8)) {

                    ue3Counter = 0;
                    writeBuffer = readBuffer & 0xff;

                    if (byteReadyOverflow) {
                        // cpu low cycle should progress only 6 reference cycles instead of 8.
                        // because SO is detected by cpu till ~400 ns cycle time, otherwise it needs
                        // another cpu cycle to be recognized.
                        // NOTE: cpu code handles recognition and execution time (one cycle later) of v flag change.
                        cpu->setSo(true);
                    }

                    via2->ca1In(!byteReadyOverflow, irqNextCycle);
                }

                if (motorAdvance) {
                    pulseDelta -= todo;

                    if (!pulseDelta) {
                        Structure1541::Pulse& pulse = (*pulseTrack)[pulseIndex];

                        if (++pulseIndex == (*pulseTrack).size()) {
                            pulseIndex = 0;

                            pulseDelta = (*pulseTrack)[pulseIndex].position + (CyclesPerRevolution300Rpm - pulse.position);

                        } else {
                            pulseDelta = (*pulseTrack)[pulseIndex].position - pulse.position;
                        }

                        if ((pulse.strength == 0xffffffff) || (randomizer.rand() < pulse.strength)) {
                            comperatorFlipFlop ^= 1;
                            pulseDuration = 0;
                        }
                    }
                }
                refCycles -= todo;
            }
        } else {
            bool flux;

            while (refCycles) {
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

                    // uf4 is a 4 bit counter.
                    // every 16 ref cycles uf4 is incremented, at least for speedzone 0.
                    // when uf4 == 2 a one is shifted in.
                    // when uf4 == (6 or 10 or 14) a zero is shifted in.
                    // if there is no further flux reversal a one will be shiftd in each 3 zeros.
                    // because of magnetic mediums can not read too much zeros in row reliable.
                    uf4Counter = (uf4Counter + 1) & 0xf;

                    if ((uf4Counter & 3) == 2) {

                        readBuffer = ((readBuffer << 1) & 0x3fe) | (uf4Counter == 2 ? 1 : 0);

                        flux = (writeBuffer & 0x80) != 0;

                        writeBuffer <<= 1;

                        ue3Counter++;

                        // SO is a edge transition like nmi, don't know when real 1541 reset line.
                        // but it have to keep active at least one whole cpu cycle to be recognized safely.
                        // it's not that important how many time passes exactly because a new trigger can only
                        // happen when off state switches to on. of course it should be happen before next byte
                        // is ready.
                        cpu->setSo(false);
                        // same like SO flag, the VIA input is edge transition
                        via2->ca1In(true, irqNextCycle);
                    }
                    // uf4: 0,1,4,5,8,9,12,13
                } else if (((uf4Counter & 2) == 0) && (ue3Counter == 8)) {

                    ue3Counter = 0;
                    writeBuffer = writeValue;

                    if (byteReadyOverflow) {
                        // cpu low cycle should progress only 6 reference cycles instead of 8.
                        // because SO is detected by cpu till ~400 ns cycle time, otherwise it needs
                        // another cpu cycle to be recognized.
                        // NOTE: cpu code handles recognition and execution time (one cycle later) of v flag change.
                        cpu->setSo(true);
                    }

                    via2->ca1In(!byteReadyOverflow, irqNextCycle);
                }

                if (motorAdvance) {
                    pulseDelta -= todo;

                    if (!writeProtected) {
                        if (!written) {
                            written = true;
                        }
                        gcrTrack->written = true;

                        if (pulseDelta) {

                            if (flux) {
                                if (pulseDelta >= (*pulseTrack)[pulseIndex].position) {
                                    (*pulseTrack).push_back(
                                            {CyclesPerRevolution300Rpm -
                                             (pulseDelta - (*pulseTrack)[pulseIndex].position),
                                             0xffffffff});

                                    pulseIndex = 0;
                                } else {

                                    (*pulseTrack).insert((*pulseTrack).begin() + pulseIndex,
                                    {(*pulseTrack)[pulseIndex].position - pulseDelta, 0xffffffff});

                                    pulseIndex++;
                                }
                            }
                            // else
                            // no new flux at this position ... there is already no flux here ... nothing to do
                        } else {

                            unsigned position = (*pulseTrack)[pulseIndex].position;

                            if (flux) {
                                if ((*pulseTrack)[pulseIndex].strength != 0xffffffff)
                                    // 1541 always write strong pulses
                                    (*pulseTrack)[pulseIndex].strength = 0xffffffff;

                                pulseIndex++;
                            } else
                                (*pulseTrack).erase((*pulseTrack).begin() + pulseIndex);

                            if (pulseIndex == (*pulseTrack).size()) {
                                pulseIndex = 0;

                                pulseDelta = (*pulseTrack)[pulseIndex].position + (CyclesPerRevolution300Rpm - position);

                            } else {
                                pulseDelta = (*pulseTrack)[pulseIndex].position - position;
                            }
                        }
                    }
                }

                refCycles -= todo;
            }
        }
    }

}