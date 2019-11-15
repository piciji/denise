
#include "lightControl.h"

namespace LIBC64 {
    
struct MagnumLightPhaser : LightControl {
    
    MagnumLightPhaser( Interface::Device* device ) : LightControl( device ) {        
        
        xOffset = 25;
        
        yOffset = -14;
    }
    
    auto read( ) -> uint8_t { 
        
        return LightControl::read();
    }
	
    auto getCursorData() -> LightControl::Cursor {
        
        return {true, getCursorGunWidth(), &cursorGun[0]};
    }
    
    auto getPotY() -> uint8_t { 
        
        return button1Pressed ? 0 : 0xff;
    }
    
    auto draw(bool midScreen = false) -> void {
		
		LightControl::draw( 3, midScreen );
    }
    
};

}