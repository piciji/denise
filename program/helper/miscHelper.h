
#pragma once

#include "../../emulation/interface.h"
#include "../../guikit/api.h"

struct MiscHelper {

    struct DisplayFont {
        std::string file;
        std::string name;
        unsigned index;
        uint16_t ident;

        auto getMode() const -> uint8_t {
            return (ident >> 14) & 3;
        }
    };
    static std::vector<DisplayFont> displayFonts;

    static auto libraryMissing(std::string plugin) -> void;
    static auto initExpansionRom(Emulator::Interface* emulator, const std::string& ident, const std::string& file) -> void;
    static auto setExpansionSelection( Emulator::Interface* emulator ) -> void;
    static auto removeExpansion( bool bootableOnly = true ) -> void;
    static auto prepareSocket(Emulator::Interface::Media* media, Emulator::Interface* emulator, std::string address) -> void;
    static auto toggle2Mhz() -> void;
    static auto hasSuperCpuActive() -> bool;

    static auto addCustomFont() -> void;
    static auto addTTF() -> void;
    static auto addTTF(unsigned mode, const std::string& _fontFile) -> void;
    static auto getTTF(uint16_t ident) -> DisplayFont*;
    static auto getTTF(const std::string& file, int fontIndex) -> DisplayFont*;
    static auto removeTTF(const std::string& file, uint8_t mode) -> bool;
};

