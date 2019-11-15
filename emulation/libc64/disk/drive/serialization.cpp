

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
    s.integer( bitCounter );
    s.integer( refCyclesPerRevolution300rpm );
    s.integer( refCyclesPerRevolution );
    s.integer( filter );
    s.integer( lastFilter );
    s.integer( ue7Counter );
    s.integer( uf4Counter );
    s.integer( randomizer.xorShift32 );
    s.integer( randCounter );
    s.integer( alternateRefTiming );
    s.integer( readValue );
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
    
    if (s.mode() == Emulator::Serializer::Mode::Load) {
        gcrTrack = structure1541.getTrackPtr( currentHalftrack );
        updateState();
    }
    
    via1->serialize( s );
    via2->serialize( s );
    cpu->serialize( s );
    
    System::serialize6502( s, cpuCtx ); 
}

}