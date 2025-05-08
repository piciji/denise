
#pragma once

#include <vector>
#include <algorithm>
#include <string>
#include <cstdint>
#include <stack>
#include <set>
#include "sectorBlock.h"
#include "filesystem.h"
#include "../interface.h"

namespace Emulator {
    struct Interface;
}

namespace LIBAMI {

// For very large hard disks, only the relevant sectors are read instead of importing a complete memory image.
struct FilesystemExt : Filesystem {
    FilesystemExt(uint64_t size, Structure structure = Structure::FFS, unsigned bSize = 512);
    ~FilesystemExt();
    
    Emulator::Interface* interface;
    Emulator::Interface::Media* media;
    uint8_t* buffer;
    unsigned offset;
    
    auto setDataSource(Emulator::Interface* interface, Emulator::Interface::Media* media, unsigned offset) -> void;
    auto importMedia(uint8_t* data, unsigned size) -> bool = delete;
    auto exportMedia(uint8_t* data, unsigned size) -> bool = delete;

    // Currently only adapted for reading the file system and dir structure
    auto getBlock(unsigned ref) -> SectorBlock*;
    auto getBlock(unsigned ref, SectorBlock::Type type) -> SectorBlock*;
};

}
