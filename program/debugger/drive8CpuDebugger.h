
#pragma once

#include "cpuDebugger.h"

struct Drive8CpuDebugger : CpuDebugger {
    explicit Drive8CpuDebugger( Emulator::Interface* emulator )
    : CpuDebugger( emulator, Mode::Drive8CPU ) {
        build();
    }

    auto saveIdent() -> std::string override {
        return "debugger_drive8cpu";
    }

    auto titleIdent() -> std::string override {
        return emulator->ident + " Debugger Drive 8 CPU";
    }

    auto isDriveCpu() -> bool override { return true; }
};
