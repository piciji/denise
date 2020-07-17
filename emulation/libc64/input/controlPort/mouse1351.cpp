
#include "analogControl.h"

#include "../../../tools/quadratureEncoder.h"

namespace LIBC64 {
    
struct Mouse1351 : AnalogControl {
    
    Emulator::QuadratureEncoder quadratureEncoder;
    unsigned cyclesElapsed;
    
    Mouse1351( Interface::Device* device ) : AnalogControl( device ) {}
    
    auto read( ) -> uint8_t { 
        
        uint8_t out = 0xff;
        
        out &= ~((system->interface->inputPoll( device->id, 2 ) & 1) << 4);
        out &= ~((system->interface->inputPoll( device->id, 3 ) & 1) << 0);
        
        return out;
    }
    
    auto updatePot() -> void {

        quadratureEncoder.poll( posX, posY, device->userData, cyclesElapsed);
        
        cyclesElapsed = 0;
    }
    
    auto getPotX() -> uint8_t { 

        updatePot();
        
        return (uint8_t) ( ( quadratureEncoder.X & 0x7f ) + 0x40 );        
    }
    
    auto getPotY() -> uint8_t { 

        updatePot();
        
        return (uint8_t) ( ( quadratureEncoder.Y & 0x7f ) + 0x40 );        
    }
    
    auto clock() -> void {
        cyclesElapsed++;
    }  
    
    auto reset() -> void {
        cyclesElapsed = 0;
        quadratureEncoder.reset();
        quadratureEncoder.setCyclesPerFrame( vicII->cyclesPerFrame() );
        quadratureEncoder.setCyclesPerSecond( vicII->frequency() );
        AnalogControl::reset();
    }  
    
    auto serialize(Emulator::Serializer& s) -> void {
        
        s.integer( cyclesElapsed );
        
        quadratureEncoder.serialize( s );
        
        AnalogControl::serialize( s );
    }
};

}
