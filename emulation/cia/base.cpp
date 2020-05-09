
#include "base.h"
#include "register.cpp"

namespace CIA {
      
Base::Base( uint8_t model, Emulator::Events* events )
:
cra( timer[T_A].control ),
crb( timer[T_B].control )
{	
	this->model = model;
    this->events = events;
    
#ifndef CIA_GLOBAL_EVENTS     
    this->events = new Emulator::Events;
#endif
	
    readPort = []( Port port, Lines* plines ) { 
        // basic mode, when lines not modified from external
        return port == PORTA ? plines->ioa : plines->iob;        
    };
    
    writePort = []( Port, Lines* ) {};    
	irqCall = [](bool state) {};
    serialCall = [](bool bit) {};
	
	for( unsigned i = 0; i < 2; i++ ) {			       
		
		timer[i].start = [this,i]() { timer[i].run |= 1; };

		timer[i].step = [this,i]() { 
            
            if ( timer[i].control & 1 )
                timer[i].run |= 2;
        };

		timer[i].stop = [this,i]() { timer[i].run &= ~1; };

		timer[i].disableOneshot = [this,i]() { timer[i].oneshot = 0; };

		timer[i].forceLoad = [this,i]() { timer[i].forceloadCycle = 1; };
        
        events->registerCallback(
            { {&(timer[i].start), 1}, {&(timer[i].step), 1}, {&(timer[i].stop), 1}, {&(timer[i].disableOneshot), 1}, {&(timer[i].forceLoad), 2} }
        );
	}
    
    updateIcrAndSetIrq = [this]() {
        icr = icrTemp;
        irqCall( true );
    };
    
    updateIcrOnly = [this]() {
        icr = icrTemp;
    };        
    
	newVersion = true;
    
    events->registerCallback( { {&updateIcrAndSetIrq, 1}, {&updateIcrOnly, 1} } );

}

auto Base::reset() -> void {
    
    lines.pra = lines.prb = 0;    
    lines.ddra = lines.ddrb = 0;
	lines.ioa = lines.iob = 0xff;
    lines.praChange = lines.prbChange = 0;
	
    icr = icrmask = 0;
    sdr = shift = 0;
    shiftCount = 0;	
	
	sdrValid = false;
    cnt = true;
	ciaShiftRespawnBug = false;
	
	registerWrite.pipelined = false;
    
	acknowledgeCycle = 0;	
	maskWriteCycle = 0;
	flagRaised = false;
	intDelay = 0;
	serialDelay = 0;
    icrTemp = 0; 
	
	for( unsigned i = 0; i < 2; i++ ) {	
		timer[i].run = 0;
		timer[i].oneshot = 0;
		timer[i].underflowCycle = 0;
		timer[i].forceloadCycle = 0;
		timer[i].latch = timer[i].counter = 0xffff;
        timer[i].control = 0;
        timer[i].toggle = true;
	}
#ifndef CIA_GLOBAL_EVENTS    
    events->clear();
#endif    
}

auto Base::processLo() -> void {               
    
#ifndef CIA_GLOBAL_EVENTS      
	events->process();
#endif    
    // collect all incomming interrupt sources of this cycle
	icrTemp = 0; 
	
    updateState<T_B>( );
	updateState<T_A>( );	
	
	if (flagRaised) {
		flagRaised = false;
		handleInterrupt( 0x10 );
	}		
	
	processTod( );	
    
    if (serialDelay & 1)
        handleInterrupt( 8 );           
  	
	newVersion ? interruptControl() : interruptControlOld();
       
    serialDelay >>= 1; 
    acknowledgeCycle <<= 1;      
	maskWriteCycle <<= 1;    
}

auto Base::processHi() -> void {	
        	
    intDelay >>= 1;
	
    decrement<T_B>( );
	decrement<T_A>( );	
	 
	// cpu write access only, read access happens between the half cycles
	if ( registerWrite.pipelined ) {
		registerWrite.pipelined = false;
		write( registerWrite.addr, registerWrite.value );
	}
	
    // disable possible force load or underflow cycle
	timer[0].forceloadCycle = timer[1].forceloadCycle = false;
	timer[0].underflowCycle = timer[1].underflowCycle = false;
    // disable possible single step
	timer[0].run &= ~2;
	timer[1].run &= ~2;	     
}

inline auto Base::interruptControl() -> void {
    // for new cia models    
    if (intDelay & 1) {
        // interrupt is incomming and allowed by icr mask, a write in mask register
        // before can cause this situation too
        icrTemp |= 0x80;
        icr |= 0x80;
        
        if (acknowledgeCycle & 1) { // we have both at same time, interrupt and acknowledge cycle
            irqCall( false );
            // interrupt is scheduled for next cycle, so cpu can not recognize it this cycle.
            // icr is reseted next cycle too with zero or the interrupts incomming this cycle
            events->add( &updateIcrAndSetIrq, 1, Emulator::Events::UpdateExisting );
        } else
            // normal interrupt behaviour
            irqCall( true );
        
    } else if (acknowledgeCycle & 1) {
        // interrupt is not incomming or is not allowed by icr mask and
        // this is an acknowledge cycle
        irqCall( false );
        // we schedule to update icr next cycle, so a possible second read in a row
        // gets the non reseted state of icr.
        // in next cycle icr will be reseted with zero or the interrupts incomming this cycle
        events->add( &updateIcrOnly, 1, Emulator::Events::UpdateExisting );
    }                 
}

inline auto Base::interruptControlOld() -> void {    
    // for old cia models, interrupt is incomming one cycle later
    if (intDelay & 1) {
        
        if (acknowledgeCycle & 1) {
            // interrupt and acknowledge cycle at the same time.
            // msb is seted but there is no interrupt sended to cpu like new cia
            icr = 0x80;
            irqCall( false );   
            // icr is reseted next cycle with zero or the interrupts incomming this cycle
			events->add( &updateIcrOnly, 1, Emulator::Events::UpdateExisting );    
           
        } else {
            // normal interrupt behaviour
            icr |= 0x80;
            irqCall( true );
        }
        
    } else if (acknowledgeCycle & 1) {
        // same behaviour like new cia, but all interrupt sources will be reseted
        // but not the msb of icr, matters when a second read in register 0d happens
        icr &= ~0x7f;
		
        irqCall( false );
        
		events->add( &updateIcrOnly, 1, Emulator::Events::UpdateExisting );    
    }
}

auto Base::handleInterrupt( uint8_t number ) -> void {
    icr |= number;
	icrTemp |= number;

    if (( (number ? number : icr) & icrmask) == 0 ) {
		// for old cias interrupts are delayed by one cycle.
		// if an underflow happens a cycle sooner followed by a second write 
		// in a row to 0xd, which disables the mask, then
		// a scheduled interrupt is discarded.
		// can not happen for new cias
		if (number == 0) // write in icr mask register
			 if(maskWriteCycle & 2) // second write in mask register in a row
				intDelay = 0;
		
        return; 
	}

	intDelay |= newVersion ? 1 : 2;
}

template<uint8_t timerId> inline auto Base::decrement( ) -> void {
	Timer* pTimer = &timer[timerId];		
	
	// run: phase in or a single step in cascade mode
	if (pTimer->counter && pTimer->run)
		pTimer->counter--;
}

template<uint8_t timerId> inline auto Base::updateState( ) -> void {
	
	Timer* pTimer = &timer[timerId];	
	
	if ( pTimer->run && (pTimer->counter == 0) ) {
		pTimer->underflowCycle = pTimer->forceloadCycle = true;
		timerId == T_A ? timerAUnderflow() : timerBUnderflow();
	}
	
	if ( pTimer->forceloadCycle ) {
		pTimer->counter = pTimer->latch;
		
        if ( pTimer->run == 1 )        
            // if a timer stop event finishes the same cycle like restart after
            // underflow, the stop event should run after restart to get priority
            events->add( &(pTimer->start), 1, Emulator::Events::BeforeOthers );
        
        pTimer->run = 0;
	}
    
	if (pTimer->underflowCycle && pTimer->oneshot) {
        
        pTimer->control &= ~1;
		
		events->remove( &(pTimer->start) );
	}
}

auto Base::timerAUnderflow() -> void {    	

	if (cra & 0x40) //SP pin is defined as output
		serialOut();
	
	timer[T_A].toggle ^= 1;
	
	if ( (crb & 0x61) == 0x41 )
		events->add( &(timer[T_B].step), 1, Emulator::Events::UpdateExisting );
	
    else if ( cnt && ((crb & 0x61) == 0x61 ))
		events->add( &(timer[T_B].step), 1, Emulator::Events::UpdateExisting );
	
    handleInterrupt( 1 );
}	

auto Base::timerBUnderflow() -> void {

	timer[T_B].toggle ^= 1;

	handleInterrupt( 2 );
    
    // timer B bug for old cias
    // if timer B underflows in acknowledge cycle, it triggers an interrupt
    // next cycle like expected but the second bit in icr is not seted
    // note: acknowledge cycle is not the cycle when the read happened but the
    // cycle after
    if ( (acknowledgeCycle & 1) && !newVersion) {
        icrTemp &= ~2;
		icr &= ~2;        
    }
}

auto Base::setFlag() -> void {
	flagRaised = true;    
}

auto Base::setNewVersion( bool state ) -> void {    
    newVersion = state;
}

auto Base::isNewVersion() -> bool {
    return newVersion;
}

// following serial and cnt emulation is experimental
auto Base::serialOut() -> void {
	//timer A defines speed for this								
	
	if ( shiftCount ) {		
		cnt ^= 1;
		
		if (!cnt) {			
			bool bit = (shift >> 7) & 1;
			shift <<= 1;
			serialCall( bit );				

		} else {

			if (--shiftCount == 1) {
				// serial interrupt happens a few cycles after Timer A underflows
				serialDelay |= 16; // bit 3 -> 4 cycle delay
			}
			
		}		
	}    	
	
	if (sdrValid) {
		// first underflow loads shift register.
		// if shift is alredy in progress, it happens parallel and is valid next underflow
		sdrValid = false;
		shift = sdr;			
		shiftCount = 8;
		cnt = true;
	}	
}

auto Base::serialFlagRespawn() -> void {
	auto pT = &timer[T_A];
	
	// first line is not really understood
	if (!ciaShiftRespawnBug && cnt && pT->counter == pT->latch);

	else if (shiftCount == 1 && cnt && pT->counter == (pT->latch - 2));

	else if (cnt && (pT->counter != (pT->latch - 1))
		&& (newVersion ? (pT->counter != (pT->latch - 3)) : true)

		) {
		handleInterrupt(8);

	} else if (!cnt && (
		(pT->counter == (pT->latch - 4))
		|| (pT->counter == (pT->latch - 3))
		|| (pT->counter == (pT->latch - 2))
		|| (pT->counter == (pT->latch - 1))
		)) {
		handleInterrupt(8);
	};

	shiftCount = 0;

	ciaShiftRespawnBug = true;
}

/**
 * external device shifts in data bit by bit
 */
auto Base::serialIn( bool bit ) -> void {
    // external device generates pulse 0 -> 1 on cnt pin to inform cia
	// that sp pin has valid data
	// for simplicity we do both steps in one operation
    cnt = true;
	
    if (cra & 0x40) //SP pin is defined as output
        return;
    
    if ((cra & 0x21) == 0x21) //timer A is driven by cnt pin transition      
		events->add( &(timer[T_A].step), 1, Emulator::Events::UpdateExisting );
    
    if ( ( crb & 0x61) == 0x21) //timer B is driven by cnt pin transition
		events->add( &(timer[T_B].step), 1, Emulator::Events::UpdateExisting );
        
    shift <<= 1;
    shift |= bit;
    
    if ( ++shiftCount == 8 ) {
        shiftCount = 0;
        //transfer complete
		sdr = shift;
        serialDelay |= 16; 
    }
}

// set cnt external without serial bit shifting
auto Base::setCnt( bool state ) -> void {
    
    if (state && !cnt) {
        
        if ((cra & 0x21) == 0x21) //timer A is driven by cnt pin transition      
			events->add( &(timer[T_A].step), 1, Emulator::Events::UpdateExisting );

        if ( ( crb & 0x61) == 0x21) //timer B is driven by cnt pin transition
			events->add( &(timer[T_B].step), 1, Emulator::Events::UpdateExisting );
    }
    
	cnt = state;
}

auto Base::serialize(Emulator::Serializer& s) -> void {
    
    s.integer( lines.pra );
    s.integer( lines.prb );
    s.integer( lines.ddra );
    s.integer( lines.ddrb );
    s.integer( lines.ioa );
    s.integer( lines.iob );
    s.integer( lines.praChange );
    s.integer( lines.prbChange );
    
    for(unsigned i = 0; i < 2; i++) {
        
        Timer& t = timer[i];
        
        s.integer( t.run );
        s.integer( t.oneshot );
        s.integer( t.underflowCycle );
        s.integer( t.forceloadCycle );
        s.integer( t.latch );
        s.integer( t.counter );
        s.integer( t.control );
        s.integer( t.toggle );        
    }
    
    s.integer( newVersion );
    s.integer( acknowledgeCycle );
    s.integer( maskWriteCycle );
    s.integer( intDelay );
    s.integer( serialDelay );
    s.integer( icrTemp );
    s.integer( flagRaised );
    s.integer( sdr );
    s.integer( sdrValid );
	s.integer( ciaShiftRespawnBug );
    s.integer( cnt );
    s.integer( shift );
    s.integer( shiftCount );
    s.integer( icrmask );
    s.integer( icr );
    s.integer( registerWrite.pipelined );
    s.integer( registerWrite.addr );
    s.integer( registerWrite.value );
}

}
