
#include "vicIIFast.h"

namespace LIBC64 {

auto VicIIFast::readReg( uint8_t addr ) -> uint8_t {
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
			value = spriteSpriteCollided;
			spriteSpriteCollided = 0;	
            canSpriteSpriteCollisionIrq = true;
			break;
        }
        case 0x1f: {
			value = spriteForegroundCollided;
			spriteForegroundCollided = 0;
            canSpriteForegroundCollisionIrq = true;
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

	
	return value;
}

auto VicIIFast::writeReg( uint8_t addr, uint8_t value ) -> void {
    addr &= 0x3f;
	
	if (useThread && visibleLine)
		while ( ready.load() ) {} 
	
    switch( addr ) {        
        case 0x00: case 0x02: case 0x04: case 0x06:
        case 0x08: case 0x0a: case 0x0c: case 0x0e: {
			Sprite* spr = &sprite[ addr >> 1 ];
			
			unsigned _x = spr->x;
            spr->x &= ~0xff;
            spr->x |= value;  
			if (_x != spr->x) {
				calcSpriteX( spr );
				calcSpriteMask( spr );
			}
			
		} break;
            
        case 0x01: case 0x03: case 0x05: case 0x07:
        case 0x09: case 0x0b: case 0x0d: case 0x0f:
            sprite[ addr >> 1 ].y = value;            
			break;
            
        case 0x10: {
			unsigned _x;
            for(unsigned i = 0; i < 8; i++) {
				Sprite* spr = &sprite[ i ];
				_x = spr->x;
				
                spr->x &= 0xff;
                spr->x |= ((value >> i) & 1) << 8;
				
				if (_x != spr->x) {
					calcSpriteX( spr );
					calcSpriteMask( spr );
				}
            }        
        } break;
        
        case 0x11: {
            bool _den = den;
			controlReg1 = value;
			irqLine &= 0xff;
			irqLine |= ((value >> 7) & 1) << 8;
			modeEcmBmm = ((value >> 6) & 1) << 2;
			modeEcmBmm |= ((value >> 5) & 1) << 1;          
			den = (value >> 4) & 1;
			rSel = (value >> 3) & 1;
			borderTop = rSel ? 51 : 55;
			borderBottom = rSel ? 251 : 247;
			yScroll = value & 7;
			updateBorderData();

			bool _badLine;

			if (!enableSequencer) {
				_badLine = allowBadlines && (yScroll == (vCounter & 7));
				if (cAccessArea && (_badLine != badLine) )
					setRdy(_badLine);

				badLine = _badLine;

				return;
			}
            
            if (!_den && den) {
                
                if (vCounter == borderTop ) 
                    vFlipFlop = false; 

                if (!allowBadlines && (vCounter == 0x30))
                    allowBadlines = true;   
            }            

			_badLine = allowBadlines && (yScroll == (vCounter & 7));  

			if (cAccessArea && (_badLine != badLine) )
				setRdy(_badLine);        
                        
			if ( badLine != _badLine ) {

				if (cycle <= 13) {
					badLine = _badLine;

					if (badLine)
						idleMode = false;

				} else if (cycle < 54 ) {

					if (cycle >= 32) {
						if (useThread)
							while (ready.load()) {}

						dmaDelay = cycle - 13;
						scanline();
					} else
						dmaDelay = cycle - 13;

				} else {

					badLine = _badLine;

					if (badLine)
						idleMode = false;
				}
			}   
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
			bool _cSel = cSel;		
			controlReg2 = value;
			modeMcm = (value >> 4) & 1;
			cSel = (value >> 3) & 1;
			xScroll = value & 7;
			updateBorderData();
			setBorderDim();

			if (cycle == 55 && _cSel && !cSel ) {
				// no border
				hFlipFlop = 0;
			}	
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
			bool _expandX;
			
			for( unsigned i = 0; i < 8; i++ ) {
				Sprite* spr = &sprite[ i ];
				
				_expandX = spr->expandX;
				spr->expandX = (value >> i) & 1;		
				
				if (_expandX != spr->expandX)
					calcSpriteMask( spr );
			}

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
}

}
