

#include "drive1541.h"

namespace LIBC64 {   
    
auto Drive1541::serialize(Emulator::Serializer& s) -> void {

    s.integer( (uint8_t&)type );
    s.integer( cycleCounter );
    s.integer( synced );
    s.integer( irqIncomming );
    s.array( ram, 2 * 1024 );
    s.integer( driveCycles );
    s.integer( accum );
    s.integer( currentHalftrack );
    s.integer( speedZone );
    s.integer( byteReadyOverflow );
    s.integer( readMode );
    s.integer( headOffset );
    s.integer( stepDirection );
    s.integer( ue3Counter );
    s.integer( refCyclesPerRevolution );
    s.integer( uf6aFlipFlop );
    s.integer( comperatorFlipFlop );
    s.integer( ue7Counter );
    s.integer( uf4Counter );
    s.integer( randomizer.xorShift32 );
    s.integer( randCounter );
    s.integer( writeValue );
    s.integer( readBuffer );
    s.integer( writeBuffer );
    s.integer( attachDelay );
    s.integer( detachDelay );
    s.integer( attachDetachDelay );
    s.integer( motorOn );
    s.integer( written );
    s.integer( loaded );
    s.integer( clockOut );
    s.integer( dataOut );
    s.integer( atnOut );
    s.integer( motorOff.slowDown );
    s.integer( motorOff.decelerationPoint );
    s.integer( motorOff.delay );
    s.integer( motorOff.pos );
    s.vector( motorOff.chunkSize );
    s.integer( writeProtected );
    s.integer( pulseIndex );
    s.integer( pulseDelta );
    s.integer( pulseDuration );
    s.integer( latchedByte );
    s.integer( byteReady );
    s.integer( dataDirection );
    s.integer( frequency );
    s.integer( side );
    s.integer( operation );

    via1->serialize( s );
    via2->serialize( s );
    cpu->serialize( s );

    s.integer( structure1541.encodingGraceful.status );

    if ((type == Type::D1570) || (type == Type::D1571)) {
        cia->serialize(s);
    }
    
    if (s.mode() == Emulator::Serializer::Mode::Load) {

        updateCycleSpeed( ((type == Type::D1570) || (type == Type::D1571)) && (via1->lines.ioa & 0x20) );

        setFirmwareByType();
        gcrTrack = structure1541.getTrackPtr( side, currentHalftrack );
        // unserialize VIA before to get state of LED
        updateDeviceState();

        if (structure1541.encodingGraceful.status)
            // state was generated during attaching P64 (gracefully)
            postAttach();

        structure1541.encodingGraceful.reset();
    }
       
    structure1541.serialize( s, written );    
}

}