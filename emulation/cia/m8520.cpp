
#include "m8520.h"

namespace CIA {

auto M8520::tod() -> void {
	if (!todActive)
		return;
	
	todIn = true;
}

auto M8520::processTod() -> void {
    if ( !todIn )
		return;

	todIn = false;
	
    todc++;
    todc &= 0xffffff;

    if (todc == alarm)
        handleInterrupt( 4 );
}

auto M8520::reset() -> void {
	
	todLatched = false;
	todActive = false;
    todIn = false;
	
	todLatch = todc = 0;
	alarm = 0;
	
	Base::reset();
}

auto M8520::read(unsigned pos) -> uint8_t {
	
	switch (pos & 0xf) {
		
        case 8:
            if (todLatched) {
                todLatched = false;
                return todLatch & 0xff;
            }
            return todc & 0xff;
			
        case 9:
            if (todLatched) {
                return (todLatch >> 8) & 0xff;
            }
			return (todc >> 8) & 0xff;
			
        case 0xa:
            if (!todLatched) {
                if ( !(crb & 0x80) ) todLatched = true;
                todLatch = todc;
            }
            return (todLatch >> 16) & 0xff;
			
        case 0xb:
            return 0;
			
		default:
			break;
	}
	
	return Base::read( pos );
}

auto M8520::write(unsigned pos, uint8_t value) -> void {
	
	switch (pos & 0xf) {
		        
        case 8:
        case 9:
        case 0xa: {
            uint8_t shifter = (pos - 8) << 3;
            bool changed;
            
            if (crb & 0x80) {
                
                changed = value != ( ( alarm >> shifter ) & 0xff);
                
				alarm = (alarm & ~(0xff << shifter) ) | (value << shifter);

            } else {

                if (pos == 8)
                    todActive = true;

                else if (pos == 0xa)
                    todActive = false;                

                changed = value != ((todc >> shifter) & 0xff);

                todc = (todc & ~(0xff << shifter)) | (value << shifter);
            }
            
            if (changed)
                if ( todc == alarm )
                    handleInterrupt( 4 );
            
            return;
        }
			
        case 0xb:
            return;
			
		case 0xe:
			value &= 0x7f;            
			break;
			
		default:
			break;
	}
		
	Base::write( pos, value );
}

auto M8520::serialize(Emulator::Serializer& s) -> void {
    
    s.integer( todLatched );
    s.integer( todActive );
    s.integer( todIn );
    s.integer( todLatch );
    s.integer( alarm );
    s.integer( todc );
    
    Base::serialize( s );
}

}