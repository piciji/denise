
#include "analogControl.h"

namespace LIBC64 {

#define MOUSE_DELTA_LIMIT 60

struct Mouse1351 : AnalogControl {

    Mouse1351( System* system, Interface::Device* device ) : AnalogControl( system, device ) {}

    int16_t deltaX;
    int16_t deltaY;

    auto poll( ) -> void {

        deltaX += interface->inputPoll( device->id, 0);
        deltaY -= interface->inputPoll( device->id, 1);
    }

    auto read( ) -> uint8_t { 
        
        uint8_t out = 0xff;
        
        out &= ~((interface->inputPoll( device->id, 2 ) & 1) << 4);
        out &= ~((interface->inputPoll( device->id, 3 ) & 1) << 0);
        
        return out;
    }
    
    auto updatePot() -> void {

        if (deltaX != 0 || deltaY != 0) {
            int _dx = std::abs(deltaX);
            int _dy = std::abs(deltaY);

            // limit movement
            if ((_dx > _dy) && (_dx > MOUSE_DELTA_LIMIT)) {
                deltaY = (int) deltaY * MOUSE_DELTA_LIMIT / _dx;
                deltaX = (deltaX < 0) ? -MOUSE_DELTA_LIMIT : MOUSE_DELTA_LIMIT;
            } else if (_dy > MOUSE_DELTA_LIMIT) {
                deltaX = (int) deltaX * MOUSE_DELTA_LIMIT / _dy;
                deltaY = (deltaY < 0) ? -MOUSE_DELTA_LIMIT : MOUSE_DELTA_LIMIT;
            }

            posX += deltaX;
            posY += deltaY;

            deltaX = deltaY = 0;
        }
    }
    
    auto getPotX() -> uint8_t {
        updatePot();

        return (uint8_t) ( (posX & 0x7f) + 0x40 ); // Bit 0: noise, Bit 7: unused (6 Bit effective)
    }
    
    auto getPotY() -> uint8_t {
        updatePot();

        return (uint8_t) ( (posY & 0x7f) + 0x40 ); // Bit 0: noise, Bit 7: unused (6 Bit effective)
    }    
    
    auto reset() -> void {
        deltaX = 0;
        deltaY = 0;
        AnalogControl::reset();
    }  
    
    auto serialize(Emulator::Serializer& s) -> void {
        
        AnalogControl::serialize( s );
    }
};

}
