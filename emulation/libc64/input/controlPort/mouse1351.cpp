
#include <chrono>
#include "analogControl.h"

#include "../../../tools/quadratureEncoder.h"

namespace LIBC64 {

#define MOUSE_DELTA_LIMIT 56

struct Mouse1351 : AnalogControl {
    
    Emulator::QuadratureEncoder quadratureEncoder;
    unsigned sysClock;
    unsigned timestamp;
    
    Mouse1351( System* system, Interface::Device* device ) : AnalogControl( system, device ) {}

    auto poll( ) -> void {

        int16_t deltaX = interface->inputPoll( device->id, 0);
        int16_t deltaY = interface->inputPoll( device->id, 1);

        int _dx = std::abs(deltaX);
        int _dy = std::abs(deltaY);

        // limit movement
        if ( (_dx > _dy) && (_dx > MOUSE_DELTA_LIMIT)) {
            deltaY = (int)deltaY * MOUSE_DELTA_LIMIT / _dx;
            deltaX = (deltaX < 0) ? -MOUSE_DELTA_LIMIT : MOUSE_DELTA_LIMIT;
        } else if (_dy > MOUSE_DELTA_LIMIT) {
            deltaX = (int)deltaX * MOUSE_DELTA_LIMIT / _dy;
            deltaY = (deltaY < 0) ? -MOUSE_DELTA_LIMIT : MOUSE_DELTA_LIMIT;
        }

        posX += deltaX;
        posY -= deltaY;
        timestamp = std::chrono::duration_cast<std::chrono::microseconds>
                (std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }

    auto read( ) -> uint8_t { 
        
        uint8_t out = 0xff;
        
        out &= ~((interface->inputPoll( device->id, 2 ) & 1) << 4);
        out &= ~((interface->inputPoll( device->id, 3 ) & 1) << 0);
        
        return out;
    }
    
    auto updatePot() -> void {

		unsigned cyclesElapsed = sysTimer.fallBackCycles( sysClock );

        quadratureEncoder.poll( cyclesElapsed );

        quadratureEncoder.hostUpdate( posX, posY, timestamp);
        
        sysClock = sysTimer.clock;
    }
    
    auto getPotX() -> uint8_t { 

        updatePot();

        return (uint8_t) ( quadratureEncoder.X & 0x7f ); // Bit 0: noise, Bit 7: unused (6 Bit effective)
    }
    
    auto getPotY() -> uint8_t { 

        updatePot();
        
        return (uint8_t) ( quadratureEncoder.Y & 0x7f ); // Bit 0: noise, Bit 7: unused (6 Bit effective)
    }    
    
    auto reset() -> void {
        sysClock = sysTimer.clock;
        quadratureEncoder.reset();
        quadratureEncoder.setCyclesPerFrame( vicII->cyclesPerFrame() );
        quadratureEncoder.setCyclesPerSecond( vicII->frequency() );
        AnalogControl::reset();
    }  
    
    auto serialize(Emulator::Serializer& s) -> void {
        
        s.integer( sysClock );
        
        quadratureEncoder.serialize( s );
        
        AnalogControl::serialize( s );
    }
};

}
