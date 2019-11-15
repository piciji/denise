
#include "lightControl.h"

namespace LIBC64 {
    
struct GunStick : LightControl {
    
    GunStick( Interface::Device* device ) : LightControl( device ) {        
        
        xOffset = 16;
        
        yOffset = 0;
    }
    
    auto setTrigger() -> void {
        
        triggerOn = [this]() {

            // we observe the pixel color of gun trigger position
            displayPtr = vicII->getCurrentLinePtr();
            displayPtr += cyclePixel;
        };
    }
    
    auto getCursorData() -> LightControl::Cursor {
        
        return {true, getCursorGunWidth(), &cursorGun[0]};
    }
	
    auto read( ) -> uint8_t { 
        
        uint8_t out = 0xff;
        
        out &= ~(button1Pressed << 4); // fire pin
        
        out &= ~( (displayPtr && (*displayPtr != 0) ) << 1); // check for non black color (light trigger: down pin)
        
        return out;
    }
    
    auto draw(bool midScreen = false) -> void {
        
        uint8_t color = 3;
        
        if ( this == system->input->controlPort2 )
            color = 4;
		
		LightControl::draw( color, midScreen );
    }
};

}