
#pragma once

#include "../../emulation/interface.h"
#include "../../guikit/api.h"

struct PaletteManager {
    
    PaletteManager(Emulator::Interface* emulator);
    ~PaletteManager();
    static auto getInstance( Emulator::Interface* emulator ) -> PaletteManager*;
    
    Emulator::Interface* emulator;
    GUIKIT::Settings* palSettings;
    
    auto save() -> bool;
    
    auto find( unsigned id ) -> Emulator::Interface::Palette*;
    auto load() -> void;
    auto sanitize() -> void;
    auto reorder(Emulator::Interface::Palette* palette) -> bool;
    auto getSize() -> unsigned;
    auto getIdent(unsigned i) -> std::string;
    auto path() -> std::string;
    auto rebuildSettings() -> void;
    auto add(Emulator::Interface::Palette& copy) -> Emulator::Interface::Palette&;
    auto remove( Emulator::Interface::Palette& palette ) -> void;
    auto getCurrentPalette() -> Emulator::Interface::Palette*;
};

extern std::vector<PaletteManager*> paletteManagers;