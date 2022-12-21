
#pragma once

#include <cstdint>
#include <ctime>
#include <string>
#include <cstring>
#include <vector>

namespace LIBAMI {

struct Filesystem;

struct SectorBlock {
    enum Type { ROOT_BLOCK, BOOT_BLOCK, BITMAP_BLOCK, BITMAP_EXT_BLOCK, DIR_BLOCK, FILE_HEADER_BLOCK, EMPTY_BLOCK } type;

    SectorBlock( Filesystem& filesystem, Type type, unsigned nr);
    ~SectorBlock();

    unsigned nr;
    int depth;
    Filesystem& filesystem;

    uint8_t* data = nullptr;

    auto init() -> void;
    auto setName(std::string name) -> void;
    auto getName() -> std::string;
    auto getNameRaw(bool indentByDepth = false) -> std::vector<uint16_t>;
    auto setBitmapBlock(unsigned pos, unsigned value) -> void;
    auto getBitmapBlock(unsigned pos) -> unsigned;
    auto setBitmapExtBlock(unsigned value) -> void;
    auto getBitmapExtBlock() -> unsigned;
    auto getParentDir() -> unsigned;
    auto getHash(unsigned pos) -> unsigned;
    auto getHashChain() -> unsigned;

    auto calcChecksum() -> unsigned;
    auto exportBlock(uint8_t* data) -> void;
    auto importBlock(uint8_t* data) -> void;

    auto bSize() -> unsigned;
    auto getAdrPtr(int offset) -> uint8_t*;
    auto write(int offset, uint32_t value) -> void;
    auto read(int offset) -> unsigned;
    auto getChecksumOffset() -> int;
    auto hashTableEntries() -> unsigned;

protected:
    auto readName(int offset, uint8_t allocatedSize) -> std::string;
    auto readNameRaw(int offset, uint8_t allocatedSize, bool indentByDepth = false) -> std::vector<uint16_t>;
    auto writeName(int offset, std::string name, uint8_t allocatedSize) -> void;
    auto writeDate(time_t unixTS, int offset) -> void;

};

}
