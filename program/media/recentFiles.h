#pragma once

#include <string>
#include <vector>

#include "../../emulation/interface.h"

namespace GUIKIT {
    struct Settings;
}

struct RecentFiles {

    RecentFiles(Emulator::Interface* emulator, const std::string& path);

    ~RecentFiles();

    std::string path;

    Emulator::Interface* emulator;

    static const unsigned maxEntries = 40;

    unsigned genericEntries = 25;
    unsigned groupEntries = 25;

    GUIKIT::Settings* settings = nullptr;

    struct Storage {
        Emulator::Interface::MediaGroup* group;
        bool alternate;
        std::vector<std::string> files;
    };

    std::vector<Storage*> storage;

    auto load() -> void;

    auto save() -> void;

    auto add(Emulator::Interface::MediaGroup* group, bool alternate, const std::string& curPath, bool updateGeneric = true) -> void;

    auto list(Emulator::Interface::MediaGroup* group, bool alternate = false, const std::string& curPath = "") -> std::vector<std::string>&;

    auto getIdent(Emulator::Interface::MediaGroup* group, bool alternate, unsigned pos) -> std::string;

    auto getStorage(Emulator::Interface::MediaGroup* group, bool alternate) -> Storage*;

    auto clear(Emulator::Interface::MediaGroup* group = nullptr, bool alternate = false) -> void;

    auto getEntries(Emulator::Interface::MediaGroup* group = nullptr) -> unsigned;

    auto setGenericEntries(unsigned entries) -> void;
};
