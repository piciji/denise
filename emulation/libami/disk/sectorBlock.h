
#pragma once

#include <cstdint>
#include <ctime>
#include <string>

#define MAX_AMI_BLOCK_SIZE (4 * 1024)

namespace LIBAMI {

struct SectorBlock {
    enum Type { ROOT_BLOCK, BOOT_BLOCK } type;

    SectorBlock( Type type, unsigned nr, bool ffs, unsigned size = 512);

    unsigned nr;
    unsigned bSize;
    bool ffs;

    uint8_t data[MAX_AMI_BLOCK_SIZE] = {0};

    auto init() -> void;

    auto write32(int offset, uint32_t value) -> void;
    auto writeDate(time_t unixTS, int offset) -> void;

    auto setName(std::string name) -> void;
};

}
