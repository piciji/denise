
#include "tape.h"
#include "../../tools/serializer.h"

namespace LIBC64 {
    
auto Tape::serialize(Emulator::Serializer& s, bool light) -> void {
    
    s.integer( enabled );    

    if (!enabled)
        return;
    
    s.array( fetchData, TAPE_FETCH_SIZE );
    s.array( writeData, TAPE_WRITE_SIZE );
    s.integer( (uint8_t&)mode );
    s.integer( (uint8_t&)nextMode );
    s.integer( writePos );
    s.integer( writeBit );
    s.integer( writeClock );
    s.integer( writeCounterClock );
    s.integer( cycles );
    s.integer( cycles999 );
    s.integer( cylcesPerSecond );
    s.integer( cyclesTotal );
    s.integer( gapsRemaining );
    s.integer( counter );
    s.integer( counterOffset );
    s.integer( motorIn );    
    s.integer( directionForward );
    s.integer( lastDirectionForward );
    s.integer( version );
    s.integer( fetchPos );
    s.integer( fetchSize );
    s.integer( curPos );
    s.integer( writeProtect );
    s.integer( writeQuestionState );
    s.integer( autoStarted );
    s.integer( wobble );

    if (light)
        return;

    if (s.mode() == Emulator::Serializer::Mode::Load) {
        updateDeviceState();
    }
}


}
