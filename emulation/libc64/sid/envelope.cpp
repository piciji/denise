
#include "reSid.h"

#define DELAY_ATTACK0    1
#define DELAY_ATTACK1    2
#define DELAY_ATTACK2    4
#define DELAY_ATTACK3    8

#define DELAY_RELEASE0   0x10
#define DELAY_RELEASE1   0x20
#define DELAY_RELEASE2   0x40
#define DELAY_RELEASE3   0x80

#define DELAY_EXPONENTIAL0   0x100
#define DELAY_EXPONENTIAL1   0x200
#define DELAY_EXPONENTIAL2   0x400

#define DELAY_ENVELOPE0   0x800
#define DELAY_ENVELOPE1   0x1000
#define DELAY_ENVELOPE2   0x2000
#define DELAY_ENVELOPE3   0x4000
#define DELAY_ENVELOPE4   0x8000

#define DELAY_ENVELOPE  (DELAY_ENVELOPE0 | DELAY_ENVELOPE1 | DELAY_ENVELOPE2 | DELAY_ENVELOPE3 | DELAY_ENVELOPE4)

#define ENVELOPE_MASK ~(0x10000 | DELAY_ATTACK0 | DELAY_RELEASE0 | DELAY_EXPONENTIAL0 | DELAY_ENVELOPE0 )

namespace LIBC64 {
    
uint16_t ReSid::Envelope::ratePeriodLookup[16] = {
    8, /*2ms*/ 31, /*8ms*/ 62, /*16ms*/ 94, /*24ms*/
    148, /*38ms*/ 219, /*56ms*/ 266, /*68ms*/ 312, /*80ms*/
    391, /*100ms*/ 976, /*250ms*/ 1953, /*500ms*/ 3125, /*800ms*/
    3906, /*1 s*/ 11719, /*3 s*/ 19531, /*5 s*/ 31250 /*8 s*/
};

Emulator::DAC<uint8_t> ReSid::Envelope::dac6581( 8, 2.20, false );
Emulator::DAC<uint8_t> ReSid::Envelope::dac8580( 8, 2.00, true );

auto ReSid::Envelope::setType( Type type ) -> void {
    
    this->type = type;
    
    dac = type == Type::MOS_6581 ? &dac6581 : &dac8580;
}

inline auto ReSid::Envelope::sustainComparator() -> uint8_t {
    
    return (sustain << 4) | sustain;
}

// inline auto ReSid::Envelope::output() -> uint8_t {
//
//     return dac->get( counter );
// }

auto ReSid::Envelope::setAttackDecay( uint8_t value ) -> void {
    
    attack = ( value >> 4 ) & 0xf;
    decay = value & 0xf;
 
    if ( state == S_ATTACK )
        ratePeriod = ratePeriodLookup[ attack ];
    
    else if ( state == S_DECAY )
        ratePeriod = ratePeriodLookup[ decay ];
}

auto ReSid::Envelope::setSustainRelease( uint8_t value ) -> void {
    
    sustain = (value >> 4) & 0xf;
    release = value & 0xf;
    
    if ( state == S_RELEASE )
        ratePeriod = ratePeriodLookup[ release ];
}

auto ReSid::Envelope::control( bool gate ) -> void {
    
    if ( gate == gateBefore )
        return;
    
    uint8_t add = 0;
    
    if (gate) {       

        if ( delay & DELAY_EXPONENTIAL0 )
            delay = (delay & ~DELAY_ENVELOPE) | DELAY_ENVELOPE2;
        
        else if (resetRateCounter)
            delay = (delay & ~DELAY_ENVELOPE) | ( (exponentialPeriod == 1) ? DELAY_ENVELOPE2 : DELAY_ENVELOPE0);

        else if ( delay & DELAY_EXPONENTIAL1 )
            add = 1;        

        state = S_DECAY;
        ratePeriod = ratePeriodLookup[ decay ];

        delay |= add ? DELAY_ATTACK0 : DELAY_ATTACK1;
        
    } else {
        
        if (delay & DELAY_ENVELOPE)
            // allow pending counter update
            add = 1;
        
        if (state == S_ATTACK) {
            delay |= add ? DELAY_RELEASE0 : DELAY_RELEASE1;
        }
        
        else if (state == S_DECAY) {
            state = S_RELEASE;
            ratePeriod = ratePeriodLookup[ release ];
        }
    }            
    
    gateBefore = gate;
}

auto ReSid::Envelope::reset() -> void {
    
    attack = decay = sustain = release = 0;
    
    counter = 0xaa;
    
    lockEnvCounter = false;
    
    rateCounter = 0;   
	
	resetRateCounter = false;
    
    ratePeriod = ratePeriodLookup[ release ];
    
    state = S_RELEASE;    
    
    exponentialCounter = 0;
    
    exponentialPeriod = 1;
    
    gateBefore = 0;	
    
    delay = 0;
}

auto ReSid::Envelope::callEnvelope() -> void {
    if (unlikely(lockEnvCounter))
        return;

    if (state == S_ATTACK) {

        ++counter &= 0xff;

        if (unlikely(counter == 0xff)) {
            state = S_DECAY;
            ratePeriod = ratePeriodLookup[ decay ];
        }

    } else // Decay or Release
        --counter &= 0xff;

    updateExponentialPeriod();
}

auto ReSid::Envelope::callExponentialCounter() -> void {
    exponentialCounter = 0;

    if (((state == S_DECAY) && (counter != sustainComparator())) // decrease volume untill seted sustain value
            || (state == S_RELEASE)) { // decrease volume untill silence

        delay = (delay & ~DELAY_ENVELOPE) | DELAY_ENVELOPE3;
    }
}

inline auto ReSid::Envelope::clock() -> void {
	
    env3 = counter;
    
    if (unlikely(delay)) {
        delay = (delay << 1) & ENVELOPE_MASK;
        
        if (delay & DELAY_RELEASE3) {
            state = S_RELEASE;
            ratePeriod = ratePeriodLookup[ release ];
        }

        if (delay & DELAY_ATTACK3) {
            state = S_ATTACK;
            ratePeriod = ratePeriodLookup[ attack ];
            lockEnvCounter = false;
        }
        
        if (delay & DELAY_EXPONENTIAL2)
            callExponentialCounter();        
        
        if (delay & DELAY_ENVELOPE4)
            callEnvelope();        
    }
	
	if (unlikely(resetRateCounter)) {
		resetRateCounter = false;
		rateCounter = 0;
		
		if ( state == S_ATTACK ) {
            exponentialCounter = 0;
            delay = (delay & ~DELAY_ENVELOPE) | DELAY_ENVELOPE2;
			
		} else if (!lockEnvCounter) {
			
            if (unlikely(++exponentialCounter == exponentialPeriod)) //non linear volume decrease
                delay |= (exponentialPeriod != 1) ? DELAY_EXPONENTIAL0 : DELAY_EXPONENTIAL1;
		}
	}	
	
	else if (rateCounter == ratePeriod) {
		resetRateCounter = true;
		return;
	}
	
	++rateCounter &= 0x7fff; //15 bit counter	
	if (unlikely(!rateCounter)) // wrap around
		rateCounter = 1;     	
}

auto ReSid::Envelope::updateExponentialPeriod() -> void {

    switch( counter ) {
        case 0xff: exponentialPeriod = 1; break;
        case 0x5d: exponentialPeriod = 2; break;
        case 0x36: exponentialPeriod = 4; break;
        case 0x1a: exponentialPeriod = 8; break;
        case 0x0e: exponentialPeriod = 16; break;
        case 0x06: exponentialPeriod = 30; break;
        case 0x00: exponentialPeriod = 1;
            lockEnvCounter = true;
            break;		
    }
}  

}
