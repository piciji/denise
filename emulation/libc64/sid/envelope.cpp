
#include "sid.h"

namespace LIBC64 {
    
uint16_t Sid::Envelope::ratePeriodLookup[16] = {
    8, /*2ms*/ 31, /*8ms*/ 62, /*16ms*/ 94, /*24ms*/
    148, /*38ms*/ 219, /*56ms*/ 266, /*68ms*/ 312, /*80ms*/
    391, /*100ms*/ 976, /*250ms*/ 1953, /*500ms*/ 3125, /*800ms*/
    3906, /*1 s*/ 11719, /*3 s*/ 19531, /*5 s*/ 31250 /*8 s*/
};

Emulator::DAC<uint8_t> Sid::Envelope::dac6581( 8, 2.20, false );
Emulator::DAC<uint8_t> Sid::Envelope::dac8580( 8, 2.00, true );

auto Sid::Envelope::setType( Type type ) -> void {
    
    this->type = type;
    
    dac = type == Type::MOS_6581 ? &dac6581 : &dac8580;
}

inline auto Sid::Envelope::sustainComparator() -> uint8_t {
    
    return (sustain << 4) | sustain;
}

inline auto Sid::Envelope::output() -> uint8_t {
    
    return dac->get( counter );
}

auto Sid::Envelope::setAttackDecay( uint8_t value ) -> void {
    
    attack = ( value >> 4 ) & 0xf;
    decay = value & 0xf;
 
    if ( state == S_ATTACK )
        ratePeriod = ratePeriodLookup[ attack ];
    
    else if ( state == S_DECAY )
        ratePeriod = ratePeriodLookup[ decay ];
}

auto Sid::Envelope::setSustainRelease( uint8_t value ) -> void {
    
    sustain = (value >> 4) & 0xf;
    release = value & 0xf;
    
    if ( state == S_RELEASE )
        ratePeriod = ratePeriodLookup[ release ];
}

auto Sid::Envelope::control( bool gate ) -> void {
    
    if ( gate == gateBefore )
        return;
    
    uint8_t add = 0;
    
    if (gate) {       

        if ( events->delay(&callExponentialCounter) == 2 )
            events->add( &callEnvelope, 2, Emulator::Events::Action::UpdateExisting );  
        
        else if (resetRateCounter)
            events->add( &callEnvelope, exponentialPeriod == 1 ? 2 : 4, Emulator::Events::Action::UpdateExisting );      
            
         else if (events->delay(&callExponentialCounter) == 1)
            add = 1;        
        
        events->add( &callDecay, 1, Emulator::Events::Action::BeforeOthers ); // accidently called in next cycle
        events->add( &callAttack, 2 + add, Emulator::Events::Action::BeforeOthers );
        
    } else if (!lockEnvCounter) {
        
        if ( events->has( &callEnvelope ) )
            // allow pending counter update
            add = 1;
        
        if (state == S_ATTACK)
            events->add( &callRelease, 2 + add, Emulator::Events::Action::BeforeOthers ); 
        
        else if (state == S_DECAY)
            events->add( &callRelease, 1, Emulator::Events::Action::BeforeOthers );
    }            
    
    gateBefore = gate;
}

auto Sid::Envelope::reset() -> void {
    
    attack = decay = sustain = release = 0;
    
    counter = counterTemp = 0xaa;
    
    lockEnvCounter = false;
    
    rateCounter = 0;   
	
	resetRateCounter = false;
    
    ratePeriod = ratePeriodLookup[ release ];
    
    state = S_RELEASE;    
    
    exponentialCounter = 0;
    
    exponentialPeriod = 1;
    
    gateBefore = 0;	
}

Sid::Envelope::Envelope() {
	
	callAttack = [this]() {

		state = S_ATTACK;
		ratePeriod = ratePeriodLookup[ attack ];
		lockEnvCounter = false;
	};

	callDecay = [this]() {

		state = S_DECAY;
		ratePeriod = ratePeriodLookup[ decay ];
	};

	callRelease = [this]() {

		state = S_RELEASE;
		ratePeriod = ratePeriodLookup[ release ];
	};

	callEnvelope = [this]() {

        if (lockEnvCounter)
            return;
        
		if (state == S_ATTACK) {

			++counter &= 0xff;
            
			if (counter == 0xff) 
                callDecay();            

		} else // Decay or Release
			--counter &= 0xff;
        
        updateExponentialPeriod();
	};
	
	callExponentialCounter = [this]() {
        
		exponentialCounter = 0;
        
        if ( ( ( state == S_DECAY ) && (counter != sustainComparator()) ) // decrease volume untill seted sustain value
            || ( state == S_RELEASE ) ) { // decrease volume untill silence
            
            events->add( &callEnvelope, 1 );
        }
	};	 
}

auto Sid::Envelope::registerCallbacks() -> void {
    
    events->registerCallback( { {&callAttack, 2}, {&callDecay, 2}, {&callRelease, 2}, {&callEnvelope, 2}, {&callExponentialCounter, 2} } );
}

inline auto Sid::Envelope::clock() -> void {
    // we update from counterTemp instead of counter, because the global event queue is running before
    // and could change the counter
	env3 = counterTemp; 
	
	if (resetRateCounter) {
		resetRateCounter = false;
		rateCounter = 0;
		
		if ( state == S_ATTACK ) {
            exponentialCounter = 0;
			events->add( &callEnvelope, 2 );  
			
		} else if (!lockEnvCounter) {
			
            if (++exponentialCounter == exponentialPeriod) //non linear volume decrease
                events->add( &callExponentialCounter, exponentialPeriod != 1 ? 2 : 1 );
		}
	}	
	
	else if (rateCounter == ratePeriod) {
		resetRateCounter = true;
		return;
	}
	
	++rateCounter &= 0x7fff; //15 bit counter	
	if (!rateCounter) // wrap around
		rateCounter = 1; 
    
    // env3 wil be updated in beginning of phase 1.
    // so don't do it here or a possible read between the half cycles get the wrong value
    counterTemp = counter;		
}

auto Sid::Envelope::updateExponentialPeriod() -> void {

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
