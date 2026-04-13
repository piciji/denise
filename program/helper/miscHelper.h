
#pragma once

#include "../../emulation/interface.h"
#include "../../guikit/api.h"

struct MiscHelper {
    static auto addCustomFont() -> void;

    static auto libraryMissing(std::string plugin) -> void;
    static auto initExpansionRom(Emulator::Interface* emulator, const std::string& ident, const std::string& file) -> void;
    static auto setExpansionSelection( Emulator::Interface* emulator ) -> void;
    static auto removeExpansion( bool bootableOnly = true ) -> void;
    static auto prepareSocket(Emulator::Interface::Media* media, Emulator::Interface* emulator, std::string address) -> void;
    static auto toggle2Mhz() -> void;
    static auto hasSuperCpuActive() -> bool;
};

