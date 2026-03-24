
#pragma once

#include "m68000/m68000.h"
#include "../../interface.h"
#include <string>

namespace Emulator {
    struct Serializer;
}

namespace LIBAMI {

typedef Emulator::Interface::DebuggerAction DebuggerAction;

struct Agnus;
struct DebuggerSnapshot;

struct Cpu : M68FAMILY::M68000 {
    Cpu(Agnus& agnus);

    auto serialize(Emulator::Serializer& s) -> void;

    auto updateSnapshot(DebuggerSnapshot& snap) -> void;

    auto getIRC() const -> uint16_t { return irc; }

    auto getIPL() const -> uint8_t { return iplPins; }

    auto parseExpressionValue(const std::string& input, int& pos) -> uint32_t;

    auto setWatchpointCondition(DebuggerAction action, unsigned addr, unsigned hitCount, unsigned hitCountMode, const std::string& expression, unsigned expressionMode) -> bool;

    auto debuggerAdd(DebuggerAction action, unsigned addr, unsigned addrTo = 0) -> void;
    auto debuggerRemove(DebuggerAction action, unsigned addr) -> void;
    auto debuggerRemove(DebuggerAction action) -> void;

    auto mnemonic() -> const char*;
};

}
