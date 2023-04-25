
#include "drive.h"

namespace LIBC64 {   
    
auto Drive::rotateD64() -> void {
    
    if (!motorRun())
        return;

    // the images were created in standard cbm dos format.
    // for cbm dos we know the amount of bits passed the
    // read/write head every second according to the speedzone.
    
    // 1.000.000 cpu cycles = bps ( bits per second for current speedzone )
    // 1 cpu cycle          = x bits
    // x bits for one cpu cycle = bps / 1.000.000 drive cpu cycles
    
    accum += rotSpeedBps[speedZone];
    
    if (accum < driveCycles)
        return;
    // one bit has moved
    accum -= driveCycles;
    
    uint8_t byte;
    uint8_t* trackPtr = gcrTrack->data;
    
    if (readMode) {
                
        if ( !loaded || !trackPtr ) 
            // no image loaded or track not present
            byte = 0;
        else
            // headOffset is the bit position within a track.            
            // we move the actual bit to the most significant bit.
            byte = trackPtr[ headOffset >> 3 ] << (headOffset & 7);
            
        headOffset++;
        // move next bit to msb.
        byte <<= 1;

        if  ( !( headOffset & 7 ) ) {

            if ( (headOffset >> 3) >= gcrTrack->size) {
                // revolution complete ... wrap around
                headOffset = 0;
            }
            // fetch next byte
            byte = (!loaded || !trackPtr) ? 0 : trackPtr[ headOffset >> 3 ];
        }

        // make room for incomming bit
        readBuffer <<= 1;
        writeBuffer <<= 1;
        // append incomming bit
        readBuffer |= (byte >> 7) & 1;
        readBuffer &= 0x3ff; // 10 bit buffer

        if ((readBuffer & 0xf) == 0)
            // when there are more than three zeros in a row, a one will be injected by drive mechanic
            readBuffer |= 1;

        // if last 10 bits in a row are non zero then there is a sync.
        // in this case data is not moving.
        if (~readBuffer & 0x3ff) {
            // no sync 
            if (++ue3Counter == 8)
                byteFetched( false );
            else if (!ca1Line)
                via2.ca1In( ca1Line = true );

        } else {
            ue3Counter = 0; //reset when sync mark detected
            if (!ca1Line)
                via2.ca1In( ca1Line = true );
        }
        
    } else {
        // because of shared bus
        readBuffer = (readBuffer << 1) & 0x3fe;

        if ((readBuffer & 0xf) == 0)
            readBuffer |= 1;

        writeBit( (writeBuffer & 0x80) != 0 );
        writeBuffer <<= 1;

        if (++ue3Counter == 8) {
            ue3Counter = 0;

            writeBuffer = writeValue; // fetch next byte to buffer

            if ( byteReadyOverflow ) {
                cpu.triggerSO();
                byteReady = true;
                via2.ca1In( ca1Line = false );
            }
        } else if (!ca1Line)
            via2.ca1In( ca1Line = true );
    }
}

auto Drive::byteFetched( bool overflowNotThisCycle ) -> void {

    ue3Counter = 0;
    latchedByte = writeBuffer = readBuffer & 0xff;

    if (byteReadyOverflow) {
        // edge transition
        cpu.triggerSO(overflowNotThisCycle ? 2 : 1);
        byteReady = true;
        via2.ca1In(ca1Line = false, overflowNotThisCycle);
    }
}

inline auto Drive::readBit() -> bool {
    uint8_t* trackPtr = gcrTrack->data;
    
    if (!loaded)
        return 0;

    unsigned byte = headOffset >> 3;
    uint8_t bit = (~headOffset) & 7; // msb is next
    
    headOffset++;

    if ( headOffset >= gcrTrack->bits )
        headOffset = 0; // wrap around the ring buffer 
    
    if (!trackPtr)
        return 0;
    
    return (trackPtr[byte] >> bit) & 1;
}

inline auto Drive::writeBit( bool state ) -> void {
    uint8_t* trackPtr = gcrTrack->data;
    
    if (!loaded)
        return;
    
    unsigned byte = headOffset >> 3;
    uint8_t bit = (~headOffset) & 7; // msb is next
    
    headOffset++;

    if ( headOffset >= gcrTrack->bits )
        headOffset = 0; // wrap around the ring buffer 
    
    if (!trackPtr || writeProtected)
        return;
        
    if (state)
        trackPtr[byte] |= 1 << bit;
    else
        trackPtr[byte] &= ~(1 << bit);    
    
    if (!written)
        written = true;
    
    gcrTrack->written = 1; // track data has changed, host have to write back
}

auto Drive::motorRun() -> bool {

    if (motorOn)
        return true;

    if (!motorOff.slowDown)
        return false;

    if (motorOff.delay) {
        motorOff.delay--;
        return true;
    }

    // Star Trekking game needs emulation of motor slow down
    unsigned decelerationPoint = motorOff.decelerationPoint;
    if (motorOff.chunkSize[decelerationPoint])
        motorOff.chunkSize[decelerationPoint]--;

    if (motorOff.chunkSize[decelerationPoint] == 0) {
        if (motorOff.decelerationPoint)
            motorOff.decelerationPoint--;
        else {
            motorOff.slowDown = false;
            return false;
        }
    }

    if (motorOff.pos++ <= decelerationPoint)
        return true;

    if (motorOff.pos == (motorOff.CHUNKS + 1) )
        motorOff.pos = 0;

    return false;
}

auto Drive::motorOffInit() -> void {

    motorOff.delay = 50000 + (rand() % 1000);
    unsigned slowDownCycles = 50000;

    if (use2Mhz()) {
        motorOff.delay <<= 1;
        slowDownCycles <<= 1;
    }

    unsigned chunkSize = slowDownCycles / motorOff.CHUNKS;
    unsigned rest = slowDownCycles % motorOff.CHUNKS;

    for(unsigned i = 0; i < motorOff.CHUNKS; i++)
        motorOff.chunkSize[i] = chunkSize;

    for(unsigned i = 0; i < rest; i++)
        motorOff.chunkSize[i % motorOff.CHUNKS]++;

    motorOff.decelerationPoint = motorOff.CHUNKS - 1;
    motorOff.pos = 0;
    motorOff.slowDown = true;
}

auto Drive::randomizeRpm() -> void {
    
    // drive speed is 300 rounds per minute
    // more realistic speed wobbles between 299,75 - 300,25
    // so we could generate a random number in a range of 0.5
    // generating random integer numbers is easier, lets scale up
    // 0.5 rpm * 100 = 50
    // 300 rpm * 100 = 30000
    unsigned adjusted = rpm + (rand() % (wobble + 1) ) - (wobble / 2);
    // there are fixed values how many bits passed the r/w head each second within a speed zone.
    // however these values are valid for a rotation speed of exactly 300 rpm
    // we solve this by a simple proportion:
    // when
    // adjusted = 1000000 cpu cycles
    // then
    // 30000 = drive cycles per second
    // drive cycles per second = 30000 * 1000000 / adjusted
    driveCycles = (30000ULL * frequency) / adjusted;
    // so we get the amount of cycles per second for adjusted motor speed.
    // now we could calculate the amount of bits passed for any amount of cpu drive cycles
    // by following proportion:
    // bits per speedzone [bps] = drive cycles per second
    // bits passed              = cpu cycles passed
    
    // bits passed = bits per speedzone * cpu cycles passed / drive cycles per second
    
    // for g64 rotation, we apply the randomness for drive speed on reference cycles
    refCyclesPerRevolution = (30000ULL * CyclesPerRevolution300Rpm) / adjusted;
}

auto Drive::updateStepper( uint8_t step ) -> bool {
    
    if (step == 1) {        
        if (currentHalftrack < ((MAX_TRACKS_1541 * 2) - 1) ) {
            currentHalftrack++;
            coilDir = 1;
            return true;            
        }
            
        coilDir = 0;

    } else if (step == 3) {
        
        if (currentHalftrack > 0) {
            currentHalftrack--;
            coilDir = 0;
            return true;
        }
            
        coilDir = 1;
        
    } else if (step == 2) {
        // Primitive 7 Sins uses this method
        if (coilDir) {
            if (currentHalftrack & 1) {
                if (updateStepper(1))
                    return updateStepper(1);
            }

        } else {
            if ((currentHalftrack & 1) == 0) {
                if (updateStepper(3))
                    return updateStepper(3);
            }
        }
    }
    
    return false;
}

auto Drive::changeHalfTrack( uint8_t step ) -> void {

    if (step != 0) {
        bool headBang = (currentHalftrack == 0) && ((step == 3) || ( (step == 2) && !coilDir));

        updateStepper(step);

        if (system->driveSounds.useFloppy)
            stepSound( headBang );
    }

    if (operation & FLUXDATA_LEVEL) {

        unsigned position = 0;

        if (pulseIndex >= 0) {
            DiskStructure::Pulse& pulse = gcrTrack->pulses[pulseIndex];

            if (pulse.position > pulseDelta)
                position = pulse.position - pulseDelta;
            else
                position = CyclesPerRevolution300Rpm - (pulseDelta - pulse.position);
        }

        gcrTrack = structure.getTrackPtr( side, currentHalftrack );
        pulseIndex = gcrTrack->firstPulse;
        pulseDelta = 1;

        while ((pulseIndex >= 0) && (gcrTrack->pulses[pulseIndex].position <= position))
            pulseIndex = gcrTrack->pulses[pulseIndex].next;

        if (pulseIndex >= 0)
            pulseDelta = gcrTrack->pulses[pulseIndex].position - position;
        else {
            pulseIndex = gcrTrack->firstPulse;
            if (pulseIndex >= 0)
                pulseDelta = (CyclesPerRevolution300Rpm - position) + gcrTrack->pulses[pulseIndex].position;
        }

        wd1770.setPulseIndex(pulseIndex, pulseDelta);

    } else {    // D64, G64
        unsigned oldTrackSize = gcrTrack->size;

        // pointer to next track
        gcrTrack = structure.getTrackPtr( side, currentHalftrack );

        if ( oldTrackSize != 0 )
            // we want to keep alignment between old and new track.
            // head offset doesn't change if both tracks have same size, otherwise we use a simple proportion
            // old head offset = new head offset
            // old size = new size
            // new head offset = old head * new size / old size
            // i heard of games which rely on correct alignment of track data.
            headOffset = ( headOffset * gcrTrack->size ) / oldTrackSize;

         else
            headOffset = 0;
    }

    if ( (type == Type::D1570) && (side == 1) ) {
        gcrTrack = dummyTrack;
        wd1770.setTrack( dummyTrack, true );
    } else
        wd1770.setTrack(gcrTrack);

    updateDeviceState( );

}

auto Drive::stepSound(bool headBang) -> void {

    DriveSound sound = headBang ? DriveSound::FloppyHeadBang : DriveSound::FloppyStep;

    system->interface->mixDriveSound( mediaConnected, sound, currentHalftrack );
}

inline auto Drive::syncFound() -> uint8_t {
    
    if (!readMode || attachDelay )
        return 0x80;
    
    return readBuffer == 0x3ff ? 0 : 0x80;
}

}

