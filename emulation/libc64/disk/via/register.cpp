
#include "via.h"

namespace LIBC64 {
          
auto Via::read( uint16_t pos ) -> uint8_t {
    pos &= 0xf;
    
	switch (pos) {
        
		case 0: { // orb    
            uint8_t out;
            
            resetIrq( 16 ); // disable CB1
            
            if ((pcr & 0xa0) != 0x20) // CB2 control 001 or 011 does not clear CB2
                resetIrq( 8 ); // disable CB2  
            
            if ( acr & 2 )
                out = lines.latchB;
            else
                out = readPort( Port::B, &lines );	

            // for prb all output pins reflect the actual contents of the register ... always
            // so overriden output pins by external influence or enabled latch (irb) doesn't matter
            
            // first clear all output pins, second use register values for output pins
            out = ( out & ~lines.ddrb ) | (lines.prb & lines.ddrb);                        
            
            if(acr & 0x80) { // override pb7 output
                out &= 0x7f;

                // toggles one time only in one shot mode, but it toggles in oneshot mode.
                // means the value is not forced to 1 when underflows in oneshot mode.
                // NOTE: by activating pb7 override in acr after write in timer 1 counter high, oneshot toggles from 1 - 0                
                if (timerAToggle)
                    out |= 0x80;
            }
            
            return out;                
        }
			
		case 1: // ora            
            resetIrq( 2 ); // disable CA1
            
            if ((pcr & 0x0a) != 0x02) // CA2 control 001 or 011 does not clear CA2
                resetIrq( 1 ); // disable CA2  
            
            // handshake or pulse output mode, but not manual
            if ((pcr & 0x0c) == 0x08) {
                if (ca2)
                    ca2Out( ca2 = 0 );
                
                if (pcr & 2) // pulse output, low for one cycle only
                    delay |= VIA_CA2_PULSE0;
            }            
                 
            // fall through
        
        case 0xf: // like ora but without handshake            
            if ( acr & 1 )
                return lines.latchA;
            
            return readPort( Port::A, &lines );		                
        
		case 3:
			return lines.ddra;

		case 2:
			return lines.ddrb;
            
        case 4: //T1CL
            resetIrq( 64 ); // clear timer 1 underflow
			return timerACounterRead & 0xff;
            
        case 5: //T1CH
			return timerACounterRead >> 8;
            
        case 6: //T1LL
			return timerALatch & 0xff;
            
        case 7: //T1LH
			return timerALatch >> 8;
            
		case 8: //T2CL
            resetIrq( 32 ); // clear timer 2 underflow
            return timerBCounterRead & 0xff;
            
		case 9: //T2CH
            return timerBCounterRead >> 8;
            
		case 0xa: //SR
            
            if (ifr & 4) {                
                shift.count = 0;
                delay |= VIA_SHIFT_WARMUP0;     
                resetIrq(4);
            }            
            
            return sdr;
            
		case 0xb:
			return acr;
			
		case 0xc:
            return pcr;
			
		case 0xd:  		
            if ( ifr & ier )
                return ifr | 0x80; // set msb if at least one incomming irq was enabled.
            
            return ifr;

        case 0xe:
			return ier | 0x80;
	}
    
   _unreachable
}

auto Via::peek( uint16_t pos ) -> uint8_t {
    pos &= 0xf;

	switch (pos) {

		case 0: { // orb
            uint8_t out;

            if ( acr & 2 )
                out = lines.latchB;
            else
                out = peekPort( Port::B, &lines );

            out = ( out & ~lines.ddrb ) | (lines.prb & lines.ddrb);

            if(acr & 0x80) { // override pb7 output
                out &= 0x7f;
                if (timerAToggle)
                    out |= 0x80;
            }

            return out;
        }

		case 1: // ora
        case 0xf: // like ora but without handshake
            if ( acr & 1 )
                return lines.latchA;

            return peekPort( Port::A, &lines );

		case 3:
			return lines.ddra;

		case 2:
			return lines.ddrb;

        case 4: //T1CL
			return timerACounterRead & 0xff;

        case 5: //T1CH
			return timerACounterRead >> 8;

        case 6: //T1LL
			return timerALatch & 0xff;

        case 7: //T1LH
			return timerALatch >> 8;

		case 8: //T2CL
            return timerBCounterRead & 0xff;

		case 9: //T2CH
            return timerBCounterRead >> 8;

		case 0xa: //SR
            return sdr;

		case 0xb:
			return acr;

		case 0xc:
            return pcr;

		case 0xd:
            if ( ifr & ier )
                return ifr | 0x80;

            return ifr;

        case 0xe:
			return ier | 0x80;
	}

   _unreachable
}

auto Via::write( uint16_t pos, uint8_t value ) -> void {
    pos &= 0xf;
    
    switch( pos ) {
        
        case 1: // ora
            resetIrq( 2 ); // disable CA1
            
            if ((pcr & 0x0a) != 0x02) // CA2 control 001 or 011 does not clear CA2                 
                resetIrq( 1 ); // disable CA2 
            
            // handshake or pulse output mode, but not manual
            // ca2 state in manual output mode doesn't change by a read or write
            if ((pcr & 0x0c) == 0x08) {
                if (ca2)
                    ca2Out( ca2 = 0 );
                
                if (pcr & 2) // pulse output, low for one cycle only
                    delay |= VIA_CA2_PULSE0;
            }
            
            // fall through
            
        case 0xf: // like pra but without handshake
            // fall through
            
        case 3: { // ddra
            if ( pos == 1 || pos == 0xf )
                lines.pra = value;
            else
                lines.ddra = value; 
            // without external influence, lines show "pra" in output mode 
            // and will be pulled up in input mode, so there is no need to "and" pra with ddra
            uint8_t ioa = lines.pra | ~lines.ddra;
            
            // show the basic state of the lines
            lines.ioa = ioa;

            writePort( Port::A, &lines );    

            lines.ioaOld = ioa;
            
        } break;
            
        case 0: // orb
            resetIrq( 16 ); // disable CB1
            
            if ((pcr & 0xa0) != 0x20) // CB2 control 001 or 011 does not clear CB2                 
                resetIrq( 8 ); // disable CB2  
            
            // handshake or pulse output mode, but not manual
            // cb2 state in manual output mode doesn't change by a read or write
            if ((pcr & 0xc0) == 0x80) {
                if (cb2)
                    cb2Out( cb2 = 0 );
                
                if (pcr & 0x20) // pulse output, low for one cycle only
                    delay |= VIA_CB2_PULSE0;
            }            
            
            // fall through
            
        case 2: { // ddrb
            if ( pos == 0 )
                lines.prb = value;
            else
                lines.ddrb = value; 
            
            uint8_t iob = lines.prb | ~lines.ddrb;
            
            // show the basic state of the lines
            lines.iob = iob;

            writePort( Port::B, &lines );  

            lines.iobOld = iob;
            
        } break;
            
        case 4: // Timer 1 count low
        case 6: // Timer 1 latch low
            timerALatch = (timerALatch & 0xff00) | value;
            
            if (delay & VIA_FORCE_LOAD_TIMERA0)
				timerACounter = (timerACounter & 0xff00) | value;

            break;
            
        case 5: // Timer 1 count high
            
            timerALatch = (timerALatch & 0xff) | (value << 8);
            
            timerACounter = timerALatch; 
            delay |= VIA_FORCE_LOAD_TIMERA0;
           
            timerAToggle = 0;
            timerATrigger = true;
            
            resetIrq( 64 ); // clear timer 1 underflow
            break;
            
        case 7: // Timer 1 latch high            
            timerALatch = (timerALatch & 0xff) | (value << 8);
            
            if (delay & VIA_FORCE_LOAD_TIMERA0)
				timerACounter = (timerACounter & 0xff) | (value << 8);
            
            resetIrq( 64 ); // clear timer 1 underflow
            break;
            
        case 8: // Timer 2 latch low
            timerBLatch = value;
            
            break;
            
        case 9: // Timer 2 count high
            timerBTrigger = true;           
            
            timerBCounter = timerBLatch | (value << 8);
            delay |= VIA_FORCE_LOAD_TIMERB0;
            
            resetIrq( 32 ); // clear timer 2 underflow
            
            break;
            
        case 0xa: // sr
            sdr = value;

            if (ifr & 4) {
                shift.count = 0;
                delay |= VIA_SHIFT_WARMUP0;
                resetIrq(4);
            }
            
            break;
            
        case 0xb: // acr            
            timerAToggle = 0;
            
            if ( (value & 0x80) && ((acr & 0x80) == 0) )
                timerAToggle = 1;
            
            if ( !(acr & 1) && (value & 1) ) // todo: latch port A ?
                lines.latchA = readPort( Port::A, &lines );

            if ( !(acr & 2) && (value & 2)) // todo: latch port B ?
                lines.latchB = readPort(Port::B, &lines);
            
            acr = value;                      
            
            isShiftT2Control = shiftT2Control();
            
            break;
            
        case 0xc: // pcr
            if ( (value & 0x8) == 0 ) { // input
                if (!ca2)
                    ca2Out(ca2 = true);
            } else if ( (value & 0xe) == 0xc ) {
                if (ca2)
                    ca2Out(ca2 = false);
            } else if ( (value & 0xe) == 0xe ) {
                if (!ca2)
                    ca2Out(ca2 = true);
            }

            if ( (value & 0x80) == 0 ) { // input
                if (!cb2)
                    cb2Out(cb2 = true);
            } else if ( (value & 0xe0) == 0xc0 ) {
                if (cb2)
                    cb2Out(cb2 = false);
            } else if ( (value & 0xe0) == 0xe0 ) {
                if (!cb2)
                    cb2Out(cb2 = true);
            }
            pcr = value;
            break;
            
        case 0xd: // ifr
            resetIrq( value );
            break;
            
        case 0xe: // ier
            if (value & 0x80)
                ier |= value & 0x7f;
            else
                ier &= ~value;
            
            delay |= VIA_UPDATE_IRQ0;
            break;
    }
}
    
}
