
#pragma once

#include "m68000/m68000.h"
#include "../../interface.h"
#include <string>

namespace Emulator {
    struct Serializer;
}

namespace LIBAMI {

typedef Emulator::Interface::DebuggerAction DebuggerAction;
typedef Emulator::Interface::DebuggerTheme DebuggerTheme;

struct Agnus;
struct DebuggerSnapshot;

struct Cpu : M68FAMILY::M68000 {
    Cpu(Agnus& agnus);

    auto serialize(Emulator::Serializer& s) -> void;

    auto updateSnapshot(DebuggerSnapshot& snap) -> void;

    auto getIRC() const -> uint16_t { return irc; }

    auto getIPL() const -> uint8_t { return iplPins; }

    auto parseExpressionValue(const std::string& input, int& pos) -> uint32_t;

    auto setWatchpointCondition(DebuggerTheme theme, DebuggerAction action, unsigned ident, unsigned hitCount, unsigned hitCountMode, const std::string& expression, unsigned expressionMode) -> bool;

    auto debuggerAdd(DebuggerAction action, unsigned ident, unsigned addr, unsigned addrTo) -> void;
    auto debuggerRemove(DebuggerAction action, unsigned ident) -> void;
    auto debuggerRemove(DebuggerAction action) -> void;

    auto mnemonic() -> const char*;
};

}
