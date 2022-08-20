
#define CIA_MASK_WRITE0 1
#define CIA_MASK_WRITE1 2

#define CIA_ACK0	4
#define CIA_ACK1	8

#define CIA_INT0	0x10
#define CIA_INT1	0x20

#define CIA_CNT0	0x40
#define CIA_CNT1	0x80
#define CIA_CNT2	0x100

// force load last one cycle only, triggered by an underflow or forced by control register
#define CIA_FL_TA0  0x200
#define CIA_FL_TA1  0x400
#define CIA_FL_TA2  0x800

// underlow lasts one cycle, pb 6/7 may need to know
#define CIA_UF_TA0  0x1000
#define CIA_UF_TA1  0x2000

#define CIA_FL_TB0  0x4000
#define CIA_FL_TB1  0x8000
#define CIA_FL_TB2  0x10000

#define CIA_UF_TB0  0x20000
#define CIA_UF_TB1  0x40000

#define CIA_INT		(CIA_INT0 | CIA_INT1)
#define CIA_CNT		(CIA_CNT0 | CIA_CNT1 | CIA_CNT2)
#define CIA_CNT_NEW	(CIA_CNT1 | CIA_CNT2)

#define CIA_MASK	~(0x80000 | CIA_MASK_WRITE0 | CIA_ACK0 | CIA_INT0 | CIA_CNT0 | CIA_FL_TA0 | CIA_UF_TA0 | CIA_FL_TB0 | CIA_UF_TB0 )

#include "base.h"
#include "register.cpp"

namespace CIA {
      
Base::Base( uint8_t model, Emulator::SystemTimer* events )
:
cra( timer[T_A].control ),
crb( timer[T_B].control )
{	
	this->model = model;
    this->events = events;
	
    readPort = []( Port port, Lines* plines ) { 
        // basic mode, when lines not modified from external
        return port == PORTA ? plines->ioa : plines->iob;        
    };
    
    writePort = []( Port, Lines* ) {};    
	irqCall = [](bool state) {};
    serialOut = [](bool spLine, bool cntLine) {};
	
	for( unsigned i = 0; i < 2; i++ ) {			       
		
		timer[i].start = [this,i]() { timer[i].run |= 1; };

		timer[i].step = [this,i]() { 
            
            if ( timer[i].control & 1 ) {
                timer[i].run |= 2;
				this->events->add( &(timer[i].stepOut), 1 );
			}
        };
		
		timer[i].stepOut = [this,i]() { timer[i].run &= ~2; };

		timer[i].stop = [this,i]() { timer[i].run &= ~1; };

		timer[i].disableOneshot = [this,i]() { timer[i].oneshot = 0; };
        
        events->registerCallback(
            { {&(timer[i].start), 1}, {&(timer[i].step), 1},{&(timer[i].stepOut), 1}, {&(timer[i].stop), 1}, {&(timer[i].disableOneshot), 1} }
        );
	}
    
    updateIcrAndSetIrq = [this]() {
        icr = icrTemp;
		// have changed CPU code to detect nmi/irq changes not at fixed point (within cycle) anymore. (for speed reasons)
		// that is not a problem for IRQ but for NMI, because it's edge sensitive.
		// so we have to make sure, the final state of the NMI line is transmitted within a cycle.
		// otherwise an unwanted state change could happen, when calling the NMI line more times a cycle
		if (!(delay & CIA_ACK0))
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
		intIncomming |= 8;
	};
    
	flipCnt = [this]() {
		if (!sdrShiftCount)
			return;		
		
		if (!cnt)
			positiveCntTransition();
		
		cnt ^= CIA_CNT0;
				
		if (cnt)			
			sdrShift <<= 1;
		else
			serialOut( (sdrShift & 0x80) != 0, false );

		if (--sdrShiftCount == 1) {
			this->events->add(&finishSdr, 2, Emulator::SystemTimer::Action::UpdateExisting);

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
	lines.ioaOld = lines.iobOld = 0xff;
    lines.praChange = lines.prbChange = 0;
	
    icr = icrmask = 0;
    sdr = sdrShift = 0;
    sdrShiftCount = 0;	
	
	sdrPending = false;
	sdrLoaded = false;
    cnt = CIA_CNT0;
	sdrForceFinish = false;
    
	delay = 0;
    icrTemp = 0; 
	intIncomming = 0;
	
	for( unsigned i = 0; i < 2; i++ ) {	
		timer[i].run = 0;
		timer[i].oneshot = 0;
		timer[i].latch = timer[i].counter = 0xffff;
        timer[i].control = 0;
        timer[i].toggle = true;
	}   
}

auto Base::clock() -> void {
           
    // collect all incomming interrupt sources of this cycle
    icrTemp = 0;

    updateState<T_B>();
    updateState<T_A>();

	if (unlikely(intIncomming)) {
		handleInterrupt( intIncomming );
		intIncomming = 0;
	}	

	if (delay & (CIA_INT1 | CIA_ACK0))
		newVersion ? interruptControl() : interruptControlOld();
	
	delay = ((delay << 1) & CIA_MASK) | cnt;
}

inline auto Base::interruptControl() -> void {
    // for new cia models    
    if (delay & CIA_INT1) {
        // interrupt is incomming and allowed by icr mask, a write in mask register
        // before can cause this situation too
        icrTemp |= 0x80;
        icr |= 0x80;
        
        if (delay & CIA_ACK0) { // we have both at same time, interrupt and acknowledge cycle
            irqCall( false );
            // interrupt is scheduled for next cycle, so cpu can not recognize it this cycle.
            // icr is reseted next cycle too with zero or the interrupts incomming this cycle
            events->add( &updateIcrAndSetIrq, 1, Emulator::SystemTimer::Action::UpdateExisting );
        } else
            // normal interrupt behaviour
            irqCall( true );
        
    } else /*if (delay & CIA_ACK0)*/ {
        // interrupt is not incomming or is not allowed by icr mask and
        // this is an acknowledge cycle
        irqCall( false );
        // we schedule to update icr next cycle, so a possible second read in a row
        // gets the non reseted state of icr.
        // in next cycle icr will be reseted with zero or the interrupts incomming this cycle
        events->add( &updateIcrOnly, 1, Emulator::SystemTimer::Action::UpdateExisting );
    }                 
}

inline auto Base::interruptControlOld() -> void {    
    // for old cia models, interrupt is incomming one cycle later
    if (delay & CIA_INT1) {
        
        if (delay & CIA_ACK0) {
            // interrupt and acknowledge cycle at the same time.
            // msb is seted but there is no interrupt sended to cpu like new cia
            icr = 0x80;
            irqCall( false );   
            // icr is reseted next cycle with zero or the interrupts incomming this cycle
			events->add( &updateIcrOnly, 1, Emulator::SystemTimer::Action::UpdateExisting );    
           
        } else {
            // normal interrupt behaviour
            icr |= 0x80;
            irqCall( true );
        }
        
    } else /*if (delay & CIA_ACK0)*/ {
        // same behaviour like new cia, but all interrupt sources will be reseted
        // but not the msb of icr, matters when a second read in register 0d happens
        icr &= ~0x7f;
		
        irqCall( false );
        
		events->add( &updateIcrOnly, 1, Emulator::SystemTimer::Action::UpdateExisting );    
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
			 if(delay & CIA_MASK_WRITE1) // second write in mask register in a row
				delay &= ~CIA_INT;
		
        return; 
	}

	delay |= newVersion ? CIA_INT1 : CIA_INT0;
}

template<uint8_t timerId> inline auto Base::updateState( ) -> void {
	
	Timer& rTimer = timer[timerId];

    if ( rTimer.run ) {
        if (rTimer.counter == 0) {
            delay |= timerId == T_A ? (CIA_UF_TA0 | CIA_FL_TA1) : (CIA_UF_TB0 | CIA_FL_TB1);

            timerId == T_A ? timerAUnderflow() : timerBUnderflow();

            if (rTimer.oneshot) {

                rTimer.control &= ~1;

                events->remove(&(rTimer.start));

                rTimer.run = 0;
            }

            rTimer.counter = rTimer.latch;
        }
        else if ( delay & ( timerId == T_A ? CIA_FL_TA1 : CIA_FL_TB1 ) )
            rTimer.counter = rTimer.latch;
        else
            rTimer.counter--;

    } else if ( delay & ( timerId == T_A ? CIA_FL_TA1 : CIA_FL_TB1 ) )
        rTimer.counter = rTimer.latch;
}

template<uint8_t timerId> inline auto Base::readCounter( ) -> uint16_t {
	Timer& rTimer = timer[timerId];
	
    if (rTimer.run && !(delay & ( timerId == T_A ? CIA_FL_TA2 : CIA_FL_TB2 )) )
		return (rTimer.counter + 1);
	
	return rTimer.counter;
}

auto Base::timerAUnderflow() -> void {    	
	if (cra & 0x40)
        shiftOut();
	
	timer[T_A].toggle ^= 1;
	
	if ( (crb & 0x61) == 0x41 )
		events->add( &(timer[T_B].step), 1, Emulator::SystemTimer::Action::UpdateExisting );    
	
    else if ( (delay & CIA_CNT1) && ((crb & 0x61) == 0x61 ))
		events->add( &(timer[T_B].step), 1, Emulator::SystemTimer::Action::UpdateExisting );    
	
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
    if ( (delay & CIA_ACK0) && !newVersion) {
        icrTemp &= ~2;
		icr &= ~2;        
    }
}

auto Base::setFlag() -> void {
	intIncomming |= 0x10;
}

auto Base::setNewVersion( bool state ) -> void {    
    newVersion = state;
}

auto Base::isNewVersion() -> bool {
    return newVersion;
}

auto Base::shiftOut() -> void {
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
 * external device shifts in data bit by bit, assumes raising CNT line
 */
auto Base::serialIn( bool bit ) -> void {
	
    if (cra & 0x40) //SP pin is defined as output
        return;    

    cnt = CIA_CNT0;
    delay |= CIA_CNT0;
	
	positiveCntTransition();
	sdrShift <<= 1;
	sdrShift |= bit;		        	
    
    if ( ++sdrShiftCount == 8 ) {
        sdrShiftCount = 0;
		sdr = sdrShift;
        this->events->add(&finishSdr, 2, Emulator::SystemTimer::Action::UpdateExisting);
    }
}

// set cnt external without serial bit shifting
auto Base::positiveCntTransition( ) -> void {
           
	if ((cra & 0x21) == 0x21) //timer A is driven by cnt pin transition      
		events->add( &(timer[T_A].step), 2, Emulator::SystemTimer::Action::UpdateExisting );

	if ( ( crb & 0x61) == 0x21) //timer B is driven by cnt pin transition
		events->add( &(timer[T_B].step), 2, Emulator::SystemTimer::Action::UpdateExisting );
}

auto Base::switchSerialDirection(bool input) -> void {
	
	if (input) { 
		if (!newVersion)
			sdrForceFinish = (delay & CIA_CNT) != CIA_CNT;
		else
			sdrForceFinish = (delay & CIA_CNT_NEW) != CIA_CNT_NEW;

		if (!sdrForceFinish) {
			if (sdrShiftCount != 2 && (events->delay(&flipCnt) == 1)  )
				sdrForceFinish = true;
		}

	} else {

		if (!cnt && sdrShiftCount)
			sdrShift <<= 1;

		if (sdrForceFinish) {
			events->add( &finishSdr, 2, Emulator::SystemTimer::Action::UpdateExisting );                
			sdrForceFinish = false;
		}
	}

	if (!cnt)
		positiveCntTransition();				

	cnt = CIA_CNT0;
	delay |= CIA_CNT0;
    serialOut(input, true);

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
	s.integer( lines.ioaOld );
	s.integer( lines.iobOld );
	s.integer( lines.praChange );
    s.integer( lines.prbChange );
    
    for(unsigned i = 0; i < 2; i++) {
        
        Timer& t = timer[i];
        
        s.integer( t.run );
        s.integer( t.oneshot );
        s.integer( t.latch );
        s.integer( t.counter );
        s.integer( t.control );
        s.integer( t.toggle );        
    }
    
	s.integer( delay );
    s.integer( newVersion );
    s.integer( icrTemp );
    s.integer( sdr );
    s.integer( sdrLoaded );
	s.integer( sdrPending );
    s.integer( cnt );
    s.integer( sdrShift );
    s.integer( sdrShiftCount );
	s.integer( sdrForceFinish );
    s.integer( icrmask );
    s.integer( icr );
	s.integer( intIncomming );
}

}

