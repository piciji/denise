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

    static const int maxEntries = 30;

    GUIKIT::Settings* settings = nullptr;

    struct Storage {
        Emulator::Interface::MediaGroup* group;
        std::vector<std::string> files;
    };

    std::vector<Storage*> storage;

    auto load() -> void;

    auto save() -> void;

    auto add(Emulator::Interface::MediaGroup* group, const std::string& curPath) -> void;

    auto list(Emulator::Interface::MediaGroup* group, const std::string& curPath = "") -> std::vector<std::string>&;

    auto getIdent(Emulator::Interface::MediaGroup* group, unsigned pos) -> std::string;

    auto getStorage(Emulator::Interface::MediaGroup* group) -> Storage*;

    auto clear(Emulator::Interface::MediaGroup* group = nullptr) -> void;
};
