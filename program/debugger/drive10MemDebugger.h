
#pragma once

#include "memDebugger.h"

struct Drive10MemDebugger : MemDebugger {
    explicit Drive10MemDebugger( Emulator::Interface* emulator )
    : MemDebugger( emulator ) {
    }

    auto saveIdent() -> std::string override {
        return "debugger_drive10mem";
    }

    auto titleIdent() -> std::string override {
        return emulator->ident + " Debugger Drive 10 Memory";
    }

    auto getTheme() -> DebuggerTheme override { return DebuggerTheme::Drive10Memory; }

    auto getDriveId() -> unsigned override { return 2; }

    auto isDriveMem() -> bool override { return true; }
};
