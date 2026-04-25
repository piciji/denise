
#pragma once

#include "cpuDebugger.h"

struct Drive11CpuDebugger : CpuDebugger {
    explicit Drive11CpuDebugger( Emulator::Interface* emulator )
    : CpuDebugger( emulator, Mode::Drive11CPU ) {
        build();
    }

    auto saveIdent() -> std::string override {
        return "debugger_drive11cpu";
    }

    auto titleIdent() -> std::string override {
        return emulator->ident + " Debugger Drive 11 CPU";
    }

    auto isDriveCpu() -> bool override { return true; }
};
