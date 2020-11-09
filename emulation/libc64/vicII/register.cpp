
#include "vicII.h"

namespace LIBC64 {

auto VicIICycle::readReg( uint8_t addr ) -> uint8_t {
	uint8_t value = 0;
	addr &= 0x3f;
    
    switch( addr ) {
        default:
            value = 0xff; //2f - 3f
			break;
        case 0x00: case 0x02: case 0x04: case 0x06:
        case 0x08: case 0x0a: case 0x0c: case 0x0e:
            value = sprite[ addr >> 1 ].x & 0xff;
			break;
        case 0x01: case 0x03: case 0x05: case 0x07:
        case 0x09: case 0x0b: case 0x0d: case 0x0f:
            value = sprite[ addr >> 1 ].y & 0xff;
			break;
        case 0x10: {
            for(unsigned i = 0; i < 8; i++)
                value |= ((sprite[i].x >> 8) & 1) << i;
            
			break;
        }
        case 0x11:
            value = (controlReg1 & ~0x80) | ((vCounter & 0x100) >> 1);
            break;
        case 0x12:
            value = vCounter & 0xff;
            break;
        case 0x13: {
            bool latchedInSecondHalfCycle = !!(irqLatchPending & (1 << Interrupt::LP));
            value = latchedInSecondHalfCycle ? lpxBefore : lpx;
        } break;
        case 0x14: {
            bool latchedInSecondHalfCycle = !!(irqLatchPending & (1 << Interrupt::LP));
            value = latchedInSecondHalfCycle ? lpyBefore : lpy;
        } break;
        case 0x15: {
            for (unsigned i = 0; i < 8; i++)
                value |= sprite[i].enabled << i;

			break;
        }
        case 0x16:
            value = controlReg2 | 0xc0;			
            break;
        case 0x17: {
            for(unsigned i = 0; i < 8; i++)
                value |= sprite[i].expandY << i;
            
			break;
        }        
        case 0x18:
            value = (vm << 4) | ((cb & 7) << 1) | 1;
            break;
        case 0x19:
            value = irqLatch | 0x70;
            break;
        case 0x1a:
            value = irqEnable | 0xf0;
            break;
        case 0x1b: {            
            for (unsigned i = 0; i < 8; i++)
                value |= sprite[i].prioMD << i;
            
			break;
        }            
        case 0x1c: {            
            for (unsigned i = 0; i < 8; i++)
                value |= sprite[i].multiColor << i;
            
			break;
        }            
        case 0x1d: {
            for (unsigned i = 0; i < 8; i++)
                value |= sprite[i].expandX << i;
            
			break;
        }        
        case 0x1e: {
			value = spriteSpriteCollidedRead;
			clearCollision = 0x1e;			
			break;
        }
        case 0x1f: {
			value = spriteForegroundCollidedRead;
			clearCollision = 0x1f;			
			break;
        }        
        case 0x20:
        case 0x21: case 0x22: case 0x23: case 0x24:
        case 0x25: case 0x26:
        case 0x27: case 0x28: case 0x29: case 0x2a:
        case 0x2b: case 0x2c: case 0x2d: case 0x2e:
			value = colorReg[ addr ] | 0xf0;
			break;
    }
	
	lastBusPhi2 = value;
	
	return value;
}

auto VicIICycle::writeReg( uint8_t addr, uint8_t value ) -> void {
    addr &= 0x3f;
	
    switch( addr ) {        
        case 0x00: case 0x02: case 0x04: case 0x06:
        case 0x08: case 0x0a: case 0x0c: case 0x0e:
            sprite[ addr >> 1 ].x &= ~0xff;
            sprite[ addr >> 1 ].x |= value;  
			break;
            
        case 0x01: case 0x03: case 0x05: case 0x07:
        case 0x09: case 0x0b: case 0x0d: case 0x0f:
            sprite[ addr >> 1 ].y = value;            
			break;
            
        case 0x10: {
            for(unsigned i = 0; i < 8; i++) {
                sprite[i].x &= 0xff;
                sprite[i].x |= ((value >> i) & 1) << 8;
            }        
        } break;
        
        case 0x11: {
            controlReg1 = value;
            irqLine &= 0xff;
            irqLine |= ((value >> 7) & 1) << 8;
			uint8_t testEcmBmm = ((value >> 5) & 3) << 3;
			disableEcmBmmTogether = (testEcmBmm == 0) && ((modeEcmBmm & 0x18) == 0x18);
            modeEcmBmm = testEcmBmm;
            den = (value >> 4) & 1;
            rSel = (value >> 3) & 1;
            borderTop = rSel ? 51 : 55;
            borderBottom = rSel ? 251 : 247;   			
            yScroll = value & 7;
            updateBorderData();
        } break;
            
        case 0x12: {
            irqLine &= ~0xff;
            irqLine |= value;
        } break;
        
        case 0x13:
		case 0x14:
			break;
        
        case 0x15: {
            for(unsigned i = 0; i < 8; i++)
                sprite[i].enabled = (value >> i) & 1;
        } break;
        
        case 0x16: {    
            controlReg2 = value;
			modeMcm = ((value >> 4) & 1) << 2;
            cSel = (value >> 3) & 1;
            xScroll = value & 7;
            updateBorderData();
        } break;

        case 0x17: {
            for (unsigned i = 0; i < 8; i++) {
				Sprite* spr = &sprite[i];				
				bool flipBefore = spr->expandYFlop;
				
				spr->expandY = (value >> i) & 1;
				if ( !spr->expandY )
					spr->expandYFlop = true;
				
				if (!flipBefore && spr->expandYFlop && (cycle == 14) )
					// sprite crunching
					spr->mc = (0x2a & (spr->mcBase & spr->mc)) | (0x15 & (spr->mcBase | spr->mc));
			}                
        } break;
        
        case 0x18: {
            vm = (value >> 4) & 15;
            cb = (value >> 1) & 7;
        } break;
        
        case 0x19: {
			// seted bits: disable, unseted bits: no change
			irqLatch &= ~((value & 0xf) | 0x80);		
            irqLatchPending |= 0x80;
        } break;
        
        case 0x1a: {
            irqEnable = value & 15;
            irqLatchPending |= 0x80;
        } break;
        case 0x1b: {
            for( unsigned i = 0; i < 8; i++ )
				sprite[i].prioMD = (value >> i) & 1;
            
            updatePrioExpand = true;
        } break;            

		case 0x1c: {
			for (unsigned i = 0; i < 8; i++) {
				sprite[i].multiColor = (value >> i) & 1;		  
            }
            updateMc = true;
		}
		break;

		case 0x1d: {
			for( unsigned i = 0; i < 8; i++ )
				sprite[i].expandX = (value >> i) & 1;		

            updatePrioExpand = true;
		} break;
		
        case 0x1e:
        case 0x1f:
            break; //not writable
            
        case 0x20:			
		case 0x21:			
        case 0x22:			
        case 0x23:			
        case 0x24:			
		case 0x25:
        case 0x26:
		case 0x27:
		case 0x28:
		case 0x29:
		case 0x2a:
		case 0x2b:
		case 0x2c:
		case 0x2d:
		case 0x2e: {			
            colorReg[ addr ] = value & 15;
            lastColorReg = addr;	
		} break;
        
        default:
            // 2f - 3f
            break;
    }
    
	
	lastBusPhi2 = value;
}


}
