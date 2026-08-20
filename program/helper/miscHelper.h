
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
    static auto removeExpansion( Emulator::Interface* emulator, bool bootableOnly = false ) -> void;
    static auto prepareSocket(Emulator::Interface::Media* media, Emulator::Interface* emulator, std::string address) -> void;
    static auto toggle2Mhz() -> void;
    static auto hasSuperCpuActive() -> bool;

    static auto addFileFonts() -> void;
    static auto addFonts() -> void;
    static auto addFonts(unsigned mode, const std::string& _fontFile) -> void;
    static auto getFont(uint16_t ident) -> DisplayFont*;
    static auto getFont(const std::string& file, int fontIndex) -> DisplayFont*;
    static auto removeFont(const std::string& file, uint8_t mode) -> bool;

    static auto applyGeometry(GUIKIT::Window* window, GUIKIT::Settings* settings, const std::string& ident, GUIKIT::Geometry defGeo) -> void;
    static auto centerGeometry(GUIKIT::Window* window, GUIKIT::Size _size, GUIKIT::Geometry _containerGeo) -> void;
};

