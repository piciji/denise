
#pragma once

#include "memDebugger.h"

struct Drive9MemDebugger : MemDebugger {
    explicit Drive9MemDebugger( Emulator::Interface* emulator )
    : MemDebugger( emulator ) {
    }

    auto saveIdent() -> std::string override {
        return "debugger_drive9mem";
    }

    auto titleIdent() -> std::string override {
        return emulator->ident + " Debugger Drive 9 Memory";
    }

    auto getTheme() -> DebuggerTheme override { return DebuggerTheme::Drive9Memory; }

    auto getDriveId() -> unsigned override { return 1; }

    auto isDriveMem() -> bool override { return true; }
};
