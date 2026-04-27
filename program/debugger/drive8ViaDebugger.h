
#pragma once

#include "viaDebugger.h"

struct Drive8ViaDebugger : ViaDebugger {
    explicit Drive8ViaDebugger( Emulator::Interface* emulator )
    : ViaDebugger( emulator ) {
    }

    auto saveIdent() -> std::string override {
        return "debugger_drive8via";
    }

    auto titleIdent() -> std::string override {
        return emulator->ident + " Debugger Drive 8 VIA";
    }

    auto getTheme() -> DebuggerTheme override { return DebuggerTheme::Drive8VIA; }

    auto getDriveId() -> unsigned override { return 0; }
};
