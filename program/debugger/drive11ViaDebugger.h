
#pragma once

#include "viaDebugger.h"

struct Drive11ViaDebugger : ViaDebugger {
    explicit Drive11ViaDebugger( Emulator::Interface* emulator )
    : ViaDebugger( emulator ) {
    }

    auto saveIdent() -> std::string override {
        return "debugger_drive11via";
    }

    auto titleIdent() -> std::string override {
        return emulator->ident + " Debugger Drive 11 VIA";
    }

    auto getTheme() -> DebuggerTheme override { return DebuggerTheme::Drive11VIA; }

    auto getDriveId() -> unsigned override { return 3; }
};
