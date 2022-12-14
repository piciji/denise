
#pragma once

#include <cstdint>
#include <ctime>
#include <string>
#include <cstring>

#define MAX_AMI_BLOCK_SIZE (4 * 1024)

namespace LIBAMI {

struct SectorBlock {
    enum Type { ROOT_BLOCK, BOOT_BLOCK } type;

    SectorBlock( Type type, unsigned nr, bool ffs = true, unsigned size = 512);

    unsigned nr;
    unsigned bSize;
    bool ffs;

    uint8_t data[MAX_AMI_BLOCK_SIZE] = {0};

    auto init() -> void;
    auto setName(std::string name) -> void;
    auto setBitmapExtBlock(unsigned value) -> void;
    auto setBitmapBlockPtr(unsigned pos, unsigned value) -> void;

    auto getAdrPtr(int offset) -> uint8_t*;
    auto write32(int offset, uint32_t value) -> void;
    auto writeName(int offset, std::string name, uint8_t allocatedSize) -> void;
    auto writeDate(time_t unixTS, int offset) -> void;

};

}
