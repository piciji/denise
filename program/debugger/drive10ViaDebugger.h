
#pragma once

#include "viaDebugger.h"

struct Drive10ViaDebugger : ViaDebugger {
    explicit Drive10ViaDebugger( Emulator::Interface* emulator )
    : ViaDebugger( emulator ) {
    }

    auto saveIdent() -> std::string override {
        return "debugger_drive10via";
    }

    auto titleIdent() -> std::string override {
        return emulator->ident + " Debugger Drive 10 VIA";
    }

    auto getTheme() -> DebuggerTheme override { return DebuggerTheme::Drive10VIA; }

    auto getDriveId() -> unsigned override { return 2; }
};
