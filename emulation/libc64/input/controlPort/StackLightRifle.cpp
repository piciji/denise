
#include "lightControl.h"

namespace LIBC64 {
    
struct StackLightRifle : LightControl {
    
    StackLightRifle( Interface::Device* device ) : LightControl( device ) {        
        
        xOffset = 17;
        
        yOffset = -2;
    }
    
    auto read( ) -> uint8_t { 
        
        uint8_t out = 0xff;
        
        out &= ~(button1Pressed << 2); // left
        
        return out & LightControl::read();
    }
    
    auto getCursorData() -> LightControl::Cursor {
        
        return {true, getCursorGunWidth(), &cursorGun[0]};
    }
    
    auto draw(bool midScreen = false) -> void {
		
		LightControl::draw( 3, midScreen );
    }
        
};

}