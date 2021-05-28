
// much more readable but unoptimized reference implementation (not part of compilation)
// for regressions i will temporary swap this in

auto Drive1541::rotateP64( ) -> void {

    bool motorAdvance = motorRun() && loaded;

    uint8_t refCycles = 16;

    if (readMode) {
        while ( refCycles-- ) {

            if ((++pulseDuration == 40) && (uf6aFlipFlop != comperatorFlipFlop)) {
                uf6aFlipFlop = comperatorFlipFlop;
                ue7Counter = speedZone & 3;
                uf4Counter = 0;

                if (ue3Counter == 8)
                    byteFetched( (16 - refCycles + todo) > 8 );

                // after an amount of time without a flux reversal
                // the rule that a one is shifted in after 3 zeros in a row
                // is violated by some randomness. means the counter registers
                // will be reset after some time but that doesn't mean it can
                // be more than 3 zeros in row shifted in but fewer.
                randCounter = randomizer.rand(0, 31) + 289; // 18 - 20 micro
            } else {

                if (randCounter)
                    randCounter--;

                if (!randCounter) {
                    ue7Counter = speedZone & 3;
                    uf4Counter = 0;

                    if (ue3Counter == 8)
                        byteFetched( (16 - refCycles + todo) > 8 );

                    randCounter = randomizer.rand(0, 367) + 33;  // 2 - 25 micro
                }
            }

            if (++ue7Counter == 16) {

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

                    // same like SO flag, the VIA input is edge transition
                    via2->ca1In(true, false);
                }
                    // uf4: 0,1,4,5,8,9,12,13
                else if (((uf4Counter & 2) == 0) && (ue3Counter == 8))
                    byteFetched( (16 - refCycles + todo) > 8 );
            }

            if (motorAdvance) {
                pulseDelta--;

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
        }
    }
}