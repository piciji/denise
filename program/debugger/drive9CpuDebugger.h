
#pragma once

#include "cpuDebugger.h"

struct Drive9CpuDebugger : CpuDebugger {
    explicit Drive9CpuDebugger( Emulator::Interface* emulator )
    : CpuDebugger( emulator, Mode::Drive9CPU ) {
        build();
    }

    auto saveIdent() -> std::string override {
        return "debugger_drive9cpu";
    }

    auto titleIdent() -> std::string override {
        return emulator->ident + " Debugger Drive 9 CPU";
    }

    auto isDriveCpu() -> bool override { return true; }
};
