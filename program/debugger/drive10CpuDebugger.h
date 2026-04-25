
#pragma once

#include "cpuDebugger.h"

struct Drive10CpuDebugger : CpuDebugger {
    explicit Drive10CpuDebugger( Emulator::Interface* emulator )
    : CpuDebugger( emulator, Mode::Drive10CPU ) {
        build();
    }

    auto saveIdent() -> std::string override {
        return "debugger_drive10cpu";
    }

    auto titleIdent() -> std::string override {
        return emulator->ident + " Debugger Drive 10 CPU";
    }

    auto isDriveCpu() -> bool override { return true; }
};
