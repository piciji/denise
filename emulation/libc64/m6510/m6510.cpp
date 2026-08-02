
#include "m6510.h"
#include "../system/system.h"
#include "../expansionPort/expansionPort.h"
#include "../disk/iec.h"
#include "../traps/traps.h"
#include "../../tools/serializer.h"
#include "../../tools/expressionParser.h"
#include "dasmHandler.h"
#include "opcodes.cpp"
#include "../system/debuggerSnapshot.h"

#define FALL_OFF_CYCLES 350000

namespace LIBC64 {
	
M6510::M6510(System* system, Emulator::SystemTimer& sysTimer, CIA::M6526& cia1, CIA::M6526& cia2, IecBus& iecBus, Traps& traps) :
system(system),
memory(system->memoryCpu),
sysTimer(sysTimer),
cia1(cia1),
cia2(cia2),
iecBus(iecBus),
traps(traps) {
	
	unChargeBit6 = [this]() { bit6charge = 0; };
	unChargeBit7 = [this]() { bit7charge = 0; };
}

auto M6510::parseExpressionValue(const std::string& input, int& pos) -> uint32_t {
    for (auto& cond : DebuggerSnapshot::breakConditions) {
        std::string token = cond.ident;
        if (input.compare(pos, token.size(), token) == 0) {
            pos += token.size();

            switch (cond.vector) {
                default: return 0;
                case 0: return vicII->getVcounterHR();
                case 1: return vicII->getCycle();
                case 2: return pc;
                case 3: return regX;
                case 4: return regY;
                case 5: return regA;
                case 6: return regS;
                case 7: return regP;

                case 8: return ddr;
                case 9: return por;
                case 10: return ioLines;
                case 11: return rdyLine;
                case 12: return irqPending;
                case 13: return nmiPending;

                case 14: return !!(regP & 1);
                case 15: return !!(regP & 2);
                case 16: return !!(regP & 4);
                case 17: return !!(regP & 8);
                case 18: return !!(regP & 0x10);
                case 19: return !!(regP & 0x40);
                case 20: return !!(regP & 0x80);

                case 100:
                case 101:
                case 102:
                case 103:
                case 104:
                case 105: {
                    int radix = 10;
                    if (input.compare(pos, 1, "$") == 0) {
                        radix = 16;
                        pos++;
                    }
                    const char* start = input.c_str() + pos;
                    char* end;
                    uint32_t value = std::strtoul(start, &end, radix);
                    if (start != end) {
                        pos += (end - start);
                        return system->peekMemoryByIdent( value, cond.vector );
                    }
                    return 0;
                }
            }
        }
    }
    return 0;
}

auto M6510::registerCallbacks() -> void {
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
	
	irqPending = nmiPending = nmiDetect = false;
	
	rdyLine = false;

	ddr = 0; //input mode
	por = 0;
	ioLines = 0;
	
	busState = 0;
    oddCycle = true;
    reg2mhz = 0;

	bit6charge = bit7charge = 0;

    control = ResetRoutine;

    M65Debugger::init();
}

template<bool mhz2, bool busLogger> auto M6510::resetRoutine() -> void {
	
	uint8_t dataBus;
	
	READ( pc )
	READ( pc )
	READ( pc )
	READ( 0x100 | regS-- )
	READ( 0x100 | regS-- )
	READ( 0x100 | regS-- )
		
	READ( 0xfffc )
	uint8_t result = dataBus;
	READ( 0xfffd )
	pc = (dataBus << 8) | result;
	SET_FLAG_I( 1 )
	
    control &= ~ResetRoutine;
}

template<bool software, bool mhz2, bool busLogger> inline auto M6510::interrupt() -> void {
	
	uint8_t dataBus;
	
	if constexpr (!software) {
		READ( pc )
		READ( pc )
	} else {
		READ_PC_INC
	}
	
	PUSH((pc >> 8) & 0xff);
	PUSH( pc & 0xff);
    appendStepOut(pc);

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

    if constexpr (!software) {
        if ((control & ExceptionPoint) && exceptionPoints.check( vector )) {
            system->debugPointReached(DebuggerTheme::CPU, DebuggerAction::ExceptionPoint, exceptionPoints, vector);
        }
    }
	
	READ( vector )
	
	uint8_t result = dataBus;
	
	SET_FLAG_I( 1 )

	READ( vector + 1 )
		
	pc = (dataBus << 8) | result;
	
	/**	 
	 * no interrupt polling at the end of this service routine (software break too)
	 * so at least one opcode is following before could interrupted again by nmi
	 */

    control &= ~IRQ;
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
	
    //external device can distinguish between input and output because of voltage level
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
		return lastBus;

	// CPU is BUS Master in second half cycle, so it recognizes last BUS value from first half cycle,
	// which is always accessed by VIC.
	return vicII->lastReadPhase1();
}

#define SYNC	\
	sysTimer.process();	\
	cia1.clock();	\
    if constexpr (busLogger)    vicII->clockLogged(); \
    else                        vicII->clock();	\
	cia2.clock();	\
	expansionPort->clock(); \
    if (system->secondDriveCable.cycleSyncing) \
	    iecBus.syncDrivesEachCycle();
    
// IO access or RAM refresh force to 1 MHz
#define SYNC2    \
if ((addr & 0xd000) == 0xd000) \
    oddCycle = true; \
if (oddCycle) { \
    sysTimer.process();    \
    cia1.clock();    \
    if constexpr (busLogger)    vicII->clockLogged(); \
    else                        vicII->clock();	\
    cia2.clock();    \
    expansionPort->clock(); \
    if (system->secondDriveCable.cycleSyncing) \
        iecBus.syncDrivesEachCycle(); \
    if ((reg2mhz & 0x80) || ((reg2mhz & 1)  && (vicII->getCycle() > 14 || vicII->getCycle() < 10))) \
        oddCycle = false; \
} else { \
    oddCycle = true; \
}

template<bool setI, bool mhz2, bool busLogger> auto M6510::busAccessUpdateFlagI( uint16_t addr ) -> void {

	busState = addr;

STEAL:	
	SAMPLE_INTERRUPT
    
    if constexpr(mhz2)  { SYNC2 }
    else                { SYNC }
		
	if( rdyLine ) {
		if (setI)	{ SET_FLAG_I(1) }
		else		{ SET_FLAG_I(0) }
        goto STEAL;
    }

    if ((control & WatchPoint) && watchPoints.check( addr )) {
        system->debugPointReached(DebuggerTheme::CPU, DebuggerAction::Watchpoint, watchPoints, addr);
    }

	if (addr == 0x0000 || addr == 0x0001)
		return;

	lastBus = memory.read( addr );
    if constexpr (busLogger && !mhz2)
        system->logCpu( addr, lastBus, false, false );
}

template<bool sampleInterrupt, bool rememberRdy, bool mhz2, bool busLogger, bool nextOp> auto M6510::busRead( uint16_t addr ) -> uint8_t {
	busState = addr;

STEAL:	
	if (sampleInterrupt)
		SAMPLE_INTERRUPT
			
    if constexpr(mhz2)  { SYNC2 }
    else                { SYNC }
		
	if( rdyLine ) {
		if (rememberRdy) {
			busState |= CPU_RDY_CYCLE;
			// todo: special behaviour for LAX and ANE. use 0xee in case of RDY ... for now
			// the real one listen to bus usage
			// busWatch();
		}
		
        goto STEAL;
    }

    if ((control & WatchPoint) && watchPoints.check( addr )) {
        system->debugPointReached(DebuggerTheme::CPU, DebuggerAction::Watchpoint, watchPoints, addr);
    }

    if (likely(addr > 0x0001)) {
        lastBus = memory.read( addr );
        if constexpr (busLogger && !mhz2)
            system->logCpu( addr, lastBus, false, nextOp );
        return lastBus;
    }

    if (addr == 0x0001) {
        uint8_t data = ioLines;

        if ( !(ddr & 0x40) ) {
            data &= ~0x40;
            data |= bit6charge;
        }

        if ( !(ddr & 0x80) ) {
            data &= ~0x80;
            data |= bit7charge;
        }

        if constexpr (busLogger && !mhz2)
            system->logCpu( addr, data, false, nextOp );

        return data;
    }

    // addr == 0
	if constexpr (busLogger && !mhz2)
	    system->logCpu( addr, ddr, false, nextOp );
	return ddr;
}

template<bool mhz2, bool busLogger> auto M6510::busWrite( uint16_t addr, uint8_t data ) -> void {
    busState = CPU_WRITE_CYCLE | addr;

    if constexpr(mhz2)  { SYNC2 }
    else                { SYNC }

    if (control & (WatchPointWrite | ModifiedCode) ) {
        modifiedCode.checkAndSet( addr );

        if ((control & WatchPointWrite) && watchPointsWrite.check( addr )) {
            system->debugPointReached(DebuggerTheme::CPU, DebuggerAction::WatchpointWrite, watchPointsWrite, addr);
        }
    }

    if (likely(addr > 0x0001)) {
        // Expansion Port DMA send AEC and BA same cycle,
        // it's possible that CPU take a 'Write' with decoupled BUS.
        // VIC send AEC three cycles later than BA, because there is a maximum of three 'Writes' in a row,
        // a 'Write' with decoupled BUS can not happen.
        // so need only check for expansion DMA
        if (expansionPort->isDma())
            return;

        lastBus = data;
        memory.write( addr, data );

        if constexpr (busLogger && !mhz2)
            system->logCpu( addr, data, true, false );

    } else if (addr == 0x0001) {
        por = data;
        updateLines();

        if (!expansionPort->isDma())
            system->ram[ 1 ] = vicII->lastReadPhase1();

        if constexpr (busLogger && !mhz2)
            system->logCpu( addr, data, true, false );

    } else {
        chargeUndefinedBits( data );
        ddr = data;
        updateLines();

        if (!expansionPort->isDma())
            system->ram[ 0 ] = vicII->lastReadPhase1();

        if constexpr (busLogger && !mhz2)
            system->logCpu( addr, data, true, false );
    }
}

auto M6510::serialize(Emulator::Serializer& s) -> void {
	
	s.integer( rdyLine );
	s.integer( irqPending );
	s.integer( nmiPending );
	s.integer( nmiDetect );
	s.integer( control );
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
    s.integer( oddCycle );
    s.integer( reg2mhz );
}
    
auto M6510::setClock(bool state, bool aggressive) -> void {    
    reg2mhz = state;
    oddCycle = !state;
	if (aggressive)
		reg2mhz |= 0x80;
}

auto M6510::flagDebugAction(int action, bool state) -> void {
    if (state)
        control |= action;
    else
        control &= ~action;
}

inline auto M6510::peek(uint16_t addr) -> uint8_t {
    return memory.peek( addr );
}

auto M6510::updateSnapshot(DebuggerSnapshot& snap) -> void {
    snap.pc = pc;
    snap.pcEdge = pcEdge;
    snap.regA = regA;
    snap.regX = regX;
    snap.regY = regY;
    snap.regS = 0x100 | regS;
    snap.ddr = ddr;
    snap.por = por;
    snap.ioLines = ioLines;
    snap.flags = STATUS;
}

auto M6510::updateFromSnapshot(DebuggerSnapshot& snap) -> void {
    pc = pcEdge = snap.pcEdge = snap.pc;
    regA = snap.regA;
    regX = snap.regX;
    regY = snap.regY;
    regS = snap.regS;
    SET_STATUS(snap.flags)
    snap.updateFromExtern = false;
}

template auto M6510::process<false, false>() -> void;
template auto M6510::process<false, true>() -> void;
template auto M6510::resetRoutine<false, false>() -> void;
template auto M6510::resetRoutine<false, true>() -> void;
template auto M6510::process<true, false>() -> void;
template auto M6510::process<true, true>() -> void;
template auto M6510::resetRoutine<true, false>() -> void;
template auto M6510::resetRoutine<true, true>() -> void;
    
}
