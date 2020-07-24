
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
        
        timer[i].disableForceLoad = [this,i]() { timer[i].forceloadCycle = 0; };
        
        events->registerCallback(
            { {&(timer[i].start), 1}, {&(timer[i].step), 1}, {&(timer[i].stop), 1}, {&(timer[i].disableOneshot), 1},
                {&(timer[i].forceLoad), 2}, {&(timer[i].disableForceLoad), 1} }
        );
	}
    
    updateIcrAndSetIrq = [this]() {
        icr = icrTemp;
        irqCall( true );
    };
    
    updateIcrOnly = [this]() {
        icr = icrTemp;
    };   
	
	startSdr = [this]() {
		
		if (!sdrLoaded) {
			sdrShift = sdr;
			sdrLoaded = true;
		} else
			sdrPending = true;
	};
	
	finishSdr = [this]() {
		sdrFlag = true;
	};
    
	flipCnt = [this]() {
		if (!sdrShiftCount)
			return;		
		
		if (!cnt)
			positiveCntTransition();
		
		cnt ^= 1;
				
		if (cnt)			
			sdrShift <<= 1;
		else
			serialCall( (sdrShift & 0x80) != 0 );			

		if (--sdrShiftCount == 1) {
			this->events->add(&finishSdr, 2, Emulator::Events::UpdateExisting);

			if (sdrPending) {
				sdrPending = false;
				sdrShift = sdr;
				sdrLoaded = true;
			} else
				sdrLoaded = false;
		}			
	};
	
	flipDummy = [this]() {};
	
	newVersion = true;
    
    events->registerCallback( { {&updateIcrAndSetIrq, 1}, {&updateIcrOnly, 1}, {&startSdr, 1}, {&finishSdr, 1}, {&flipCnt, 1}, {&flipDummy, 1}  } );
}

auto Base::reset() -> void {
    
    lines.pra = lines.prb = 0;    
    lines.ddra = lines.ddrb = 0;
	lines.ioa = lines.iob = 0xff;
    lines.praChange = lines.prbChange = 0;
	
    icr = icrmask = 0;
    sdr = sdrShift = 0;
    sdrShiftCount = 0;	
	
	sdrPending = false;
	sdrLoaded = false;
    cnt = true;
	cntHistory = 0;
	sdrForceFinish = false;
    
	acknowledgeCycle = 0;	
	maskWriteCycle = 0;
	flagRaised = false;
	intDelay = 0;
	sdrFlag = false;
    icrTemp = 0; 
	
	for( unsigned i = 0; i < 2; i++ ) {	
		timer[i].run = 0;
		timer[i].oneshot = 0;
		timer[i].underflowCycle = 0;
		timer[i].forceloadCycle = 0;
		timer[i].latch = timer[i].counter = timer[i].counterRead = 0xffff;
        timer[i].control = 0;
        timer[i].toggle = true;
	}
#ifndef CIA_GLOBAL_EVENTS    
    events->clear();
#endif    
}

auto Base::clock() -> void {
    // don't disable it at cycle end, because of a possible register write afterwards
    timer[0].underflowCycle = timer[1].underflowCycle = false;
        
#ifndef CIA_GLOBAL_EVENTS      
    events->process();
#endif    
    // collect all incomming interrupt sources of this cycle
    icrTemp = 0;

    updateState<T_B>();
    updateState<T_A>();

    if (flagRaised) {
        flagRaised = false;
        handleInterrupt(0x10);
    }

    processTod();

    if (sdrFlag) {
		sdrFlag = false;
        handleInterrupt(8);
	}

    newVersion ? interruptControl() : interruptControlOld();

    acknowledgeCycle <<= 1;
    maskWriteCycle <<= 1;  			
	cntHistory = (cntHistory << 1) | cnt;

    // hi cycle
    intDelay >>= 1;

    decrement<T_B>();
    decrement<T_A>();

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
	pTimer->counterRead = pTimer->counter;
    
	// run: phase in or a single step in cascade mode
	if (pTimer->run)
		pTimer->counter--;
}

template<uint8_t timerId> inline auto Base::updateState( ) -> void {
	
	Timer* pTimer = &timer[timerId];	
	
	if ( pTimer->run && (pTimer->counter == 0) ) {
		pTimer->underflowCycle = pTimer->forceloadCycle = true;
		timerId == T_A ? timerAUnderflow() : timerBUnderflow();
	}	
	
	if ( pTimer->forceloadCycle ) {
        // a possible force load placed by register write get priority, so run this sooner
        events->add( &(pTimer->disableForceLoad), 1, Emulator::Events::BeforeOthers ); 
        
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
	
	if (cra & 0x40)
		serialOut();
	
	timer[T_A].toggle ^= 1;
	
	if ( (crb & 0x61) == 0x41 )
		events->add( &(timer[T_B].step), 1, Emulator::Events::UpdateExisting );
	
    else if ( (cntHistory & 2) && ((crb & 0x61) == 0x61 ))
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

auto Base::serialOut() -> void {
	//timer A defines speed for this		
	
	if ( sdrLoaded && !sdrShiftCount)
		sdrShiftCount = 16;	
		
	if (!sdrShiftCount)
		return;
	
	if ( events->has(&flipDummy) || events->has(&flipCnt) )
		events->add( &flipDummy, 2 ); // you need at least one cycle delay to detect a new transition
	else
		events->add( &flipCnt, 2 );
}

/**
 * external device shifts in data bit by bit
 */
auto Base::serialIn( bool newCnt, bool bit ) -> void {
	
	if (newCnt == cnt)
		return;
	
    if (cra & 0x40) //SP pin is defined as output
        return;    
	
	cnt = newCnt;
	
	if (!cnt)
		return;
	
	positiveCntTransition();
	sdrShift <<= 1;
	sdrShift |= bit;		        	
    
    if ( ++sdrShiftCount == 8 ) {
        sdrShiftCount = 0;
        //transfer complete
		sdr = sdrShift;
        this->events->add(&finishSdr, 2, Emulator::Events::UpdateExisting);
    }
}

// set cnt external without serial bit shifting
auto Base::positiveCntTransition( ) -> void {
           
	if ((cra & 0x21) == 0x21) //timer A is driven by cnt pin transition      
		events->add( &(timer[T_A].step), 2, Emulator::Events::UpdateExisting );

	if ( ( crb & 0x61) == 0x21) //timer B is driven by cnt pin transition
		events->add( &(timer[T_B].step), 2, Emulator::Events::UpdateExisting );
}

auto Base::switchSerialDirection(bool input) -> void {
	
	if (input) { 
		if (!newVersion)
			sdrForceFinish = (cntHistory & 0x7) != 0x7;
		else
			sdrForceFinish = (cntHistory & 0x6) != 0x6;

		if (!sdrForceFinish) {
			if (sdrShiftCount != 2 && (events->delay(&flipCnt) == 1)  )
				sdrForceFinish = true;
		}

	} else {

		if (!cnt && sdrShiftCount)
			sdrShift <<= 1;

		if (sdrForceFinish) {
			events->add( &finishSdr, 2, Emulator::Events::UpdateExisting );                
			sdrForceFinish = false;
		}
	}

	if (!cnt)
		positiveCntTransition();				

	cnt = true;
	cntHistory |= 1;

	events->remove(&flipCnt);
	events->remove(&flipDummy);
	sdrShiftCount = 0;
	sdrPending = false;
	sdrLoaded = false;
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
        s.integer( t.counterRead );
        s.integer( t.control );
        s.integer( t.toggle );        
    }
    
    s.integer( newVersion );
    s.integer( acknowledgeCycle );
    s.integer( maskWriteCycle );
    s.integer( intDelay );
    s.integer( sdrFlag );
    s.integer( icrTemp );
    s.integer( flagRaised );
    s.integer( sdr );
	s.integer( sdrFlag );
    s.integer( sdrLoaded );
	s.integer( sdrPending );
    s.integer( cnt );
	s.integer( cntHistory );
    s.integer( sdrShift );
    s.integer( sdrShiftCount );
	s.integer( sdrForceFinish );
    s.integer( icrmask );
    s.integer( icr );
}

}
