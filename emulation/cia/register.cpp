
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
			return timer[T_A].counter & 0xff;
            
        case 5:
			return timer[T_A].counter >> 8;
            
        case 6:
			return timer[T_B].counter & 0xff;
            
        case 7:
			return timer[T_B].counter >> 8;
            
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
			acknowledgeCycle |= 1;
			
            return icr;
		}
        case 0xe:
			return timer[T_A].control & 0xef;
            
        case 0xf:						
			return timer[T_B].control & 0xef;
	}
}

// the write is internal valid at the end of second half cycle.
// the Vic should be synced before, in order to get right value in case of lightpen trigger

auto Base::writePipelined(unsigned pos, uint8_t value) -> void {
	registerWrite.addr = pos & 0xf;
	registerWrite.value = value;
	registerWrite.pipelined = true;
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
            
            if (ioa != lines.ioa) {
                // show the basic state of the lines
                lines.ioa = ioa;
                
                writePort( PORTA, &lines );                                
            }
            
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
            
            if (iob != lines.iob) {
                
                lines.iob = iob;
                
                writePort( PORTB, &lines );                                
            }
            			
			break;
        }
        case 4:		
        case 6:            
            pT = pos == 4 ? &timer[T_A] : &timer[T_B];
                    
			pT->latch = (pT->latch & 0xff00) | value;
			
			if (pT->forceloadCycle)
				pT->counter = (pT->counter & 0xff00) | value;
			
            break;
            
        case 5:
        case 7:
			pT = pos == 5 ? &timer[T_A] : &timer[T_B];
            
			pT->latch = (pT->latch & 0xff) | (value << 8);
            			
			if ( !(pT->control & 1) || (pT->forceloadCycle) )
                pT->counter = pT->latch;

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
			
			if ( timer[T_A].control & 0x40 ) // sdr output
                sdrValid = true;
			else
				shiftCount = 0; // not sure ???
			
			break;
            
		case 0xd:
			if (value & 0x80)
                icrmask |= value & 0x7F;
            else
                icrmask &= ~value;
			
			maskWriteCycle |= 1;
            
            handleInterrupt( 0 );
			break;
			
		case 0xe:  
        case 0xf:
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
				events->add( &(pT->disableOneshot), 2, Emulator::Events::WhenNotExistsOnly );
			
			// force load (one time)
			if ( value & 0x10 )
				// delayed one cycle
                // when fired, it can not be disabled next cycle
				events->add( &(pT->forceLoad), 2 );
				
            if (pos == 0xf && (value & 0x40) ) 
                // timer B in cascaded mode, so stop phase-in if needed
                events->add( &(pT->stop), 2, Emulator::Events::WhenNotExistsOnly );
            
            else {
                // start + phase in
                if ( (value & 1) && !(value & 0x20) )
                    events->add( &(pT->start), 2, Emulator::Events::WhenNotExistsOnly );
                else
                    // stop the timer is delayed by one cycle
                    events->add( &(pT->stop), 2, Emulator::Events::WhenNotExistsOnly );                
            }
			
            pT->control = value;        									
            break;
	}
}

auto Base::adjustBit6And7( uint8_t& inOut ) -> void {
    Timer* pT = &timer[T_B];
    
    if (pT->control & 2) {
        inOut &= ~0x80;
		if ( ( (pT->control & 4) ? pT->toggle : (pT->underflowCycle) ) )
            inOut |= 0x80;
    }

    pT = &timer[T_A];
    
    if (pT->control & 2) {
        inOut &= ~0x40;
		if ( ( (pT->control & 4) ? pT->toggle : (pT->underflowCycle) ) )
            inOut |= 0x40;
    }
}

}