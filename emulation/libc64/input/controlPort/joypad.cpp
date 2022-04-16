
#include "controlPort.h"

namespace LIBC64 {
    
struct Joypad : ControlPort {
    
    Joypad( Interface::Device* device ) : ControlPort( device ) {}
    
    auto read( ) -> uint8_t { 
        
        uint8_t out = 0xff;
        
        for( unsigned i = 0; i < 5; i++ )
            out &= ~((system->interface->inputPoll( device->id, i ) & 1) << i);
        
        return out;
    }

    auto getPotX() -> uint8_t {
        return (system->interface->inputPoll( device->id, 5 ) & 1) ? 0 : 0xff;
    }

    auto useJitPolling() -> bool {
        return true;
    }
};

}