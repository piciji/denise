
#pragma once

#include "cpuDebugger.h"

struct ScpuDebugger : CpuDebugger {
    explicit ScpuDebugger( Emulator::Interface* emulator )
    : CpuDebugger( emulator, Mode::SCPU ) {
        build();
    }

    auto screenIdent() -> std::string override {
        return "debugger_scpu";
    }

    auto titleIdent() -> std::string override {
        return emulator->ident + " Debugger SCPU";
    }
};
