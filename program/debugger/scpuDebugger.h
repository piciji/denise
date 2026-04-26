
#pragma once

#include "cpuDebugger.h"

struct ScpuDebugger : CpuDebugger {
    explicit ScpuDebugger( Emulator::Interface* emulator )
    : CpuDebugger( emulator ) {
    }

    auto saveIdent() -> std::string override {
        return "debugger_scpu";
    }

    auto titleIdent() -> std::string override {
        return emulator->ident + " Debugger SCPU";
    }

    auto getTheme() -> DebuggerTheme override { return DebuggerTheme::SCPU; }
};
