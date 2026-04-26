
#pragma once

#include "cpuDebugger.h"

struct Drive10CpuDebugger : CpuDebugger {
    explicit Drive10CpuDebugger( Emulator::Interface* emulator )
    : CpuDebugger( emulator ) {
    }

    auto saveIdent() -> std::string override {
        return "debugger_drive10cpu";
    }

    auto titleIdent() -> std::string override {
        return emulator->ident + " Debugger Drive 10 CPU";
    }

    auto isDriveCpu() -> bool override { return true; }

    auto getTheme() -> DebuggerTheme override { return DebuggerTheme::Drive10CPU; }

    auto getDriveId() -> unsigned override { return 2; }
};
