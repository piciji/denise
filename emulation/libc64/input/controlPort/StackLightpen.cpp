
#include "lightControl.h"

namespace LIBC64 {
    
struct StackLightpen : LightControl {
    
    StackLightpen( System* system, Interface::Device* device ) : LightControl( system, device ) {
        
        xOffset = 24;
                
        yOffset = 0;
    }
    
    auto read( ) -> uint8_t { 
        
        uint8_t out = 0xff;
        
        out &= ~(button2Pressed << 2); // left
        
        return out & LightControl::read();
    }   
    
    auto getCursorData() -> LightControl::Cursor {
        
        return {false, getCursorPenWidth(), &cursorPen[0]};
    }
	
    auto useButton1Internal() -> bool {
        
        return true;
    }
    
    auto draw() -> void {
		
		LightControl::draw( 7 );

    }
};

}