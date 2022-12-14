
#pragma once

#include <vector>
#include "sectorBlock.h"

namespace LIBAMI {

struct Filesystem {
    Filesystem(uint8_t* data, unsigned size, unsigned blockSize = 512);

    std::vector<SectorBlock*> sectorBlocks;
};

}
