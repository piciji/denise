
#include "base.h"

namespace CIA {

auto Base::read( unsigned pos ) -> uint8_t {
        
	switch (pos & 0xf) {
        
		case 0:                                                
			return readPort( PORTA, &lines );
			
		case 1: {
            uint8_t out = readPort( PORTB, &lines );
			
            adjustBit6And7( out );
			
			return out;
		}
		case 2:
			return lines.ddra;

		case 3:
			return lines.ddrb;
            
        case 4:
			return readCounter<T_A>() & 0xff;
            
        case 5:
			return readCounter<T_A>() >> 8;
            
        case 6:
			return readCounter<T_B>() & 0xff;
            
        case 7:
			return readCounter<T_B>() >> 8;
            
		case 8:
		case 9:
		case 0xa:
		case 0xb:
			//tod handling differs between cia versions
			//hence is handled in child class
			return 0;
			
		case 0xc:				
            return sdr;
			
		case 0xd: {      		
			delay |= CIA_ACK0;
			
            return icr;
		}
        case 0xe:
			return timer[T_A].control & 0xef;
            
        case 0xf:						
			return timer[T_B].control & 0xef;
	}

    __unreachable
}

auto Base::write( unsigned pos, uint8_t value ) -> void {
	Timer* pT;
            
	switch( pos & 0xf ) {
		
		case 0:
        case 2: {
            if (pos == 0) {
                lines.pra = value;
                lines.praChange = true;
            } else {                
                lines.ddra = value;    
                lines.praChange = false;
            }
            // without external influence, lines show "pra" in output mode 
            // and will be pulled up in input mode, so there is no need to "and" pra with ddra
            uint8_t ioa = lines.pra | ~lines.ddra;

            // show the basic state of the lines
            lines.ioa = ioa;

            writePort( PORTA, &lines );

            lines.ioaOld = ioa;
            
			break;
		}	
		case 1:
        case 3: {
            if (pos == 1) {
                lines.prb = value;
                lines.prbChange = true;
            } else {
                lines.ddrb = value;    
                lines.prbChange = false;
            }
            
            uint8_t iob = lines.prb | ~lines.ddrb;
            
            adjustBit6And7( iob );
                
            lines.iob = iob;

            writePort( PORTB, &lines );

            lines.iobOld = iob;

			break;
        }
        case 4:		
            pT = &timer[T_A];
            
            pT->latch = (pT->latch & 0xff00) | value;
            
            if (delay & CIA_FL_TA2 )
				pT->counter = pT->latch;
            
            break;
        case 6:
            pT = &timer[T_B];
                    
			pT->latch = (pT->latch & 0xff00) | value;
			
            if (delay & CIA_FL_TB2)
				pT->counter = pT->latch;				
            
            break;            
        case 5:
            pT = &timer[T_A];

            pT->latch = (pT->latch & 0xff) | (value << 8);

            if (delay & CIA_FL_TA2)
                pT->counter = pT->latch;
            else if (!(pT->control & 1))
                delay |= CIA_FL_TA0;
            
            break;
                
        case 7:
			pT = &timer[T_B];
            
			pT->latch = (pT->latch & 0xff) | (value << 8);
            			
            if (delay & CIA_FL_TB2 )
                pT->counter = pT->latch;
			else if ( !(pT->control & 1))
                delay |= CIA_FL_TB0;
            
            break;            

        case 8:
        case 9:
        case 0xa:
        case 0xb:
			//tod handling differs between cia versions
			//hence is handled in derived class
            break;
			
		case 0xc:
			sdr = value;

			events->add( &startSdr, 2, Emulator::SystemTimer::Action::UpdateExisting );
			
			break;
            
		case 0xd:
			if (value & 0x80)
                icrmask |= value & 0x7F;
            else
                icrmask &= ~value;
			
			delay |= CIA_MASK_WRITE0;
            
			if ((delay & CIA_ACK1) == 0)
				handleInterrupt( 0 );
			
			break;
			
		case 0xe:
			if ((value ^ cra) & 0x40)
				switchSerialDirection( (value & 0x40) == 0 );			
			
        case 0xf: {
            pT = pos == 0xe ? &timer[T_A] : &timer[T_B];			
            // to do: oneshot stops the timer, is this recognized for 'toggle' reset
            // or have to write 'stop' to control register first
			if ((value & 1) && !(pT->control & 1))
				pT->toggle = true;
			
			// one shot
			if (value & 8) { // active
				pT->oneshot = true; // no delay
				events->remove( &(pT->disableOneshot) ); 
			} else
				// disabling one shot has one cycle delay
				events->add( &(pT->disableOneshot), 2, Emulator::SystemTimer::Action::WhenNotExistsOnly );
			
			// force load (one time)
			if ( value & 0x10 ) {
				// delayed one cycle
                // when fired, it can not be disabled next cycle
                delay |= (pos == 0xe) ? CIA_FL_TA0 : CIA_FL_TB0;
			}
            if (pos == 0xf && (value & 0x40) ) {
                // timer B in cascaded mode, so stop phase-in if needed
                events->add( &(pT->stop), 2, Emulator::SystemTimer::Action::WhenNotExistsOnly );
            
            } else {
                // start + phase in
                if ( (value & 1) && !(value & 0x20) ) {
                    events->add( &(pT->start), 2, Emulator::SystemTimer::Action::WhenNotExistsOnly );
					
                } else {
                    // stop the timer is delayed by one cycle
                    events->add( &(pT->stop), 2, Emulator::SystemTimer::Action::WhenNotExistsOnly );                
				}
            }						
			
            pT->control = value;            
		} break;
	}
}

auto Base::adjustBit6And7( uint8_t& inOut ) -> void {
    Timer* pT = &timer[T_B];
    
    if (pT->control & 2) {
        inOut &= ~0x80;
        if ( ( (pT->control & 4) ? pT->toggle : (delay & CIA_UF_TB1) ) )
            inOut |= 0x80;
    }

    pT = &timer[T_A];
    
    if (pT->control & 2) {
        inOut &= ~0x40;
        if ( ( (pT->control & 4) ? pT->toggle : (delay & CIA_UF_TA1) ) )
            inOut |= 0x40;
    }
}

}