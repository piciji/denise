
#pragma once

#include "viaDebugger.h"

struct Drive9ViaDebugger : ViaDebugger {
    explicit Drive9ViaDebugger( Emulator::Interface* emulator )
    : ViaDebugger( emulator ) {
    }

    auto saveIdent() -> std::string override {
        return "debugger_drive9via";
    }

    auto titleIdent() -> std::string override {
        return emulator->ident + " Debugger Drive 9 VIA";
    }

    auto getTheme() -> DebuggerTheme override { return DebuggerTheme::Drive9VIA; }

    auto getDriveId() -> unsigned override { return 1; }
};
