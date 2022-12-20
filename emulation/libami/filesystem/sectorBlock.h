
#pragma once

#include <cstdint>
#include <ctime>
#include <string>
#include <cstring>

namespace LIBAMI {

struct Filesystem;

struct SectorBlock {
    enum Type { ROOT_BLOCK, BOOT_BLOCK, BITMAP_BLOCK, BITMAP_EXT_BLOCK, DIR_BLOCK, FILE_HEADER_BLOCK, EMPTY_BLOCK } type;

    SectorBlock( Filesystem& filesystem, Type type, unsigned nr);
    ~SectorBlock();

    unsigned nr;
    Filesystem& filesystem;

    uint8_t* data = nullptr;

    auto init() -> void;
    auto setName(std::string name) -> void;
    auto getName() -> std::string;
    auto setBitmapBlockPtr(unsigned pos, unsigned value) -> void;
    auto getBitmapBlockPtr(unsigned pos) -> unsigned;
    auto setBitmapExtBlock(unsigned value) -> void;
    auto getBitmapExtBlock() -> unsigned;
    auto getParentDir() -> unsigned;
    auto getHashRef(unsigned pos) -> unsigned;
    auto getHashChainRef() -> unsigned;

    auto calcChecksum() -> unsigned;
    auto exportBlock(uint8_t* data) -> void;
    auto importBlock(uint8_t* data) -> void;

    auto bSize() -> unsigned;
    auto getAdrPtr(int offset) -> uint8_t*;
    auto write32(int offset, uint32_t value) -> void;
    auto read32(int offset) -> unsigned;
    auto writeName(int offset, std::string name, uint8_t allocatedSize) -> void;
    auto readName(int offset, uint8_t allocatedSize) -> std::string;
    auto writeDate(time_t unixTS, int offset) -> void;
    auto getChecksumOffset() -> int;
    auto hashTableEntries() -> unsigned;

};

}
