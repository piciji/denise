
#include "drive.h"

namespace LIBC64 {   
    
auto Drive::rotateD64() -> void {
    static unsigned _driveCycles;

    if (!mechanics.motorDelay) {
        if (!motorOn)
            return;
        _driveCycles = driveCycles;
    } else {
        _driveCycles = driveCycles;
        if(!motorRun(_driveCycles))
            return;
    }

    accum += rotSpeedBps[speedZone];
    
    if (accum < _driveCycles)
        return;
    // one bit has moved
    accum -= _driveCycles;
    
    uint8_t byte;
    uint8_t* trackPtr = gcrTrack->data;
    
    if (readMode) {
        if ( !loaded || !trackPtr )
            byte = 0;
        else
            byte = trackPtr[ headOffset >> 3 ] << (headOffset & 7);
            
        headOffset++;
        byte <<= 1; // move next bit to msb.

        if  ( !( headOffset & 7 ) ) {

            if ( (headOffset >> 3) >= gcrTrack->size)
                headOffset = 0;

            byte = (!loaded || !trackPtr) ? 0 : trackPtr[ headOffset >> 3 ];
        }

        readBuffer <<= 1;
        writeBuffer <<= 1;

        readBuffer |= (byte >> 7) & 1;
        readBuffer &= 0x3ff; // 10 bit buffer

        if ((readBuffer & 0xf) == 0)
            readBuffer |= 1; // simulate oscillation

        if (~readBuffer & 0x3ff) {
            // no sync 
            if (++ue3Counter == 8)
                byteFetched( false );
            else if (!ca1Line)
                via2.ca1In( ca1Line = true );

        } else {
            ue3Counter = 0;
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
            byteWritten(false);

        } else if (!ca1Line)
            via2.ca1In( ca1Line = true );

        // switch to more accurate G64 handling: SpeedAlignTest_Equalizer.d64
        operation &= ~DECODEDDATA_LEVEL;
        operation |= ENCODEDDATA_LEVEL;
    }
}

auto Drive::byteWritten( bool overflowNotThisCycle ) -> void {
    ue3Counter = 0;
    writeBuffer = writeValue;

    if (operation & DRIVE_MODE_154x) {
        if (byteReadyOverflow) {
            // edge transition
            cpu.triggerSO(overflowNotThisCycle ? 2 : 1);
            via2.ca1In(ca1Line = false, overflowNotThisCycle);
        }
    } else {
        if (byteReadyOverflow)
            cpu.triggerSO(overflowNotThisCycle ? 2 : 1);

        byteReady = true;
        via2.ca1In(ca1Line = false, overflowNotThisCycle);
    }
}

auto Drive::byteFetched( bool overflowNotThisCycle ) -> void {
    ue3Counter = 0;
    latchedByte = writeBuffer = readBuffer & 0xff;

    if (operation & DRIVE_MODE_154x) {
        if (byteReadyOverflow) {
            // edge transition
            cpu.triggerSO(overflowNotThisCycle ? 2 : 1);
            // true for 1540 and 1541 (long board) or 1541C (long board)...there are 1541C with short board
            // todo: I guess 1541-II behaves like 1571, means ca1In don't depends on "byteReadyOverflow"
            via2.ca1In(ca1Line = false, overflowNotThisCycle);
        }
    } else {
        if (byteReadyOverflow)
            cpu.triggerSO(overflowNotThisCycle ? 2 : 1);

        byteReady = true; // only needed for 1570/1571
        via2.ca1In(ca1Line = false, overflowNotThisCycle);
    }
}

inline auto Drive::readBit() -> bool {
    uint8_t* trackPtr = gcrTrack->data;
    
//    if (!loaded)
  //      return 0;

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

auto Drive::motorRun(unsigned& refCycles) -> bool {
    unsigned fallBackCycles = MaxAccDecMotorTime - --mechanics.motorDelay;

    if (fallBackCycles & 0xff) {
        refCycles = mechanics.refCycles;
        return true;
    }

    if (use2Mhz())
        fallBackCycles >>= 1;

    float timeScaled = (float)fallBackCycles / 256.0f;
    float slopeFactor;
    float rpmFactor;

    if (motorOn) {
        slopeFactor = (float)(mechanics.acceleration) / 65536.0f;
        rpmFactor = -powf(0.40f, timeScaled * slopeFactor) + 1.0f;

        if (rpmFactor >= 0.999) {
           // system->interface->log("fullspeed");
           // system->interface->log(fallBackCycles / 1000, 0);
            mechanics.motorDelay = 0;
        }

    } else {
        slopeFactor = (float)(mechanics.deceleration) / 65536.0f;
        rpmFactor = powf(0.40f, timeScaled * slopeFactor);

        if (rpmFactor <= 0.001) {
           // system->interface->log("stopped");
           // system->interface->log(fallBackCycles / 1000, 0);
            mechanics.motorDelay = 0;
            return false;
        }
    }

    refCycles = (unsigned)( (float)refCycles * rpmFactor );

    mechanics.refCycles = refCycles; // to speed things up use this for next 255 cycles

    return true;
}

auto Drive::motorChangeInit() -> void {
    //system->interface->log(motorOn ? "Motor ON" : "Motor OFF");

    if (mechanics.enabled) {
        if ((motorOn && mechanics.acceleration) || (!motorOn && mechanics.deceleration)) {
            mechanics.motorDelay = MaxAccDecMotorTime - mechanics.motorDelay;
            mechanics.refCycles = (operation & DECODEDDATA_LEVEL) ? driveCycles : refCyclesPerRevolution;
        } else
            mechanics.motorDelay = 0;
    } else
        mechanics.motorDelay = 0;
}

auto Drive::randomizeRpm(std::vector<Drive*>& drivesEnabled) -> void {
    unsigned adjustedRpm = rpm;

    if (wobble) { // todo: How does the wobble really behave? sine wave or more random ?
        if (wobbleLimit < 0) { // neg
            if (--wobblePos < wobbleLimit)
                wobbleLimit = wobble >> 1;
        } else {
            if (++wobblePos > wobbleLimit)
                wobbleLimit = -(int)(wobble >> 1);
        }
        adjustedRpm += wobblePos;
    }

    for (auto drive : drivesEnabled)
        drive->driveCycles = (30000ULL * drive->frequency) / adjustedRpm;

    refCyclesPerRevolution = (30000ULL * CyclesPerRevolution300Rpm) / adjustedRpm;
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
        unsigned oldTrackSize = gcrTrack ? gcrTrack->size : 0;

        // pointer to next track
        gcrTrack = structure.getTrackPtr( side, currentHalftrack );

        if ( oldTrackSize != 0 )
            // we want to keep alignment between old and new track.
            // head offset doesn't change if both tracks have same size, otherwise we use a simple proportion
            // old head offset = new head offset
            // old size = new size
            // new head offset = old head * new size / old size
            headOffset = ( headOffset * gcrTrack->size ) / oldTrackSize;

         else
            headOffset = 0;
    }

    wd1770.setTrack(gcrTrack);

    (operation & DRIVE_MODE_158x) ? updateDeviceState1581() : updateDeviceState();
}

auto Drive::stepSound(bool headBang) -> void {

    DriveSound sound = headBang ? DriveSound::FloppyHeadBang : DriveSound::FloppyStep;

    system->interface->mixDriveSound( media, sound, false, currentHalftrack );
}

inline auto Drive::syncFound() -> uint8_t {
    
    if (!readMode || attachDelay )
        return 0x80;
    
    return readBuffer == 0x3ff ? 0 : 0x80;
}

}
