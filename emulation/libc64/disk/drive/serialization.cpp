

#include "drive1541.h"

namespace LIBC64 {   
    
auto Drive1541::serialize(Emulator::Serializer& s) -> void {
    
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
    s.integer( bitCounter );
    s.integer( refCyclesPerRevolution300rpm );
    s.integer( refCyclesPerRevolution );
    s.integer( filter );
    s.integer( lastFilter );
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
    
    if (s.mode() == Emulator::Serializer::Mode::Load) {
        gcrTrack = structure1541.getTrackPtr( currentHalftrack );
        updateDeviceState();
    }
       
    structure1541.serialize( s, written );
    
    via1->serialize( s );
    via2->serialize( s );
    cpu->serialize( s );    
}

}