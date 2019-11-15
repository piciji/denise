
#include "via.h"

namespace LIBC64 {
    
auto Via::serialize(Emulator::Serializer& s) -> void {
    
    s.integer( lines.pra );
    s.integer( lines.prb );
    s.integer( lines.ddra );
    s.integer( lines.ddrb );
    s.integer( lines.ioa );
    s.integer( lines.ioaOld );
    s.integer( lines.iob );
    s.integer( lines.iobOld );
    s.integer( lines.latchA );
    s.integer( lines.latchB );
    s.integer( registerWrite.pipelined );
    s.integer( registerWrite.addr );
    s.integer( registerWrite.value );
    
    for( unsigned i = 0; i < 2; i++ ) {
        Timer& t = timer[i];
        
        s.integer( t.forceloadCycle );
        s.integer( t.counterUpdated );
        s.integer( t.latch );
        s.integer( t.counter );
        s.integer( t.toggle );
        s.integer( t.trigger );
        s.integer( t.step );
    }
    
    s.integer( ifr );
    s.integer( ier );
    s.integer( pcr );
    s.integer( acr );
    s.integer( sdr );
    s.integer( ca1 );
    s.integer( ca2 );
    s.integer( cb1 );
    s.integer( cb2 );

    s.integer( shift.warmUp );
    s.integer( shift.toggle );
    s.integer( shift.irqTrigger );
    s.integer( shift.active );
    s.integer( shift.count );

    s.integer( ca2StatePulse );
    s.integer( cb2StatePulse );
    s.integer( updateIrq );
    s.integer( isShiftT2Control );
}
    
}