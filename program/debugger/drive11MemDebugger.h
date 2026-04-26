
#pragma once

#include "memDebugger.h"

struct Drive11MemDebugger : MemDebugger {
    explicit Drive11MemDebugger( Emulator::Interface* emulator )
    : MemDebugger( emulator ) {
    }

    auto saveIdent() -> std::string override {
        return "debugger_drive11mem";
    }

    auto titleIdent() -> std::string override {
        return emulator->ident + " Debugger Drive 11 Memory";
    }

    auto getTheme() -> DebuggerTheme override { return DebuggerTheme::Drive11Memory; }

    auto getDriveId() -> unsigned override { return 3; }

    auto isDriveMem() -> bool override { return true; }
};
