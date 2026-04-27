
#include "m68000.h"
#include "../agnus/agnus.h"
#include "../../tools/serializer.h"
#include "../system/debuggerSnapshot.h"

namespace LIBAMI {

Cpu::Cpu(Agnus& agnus) : M68FAMILY::M68000(agnus) {

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

auto Cpu::debuggerAdd(DebuggerAction action, unsigned addr, unsigned addrTo) -> void {
    switch (action) {
        case DebuggerAction::Breakpoint:        breakPoints.add( addr ); break;
        case DebuggerAction::Watchpoint:        watchPoints.add( addr ); break;
        case DebuggerAction::WatchpointWrite:   watchPointsWrite.add( addr ); break;
        case DebuggerAction::ExceptionPoint:    exceptionPoints.add( addr ); break;
        case DebuggerAction::ModifiedCode:      modifiedCode.add( addr, addrTo ); break;
        case DebuggerAction::History:           historyHandler.enable(); break;
        default:
            break;
    }
}

auto Cpu::debuggerRemove(DebuggerAction action, unsigned addr) -> void {
    switch (action) {
        case DebuggerAction::Breakpoint:        breakPoints.remove( addr ); break;
        case DebuggerAction::Watchpoint:        watchPoints.remove( addr ); break;
        case DebuggerAction::WatchpointWrite:   watchPointsWrite.remove( addr ); break;
        case DebuggerAction::ExceptionPoint:    exceptionPoints.remove( addr ); break;
        case DebuggerAction::History:           historyHandler.disable( ); break;
        case DebuggerAction::ModifiedCode:      modifiedCode.disable(); break;
        default:
            break;
    }
}

auto Cpu::debuggerRemove(DebuggerAction action) -> void {
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

auto Cpu::parseExpressionValue(const std::string& input, int& pos) -> uint32_t {
    for (auto& cond : DebuggerSnapshot::breakConditions) {
        std::string token = cond.ident;
        if (input.compare(pos, token.size(), token) == 0) {
            pos += token.size();

            switch (cond.vector) {
                default: return 0;
                case 0: return ref.vPos;
                case 1: return ref.hPos;
                case 2: return irc;
                case 3: return ird;
                case 4: return iplPins;
                case 5: return pc;
                case 6: return usp;
                case 7: return ssp;

                case 8: return regsA[0];
                case 9: return regsA[1];
                case 10: return regsA[2];
                case 11: return regsA[3];
                case 12: return regsA[4];
                case 13: return regsA[5];
                case 14: return regsA[6];
                case 15: return regsA[7];

                case 16: return regsD[0];
                case 17: return regsD[1];
                case 18: return regsD[2];
                case 19: return regsD[3];
                case 20: return regsD[4];
                case 21: return regsD[5];
                case 22: return regsD[6];
                case 23: return regsD[7];

                case 24: return c;
                case 25: return v;
                case 26: return z;
                case 27: return n;
                case 28: return x;
                case 29: return i;
                case 30: return s;

                case 100: {
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
                        return ref.peekWord(value);
                    }
                    return 0;
                }
            }
        }
    }
    return 0;
}

auto Cpu::setWatchpointCondition(DebuggerTheme theme, DebuggerAction action, unsigned addr, unsigned hitCount, unsigned hitCountMode, const std::string& expression, unsigned expressionMode) -> bool {
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

    if (theme == DebuggerTheme::Copper) {
        switch (action) {
            case DebuggerAction::Breakpoint:
                ref.copper.breakPoints.setBreakpointCondition( addr, hitCount, hitCountMode, expressionError ? "" : expression, expressionMode );
                break;
            case DebuggerAction::Watchpoint:
                ref.copper.watchPoints.setBreakpointCondition( addr, hitCount, hitCountMode, expressionError ? "" : expression, expressionMode );
                break;
            default: break;
        }
    } else {
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
    }

    return !expressionError;
}

auto Cpu::mnemonic() -> const char* {
    static const char* intrMnemonics[] {
        "TRACE", "IRQ"
    };

    if (control & TraceScheduled)
        return intrMnemonics[0];

    if (control & IRQ)
        return intrMnemonics[1];

    return mnemonics[ird];
}

auto Cpu::updateSnapshot(DebuggerSnapshot& snap) -> void {
    std::copy(std::begin(regsD), std::end(regsD), std::begin(snap.regsD));
    std::copy(std::begin(regsA), std::end(regsA), std::begin(snap.regsA));
    snap.pc = pc;
    snap.pcEdge = pcEdge;
    snap.irc = irc;
    snap.ird = ird;
    snap.usp = usp;
    snap.ssp = ssp;
    snap.flags = getSR();

    snap.ipl = iplPins;
    snap.stp = control & Stop;
    snap.hlt = control & Halt;
}

auto Cpu::serialize(Emulator::Serializer& s) -> void {
    s.array(regsD);
    s.array(regsA);
    s.integer(pc);
    s.integer(usp);
    s.integer(ssp);
    s.integer(irc);
    s.integer(ird);
    s.integer(c);
    s.integer(v);
    s.integer(z);
    s.integer(n);
    s.integer(x);
    s.integer(i);
    s.integer(this->s);
    s.integer(iplPins);
    s.integer(iplSample);
    s.integer(control);
}

}
