#pragma once

#include <string>
#include <vector>

#include "../../emulation/interface.h"

namespace GUIKIT {
    struct Settings;
}

struct RecentFiles {

    std::string path;

    Emulator::Interface* emulator;

    const int maxEntries = 30;

    RecentFiles(Emulator::Interface* emulator, const std::string& path);

    ~RecentFiles();

    GUIKIT::Settings* settings = nullptr;

    auto load() -> void;

    auto save() -> void;

    auto add(Emulator::Interface::MediaGroup& group, const std::string& path) -> void;

    auto list(Emulator::Interface::MediaGroup& group, const std::string& curPath = "") -> std::vector<std::string>;

    auto getIdent(Emulator::Interface::MediaGroup& group, unsigned pos) -> std::string;
};
