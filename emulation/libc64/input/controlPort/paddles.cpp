
#include "analogControl.h"

namespace LIBC64 {
    
struct Paddles : AnalogControl {
    
    int16_t lastX;
    int16_t lastY;
    int16_t paddleX;
    int16_t paddleY;
        
    Paddles( System* system, Interface::Device* device ) : AnalogControl( system, device ) {}
    
    auto read( ) -> uint8_t { 
        
        uint8_t out = 0xff;
        
        out &= ~((system->interface->inputPoll( device->id, 2 ) & 1) << 2);
        out &= ~((system->interface->inputPoll( device->id, 3 ) & 1) << 3);
        
        return out;
    }

    auto getPotX() -> uint8_t {

        paddleX = paddleX + ( (posX - lastX) >> 2);
        lastX = posX;

        // paddles have a fixed range
        if (paddleX > 255)
            paddleX = 255; 
        
        if (paddleX < 0)
            paddleX = 0;

        return 0xff - (uint8_t) paddleX;                
    }

    auto getPotY() -> uint8_t {

        paddleY = paddleY + ( (posY - lastY) >> 2);
        lastY = posY;
        
        if (paddleY > 255)
            paddleY = 255; 
        
        if (paddleY < 0)
            paddleY = 0;
        
        return 0xff - (uint8_t) paddleY;
    }
    
    auto reset() -> void {
        
        lastX = lastY = 0;
        paddleX = paddleY = 0;
        
        AnalogControl::reset();
    }

    auto serialize(Emulator::Serializer& s) -> void {

        s.integer(lastX);
        s.integer(lastY);
        s.integer(paddleX);
        s.integer(paddleY);

        AnalogControl::serialize(s);
    }
};

}