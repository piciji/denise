
#pragma once

#include "../../tools/watcher.h"
#include "../../interface.h"
#include <cstdint>
#include <string>
#include <optional>

namespace LIBC64 {

typedef Emulator::Interface::DebuggerAction DebuggerAction;

struct M65Debugger {
    friend struct WatchPoints;
    friend struct ModifiedCodes;
    friend struct HistoryHandler;

    M65Debugger();
    virtual ~M65Debugger() = default;

    enum {
        WatchPoint = 8, WatchPointWrite = 0x10, BreakPoint = 0x20, ExceptionPoint = 0x40,
        SoftStop = 0x80, ModifiedCode = 0x100, History = 0x200
    };

    Emulator::WatchPoints watchPoints = Emulator::WatchPoints();
    Emulator::WatchPoints watchPointsWrite = Emulator::WatchPoints();
    Emulator::WatchPoints breakPoints = Emulator::WatchPoints();
    Emulator::WatchPoints exceptionPoints = Emulator::WatchPoints();
    Emulator::ModifiedCodes modifiedCode = Emulator::ModifiedCodes();
    Emulator::HistoryHandler<uint8_t> historyHandler = Emulator::HistoryHandler<uint8_t>();
    std::optional<uint16_t> softStep = std::nullopt;
    std::vector<uint16_t> stepOuts;
    uint16_t pcEdge;

    auto init() -> void;

    auto disassemble(uint16_t addr, unsigned& bytes, const uint8_t* memSnap = nullptr) -> std::string;
    auto disassembleData(uint16_t addr, unsigned bytes) -> std::string;
    auto disassembleTrace(unsigned i, uint8_t& flags) -> std::string;

    auto debuggerAdd(DebuggerAction action, uint16_t addr, uint16_t addrTo = 0) -> void;
    auto debuggerRemove(DebuggerAction action, uint16_t addr) -> void;
    auto debuggerRemove(DebuggerAction action) -> void;

    auto checkSoftStop(uint16_t addr) -> bool;
    auto debuggerStepOver() -> void;
    auto debuggerStepInto() -> void;
    auto debuggerStepOut() -> bool;

    auto appendStepOut(uint16_t addr) -> void;

    auto setWatchpointCondition(DebuggerAction action, unsigned addr, unsigned hitCount, unsigned hitCountMode, const std::string& expression, unsigned expressionMode) -> bool;

    auto hasModifiedCode() -> bool { return modifiedCode.getAndForget(); }

    virtual auto parseExpressionValue(const std::string& input, int& pos) -> uint32_t = 0;
    virtual auto peek(uint16_t addr) -> uint8_t = 0;
    virtual auto flagDebugAction(int action, bool state) -> void = 0;
};

}
