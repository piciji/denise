
#include "lightControl.h"

namespace LIBC64 {
    
struct InkwellLightpen : LightControl {
    
    InkwellLightpen( Interface::Device* device ) : LightControl( device ) {        
        
        xOffset = 20;
        
        yOffset = 0;
    }
    
    auto read( ) -> uint8_t { 
        
        uint8_t out = 0xff;
        
        out &= ~(button1Pressed << 2); // left
        
        return out & LightControl::read();
    }

    auto getCursorData() -> LightControl::Cursor {
        
        return {false, getCursorPenWidth(), &cursorPen[0]};
    }
    
    auto getPotY() -> uint8_t { 
        
        return button2Pressed ? 0 : 0xff;
    }

    auto draw(bool midScreen = false) -> void {
		
		LightControl::draw( 7, midScreen );

    }
        
};

}