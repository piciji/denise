
#include "via.h"
#include "register.cpp"
#include "serialization.cpp"


namespace LIBC64 {

Via::Via( uint8_t model ) {	
    
	this->model = model;
	
    readPort = []( Port port, Lines* plines ) { 
        // basic mode, when lines not modified from external
        return port == Port::A ? plines->ioa : plines->iob;        
    };
    
    writePort = []( Port, Lines* ) {};    
	irqCall = [](bool state) {};
    
    // 'unused line' is default behaviour: external device use pullup resistors to keep these lines always high
    // i assume an ouptut of 0 from via side doesn't change the line while external device pulls this line up ???    
    // override these callbacks if external device uses these lines
    ca2Out = [this](bool state) { ca2 = 1; };
    cb2Out = [this](bool state) { cb2 = 1; };
    cb1Out = [this](bool state) { cb1 = 1; };    	
}

auto Via::reset() -> void {
    
    lines.pra = lines.prb = 0;    
    lines.ddra = lines.ddrb = 0;
	lines.ioa = lines.iob = 0xff;
    lines.ioaOld = lines.iobOld = 0xff;
    lines.latchA = lines.latchB = 0;
    
    sdr = 0;  // not reseted
    
    ifr = ier = 0;
    updateIrq = 0;
    pcr = acr = 0;
    ca2StatePulse = cb2StatePulse = 0;	
	registerWrite.pipelined = false;
    isShiftT2Control = false;
        
    ca1 = cb1 = 0;
    ca2 = cb2 = 0;

    shift.warmUp = false;
    shift.toggle = true;
    shift.irqTrigger = false;
    shift.active = false;
    shift.count = 0;
        	
	for( unsigned i = 0; i < 2; i++ ) {	
		timer[i].forceloadCycle = 0;		
        timer[i].counterUpdated = false;
        
        if (i == 0)
            timer[i].latch = timer[i].counter = (223 << 8) | 0xff;
        else
            timer[i].latch = timer[i].counter = 0xffff;
        
        timer[i].toggle = 0;
        timer[i].step = 0;
        timer[i].trigger = 0;
	}
}

// a transition of prb bit 6 advance timer B, when not in oneshot mode.
// not in use for drive 1541, via2 pb6 / pb5 reads drive ident.
// drive ident is always the same, so there is no pulse.
auto Via::pb6Pulse() -> void {
    
    if ( acr & 0x20 )
        timer[Timer::B].step = true;   
}
// calls for transitions of ca1, ca2, cb1, cb2
auto Via::ca1In( bool state ) -> void {
    
    if (ca1 == state) // edge transition check
        return;
    
    ca1 = state;
    
    if (state != (pcr & 1) ) // ca1 control, wrong direction of transition
        return; 
    
    // when ca2 is in output mode
    if ((pcr & 0xe) == 8)
        ca2Out( ca2 = 1 );  // handshake output mode     
    
    // pure assumption:
    // input irqs comming in first half cycle are detected by cpu this cycle
    // input irqs comming in second half cycle are detected by cpu next cycle
    // i.e. drive 1541 g64 accuracy mode could input ca1 in any half cycle
    setIrq( 2 );
    
    // latch port A
    lines.latchA = readPort( Port::A, &lines );	   
}

auto Via::ca2In( bool state ) -> void { // unused for drive 1541

    if (pcr & 8) // ca2 in output mode
        // don't update input
        return;
    
    if (ca2 == state) // edge transition check
        return;
    
    ca2 = state;
            
    if ( (state ? 4 : 0) != (pcr & 4) ) // ca2 control, wrong direction of transition
        return;
    
    setIrq( 1 );
}

auto Via::cb1In( bool state ) -> void { // unused for drive 1541

    if (cb1 == state) // edge transition check
        return;

    cb1 = state;
        
    if ( shiftCb1Control() )
        shiftTiming<false>( );
    
    if ( (state ? 0x10 : 0) != (pcr & 0x10) )
        return; // cb1 control, wrong direction of transition
    
    // when cb2 is in output mode
    if ((pcr & 0xe0) == 0x80)
        cb2Out( cb2 = 1 ); // handshake output mode 
    
    setIrq( 16 );
    
    // latch port B
    lines.latchB = readPort( Port::B, &lines );		
}

auto Via::cb2In( bool state ) -> void { // unused for drive 1541
    
    if ( pcr & 0x80 ) // cb2 in output mode
        return;
            
    if (cb2 == state) // edge transition check
        return;
    
    cb2 = state;
    
    if ( (state ? 0x40 : 0) != (pcr & 0x40) ) // cb2 control, wrong direction of transition
        return;

    setIrq( 8 );
}
   
auto Via::processLo() -> void {              
    
    shifter();
    
    ca2StatePulse >>= 1;
    cb2StatePulse >>= 1;
        
    updateState<Timer::A>( );
	updateState<Timer::B>( );        
    
    handleSystemClockShift();    
    
    if (shift.warmUp) {
        shift.warmUp = false;  
        shift.toggle = true;
        cb1Out( cb1 = 1 );
    }       
        
    if (updateIrq) {
        updateIrq = 0;
        handleInterrupt();
    }  
}

// Note: a possible register read runs here between the half cycles

auto Via::processHi() -> void {   
    
    decrement<Timer::A>( );
	decrement<Timer::B>( );      
    
    if ( registerWrite.pipelined ) {
		registerWrite.pipelined = false;
		write( registerWrite.addr, registerWrite.value );
	}
    
    if (ca2StatePulse & 1)
        ca2Out( ca2 = 1 ); 
    
    if (cb2StatePulse & 1)
        cb2Out( cb2 = 1 );   
    
    timer[Timer::A].forceloadCycle = timer[Timer::B].forceloadCycle = 0;
    timer[Timer::B].step = false;        
}


template<unsigned timerId> inline auto Via::updateState( ) -> void {
	
    Timer* pT = &timer[timerId];
        
    if (pT->counterUpdated) {
        pT->counterUpdated = false;
        // the cycle after updating the counter from latch doesn't check for overflows.
        // otherwise a latch of 0xffff would overflow each cycle without counting down.
        return;
    }
    
    if ( (timerId == Timer::B) && isShiftT2Control ) { // shift for timer B only
        // lasts: timer B latch + 2 cycles
        // i.e. latch = 3 -> 2, 1, 0, 0xff, reload counter = 5 cycles
        if ( (pT->counter & 0xff) == 0xff ) { // counter is low byte only                       

            if (!shift.warmUp) {
                if (!shift.toggle || (shift.count != 8)) {
                    shift.toggle ^= 1;
                    shiftTiming<true>( );
                } 
            }
            pT->forceloadCycle = 2; // reload 8 bit low counter    
        }       
    }
        
	if (pT->counter == 0xffff) {        
        
        if ( timerId == Timer::A ) {
            pT->forceloadCycle = 1;
            
            if (pT->trigger) {
                pT->toggle ^= 1;
                setIrq( 64 );
            }                
            
            // disable trigger in one shot mode only, a write in timer 1 counter hi is needed
            // to reactivate the trigger
            if ( !(acr & 0x40) )
                pT->trigger = false; 
            
        } else {
            
            if (pT->trigger)   
                setIrq( 32 );
            
            // disable trigger in one shot mode and pulse counting mode, a write in timer 2
            // counter hi is needed to reactivate the trigger
            pT->trigger = false; 
        }            
	}
}

template<unsigned timerId> inline auto Via::decrement( ) -> void {	
    Timer* pT = &timer[timerId];	
    
    // counter is updated by latch this cycle, so no decrementing
    if ( pT->forceloadCycle & 1 ) {
        pT->counter = pT->latch;
        pT->counterUpdated = true;
        return;
        
    } else if ( pT->forceloadCycle & 2 ) { // T2 shift for timer B only
        pT->counter = (pT->counter & 0xff00) | (pT->latch & 0xff);
        pT->counterUpdated = true;               
        return;
    }
    // timer A decrements always
    // timer B decrements in oneshot mode only, otherwise a pulse on pb6 is needed ( single step )   
    if ( (timerId == Timer::A) || ( !(acr & 0x20) ) || pT->step )
        pT->counter--;    
}

inline auto Via::shiftT2FreeRunning() -> bool {
    
    return (acr & 0x1c) == 0x10; // shift out free running under T2 control
}

inline auto Via::shiftT2Control() -> bool {
    
    if (((acr & 0x0c) == 0x04) // shift in/out under T2 control
    || shiftT2FreeRunning() ) // shift out free running under T2 control
        return true;
    
    return false;
}

auto Via::shiftCb1Control() -> bool {
    
    if ( (acr & 0xc) == 0xc ) // serial shift is under cb1 control
        return true;    
    
    return false;
}

inline auto Via::shiftDisabled() -> bool {
    
    return (acr & 0x1c) == 0;
}

inline auto Via::shiftSystemClock() -> bool {
    
    return (acr & 0x0c) == 0x08;
}

inline auto Via::shiftOut() -> bool {
    
    return acr & 0x10;
}

auto Via::shifter( ) -> void {
    
    if (!shift.active)
        return;
    
    shift.active = false;
        
    if ( shiftOut() ) { // shift out modes

        cb2 = (sdr >> 7) & 1;
        
        cb2Out( cb2 );

        // sdr is rolled left
        sdr = ((sdr << 1 ) & 0xfe) | ((sdr >> 7) & 1);

    } else { // shift in modes

        // cb1 generates an out going pulse to inform external device to put next bit on cb2
        sdr = (sdr << 1) | cb2;
    }  
    
    if ( shiftT2FreeRunning() || shiftDisabled() )
        // no interrupts, no counter updates
        return;
    
    if ( ++shift.count == 8 ) {
        
        // don't stop when byte complete for these modes
        if ( shiftCb1Control() )
            shift.count = 0;
        
        if ( !shiftOut() )
            // we are a cycle after last positive going edge of cb1
            setIrq(4);
        else
            // we are a cycle after last negative going edge.
            // shift out lasts until next positive going edge.
            // afterwards the irq will be set. so remeber it.
            shift.irqTrigger = true;                
    }    
}

inline auto Via::handleInterrupt( ) -> void {
    // inform cpu this cycle
    irqCall( (ier & ifr) != 0 );      
}

inline auto Via::setIrq( uint8_t pos ) -> void {
    // inform cpu next cycle
    ifr |= pos;
    updateIrq = 1;
}

inline auto Via::resetIrq( uint8_t pos ) -> void {
    // inform cpu next cycle
    ifr &= ~pos;
    updateIrq = 1;
}

inline auto Via::handleSystemClockShift() -> void {
    // shift in/out by system clock
    if (!shiftSystemClock() || shift.warmUp)
        return;
             
    if ( !shift.toggle || (shift.count != 8) ) {        
        shift.toggle ^= 1;    
        shiftTiming<true>( );        
    }        
}

template<bool cb1Output> inline auto Via::shiftTiming() -> void {

    if (cb1Output)      
        cb1 = shift.toggle;
      
    if ( shiftOut() ) {
        
        if (!cb1) {       
            // shift out happens one cycle after each negative going edge
            shift.active = true;
        } else if (shift.irqTrigger) {
            shift.irqTrigger = false;
            setIrq(4);                   
        }
        
    } else if (cb1)
        // shift in happens one cycle after each positive going edge
        shift.active = true;    
    
    if (cb1Output)
        cb1Out( cb1 );
}

}
