
#include "m6510.h"
#include "../system/system.h"
#include "../expansionPort/expansionPort.h"

#define FALL_OFF_CYCLES 350000

namespace LIBC64 {	
	
M6510* cpu = nullptr;	
	
#include "opcodes.cpp"	
	
M6510::M6510() {
	
	unChargeBit6 = [this]() { bit6charge = 0; };
	unChargeBit7 = [this]() { bit7charge = 0; };
	
	sysTimer.registerCallback( { { &unChargeBit6, 1 }, { &unChargeBit7, 1 } } );
}	

auto M6510::power() -> void {

	regS = 0x00;
	//some of these values could be random on first power on
	regX = 0x00;
	regY = 0x00;
	regA = 0xaa;
	pc = 0x00ff;	
	SET_STATUS( 0x02 )

	reset();
}

auto M6510::reset() -> void {
	
	irqPending = nmiPending =
	nmiDetect = interruptSampled = false;
	
	rdyLine = false;
	
	killed = false;

	ddr = 0; //input mode
	por = 0;
	ioLines = 0;
	
	busState = 0;

	bit6charge = bit7charge = 0;
	
	callResetRoutine = true;
}

auto M6510::resetRoutine() -> void {
	
	uint8_t dataBus;
	
	READ( pc )
	READ( pc )
	READ( pc )
	READ( 0x100 | regS-- )
	READ( 0x100 | regS-- )
	READ( 0x100 | regS-- )
		
	READ( 0xfffc )
	pc = dataBus;
	READ( 0xfffd )
	pc |= dataBus << 8;
	SET_FLAG_I( 1 )
	
	callResetRoutine = false;
}

template<bool software> inline auto M6510::interrupt() -> void {	
	
	uint8_t dataBus;
	
	if (!software) {
		READ( pc )
		READ( pc )
	} else {
		READ_PC_INC
	}
	
	PUSH((pc >> 8) & 0xff);
	PUSH( pc & 0xff);

	uint16_t vector = 0xfffe;

	/**
	 * NOTE: new pending nmi's recognized in the beginning of this service routine
	 * (software break too) will be lost
	 */
	if (nmiPending) {
		nmiPending = false;
		vector = 0xfffa; // a late nmi can hijack irq
	}

	SET_FLAG_B( software )
	
	PUSH( STATUS );
	
	READ( vector )
	
	pc = dataBus;
	
	SET_FLAG_I( 1 )

	READ( vector + 1 )
		
	pc |= dataBus << 8;
	
	/**	 
	 * no interrupt polling at the end of this service routine (software break too)
	 * so at least one opcode is following before could interrupted again by nmi
	 */

	interruptSampled = false;
}

auto M6510::setIrq(bool state) -> void {
	// level sensitive
	irqPending = state;
}

auto M6510::setNmi(bool state) -> void {
	// edge sensitive ( triggers only: 0 -> 1)
	
	if (!nmiDetect && state)
		nmiPending = true;

	nmiDetect = state;
}

auto M6510::setRdy(bool state) -> void {
	//halts the cpu in next read
	rdyLine = state;
}

auto M6510::updateIoLines( uint8_t pullup, uint8_t pulldown ) -> void {    
    this->pullup = pullup;
    this->pulldown = pulldown;
    
    updateLines();
}

inline auto M6510::updateLines() -> void {			

	ioLines = ( por & ddr ) | ( ~ddr & ( (pullup | ioLines) & ~pulldown ) );
	
    //external device can distingish between input and output because of voltage level 
	system->updatePort( ioLines, ddr );
}

auto M6510::chargeUndefinedBits( uint8_t newDdr ) -> void {
	
	if ( (ddr & 0x80) && !(newDdr & 0x80) ) {
	
		bit7charge = por & 0x80;
		sysTimer.add( &unChargeBit7, FALL_OFF_CYCLES, Emulator::SystemTimer::Action::UpdateExisting );
	}
	
	if ( (ddr & 0x40) && !(newDdr & 0x40) ) {
	
		bit6charge = por & 0x40;
		sysTimer.add( &unChargeBit6, FALL_OFF_CYCLES, Emulator::SystemTimer::Action::UpdateExisting );
	}
}

auto M6510::busWatch() -> uint8_t {
	// CPU watches BUS in case of RDY.

	// vicII sends AEC to CPU, 3 cycles after it sends BA(RDY) to CPU.
	// expansion port DMA pulls AEC low too.
	if ( vicII->isAecLow() || expansionPort->isDma() )
		// CPU is in tri-state and decoupled from BUS
		// todo: memory last used bus value (read or write)
		return (busState >> 16) & 0xff;

	// CPU is BUS Master in second half cycle, so it recognizes last BUS value from first half cycle,
	// which is always accessed by VIC.
	return vicII->lastReadPhase1();
}

#define SYNC	\
	sysTimer.process();	\
	cia1->clock();	\
	vicII->clock();	\
	cia2->clock();	\
	expansionPort->clock();

template<bool setI> auto M6510::busAccessUpdateFlagI( uint16_t addr ) -> void { 
		
	busState = addr;

STEAL:	
	SAMPLE_INTERRUPT
			
	SYNC
		
	if( rdyLine ) {
		if (setI)	{ SET_FLAG_I(1) }
		else		{ SET_FLAG_I(0) }
        goto STEAL;
    }

	if (addr == 0x0000 || addr == 0x0001)
		return;

	system->memoryCpu.read( addr );	
}

template<bool sampleInterrupt, bool rememberRdy> auto M6510::busRead( uint16_t addr ) -> uint8_t {        
	busState = addr;

STEAL:	
	if (sampleInterrupt)
		SAMPLE_INTERRUPT
			
	SYNC
		
	if( rdyLine ) {        
		if (rememberRdy) {
			busState |= CPU_RDY_CYCLE;
			// todo: special behaviour for LAX and ANE. use 0xee in case of RDY ... for now
			// the real one listen to bus usage
			// busWatch();
		}
		
        goto STEAL;
    }		
    
	if (unlikely(addr == 0x0000)) {        
		return ddr;   
    }
    
	if (unlikely(addr == 0x0001)) {
        
		uint8_t data = ioLines;
		
		if ( !(ddr & 0x40) ) {
			data &= ~0x40;
			data |= bit6charge;
		}

		if ( !(ddr & 0x80) ) {
			data &= ~0x80;
			data |= bit7charge;
		}
		
		return data;
	}		
	  
	return system->memoryCpu.read( addr );
}

auto M6510::busWrite( uint16_t addr, uint8_t data ) -> void {
    busState = CPU_WRITE_CYCLE | addr;

	SYNC
	
    // places write on bus for $00 and $01 too.
	// but for these two addresses
    // it seems only the address is selected on bus in write mode but not the data.
    // so the write happens with last data on external bus.
    // it's always VIC data, readed in first half cycle.
    
	if (addr == 0x0000) {						
		
		chargeUndefinedBits( data );
		ddr = data;		
		updateLines();
		
		if (!expansionPort->isDma())
			system->ram[ 0 ] = vicII->lastReadPhase1();
		
	} else if (addr == 0x0001) {
		
		por = data;
		updateLines();			
		
		if (!expansionPort->isDma())
			system->ram[ 1 ] = vicII->lastReadPhase1();
        
	} else {
		// Expansion Port DMA send AEC and BA same cycle,
		// so it's possible that CPU take a 'Write' whith decoupled BUS.
		// VIC send AEC three cycles later than BA, because there is a maximum of three 'Writes' in a row,
		// a 'Write' with decoupled BUS can not happen.	
		// so need only check for expansion DMA
		if (expansionPort->isDma())
			return;
		
		system->memoryCpu.write( addr, data );
	}        
}

auto M6510::setMagicForAne(uint8_t magicAne) -> void {
	this->magicAne = magicAne;
}

auto M6510::setMagicForLax(uint8_t magicLax) -> void {
	this->magicLax = magicLax;
}

auto M6510::serialize(Emulator::Serializer& s) -> void {
	
	s.integer( rdyLine );
	s.integer( irqPending );
	s.integer( nmiPending );
	s.integer( nmiDetect );
	s.integer( interruptSampled );
	s.integer( callResetRoutine );
	s.integer( killed );
	s.integer( busState );
	s.integer( pc );
	s.integer( regX );
	s.integer( regY );
	s.integer( regA );
	s.integer( regS );
	s.integer( regP );
	s.integer( flagZ );
	s.integer( flagN );
	s.integer( ddr );
	s.integer( por );
	s.integer( ioLines );
	s.integer( pullup );
	s.integer( pulldown );
	s.integer( bit6charge );
	s.integer( bit7charge );
	s.integer( magicAne );
	s.integer( magicLax );
}

}
