
#include "w65816.h"
#include "dasmHandler.h"

#ifdef W65816_REF
    #ifdef W65816_REF_INCLUDE
        #include W65816_REF_INCLUDE
    #endif
    #define REF_CALL ref.
#else
    #define REF_CALL
#endif

#define READ_BYTE           REF_CALL readByte
#define PEEK_BYTE           REF_CALL peekByte
#define READ_VECTOR_BYTE    REF_CALL readVectorByte
#define WRITE_BYTE          REF_CALL writeByte
#define IDLE_CYCLE          REF_CALL idleCycle
#define OUTPUT_RDY_LOW      REF_CALL outputRDYLineLow
#define SET_MEMORY_LOCK     REF_CALL setMemoryLock
#define TRAP_HANDLER        REF_CALL trapHandler
#define DEBUG_POINT_REACHED REF_CALL debugPointReached

#define CHECK_INTR     { if(lines & (NMI_TRANSITION | IRQ_LINE)) checkForInterrupt(); }

#define PAGE_CROSSED(a1, a2) (((a1) ^ (a2)) & 0xff00)

#include "instructions.cpp"
#include "alu.cpp"

namespace WDCFAMILY {

auto W65816::process()->void {

    if (control) {
        if (control & WAI) {
            // WAI sets RDY (bidirectional) low and repeats the same cycle. It's same behavior like external RDY change.
            // Since "WAI" can last a very long time, it is covered here to keep the emulation responsive.
            // Otherwise, the UI may not be refreshed in time. Furthermore, no "RDY" check is required in each cycle.
            // This requires additional power and can be switched off if no external "RDY" change is planned.
            // IRQ/NMI set RDY hi again and resume processing but only if RDY is not forced low from external.
            CHECK_INTR
            return IDLE_CYCLE((pbr << 16) | pc);
        }

        if (control & NMI_PENDING) {
            control &= ~NMI_PENDING;
            interrupt( modeE ? 0xfffa : 0xffea );
            if (control & (BreakPoint | SoftStop))
                controlBreaks();
            return;
        }

        if (control & IRQ_PENDING) {
            control &= ~IRQ_PENDING;
            interrupt( modeE ? 0xfffe : 0xffee );
            pcEdge = (pbr << 16) | pc;
            if (control & (BreakPoint | SoftStop))
                controlBreaks();
            return;
        }

        // check STP and RESET last for performance reasons
        if (control & STP) {
            return IDLE_CYCLE((pbr << 16) | pc);
        }

        if (control & RESET) {
            control &= ~RESET;
            return interrupt( 0xfffc );
        }

        if (control & History)
            loadTrace(historyHandler.getNext());

        switch(readPC()) {
            #include "optable.h"
        }

        pcEdge = (pbr << 16) | pc;
        if (control & (BreakPoint | SoftStop))
            controlBreaks();

        return;
    }

    switch(readPC()) {
        #include "optable.h"
    }
}

auto W65816::loadTrace(Emulator::HistoryEntry<uint8_t>& entry) -> void {
    uint32_t addr = pcEdge;
    entry.addr = addr;
    entry.flags= p;

    for (int i = 0; i < 4; ++i) {
        entry.mem[i] = PEEK_BYTE( addr++ );
    }
}

inline auto W65816::controlBreaks() -> void {
    if ((control & SoftStop) && checkSoftStop(pcEdge)) {
        DEBUG_POINT_REACHED(SoftStop, pcEdge);
    } else if ((control & BreakPoint) && breakPoints.check(pcEdge)) {
        DEBUG_POINT_REACHED(BreakPoint, pcEdge);
    }
}

template<bool hardware> auto W65816::interrupt(const uint16_t& vector) -> void {
    if constexpr(hardware) {
        readPCNoInc();
        readPCIdle();
    } else
        readPC();

    if(!modeE)
        push(pbr);

    push(pc >> 8);
    push(pc & 0xff);
    (hardware && modeE) ? push(p & ~0x10) : push(p);
    p.i = true;
    p.d = false;

    if constexpr (hardware) {
        if ((control & ExceptionPoint) && exceptionPoints.check( vector )) {
            DEBUG_POINT_REACHED(ExceptionPoint, vector);
        }
    }

    uint16_t newPC = read<VECTOR>(vector);
    newPC |= read<SAMPLE_INTR | VECTOR>(vector + 1) << 8;
    pc = newPC;
    pbr = 0;
}

auto W65816::power() -> void {
    modeE = true;
    pc = 0;
    pbr = 0;
    dbr = 0;
    a = 0;
    x = 0;
    y = 0;
    s = 0x01ff;
    d = 0;
    p = 0x34;
    lines = 0;
    control = RESET;

    stepOuts.clear();
    watchPoints.reset();
    breakPoints.reset();
    exceptionPoints.reset();
    modifiedCode.disable();
    historyHandler.flagWhenNeeded();
}

auto W65816::setNmiLineLow(bool state) -> void {
    if (state) {
        if ((lines & NMI_LINE) == 0)
            lines |= NMI_TRANSITION;
        lines |= NMI_LINE;
    } else
        lines &= ~NMI_LINE;
}

auto W65816::setIrqLineLow(bool state) -> void {
    if (state)
        lines |= IRQ_LINE;
    else
        lines &= ~IRQ_LINE;
}

auto W65816::setRdyLineLow(bool state) -> void {
    if (state) lines |= RDY_LINE;
    else {
        lines &= ~RDY_LINE;
        control &= ~WAI;
    }
}

auto W65816::checkForInterrupt() -> void {
    if (lines & NMI_TRANSITION) {
        lines &= ~NMI_TRANSITION;
        control &= ~WAI;
        control |= NMI_PENDING;
    }

    if (lines & IRQ_LINE) {
        // will re-trigger if an external device doesn't change line before the next interrupt check
        if (!p.i)
            control |= IRQ_PENDING;
        control &= ~WAI;
    }
}

inline auto W65816::idle2() -> void {
    if(d & 0xff)
        readPCIdle();
}

inline auto W65816::idle4(const uint16_t a1, const uint16_t a2) -> void {
    if(!p.x || PAGE_CROSSED(a1, a2))
        readBankIdle((a1 & 0xff00) | (a2 & 0xff));
}

inline auto W65816::idleIrq() -> void {
    if (control & (IRQ_PENDING | NMI_PENDING))
        readPCNoInc<SAMPLE_INTR>();
    else
        readPCIdle<SAMPLE_INTR>();
}

template<uint8_t actions> inline auto W65816::idle(uint32_t addr) -> void {
    if constexpr (actions & SAMPLE_INTR)
        CHECK_INTR

#ifdef SUPPORT_RDY
    while (lines & RDY_LINE) {
        IDLE_CYCLE(addr);
        if constexpr (actions & SET_FLAG_I)     p.i = true;
        if constexpr (actions & CLEAR_FLAG_I)   p.i = false;

        if constexpr (actions & SAMPLE_INTR)
            CHECK_INTR
    }
#endif

    if ((control & WatchPoint) && watchPoints.check( addr )) {
        DEBUG_POINT_REACHED(WatchPoint, addr);
    }

    IDLE_CYCLE(addr);
}

template<uint8_t actions> inline auto W65816::read(uint32_t addr) -> uint8_t {
    if constexpr (actions & SAMPLE_INTR)
        CHECK_INTR

#ifdef SUPPORT_RDY
    while (lines & RDY_LINE) {
        IDLE_CYCLE(addr);
        if constexpr (actions & SET_FLAG_I)     p.i = true;
        if constexpr (actions & CLEAR_FLAG_I)   p.i = false;

        if constexpr (actions & SAMPLE_INTR)
            CHECK_INTR
    }
#endif

    if ((control & WatchPoint) && watchPoints.check( addr )) {
        DEBUG_POINT_REACHED(WatchPoint, addr);
    }

#ifdef SEPARATE_VECTOR_READ
    if constexpr (!!(actions & VECTOR))
        return READ_VECTOR_BYTE((uint16_t)addr);
#endif
    return READ_BYTE(addr);
}

template<uint8_t actions> inline auto W65816::write(uint32_t addr, uint8_t value) -> void {
    if constexpr (actions & SAMPLE_INTR)
        CHECK_INTR

#ifdef SUPPORT_RDY
    while (lines & RDY_LINE) {
        IDLE_CYCLE(addr);
        if constexpr (actions & SAMPLE_INTR)
            CHECK_INTR
    }
#endif

    if (control & (WatchPoint | ModifiedCode) ) {
        modifiedCode.checkAndSet( addr );

        if ((control & WatchPointWrite) && watchPointsWrite.check( addr )) {
            DEBUG_POINT_REACHED(WatchPointWrite, addr);
        }
    }

    WRITE_BYTE(addr, value);
}

template<uint8_t actions> inline auto W65816::readBank(uint32_t addr) -> uint8_t {
    return read<actions>( ((dbr << 16) + addr) & 0xffffff );
}

template<uint8_t actions> inline auto W65816::readBankIdle(uint32_t addr) -> void {
    idle<actions>( ((dbr << 16) + addr) & 0xffffff );
}

template<uint8_t actions> inline auto W65816::readPC() -> uint8_t {
    return read<actions>((pbr << 16) | pc++);
}

template<uint8_t actions> inline auto W65816::readPCNoInc() -> uint8_t {
    return read<actions>((pbr << 16) | pc);
}

template<uint8_t actions> inline auto W65816::readPCIdle() -> void {
    idle<actions>((pbr << 16) | pc);
}

template<uint8_t actions> inline auto W65816::readStack(uint32_t addr) -> uint8_t {
    return read<actions>((s + addr) & 0xffff );
}

template<uint8_t actions> inline auto W65816::writeBank(uint32_t addr, uint8_t data) -> void {
    write<actions>( ((dbr << 16) + addr) & 0xffffff, data );
}

template<uint8_t actions> inline auto W65816::writeStack(uint32_t addr, uint8_t data) -> void {
    write<actions>((s + addr) & 0xffff, data );
}

template<uint8_t actions> auto W65816::push(uint8_t data) -> void {
    write<actions>(s, data);
    if constexpr (!!(actions & NATIVE)) s--;
    else { modeE ? decByteL(s) : (void)s--; }
}

template<uint8_t actions> auto W65816::pull() -> uint8_t {
    if constexpr (!!(actions & NATIVE)) s++;
    else { modeE ? incByteL(s) : (void)s++; }
    return read<actions>(s);
}

auto W65816::observeRegLength(uint8_t newVal) -> void {
    uint8_t oldVal = (p.x << 4) | (p.m << 5);

    if ((oldVal ^ newVal) & (0x20 | 0x10))
        modifiedCode.checkAndSet( (pbr << 16) | pc );
}

inline auto W65816::directAdr(uint32_t addr) -> uint32_t {
    if(modeE && ((d & 0xff) == 0) )
        return (d & 0xff00) | (addr & 0xff);

    return (d + addr) & 0xffff;
}

auto W65816::getDirectAddressIndirect(uint32_t offset) -> uint16_t {
    uint8_t lsb = read( directAdr(offset) );

    if(!modeE || ((d & 0xff) == 0))
        return (read( directAdr(offset + 1) ) << 8) | lsb;

    uint16_t addr = directAdr(offset + 1);

    if((addr & 0xff) == 0) // if +1 wraps page -> undo
        return (read((uint16_t)(addr - 0x100)) << 8) | lsb;

    return (read(addr) << 8) | lsb;
}

inline auto W65816::decByteL(uint16_t& reg) -> void {
    uint8_t byte = reg & 0xff;
    byte--;
    reg = (reg & 0xff00) | byte;
}

inline auto W65816::incByteL(uint16_t& reg) -> void {
    uint8_t byte = reg & 0xff;
    byte++;
    reg = (reg & 0xff00) | byte;
}

inline auto W65816::setByteL(uint16_t& reg, uint8_t byte) -> void {
    reg = (reg & 0xff00) | byte;
}

inline auto W65816::setByteH(uint16_t& reg, const uint8_t& byte) -> void {
    reg = (reg & 0x00ff) | (byte << 8);
}

// #
#define A_IMMEDIATE                         case 0x09: case 0x29: case 0x49: case 0x69: case 0x89: case 0xa9: case 0xc9: case 0xe9:
#define A_IMMEDIATE_X                       case 0xa0: case 0xa2: case 0xc0: case 0xe0:
#define A_REP_SEP                           case 0xc2: case 0xe2:
// A
#define A_ACCUMULATOR                       case 0x0a: case 0x1a: case 0x2a: case 0x3a: case 0x4a: case 0x6a:
// r
#define A_PC_RELATIVE                       case 0x10: case 0x30: case 0x50: case 0x70: case 0x80: case 0x90: case 0xb0: case 0xd0: \
                                            case 0xf0:
// rl
#define A_PC_RELATIVE_LONG                  case 0x82:
// I
#define A_IMPLIED                           case 0x18: case 0x1b: case 0x38: case 0x3b: case 0x58: case 0x5b: case 0x78: \
                                            case 0x7b: case 0x88: case 0x8a: case 0x98: case 0x9a: case 0x9b: case 0xa8: case 0xaa: \
                                            case 0xb8: case 0xba: case 0xbb: case 0xc8: case 0xca: case 0xcb: case 0xd8: case 0xdb: \
                                            case 0xe8: case 0xea: case 0xeb: case 0xf8:
#define A_XCE                               case 0xfb:
#define A_WDM                               case 0x42:
// s
#define A_STACK                             case 0x08: case 0x0b: case 0x2b: case 0x48: \
                                            case 0x4b: case 0x5a: case 0x60: case 0x68: case 0x6b: case 0x7a: case 0x8b: \
                                            case 0xab: case 0xda: case 0xfa:

#define A_PLP_RTI                           case 0x28: case 0x40:
#define A_PEA_PER                           case 0xf4: case 0x62:
#define A_BRK_COP_PEI                       case 0x00: case 0x02: case 0xd4:

// d
#define A_DIRECT                            case 0x04: case 0x05: case 0x06: case 0x14: case 0x24: case 0x25: case 0x26: case 0x45: \
                                            case 0x46: case 0x64: case 0x65: case 0x66: case 0x84: case 0x85: case 0x86: case 0xa4: \
                                            case 0xa5: case 0xa6: case 0xc4: case 0xc5: case 0xc6: case 0xe4: case 0xe5: case 0xe6:
// d,x
#define A_DIRECT_INDEXED_WITH_X             case 0x15: case 0x16: case 0x34: case 0x35: case 0x36: case 0x55: case 0x56: case 0x74: \
                                            case 0x75: case 0x76: case 0x94: case 0x95: case 0xb4: case 0xb5: case 0xd5: case 0xd6: \
                                            case 0xf5: case 0xf6:
// d,y
#define A_DIRECT_INDEXED_WITH_Y             case 0x96: case 0xb6:
// (d)
#define A_DIRECT_INDIRECT                   case 0x12: case 0x32: case 0x52: case 0x72: case 0x92: case 0xb2: case 0xd2: case 0xf2:
// (d,x)
#define A_INDEXED_INDIRECT                  case 0x01: case 0x21: case 0x41: case 0x61: case 0x81: case 0xa1: case 0xc1: case 0xe1:
// (d),y
#define A_INDIRECT_INDEXED                  case 0x11: case 0x31: case 0x51: case 0x71: case 0x91: case 0xb1: case 0xd1: case 0xf1:
// [d]
#define A_DIRECT_INDIRECT_LONG              case 0x07: case 0x27: case 0x47: case 0x67: case 0x87: case 0xa7: case 0xc7: case 0xe7:
// [d],y
#define A_DIRECT_INDIRECT_LONG_INDEXED      case 0x17: case 0x37: case 0x57: case 0x77: case 0x97: case 0xb7: case 0xd7: case 0xf7:
// a
#define A_ABSOLUTE                          case 0x0c: case 0x0d: case 0x0e: case 0x1c: case 0x20: case 0x2c: case 0x2d: case 0x2e: \
                                            case 0x4c: case 0x4d: case 0x4e: case 0x6d: case 0x6e: case 0x8c: case 0x8d: case 0x8e: \
                                            case 0x9c: case 0xac: case 0xad: case 0xae: case 0xcd: case 0xce: case 0xec: case 0xed: \
                                            case 0xee: case 0xcc:
// a,x
#define A_ABSOLUTE_INDEXED_WITH_X           case 0x1d: case 0x1e: case 0x3c: case 0x3d: case 0x3e: case 0x5d: case 0x5e: case 0x7d: \
                                            case 0x7e: case 0x9d: case 0x9e: case 0xbc: case 0xbd: case 0xdd: case 0xde: case 0xfd: \
                                            case 0xfe:
// a,y
#define A_ABSOLUTE_INDEXED_WITH_Y           case 0x19: case 0x39: case 0x59: case 0x79: case 0x99: case 0xb9: case 0xbe: case 0xd9: \
                                            case 0xf9:
// al
#define A_ABSOLUTE_LONG                     case 0x0f: case 0x22: case 0x2f: case 0x4f: case 0x5c: case 0x6f: case 0x8f: case 0xaf: \
                                            case 0xcf: case 0xef:
// al,x
#define A_ABSOLUTE_LONG_INDEXED             case 0x1f: case 0x3f: case 0x5f: case 0x7f: case 0x9f: case 0xbf: case 0xdf: case 0xff:
// d,s
#define A_STACK_RELATIVE                    case 0x03: case 0x23: case 0x43: case 0x63: case 0x83: case 0xa3: case 0xc3: case 0xe3:
// (d,s),y
#define A_STACK_RELATIVE_INDIRECT_INDEXED   case 0x13: case 0x33: case 0x53: case 0x73: case 0x93: case 0xb3: case 0xd3: case 0xf3:
// (a)
#define A_ABSOLUTE_INDIRECT                 case 0x6c: case 0xdc:
// (a,x)
#define A_ABSOLUTE_INDEXED_INDIRECT         case 0x7c: case 0xfc:
// xyz
#define A_BLOCK_MOVE                        case 0x44: case 0x54:

#define PeekByte(o) (memSnap ? *(memSnap + (o)) : PEEK_BYTE( _pc + (o) ))
#define PeekWord    (memSnap ? *(memSnap + 1) | (*(memSnap + 2) << 8) : PEEK_BYTE( _pc + 1 ) | (PEEK_BYTE( _pc + 2 ) << 8))
#define PeekLong    (PeekWord | (PEEK_BYTE(3) << 16))

auto W65816::disassemble(uint32_t addr, unsigned& bytes, const uint8_t* memSnap) -> std::string {
    DasmHandler65816 d;
    uint32_t _pc = addr;
    uint8_t opcode = PeekByte(0);
    uint16_t operand;
    uint8_t mask = 0x20; // m
    bytes = 2;
    d.Ins( opcode ).tab();

    switch (opcode) {
        A_IMMEDIATE_X   mask = 0x10;
        A_IMMEDIATE
            operand = PeekByte(1);
            if ((p & mask) == 0) { // danger, register length may have changed.
                bytes = 3;
                operand |= PeekByte(2) << 8;
            }
            d.immediate( operand );
            break;
        A_REP_SEP // could change register length
            operand = PeekByte(1);
            d.immediate( operand );
            break;
        A_ACCUMULATOR
        A_IMPLIED
        A_STACK
            bytes = 1;
            break;
        A_PC_RELATIVE
            d.absolute( _pc + 2 + static_cast<int8_t>(PeekByte( 1 )) );
            break;
        A_PC_RELATIVE_LONG
            bytes = 3;
            d.absolute( _pc + 2 + static_cast<int16_t>(PeekWord) );
            break;
        A_XCE
        A_PLP_RTI // could change register length
            bytes = 1;
            break;
        A_WDM
            break;
        A_PEA_PER
            bytes = 3;
            break;
        A_BRK_COP_PEI
            break;
        A_DIRECT
            d.direct(PeekByte( 1 ));
            break;
        A_DIRECT_INDEXED_WITH_X
            d.directIndexedX(PeekByte( 1 ));
            break;
        A_DIRECT_INDEXED_WITH_Y
            d.directIndexedY(PeekByte( 1 ));
            break;
        A_DIRECT_INDIRECT
            d.indirect(PeekByte( 1 ));
            break;
        A_INDEXED_INDIRECT
            d.indexedIndirect(PeekByte( 1 ));
            break;
        A_INDIRECT_INDEXED
            d.indirectIndexed(PeekByte( 1 ));
            break;
        A_DIRECT_INDIRECT_LONG
            d.directIndirectLong(PeekByte( 1 ));
            break;
        A_DIRECT_INDIRECT_LONG_INDEXED
            d.indirectIndexedLong(PeekByte( 1 ));
            break;
        A_ABSOLUTE
            bytes = 3;
            d.absolute( PeekWord );
            break;
        A_ABSOLUTE_INDEXED_WITH_X
            bytes = 3;
            d.absoluteIndexedX(PeekWord);
            break;
        A_ABSOLUTE_INDEXED_WITH_Y
            bytes = 3;
            d.absoluteIndexedY(PeekWord);
            break;
        A_ABSOLUTE_LONG
            bytes = 4;
            d.absolute( PeekLong );
            break;
        A_ABSOLUTE_LONG_INDEXED
            bytes = 4;
            d.absoluteIndexedX( PeekLong );
            break;
        A_STACK_RELATIVE
            d.stackRelative(PeekByte( 1 ));
            break;
        A_STACK_RELATIVE_INDIRECT_INDEXED
            d.stackRelativeIndirectIndexed(PeekByte( 1 ));
            break;
        A_ABSOLUTE_INDIRECT
            bytes = 3;
            d.indirect(PeekWord);
            break;
        A_ABSOLUTE_INDEXED_INDIRECT
            bytes = 3;
            d.indexedIndirect(PeekWord);
            break;
        A_BLOCK_MOVE
            bytes = 3;
            d.move(PeekByte( 1 ), PeekByte( 2 ));
            break;
    }

    return d.str;
}

auto W65816::disassembleData(uint32_t addr, unsigned bytes) -> std::string {
    DasmHandler65816 d;
    d.hex24( addr );
    d.str.append( "|" );

    for(unsigned i = 0; i < bytes; ++i) {
        if (i)
            d.str.append( " " );

        d.hex8( PEEK_BYTE( addr + i ) );
    }
    return d.str;
}

auto W65816::flagDebugAction(int action, bool state) -> void {
    if (state)
        control |= action;
    else
        control &= ~action;
}

auto W65816::disassembleTrace(unsigned i, uint8_t& flags) -> std::string {
    DasmHandler65816 d;
    unsigned bytes;
    Emulator::HistoryEntry<uint8_t>* historyEntry = historyHandler.get(i);
    if (!historyEntry)
        return "";
    d.hex24( historyEntry->addr );
    d.str.append( "|" );
    d.str.append( disassemble( historyEntry->addr, bytes, &historyEntry->mem[0]) );
    flags = historyEntry->flags;
    return d.str;
}

auto W65816::checkSoftStop(uint32_t addr) -> bool {
    if (!softStep.has_value() || softStep.value_or(0) == addr) {
        control &= ~SoftStop;
        return true;
    }
    return false;
}

auto W65816::debuggerStepOver() -> void {
    unsigned bytes;
    disassemble( pcEdge, bytes );
    softStep = (pcEdge & 0xff0000) | ((pcEdge + bytes) & 0xffff);
    control |= SoftStop;
}

auto W65816::debuggerStepInto() -> void {
    softStep = std::nullopt;
    control |= SoftStop;
}

auto W65816::debuggerStepOut() -> bool {
    if (stepOuts.empty())
        return false;
    softStep = stepOuts.back();
    control |= SoftStop;
    return true;
}

inline auto W65816::appendStepOut(uint32_t addr) -> void {
    if (stepOuts.size() == stepOuts.capacity())
        stepOuts.clear();
    stepOuts.push_back( addr );
}

auto W65816::init() -> void {
    stepOuts.reserve(32);

    watchPoints.callback = [this](bool state) { this->flagDebugAction( WatchPoint, state ); };
    watchPointsWrite.callback = [this](bool state) { this->flagDebugAction( WatchPointWrite, state ); };
    breakPoints.callback = [this](bool state) { this->flagDebugAction( BreakPoint, state ); };
    exceptionPoints.callback = [this](bool state) { this->flagDebugAction( ExceptionPoint, state ); };
    modifiedCode.callback = [this](bool state) { this->flagDebugAction( ModifiedCode, state ); };
    historyHandler.callback = [this](bool state) { this->flagDebugAction( History, state ); };
}

}
