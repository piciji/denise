
#include "drive1541.h"

namespace LIBC64 {   
    
auto Drive1541::rotateD64() -> void {    
    
    if (!motorRun())
        return;

    // d64 images are cracked versions of originals, the copy protection is removed
    // or not checked. the images were recreated in standard cbm dos format.
    // d64 contains user data only. there are no disk structure data like sync bits.
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
            // when there are more than three zeros in a row, a one will be injected by
            // drive mechanic
            readBuffer |= 1;

        // if last 10 bits in a row are non zero then there is a sync.
        // in this case data is not moving.
        if (~readBuffer & 0x3ff) {
            // no sync 
            if (++bitCounter == 8) {
                bitCounter = 0;   

                writeBuffer = readBuffer & 0xff; // because of shared bus

                if (byteReadyOverflow)
                    cpu->setSo( 1 );

                via2->ca1In( !byteReadyOverflow );

            } else {
                cpu->setSo( 0 );
                via2->ca1In( true );
            }
        } else
            bitCounter = 0; //reset when sync mark detected 
        
    } else {
        // because of shared bus
        readBuffer = (readBuffer << 1) & 0x3fe;

        if ((readBuffer & 0xf) == 0)
            readBuffer |= 1;

        writeBit( (writeBuffer & 0x80) != 0 );
        writeBuffer <<= 1;

        if (++bitCounter == 8) {
            bitCounter = 0;

            writeBuffer = writeValue; // fetch next byte to buffer

            if ( byteReadyOverflow ) 
                cpu->setSo( 1 );

            via2->ca1In( !byteReadyOverflow );
        } else {
            cpu->setSo( 0 );                    
            via2->ca1In( true );
        }
    }
}    
    
auto Drive1541::rotateG64( unsigned refCycles ) -> void {   

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
    
    // we know the amount of bit cells for any track and the amount of ref cycles for a
    // complete revolution. ref cycles / bit cells = ref cycles for a single bit cell.
    // the fraction of the division would be a problem, so we scale each ref cycle up
    // by the amount of bit cells for the current track. that way we sum up the scaled
    // ref cycles and compare this value with the total amount of ref cycles per revolution.
    // if exceeded we reach a new bit cell. 
    
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
                    
                    writeBuffer <<= 1;
                    
                    if ( readBuffer == 0x3ff )
                        bitCounter = 0;
                        
                    else {
                        
                        if (++bitCounter == 8) {
                            bitCounter = 0;
                            writeBuffer = readBuffer & 0xff;

                            if (byteReadyOverflow) {
                                // cpu low cycle should progress only 6 reference cycles instead of 8.
                                // because SO is detected by cpu at ~400 ns cycle time, otherwise it needs
                                // another cpu cycle to be recognized. 
                                // NOTE: cpu code handles recognition and execution time of v flag change.
                                cpu->setSo( true );
                            }
                            via2->ca1In( !byteReadyOverflow );
                        } else {
                            // SO is a edge transition like nmi, don't know when real 1541 reset line.
                            // but it have to keep active at least one whole cpu cycle to be recognized safely.
                            // it's not that important how many time passes exactly because a new trigger can only
                            // happen when off state switches to on. of course it should be happen before next byte
                            // is ready.
                            cpu->setSo( false );  
                            // same like Cpu SO flag the via input is edge transition
                            via2->ca1In( true );
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

                    if (++bitCounter == 8) {
                        bitCounter = 0;

                        writeBuffer = writeValue; // fetch next byte to buffer

                        if (byteReadyOverflow)
                            cpu->setSo(true);

                        via2->ca1In(!byteReadyOverflow);
                    } else {                       
                        cpu->setSo(false);                    
                        via2->ca1In( true );
                    }
                }
            }
        }
    }
}

inline auto Drive1541::calculateRefCyclesPerRevolution() -> void {
    
    // 1541 drive runs with 16 MHz, called reference cycles.
    // clock is divided by 16 for cpu, means 1.000.000 cpu cycles per second.
    // drive speed is 300 rounds per minute, means 300 / 60 = 5 rounds per second.
    // one revolution has 16.000.000 / 5 reference cycles
    
    refCyclesPerRevolution300rpm = 16000000 / (300 / 60);
}
    
inline auto Drive1541::readBit() -> bool {
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

inline auto Drive1541::writeBit( bool state ) -> void {
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
        system->serializationSize += structure1541.getStateImageSize();
    
    gcrTrack->written = true; // track data has changed, host have to write back    
    
    written = true;
}

auto Drive1541::motorRun() -> bool {

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

auto Drive1541::motorOffInit() -> void {

    motorOff.delay = 30000 + (rand() % 10000);
    unsigned slowDownCycles = 100000;

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

auto Drive1541::randomizeRpm() -> void {
    
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
    driveCycles = (30000ULL * 1000000ULL) / adjusted;    
    // so we get the amount of cycles per second for adjusted motor speed.
    // now we could calculate the amount of bits passed for any amount of cpu drive cycles
    // by following proportion:
    // bits per speedzone [bps] = drive cycles per second
    // bits passed              = cpu cycles passed
    
    // bits passed = bits per speedzone * cpu cycles passed / drive cycles per second
    
    // for g64 rotation, we apply the randomness for drive speed on reference cycles
    refCyclesPerRevolution = (30000ULL * refCyclesPerRevolution300rpm) / adjusted;    
}

// 1 - 0 = 1
// 2 - 1 = 1
// 3 - 2 = 1
// 0 - 3 = 1
// 1 - 0 = 1
// 2 - 1 = 1
// 1 - 2 = (-1) 3
// 0 - 1 = (-1) 3
// 3 - 0 = 3
// 2 - 3 = (-1) 3

auto Drive1541::updateStepper( uint8_t step ) -> bool {
    
    if (step == 1) {        
        if (currentHalftrack < ((MAX_TRACKS_1541 * 2) - 1) ) {
            currentHalftrack++;
            stepDirection = 1;
            return true;            
        }
            
        stepDirection = -1;
        
    } else if (step == 3) {
        
        if (currentHalftrack > 0) {
            currentHalftrack--;
            stepDirection = -1;
            return true;
        }
            
        stepDirection = 1;
        
    } else if (step == 2) {
        // Primitive 7 Sins uses this method
        if (stepDirection == 1) {            
            if (currentHalftrack & 1) {
                if (updateStepper(1))
                    return updateStepper(1);
            }
            
        } else if (stepDirection == -1) {
            if ((currentHalftrack & 1) == 0) {
                if (updateStepper(3))
                    return updateStepper(3);
            }
        } 
    }
    
    return false;
}

auto Drive1541::changeHalfTrack( uint8_t step ) -> void {
                    
    updateStepper( step );
       
    unsigned oldTrackSize = gcrTrack->size;

    // pointer to next track
    gcrTrack = structure1541.getTrackPtr( currentHalftrack );    
    
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
    
    updateState( );    
}

inline auto Drive1541::syncFound() -> uint8_t {
    
    if (!readMode || attachDelay )
        return 0x80;
    
    return readBuffer == 0x3ff ? 0 : 0x80;
}

}
