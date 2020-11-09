
#include "lightControl.h"

namespace LIBC64 {
    
struct GunStick : LightControl {
    
	uint8_t* linePtr = nullptr;
	unsigned vLatch = 0;
	
    GunStick( Interface::Device* device ) : LightControl( device ) {        
        
        xOffset = 16;
        
        yOffset = 0;
    }
    
    auto setTrigger() -> void {
        
        triggerOn = [this]() {

            // we observe the pixel color of gun trigger position
            linePtr = vicII->getCurrentLinePtr();
			displayPtr = vicII->getCurrentFramePtr();
			
			linePtr += cyclePixel;
            displayPtr += cyclePixel;
			
			vLatch = vicII->getVcounter();
        };
    }
    
    auto getCursorData() -> LightControl::Cursor {
        
        return {true, getCursorGunWidth(), &cursorGun[0]};
    }
	
    auto read( ) -> uint8_t { 
        
        uint8_t out = 0xff;
        
        out &= ~(button1Pressed << 4); // fire pin
        
        out &= ~( nonBlack() << 1); // check for non black color (light trigger: down pin)
        
        return out;
    }
	
	auto nonBlack() -> bool {
		
		if (!displayPtr)
			return false;
		
		if (vLatch == vicII->getVcounter())
			return (*linePtr & 0xf) != 0;
		
		return (*displayPtr & 0xf) != 0;
	}
    
    auto draw(bool midScreen = false) -> void {
        
        uint8_t color = 3;
        
        if ( this == system->input->controlPort2 )
            color = 4;
		
		LightControl::draw( color, midScreen );
    }
};

}