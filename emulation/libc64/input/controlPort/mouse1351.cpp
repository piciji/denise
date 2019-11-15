
#include "analogControl.h"

#include "../../../tools/quadratureEncoder.h"

namespace LIBC64 {
    
struct Mouse1351 : AnalogControl {
    
    Emulator::QuadratureEncoder quadratureEncoder;
    unsigned clock;
    
    Mouse1351( Interface::Device* device ) : AnalogControl( device ) {}
    
    auto read( ) -> uint8_t { 
        
        uint8_t out = 0xff;
        
        out &= ~((system->interface->inputPoll( device->id, 2 ) & 1) << 4);
        out &= ~((system->interface->inputPoll( device->id, 3 ) & 1) << 0);
        
        return out;
    }
    
    auto updatePot() -> void {

        quadratureEncoder.poll( posX, posY, device->userData, clock);
        
        clock = 0;
    }
    
    auto getPotX() -> uint8_t { 

        updatePot();
        
        return (uint8_t) ( ( quadratureEncoder.X & 0x7f ) + 0x40 );        
    }
    
    auto getPotY() -> uint8_t { 

        updatePot();
        
        return (uint8_t) ( ( quadratureEncoder.Y & 0x7f ) + 0x40 );        
    }
    
    auto tick() -> void {
        clock++;
    }  
    
    auto reset() -> void {
        clock = 0;
        quadratureEncoder.reset();
        quadratureEncoder.setCyclesPerFrame( system->getCyclesPerFrame() );
        quadratureEncoder.setCyclesPerSecond( system->getCyclesPerSecond() );
        AnalogControl::reset();
    }  
    
    auto serialize(Emulator::Serializer& s) -> void {
        
        s.integer( clock );
        
        quadratureEncoder.serialize( s );
        
        AnalogControl::serialize( s );
    }
};

}
