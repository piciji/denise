
#pragma once

#include <vector>
#include <algorithm>
#include <string>
#include <cstdint>
#include <stack>
#include <set>
#include "sectorBlock.h"
#include "../interface.h"

namespace LIBAMI {

struct SectorBlock;

struct Filesystem {
    enum class Structure { OFS, FFS } structure;
    Filesystem(uint64_t size, Structure structure = Structure::OFS, unsigned bSize = 512);
    virtual ~Filesystem();

    unsigned bSize;
    unsigned blockCount;
    SectorBlock* rootBlock = nullptr;
    SectorBlock* cd = nullptr;

    std::vector<SectorBlock*> blocks;
    std::vector<unsigned> bmBlockRefs;

    auto format(std::string name = "", bool bootblock = false) -> void;
    auto exportMedia(uint8_t* data, unsigned size) -> bool;
    auto importMedia(uint8_t* data, unsigned size) -> bool;
    auto clear() -> void;
    auto isOFS() -> bool { return structure == Structure::OFS; }
    auto volSize() -> unsigned { return blockCount * bSize; }

    auto markBlockAsFree(unsigned ref) -> void;
    auto markBlockAsAllocated(unsigned ref) -> void;
    auto isFree(unsigned ref) -> bool { return accessBitmapAllocation(ref); }
    auto countFreeBlocks() -> unsigned;
    auto accessBitmapAllocation(unsigned ref, int update = -1 ) -> bool;
    auto calculateChecksums() -> void;
    auto addBootblock() -> void;

    auto getDirectory(bool noFilesInSubdirs = false) -> std::vector<Emulator::Interface::Listing>;
    auto getPathRaw(SectorBlock* block) -> std::vector<uint16_t>;
    auto traverse( SectorBlock* from, std::stack<SectorBlock*>& result ) -> void;

    auto getBitmapExtBlock(unsigned ref) -> SectorBlock*;
    auto getBitmapBlock(unsigned ref) -> SectorBlock*;
    auto getHashTableBlock(unsigned ref) -> SectorBlock*;
    auto getExtensionBlock(unsigned ref) -> SectorBlock*;
    virtual auto getBlock(unsigned ref) -> SectorBlock*;

    auto getRootBlockRef() -> unsigned { return blockCount >> 1; }
    auto countBitmapPointersEachExtBlock() const -> unsigned { return (bSize / 4) - 1; } // each ext bitmap block can points to this much bitmap blocks
    auto countBitsEachBitmapBlock() const -> unsigned { return (bSize - 4) * 8; } // for this much blocks a single bitmap block describres allocation
    auto predictType(unsigned ref, uint8_t* buffer) -> SectorBlock::Type;

    auto currentDir() -> SectorBlock*;
    auto allocateEmptyBlock() -> SectorBlock*;
    auto placeNewBlock(SectorBlock::Type type) -> SectorBlock*;
    auto createFile(const std::string& name, const std::string& str) -> SectorBlock* { return createBase(name, false, (uint8_t*)str.c_str(), str.size()); }
    auto createFile(const std::string& name, uint8_t* data = nullptr, unsigned size = 0) -> SectorBlock* { return createBase(name, false, data, size); }
    auto createDir(const std::string& name) -> SectorBlock* { return createBase(name, true); }
    auto createBase(const std::string& name, bool isDir, uint8_t* data = nullptr, unsigned size = 0) -> SectorBlock*;
    auto getLastChainBlock(SectorBlock* block) -> SectorBlock*;
    auto seek(std::string name) -> SectorBlock*;
    auto changeDir(const std::string& name) -> SectorBlock*;
    auto requiredDataBlocks(unsigned fileSize) -> unsigned;
    auto requiredExtensionBlocks(unsigned fileSize) -> unsigned;
    auto writeData(SectorBlock* fhBlock, uint8_t* data, unsigned size) -> bool;
    auto referenceDataBlock(SectorBlock* fhBlock, SectorBlock* dataBlock) -> bool;

    auto referenceBitmaps() -> void;


    template<typename T>
    static auto find(std::vector<T>& v, T element) -> bool {
        return std::find(v.begin(), v.end(), element) != v.end();
    }
    template<typename T>
    static auto combine(std::vector<T>& target, const std::vector<T>& source, bool append = false) -> void {
        target.insert( append ? target.end() : target.begin(), source.begin(), source.end());
    }
};

}
