
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
    pcr = acr = 0;
    isShiftT2Control = false;
        
    ca1 = cb1 = 0;
    ca2 = cb2 = 0;

    shift.toggle = true;
    shift.irqTrigger = false;
    shift.active = false;
    shift.count = 0;
        
    timerACounterRead = timerACounter = timerALatch = (223 << 8) | 0xff;    
    timerBCounterRead = timerBCounter = 0xffff;
    timerBLatch = 0xff;
    
    timerATrigger = false;
    timerAToggle = false;
    
    timerBTrigger = false;     
    
    delay = 0;
}

// a transition of prb bit 6 advance timer B, when not in oneshot mode.
// not in use for drive 1541, via2 pb6 / pb5 reads drive ident.
// drive ident is always the same, so there is no pulse.
auto Via::pb6Pulse() -> void {
    
    if ( acr & 0x20 )
        delay |= VIA_STEP_TIMERB0;
}
// calls for transitions of ca1, ca2, cb1, cb2
auto Via::ca1In( bool state, bool irqNextCycle ) -> void {
    
    if (ca1 == state) // edge transition check
        return;
    
    ca1 = state;
    
    if (state != (pcr & 1) ) // ca1 control, wrong direction of transition
        return; 
    
    // when ca2 is in output mode
    if ((pcr & 0xe) == 8)
        ca2Out( ca2 = 1 );  // handshake output mode
    
    // latch port A
    if ( !(ifr & 2) )
        lines.latchA = readPort( Port::A, &lines );

    setIrq( 2, irqNextCycle ? VIA_UPDATE_IRQ0 : VIA_UPDATE_IRQ1 );
}

auto Via::ca2In( bool state, bool irqNextCycle ) -> void { // unused for drive 1541

    if (pcr & 8) // ca2 in output mode
        // don't update input
        return;
    
    if (ca2 == state) // edge transition check
        return;
    
    ca2 = state;
            
    if ( (state ? 4 : 0) != (pcr & 4) ) // ca2 control, wrong direction of transition
        return;
    
    setIrq( 1, irqNextCycle ? VIA_UPDATE_IRQ0 : VIA_UPDATE_IRQ1 );
}

auto Via::cb1In( bool state, bool irqNextCycle ) -> void { // unused for drive 1541

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
    
    // latch port B
    if ( !(ifr & 16) )
        lines.latchB = readPort( Port::B, &lines );

    setIrq( 16, irqNextCycle ? VIA_UPDATE_IRQ0 : VIA_UPDATE_IRQ1 );
}

auto Via::cb2In( bool state, bool irqNextCycle ) -> void { // unused for drive 1541
    
    if ( pcr & 0x80 ) // cb2 in output mode
        return;
            
    if (cb2 == state) // edge transition check
        return;
    
    cb2 = state;
    
    if ( (state ? 0x40 : 0) != (pcr & 0x40) ) // cb2 control, wrong direction of transition
        return;

    setIrq( 8, irqNextCycle ? VIA_UPDATE_IRQ0 : VIA_UPDATE_IRQ1 );
}
   
auto Via::process() -> void {  
    
    delay = (delay << 1) & VIA_MASK;
    
    if (shift.active)
        shifter();
    
    updateTimerA();
    updateTimerB();
    
    // shift in/out by system clock
    if (shiftSystemClock() && ((delay & VIA_SHIFT_WARMUP1) == 0) && (!shift.toggle || (shift.count != 8)) ) {
        shift.toggle ^= 1;    
        shiftTiming<true>( );                
    }    

    if (delay) {
        if (delay & VIA_CA2_PULSE1)
            ca2Out(ca2 = 1);
        else if (delay & VIA_CB2_PULSE1)
            cb2Out(cb2 = 1); 

        if (delay & VIA_UPDATE_IRQ2)
            handleInterrupt();
        
        if (delay & VIA_SHIFT_WARMUP1) {
            shift.toggle = true;
            cb1Out(cb1 = 1);
        }            
    }
}

inline auto Via::updateTimerA( ) -> void {
    timerACounterRead = timerACounter;
    
    if(delay & VIA_FORCE_LOAD_TIMERA1);
    
    else if (timerACounter == 0xffff) {
        delay |= VIA_FORCE_LOAD_TIMERA0;
        timerACounter = timerALatch;

        if (timerATrigger) {
            timerAToggle ^= 1;
            setIrq(64);
            
            // disable trigger in one shot mode only, a write in timer 1 counter hi is needed
            // to reactivate the trigger
            if (!(acr & 0x40))
                timerATrigger = false; 
        }
        
        return;
    }  
    
    timerACounter--;
}

inline auto Via::updateTimerB( ) -> void {
    timerBCounterRead = timerBCounter;

    if (timerBTrigger && (timerBCounter == 0xffff)) {
        setIrq(32);
        // disable trigger in one shot mode and pulse counting mode, a write in timer 2
        // counter hi is needed to reactivate the trigger
        timerBTrigger = false;
    }

    if(delay & VIA_FORCE_LOAD_TIMERB1);
    
    else if (isShiftT2Control) { // shift for timer B only
        // lasts: timer B latch + 2 cycles
        // i.e. latch = 3 -> 2, 1, 0, 0xff, reload counter = 5 cycles
        if ((timerBCounter & 0xff) == 0xff) { // counter is low byte only                       

            if (!(delay & VIA_SHIFT_WARMUP1) ) {
                if (!shift.toggle || (shift.count != 8)) {
                    shift.toggle ^= 1;
                    shiftTiming<true>();
                }
            }
            
            delay |= VIA_FORCE_LOAD_TIMERB0;
            timerBCounter = (timerBCounter & 0xff00) | timerBLatch;
            return;
        }
    }
    
    if ( !(acr & 0x20) || (delay & VIA_STEP_TIMERB1) )
        timerBCounter--;
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

inline auto Via::shiftCb1Control() -> bool {
    
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

auto Via::handleInterrupt( ) -> void {
    // inform cpu this cycle
    irqCall( (ier & ifr) != 0 );      
}

inline auto Via::setIrq( uint8_t pos, unsigned irqDelay ) -> void {
    ifr |= pos;
    delay |= irqDelay;
}

inline auto Via::resetIrq( uint8_t pos ) -> void {
    // inform cpu next cycle
    ifr &= ~pos;
    delay |= VIA_UPDATE_IRQ1;
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
