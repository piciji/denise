#pragma once

#include <string>
#include <vector>

#include "../../emulation/interface.h"

namespace GUIKIT {
    struct Settings;
}

#define MAX_RECENT_ENTIRIES 40u

struct RecentFiles {

    RecentFiles(Emulator::Interface* emulator, const std::string& path);

    ~RecentFiles();

    std::string path;

    Emulator::Interface* emulator;

    unsigned genericEntries = 25;
    unsigned groupEntries = 25;

    GUIKIT::Settings* settings = nullptr;

    struct FileIdent {
        std::string path;
        std::string file;
        unsigned id = 0;
    };

    struct Storage {
        Emulator::Interface::MediaGroup* group;
        bool alternate;
        std::vector<FileIdent> files;
    };

    std::vector<Storage*> storage;

    auto load() -> void;

    auto save() -> void;

    auto add(Emulator::Interface::MediaGroup* group, bool alternate, const FileIdent& fileIdent, bool updateGeneric = true) -> void;

    auto list(Emulator::Interface::MediaGroup* group, bool alternate = false, const FileIdent& fileIdent = {"", "", 0}) -> std::vector<FileIdent>&;

    auto getIdentBase(Emulator::Interface::MediaGroup* group, bool alternate) -> std::string;

    auto getIdentPath(Emulator::Interface::MediaGroup* group, bool alternate, unsigned pos) -> std::string;

    auto getIdentFile(Emulator::Interface::MediaGroup* group, bool alternate, unsigned pos) -> std::string;

    auto getIdentId(Emulator::Interface::MediaGroup* group, bool alternate, unsigned pos) -> std::string;

    auto getStorage(Emulator::Interface::MediaGroup* group, bool alternate) -> Storage*;

    auto clear(Emulator::Interface::MediaGroup* group = nullptr, bool alternate = false) -> void;

    auto getEntries(Emulator::Interface::MediaGroup* group = nullptr) -> unsigned;

    auto setGenericEntries(unsigned entries) -> void;
};
