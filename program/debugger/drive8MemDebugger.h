
#pragma once

#include "memDebugger.h"

struct Drive8MemDebugger : MemDebugger {
    explicit Drive8MemDebugger( Emulator::Interface* emulator )
    : MemDebugger( emulator ) {
    }

    auto saveIdent() -> std::string override {
        return "debugger_drive8mem";
    }

    auto titleIdent() -> std::string override {
        return emulator->ident + " Debugger Drive 8 Memory";
    }

    auto getTheme() -> DebuggerTheme override { return DebuggerTheme::Drive8Memory; }

    auto isDriveMem() -> bool override { return true; }

    auto getDriveId() -> unsigned override { return 0; }
};
