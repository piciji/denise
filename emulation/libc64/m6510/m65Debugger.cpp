
#include "m65Debugger.h"
#include "dasmHandler.h"

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

#define PeekByte(o)         (memSnap ? *(memSnap + (o)) : peek( _pc + (o) ))
#define PeekWord            (memSnap ? *(memSnap + 1) | (*(memSnap + 2) << 8) : peek( _pc + 1 ) | (peek( _pc + 2 ) << 8))

namespace LIBC64 {

M65Debugger::M65Debugger() {
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

auto M65Debugger::init() -> void {
    stepOuts.clear();

    watchPoints.reset();
    watchPointsWrite.reset();
    breakPoints.reset();
    exceptionPoints.reset();
    modifiedCode.disable();
    historyHandler.flagWhenNeeded();
}

auto M65Debugger::appendStepOut(uint16_t addr) -> void {
    if (stepOuts.size() == stepOuts.capacity())
        stepOuts.clear();
    stepOuts.push_back( addr );
}

auto M65Debugger::disassemble(uint16_t addr, unsigned& bytes, const uint8_t* memSnap) -> std::string {
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

auto M65Debugger::disassembleData(uint16_t addr, unsigned bytes) -> std::string {
    DasmHandler d;
    d.hex16( addr );
    d.str.append( "|" );

    for(unsigned i = 0; i < bytes; ++i) {
        if (i)
            d.str.append( " " );

        d.hex8( peek( addr + i ) );
    }
    return d.str;
}

auto M65Debugger::disassembleTrace(unsigned i, uint8_t& flags) -> std::string {
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

auto M65Debugger::checkSoftStop(uint16_t addr) -> bool {
    if (!softStep.has_value() || softStep.value_or(0) == addr) {
        flagDebugAction(SoftStop, false);
        return true;
    }
    return false;
}

auto M65Debugger::debuggerStepOver() -> void {
    unsigned bytes;
    disassemble( pcEdge, bytes );

    softStep = pcEdge + bytes;
    flagDebugAction(SoftStop, true);
}

auto M65Debugger::debuggerStepInto() -> void {
    softStep = std::nullopt;
    flagDebugAction(SoftStop, true);
}

auto M65Debugger::debuggerStepOut() -> bool {
    if (stepOuts.empty())
        return false;
    softStep = stepOuts.back();
    flagDebugAction(SoftStop, true);
    return true;
}

auto M65Debugger::debuggerAdd(DebuggerAction action, uint16_t addr, uint16_t addrTo) -> void {
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

auto M65Debugger::debuggerRemove(DebuggerAction action, uint16_t addr) -> void {
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

auto M65Debugger::debuggerRemove(DebuggerAction action) -> void {
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

auto M65Debugger::setWatchpointCondition(DebuggerAction action, unsigned addr, unsigned hitCount, unsigned hitCountMode, const std::string& expression, unsigned expressionMode) -> bool {
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
        default: break;
    }

    return !expressionError;
}

}