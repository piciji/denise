
#include "tape.h"
#include "../../tools/serializer.h"

namespace LIBC64 {
    
auto Tape::serialize(Emulator::Serializer& s, bool light) -> void {
    
    s.integer( enabled );    

    if (!enabled)
        return;

    s.integer( (uint8_t&)mode );
    s.integer( (uint8_t&)nextMode );
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
    s.integer( curPos );
    s.integer( writeProtect );
    s.integer( writeQuestionState );
    s.integer( autoStarted );
    s.integer( wobble );

    if (s.mode() == Emulator::Serializer::Mode::Load) {
        writePos = 0;
        if (!light)
            updateDeviceState(true);
    } else if (writePos)
        writeBuffer();
}


}
