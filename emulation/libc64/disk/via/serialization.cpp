
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
    
    s.integer( timerACounter );
    s.integer( timerALatch );
    s.integer( timerACounterRead );
    s.integer( timerATrigger );
    s.integer( timerAToggle );
    s.integer( timerBCounter );
    s.integer( timerBLatch );
    s.integer( timerBCounterRead );
    s.integer( timerBTrigger );    
    
    s.integer( ifr );
    s.integer( ier );
    s.integer( pcr );
    s.integer( acr );
    s.integer( sdr );
    s.integer( ca1 );
    s.integer( ca2 );
    s.integer( cb1 );
    s.integer( cb2 );

    s.integer( shift.toggle );
    s.integer( shift.irqTrigger );
    s.integer( shift.active );
    s.integer( shift.count );

    s.integer( isShiftT2Control );
    s.integer( delay );
}
    
}