
#pragma once

#include "memDebugger.h"

struct MemScpuDebugger : MemDebugger {
    explicit MemScpuDebugger( Emulator::Interface* emulator )
    : MemDebugger( emulator, Mode::MemorySCPU ) {
        build();
    }

    auto saveIdent() -> std::string override {
        return "debugger_memscpu";
    }

    auto titleIdent() -> std::string override {
        return emulator->ident + " Debugger Memory SCPU";
    }
};
