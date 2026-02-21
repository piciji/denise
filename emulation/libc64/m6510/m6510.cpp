
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
    stepOuts.reserve(32);

    watchPoints.callback = [this](bool state) { this->flagDebugAction( WatchPoint, state ); };
    watchPointsWrite.callback = [this](bool state) { this->flagDebugAction( WatchPointWrite, state ); };
    breakPoints.callback = [this](bool state) { this->flagDebugAction( BreakPoint, state ); };
    exceptionPoints.callback = [this](bool state) { this->flagDebugAction( ExceptionPoint, state ); };
    modifiedCode.callback = [this](bool state) { this->flagDebugAction( ModifiedCode, state ); };
    historyHandler.callback = [this](bool state) { this->flagDebugAction( History, state ); };

    watchPoints.expressionCallback = [this](const std::string& input, int& pos) {
        return parseExpressionValue(input, pos);
    };
    watchPointsWrite.expressionCallback = [this](const std::string& input, int& pos) {
        return parseExpressionValue(input, pos);
    };
    breakPoints.expressionCallback = [this](const std::string& input, int& pos) {
        return parseExpressionValue(input, pos);
    };
    exceptionPoints.expressionCallback = [this](const std::string& input, int& pos) {
        return parseExpressionValue(input, pos);
    };
}

auto M6510::parseExpressionValue(const std::string& input, int& pos) -> uint32_t {
    for (auto& cond : DebuggerSnapshot::breakConditions) {
        std::string token = cond.ident;
        if (input.compare(pos, token.size(), token) == 0) {
            pos += token.size();

            switch (cond.vector) {
                default: return 0;
                case 0: return vicII->getVcounter();
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
                case 101: {
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
                        if (cond.vector == 100)
                            return system->ram[value & 0xffff];

                        return system->memoryCpu.peek( value );
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
    stepOuts.clear();
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
    watchPoints.reset();
    watchPointsWrite.reset();
    breakPoints.reset();
    exceptionPoints.reset();
    modifiedCode.disable();
    historyHandler.flagWhenNeeded();
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
            system->debugPointReached(DebuggerAction::ExceptionPoint, vector);
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

inline auto M6510::appendStepOut(uint16_t addr) -> void {
    if (stepOuts.size() == stepOuts.capacity())
        stepOuts.clear();
    stepOuts.push_back( addr );
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
        system->debugPointReached(DebuggerAction::Watchpoint, addr);
    }

	if (addr == 0x0000 || addr == 0x0001)
		return;

	lastBus = memory.read( addr );
    if constexpr (busLogger && !mhz2)
        system->logCpu( addr, lastBus );
}

template<bool sampleInterrupt, bool rememberRdy, bool mhz2, bool busLogger> auto M6510::busRead( uint16_t addr ) -> uint8_t {
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
        system->debugPointReached(DebuggerAction::Watchpoint, addr);
    }

    if (likely(addr > 0x0001)) {
        lastBus = memory.read( addr );
        if constexpr (busLogger && !mhz2)
            system->logCpu( addr, lastBus );
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
            system->logCpu( addr, data );

        return data;
    }

    // addr == 0
	if constexpr (busLogger && !mhz2)
	    system->logCpu( addr, ddr );
	return ddr;
}

template<bool mhz2, bool busLogger> auto M6510::busWrite( uint16_t addr, uint8_t data ) -> void {
    busState = CPU_WRITE_CYCLE | addr;

    if constexpr(mhz2)  { SYNC2 }
    else                { SYNC }

    if (control & (WatchPoint | ModifiedCode) ) {
        modifiedCode.checkAndSet( addr );

        if ((control & WatchPointWrite) && watchPointsWrite.check( addr )) {
            system->debugPointReached(DebuggerAction::WatchpointWrite, addr);
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
            system->logCpu( addr, data );

    } else if (addr == 0x0001) {
        por = data;
        updateLines();

        if (!expansionPort->isDma())
            system->ram[ 1 ] = vicII->lastReadPhase1();

        if constexpr (busLogger && !mhz2)
            system->logCpu( addr, data );

    } else {
        chargeUndefinedBits( data );
        ddr = data;
        updateLines();

        if (!expansionPort->isDma())
            system->ram[ 0 ] = vicII->lastReadPhase1();

        if constexpr (busLogger && !mhz2)
            system->logCpu( addr, data );
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

#define A_BREAK             case 0x00

#define A_IMPLIED                      case 0x02: case 0x08: case 0x0a: case 0x12: case 0x18: case 0x1a: case 0x22: case 0x28: \
                            case 0x2a: case 0x32: case 0x38: case 0x3a: case 0x40: case 0x42: case 0x48: case 0x4a: case 0x52: \
                            case 0x58: case 0x5a: case 0x60: case 0x62: case 0x68: case 0x6a: case 0x72: case 0x78: case 0x7a: \
                            case 0x88: case 0x8a: case 0x92: case 0x98: case 0x9a: case 0xa8: case 0xaa: case 0xb2: case 0xb8: \
                            case 0xba: case 0xc8: case 0xca: case 0xd2: case 0xd8: case 0xda: case 0xe8: case 0xea: case 0xf2: \
                            case 0xf8: case 0xfa:
#define A_INDEXED_INDIRECT  case 0x01: case 0x03: case 0x21: case 0x23: case 0x41: case 0x43: case 0x61: case 0x63: case 0x81: \
                            case 0x83: case 0xa1: case 0xa3: case 0xc1: case 0xc3: case 0xe1: case 0xe3:
#define A_ZERO_PAGE         case 0x04: case 0x05: case 0x06: case 0x07: case 0x24: case 0x25: case 0x26: case 0x27: case 0x44: \
                            case 0x45: case 0x46: case 0x47: case 0x64: case 0x65: case 0x66: case 0x67: case 0x84: case 0x85: \
                            case 0x86: case 0x87: case 0xa4: case 0xa5: case 0xa6: case 0xa7: case 0xc4: case 0xc5: case 0xc6: \
                            case 0xc7: case 0xe4: case 0xe5: case 0xe6: case 0xe7:

#define A_IMMEDIATE         case 0x09: case 0x0b: case 0x29: case 0x2b: case 0x49: case 0x4b: case 0x69: case 0x6b: case 0x80: \
                            case 0x82: case 0x89: case 0x8b: case 0xa0: case 0xa2: case 0xa9: case 0xab: case 0xc0: case 0xc2: \
                            case 0xc9: case 0xcb: case 0xe0: case 0xe2: case 0xe9: case 0xeb:
#define A_ABSOLUTE          case 0x0c: case 0x0d: case 0x0e: case 0x0f: case 0x20: case 0x2c: case 0x2d: case 0x2e: case 0x2f: \
                            case 0x4c: case 0x4d: case 0x4e: case 0x4f: case 0x6d: case 0x6e: case 0x6f: case 0x8c: case 0x8d: \
                            case 0x8e: case 0x8f: case 0xac: case 0xad: case 0xae: case 0xaf: case 0xcc: case 0xcd: case 0xce: \
                            case 0xcf: case 0xec: case 0xed: case 0xee: case 0xef:
#define A_RELATIVE          case 0x10: case 0x30: case 0x50: case 0x70: case 0x90: case 0xb0: case 0xd0: case 0xf0:
#define A_INDIRECT_INDEXED  case 0x11: case 0x13: case 0x31: case 0x33: case 0x51: case 0x53: case 0x71: case 0x73: case 0x91: \
                            case 0x93: case 0xb1: case 0xb3: case 0xd1: case 0xd3: case 0xf1: case 0xf3:
#define A_ZERO_INDEXED_X    case 0x14: case 0x15: case 0x16: case 0x17: case 0x34: case 0x35: case 0x36: case 0x37: case 0x54: \
                            case 0x55: case 0x56: case 0x57: case 0x74: case 0x75: case 0x76: case 0x77: case 0x94: case 0x95: \
                            case 0xb4: case 0xb5: case 0xd4: case 0xd5: case 0xd6: case 0xd7: case 0xf4: case 0xf5: case 0xf6: \
                            case 0xf7:
#define A_ABS_INDEXED_Y     case 0x19: case 0x1b: case 0x39: case 0x3b: case 0x59: case 0x5b: case 0x79: case 0x7b: case 0x99: \
                            case 0x9b: case 0x9e: case 0x9f: case 0xb9: case 0xbb: case 0xbe: case 0xbf: case 0xd9: case 0xdb: \
                            case 0xf9: case 0xfb:
#define A_ABS_INDEXED_X     case 0x1c: case 0x1d: case 0x1e: case 0x1f: case 0x3c: case 0x3d: case 0x3e: case 0x3f: case 0x5c: \
                            case 0x5d: case 0x5e: case 0x5f: case 0x7c: case 0x7d: case 0x7e: case 0x7f: case 0x9c: case 0x9d: \
                            case 0xbc: case 0xbd: case 0xdc: case 0xdd: case 0xde: case 0xdf: case 0xfc: case 0xfd: case 0xfe: \
                            case 0xff:
#define A_INDIRECT          case 0x6c:
#define A_ZERO_INDEXED_Y    case 0x96: case 0x97: case 0xb6: case 0xb7:

#define PeekByte(o)         (memSnap ? *(memSnap + (o)) : memory.peek( _pc + (o) ))
#define PeekWord            (memSnap ? *(memSnap + 1) | (*(memSnap + 2) << 8) : memory.peek( _pc + 1 ) | (memory.peek( _pc + 2 ) << 8))

auto M6510::disassemble(uint16_t addr, unsigned& bytes, const uint8_t* memSnap) -> std::string {
    DasmHandler d;
    uint16_t _pc = addr;
    uint8_t opcode = PeekByte(0);

    d.Ins( opcode ).tab();

    switch (opcode) {
        A_INDEXED_INDIRECT
            bytes = 2;
            d.indexedIndirect( PeekByte(1) );
            break;
        A_INDIRECT_INDEXED
            bytes = 2;
            d.indirectIndexed(  PeekByte(1) );
            break;
        A_ZERO_PAGE
            bytes = 2;
            d.zeroPage( PeekByte(1) );
            break;
        A_ZERO_INDEXED_X
            bytes = 2;
            d.zeroPageIndexedX( PeekByte(1) );
            break;
        A_ZERO_INDEXED_Y
            bytes = 2;
            d.zeroPageIndexedY( PeekByte(1) );
            break;
        A_IMMEDIATE
            bytes = 2;
            d.immediate( PeekByte(1) );
            break;
        A_RELATIVE
            bytes = 2;
            d.absolute( _pc + 2 + static_cast<int8_t>(PeekByte( 1 )) );
            break;
        A_ABSOLUTE
            bytes = 3;
            d.absolute( PeekWord );
            break;
        A_ABS_INDEXED_Y
            bytes = 3;
            d.absIndexedY( PeekWord );
            break;
        A_ABS_INDEXED_X
            bytes = 3;
            d.absIndexedX( PeekWord );
            break;
        A_INDIRECT
            bytes = 3;
            d.indirect( PeekWord );
            break;
        A_BREAK:
            bytes = 2;
            break;
        default:
            bytes = 1;
            break;
    }

    return d.str;
}

auto M6510::disassembleData(uint16_t addr, unsigned bytes) -> std::string {
    DasmHandler d;
    d.hex16( addr );
    d.str.append( "|" );

    for(unsigned i = 0; i < bytes; ++i) {
        if (i)
            d.str.append( " " );

        d.hex8( memory.peek( addr + i ) );
    }
    return d.str;
}

auto M6510::flagDebugAction(int action, bool state) -> void {
    if (state)
        control |= action;
    else
        control &= ~action;
}

auto M6510::disassembleTrace(unsigned i, uint8_t& flags) -> std::string {
    DasmHandler d;
    unsigned bytes;
    Emulator::HistoryEntry<uint8_t>* historyEntry = historyHandler.get(i);
    if (!historyEntry)
        return "";
    d.hex16( historyEntry->addr );
    d.str.append( "|" );
    d.str.append( disassemble( historyEntry->addr, bytes, &historyEntry->mem[0]) );
    flags = historyEntry->flags;
    return d.str;
}

auto M6510::checkSoftStop(uint16_t addr) -> bool {
    if (!softStep.has_value() || softStep.value() == addr) {
        control &= ~SoftStop;
        return true;
    }
    return false;
}

auto M6510::debuggerStepOver() -> void {
    unsigned bytes;
    disassemble( pcEdge, bytes );

    softStep = pcEdge + bytes;
    control |= SoftStop;
}

auto M6510::debuggerStepInto() -> void {
    softStep = std::nullopt;
    control |= SoftStop;
}

auto M6510::debuggerStepOut() -> bool {
    if (stepOuts.empty())
        return false;
    softStep = stepOuts.back();
    control |= SoftStop;
    return true;
}

auto M6510::debuggerAdd(DebuggerAction action, uint16_t addr, uint16_t addrTo) -> void {
    switch (action) {
        case DebuggerAction::Breakpoint:        breakPoints.add( addr ); break;
        case DebuggerAction::Watchpoint:        watchPoints.add( addr ); break;
        case DebuggerAction::WatchpointWrite:   watchPointsWrite.add( addr ); break;
        case DebuggerAction::ExceptionPoint:    exceptionPoints.add( addr ); break;
        case DebuggerAction::History:           historyHandler.enable(); break;
        case DebuggerAction::ModifiedCode:      modifiedCode.add( addr, addrTo ); break;
        default:
            break;
    }
}

auto M6510::debuggerRemove(DebuggerAction action, uint16_t addr) -> void {
    switch (action) {
        case DebuggerAction::Breakpoint:        breakPoints.remove( addr ); break;
        case DebuggerAction::Watchpoint:        watchPoints.remove( addr ); break;
        case DebuggerAction::WatchpointWrite:   watchPointsWrite.remove( addr ); break;
        case DebuggerAction::ExceptionPoint:    exceptionPoints.remove( addr ); break;
        case DebuggerAction::History:           historyHandler.disable( ); break;
        default:
            break;
    }
}

auto M6510::debuggerRemove(DebuggerAction action) -> void {
    switch (action) {
        case DebuggerAction::Breakpoint:        breakPoints.removeAll(); break;
        case DebuggerAction::Watchpoint:        watchPoints.removeAll(); break;
        case DebuggerAction::WatchpointWrite:   watchPointsWrite.removeAll(); break;
        case DebuggerAction::ExceptionPoint:    exceptionPoints.removeAll(); break;
        case DebuggerAction::History:           historyHandler.disable(); break;
        case DebuggerAction::ModifiedCode:      modifiedCode.disable(); break;
        default:
            break;
    }
}

auto M6510::setWatchpointCondition(DebuggerAction action, unsigned addr, unsigned hitCount, unsigned hitCountMode, const std::string& expression, unsigned expressionMode) -> bool {
    bool expressionError = false;

    if (!expression.empty()) {
        ExpressionParser parser;
        parser.setExpression( expression );
        parser.callback = [this](const std::string& input, int& pos) {
            return parseExpressionValue(input, pos);
        };

        try {
            parser.parse();
        } catch (ExpressionParseError& e) {
            expressionError = true;
        }
    }

    switch (action) {
        case DebuggerAction::Breakpoint:
            breakPoints.setBreakpointCondition( addr, hitCount, hitCountMode, expressionError ? "" : expression, expressionMode );
            break;
        case DebuggerAction::Watchpoint:
            watchPoints.setBreakpointCondition( addr, hitCount, hitCountMode, expressionError ? "" : expression, expressionMode );
            break;
        case DebuggerAction::WatchpointWrite:
            watchPointsWrite.setBreakpointCondition( addr, hitCount, hitCountMode, expressionError ? "" : expression, expressionMode );
            break;
        case DebuggerAction::ExceptionPoint:
            exceptionPoints.setBreakpointCondition( addr, hitCount, hitCountMode, expressionError ? "" : expression, expressionMode );
            break;
    }

    return !expressionError;
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

template auto M6510::process<false, false>() -> void;
template auto M6510::process<false, true>() -> void;
template auto M6510::resetRoutine<false, false>() -> void;
template auto M6510::resetRoutine<false, true>() -> void;
template auto M6510::process<true, false>() -> void;
template auto M6510::process<true, true>() -> void;
template auto M6510::resetRoutine<true, false>() -> void;
template auto M6510::resetRoutine<true, true>() -> void;
    
}
