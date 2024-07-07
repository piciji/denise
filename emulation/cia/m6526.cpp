
#include "m6526.h"

namespace CIA {

auto M6526::tod() -> void {
	// cia tod line impulse
	if (!todActive)
		return;	
	
	++tickCounter &= 7;
	unsigned _compare = ( cra & 0x80 ) ? 5 : 6;

	if ( tickCounter < _compare )
		return;

	if ( tickCounter > _compare ) {
		tickCounter = 0;
		return;
	}

	tickCounter = 0;
	
	uint8_t ts = ( todc >> 0 ) & 0x0f; // tenth second	
	uint8_t sL = ( todc >> 8 ) & 0x0f; // seconds [x0-x9]
	uint8_t sH = ( todc >> 12 ) & 0x0f; // seconds [0x-5x] 
	uint8_t mL = ( todc >> 16 ) & 0x0f; // minutes [x0-x9]
	uint8_t mH = ( todc >> 20) & 0x0f; // minutes [0x-5x]
	uint8_t hL = ( todc >> 24 ) & 0x0f; // hours [x1-x9]
	uint8_t hH = ( todc >> 28 ) & 0x01; // hours [0x-1x]
	uint8_t pm = (todc >> 24) & 0x80; 
	
	/* tenth seconds (0-9) */
	ts = ( ts + 1 ) & 0x0f;
	if ( ts == 10 ) {
		ts = 0;
		// seconds
		sL = ( sL + 1 ) & 0x0f; // x0-x9
		if ( sL == 10 ) {
			sL = 0;
			sH = ( sH + 1 ) & 0x07; // 0x-5x
			if ( sH == 6 ) {
				sH = 0;
				// minutes
				mL = ( mL + 1 ) & 0x0f; // x0-x9
				if ( mL == 10 ) {
					mL = 0;
					mH = ( mH + 1 ) & 0x07; // 0x-5x
					if ( mH == 6 ) {
						mH = 0;
						// hours 1-12
						hL = ( hL + 1 ) & 0x0f;
						if ( hH ) {
							// 11 -> 12
							if ( hL == 2 )
								pm ^= 0x80;			
							// 12 -> 1
							if ( hL == 3 ) {
								hL = 1;
								hH = 0;
							}
							
						} else if ( hL == 10 ) {
							hL = 0;
							hH = 1;
						}
					}
				}
			}
		}
	}	
	
	todc = ts | (sL << 8) | (sH << 12) | (mL << 16) | (mH << 20) | (hL << 24) | (hH << 28) | (pm << 24);

	if ( todc == alarm )
		intIncomming |= 4;
}

auto M6526::reset() -> void {
	
	todLatched = false;
	todActive = false;
	
	todLatch = todc = 1 << 24; // init with 1 hour
	alarm = 0;
	tickCounter = 0;
	
	Base::reset();
}

auto M6526::read(unsigned pos) -> uint8_t {
	pos &= 0xf;
	
	switch (pos) {		
		case 8:
		case 9:
		case 0xa:
		case 0xb: {
			uint8_t shifter = (pos - 8) << 3;
			
			if(!todLatched)
				todLatch = todc;
			
			if (pos == 8)
				todLatched = false;
			
			else if (pos == 0xb)
				todLatched = true;
		
			return (todLatch >> shifter) & 0xff;
		} break;
			
		default:
			break;
	}
	
	return Base::read( pos );
}

auto M6526::write(unsigned pos, uint8_t value) -> void {
	pos &= 0xf;
	
	switch (pos) {
		case 8:
		case 9:
		case 0xa:
		case 0xb: {
			uint8_t shifter = (pos - 8) << 3;
			
			if (pos == 8)
				value &= 0x0f;
			else if (pos == 9)
				value &= 0x7f;
			else if (pos == 0xa)
				value &= 0x7f;
			else { // 0xb
				value &= 0x9f;
				
				if ( ( ( value & 0x1f ) == 0x12 ) && !( crb & 0x80 ) )
					value ^= 0x80; //flip AM / PM
			}
			
            bool changed;
            
			if (crb & 0x80) {
                
                changed = value != ( ( alarm >> shifter ) & 0xff);
                
				alarm = (alarm & ~(0xff << shifter) ) | (value << shifter);
				                
			} else {
				if (pos == 8) {
					if (!todActive)
						tickCounter = 0;
									
					todActive = true;

				} else if (pos == 0xb) 				
					todActive = false;
				
                changed = value != ( ( todc >> shifter ) & 0xff);
				
				todc = (todc & ~(0xff << shifter) ) | (value << shifter);
			}

            if (changed)
                if ( todc == alarm )
					intIncomming |= 4;
			
			return;
		}
	
		default:
			break;
	}
		
	Base::write( pos, value );
}

auto M6526::serialize(Emulator::Serializer& s) -> void {
    
    s.integer( todLatched );
    s.integer( todActive );
    s.integer( todLatch );
    s.integer( alarm );
    s.integer( todc );
    s.integer( tickCounter );
    
    Base::serialize( s );
}

}
