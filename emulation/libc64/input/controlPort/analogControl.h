
#pragma once

#include "controlPort.h"

namespace LIBC64 {
    
struct AnalogControl : ControlPort {
    
    AnalogControl( Interface::Device* device ) : ControlPort( device ) {}    
    
    int16_t posX;
    int16_t posY;
    
    auto poll( ) -> void {
    // driver reported deltas will be added one time after each single global input polling in vsync
        posX += system->interface->inputPoll( device->id, 0);
        posY -= system->interface->inputPoll( device->id, 1);
    }   
    
    virtual auto reset() -> void {
        posX = 0;
        posY = 0;        
    }
    
    virtual auto serialize(Emulator::Serializer& s) -> void {
        
        s.integer( posX );
        s.integer( posY );
    }
    
    auto useJitPolling() -> bool {
        return false;
    }
};

}