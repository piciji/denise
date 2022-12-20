
#pragma once

#include <vector>
#include <algorithm>
#include <string>
#include <cstdint>
#include "sectorBlock.h"
#include "../interface.h"

namespace LIBAMI {

struct SectorBlock;

struct Filesystem {
    enum class Structure { OFS, FFS } structure;
    Filesystem(Structure structure, unsigned size, unsigned bSize = 512);
    ~Filesystem();

    unsigned bSize;
    unsigned blockCount;

    std::vector<SectorBlock*> blocks;
    std::vector<unsigned> bmBlockRefs;

    auto format(std::string name = "", bool bootblock = false) -> void;
    auto exportMedia(uint8_t* data, unsigned size) -> bool;
    auto importMedia(uint8_t* data, unsigned size) -> bool;
    auto clear() -> void;

    auto markBlockAsFree(unsigned ref) -> void;
    auto markBlockAsAllocated(unsigned ref) -> void;
    auto accessBitmapAllocation(unsigned ref, int update = -1 ) -> bool;
    auto calculateChecksums() -> void;
    auto addBootblock() -> void;

    auto getDirectory() -> Emulator::Interface::Listing&;
    auto traverseDir( SectorBlock* from, std::vector<SectorBlock*>& storage, Emulator::Interface::Listing* listing ) -> void;

    auto getBmExtBlock(unsigned ref) -> SectorBlock*;
    auto getBitmapBlock(unsigned ref) -> SectorBlock*;
    auto getHashChainBlock(unsigned ref) -> SectorBlock*;
    auto getBlock(unsigned ref) -> SectorBlock*;

    auto getRootBlockRef() -> unsigned { return blockCount >> 1; }
    auto countBitmapPointersEachExtBlock() const -> unsigned { return (bSize / 4) - 1; } // each ext bitmap block can points to this much bitmap blocks
    auto countBitsEachBitmapBlock() const -> unsigned { return (bSize - 4) * 8; } // for this much blocks a single bitmap block describres allocation
    auto predictType(unsigned ref, uint8_t* buffer) -> SectorBlock::Type;

    auto referenceBitmaps() -> void;


    template<typename T>
    static auto find(std::vector<T>& v, T element) -> bool {
        return std::find(v.begin(), v.end(), element) != v.end();
    }
};

}
