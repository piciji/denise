
#include "analogControl.h"

namespace LIBC64 {
    
struct MouseNeos : AnalogControl {    
    
    enum class NeosState : uint8_t { XH = 0, XL = 1, YH = 2, YL = 3 } neosState;
    uint8_t neosStrobeBefore;
    uint8_t neosTimeoutCycles;
    uint8_t neosX;
    uint8_t neosY;
    uint8_t lastX;
    uint8_t lastY;
    std::function<void ()> neosTimer;
    
    MouseNeos( System* system, Interface::Device* device ) : AnalogControl( system, device ) {
        neosTimer = []() {};
    }
    
    auto read( ) -> uint8_t { 
        
        uint8_t out = 0xff;
        // neos right button is connected to Potentiometer X
        out &= ~((interface->inputPoll( device->id, 2 ) & 1) << 4);
        out &= ~0xf;

        if ( (neosState != NeosState::XH) && !sysTimer.has( &neosTimer ) ) {
            // too many cycles elapsed between polling the half bytes ... reinit
            neosState = NeosState::XH;
            neosFetchMovement( );
        }

        // the neos mouse uses the four direction pins of the control port to send
        // half bytes, which decode the movement deltas.
        // cia writes to fire button pin switches between the half bytes to read from.
        // the AD conversion happens in the neos mouse unlike the 1351 mouse.
        if ( out & 0x10 ) { // only if fire button is unpressed
            switch (neosState) {
                case NeosState::XH:
                    out |= (neosX >> 4) & 0xf;
                    break;
                case NeosState::XL:
                    out |= neosX & 0xf;
                    break;
                case NeosState::YH:
                    out |= (neosY >> 4) & 0xf;
                    break;
                case NeosState::YL:
                    out |= neosY & 0xf;
                    break;
            }
        }
        
        return out;
    }
    
    auto write( uint8_t value ) -> void {

        if ((value & 16) == (neosStrobeBefore & 16))
            return;
                
        // bit 5 has changed
        switch (neosState) {
            case NeosState::XH: // 0 -> 1
                if (value & 16)
                    neosState = NeosState::XL;
                break;
            case NeosState::XL: // 1 -> 0
                if (neosStrobeBefore & 16)
                    neosState = NeosState::YH;
                break;
            case NeosState::YH: // 0 -> 1
                if (value & 16)
                    neosState = NeosState::YL;
                break;
            case NeosState::YL: // 1 -> 0
                if (neosStrobeBefore & 16) {
                    neosState = NeosState::XH;
                    neosFetchMovement();
                }
                break;
        }

        sysTimer.add( &neosTimer, neosTimeoutCycles, Emulator::SystemTimer::Action::UpdateExisting);

        neosStrobeBefore = value;
    }
    
    auto neosFetchMovement() -> void {
        
        // downscale to 8 bit
        uint8_t x8bit = (uint8_t)(posX >> 1);
        uint8_t y8bit = (uint8_t)(posY >> 1);       
        
        // max delta is in a 8 bit range
        neosX = (uint8_t)(lastX - x8bit);
        lastX = x8bit;

        neosY = (uint8_t)(y8bit - lastY);
        lastY = y8bit;        
    }    
    
    auto getPotX() -> uint8_t { 
        return (interface->inputPoll( device->id, 3 ) & 1) ? 0xff : 0;
    }     
    
    auto reset() -> void {
        neosX = neosY = 0;
        lastX = lastY = 0;
        neosState = NeosState::YL;
        neosStrobeBefore = 0xff;
        neosTimeoutCycles = ( vicII->frequency() / 10000 ) * 2;
        sysTimer.remove( &neosTimer );
        AnalogControl::reset();
    }
    
    auto serialize(Emulator::Serializer& s) -> void {
        
        s.integer( lastX );
        s.integer( lastY );
        s.integer( (uint8_t&)neosState );
        s.integer( neosStrobeBefore );
        s.integer( neosTimeoutCycles );
        s.integer( neosX );
        s.integer( neosY );
                
        AnalogControl::serialize( s );
    }
};

}
