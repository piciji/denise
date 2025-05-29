
#include "filesystemExt.h"
#include "../../interface.h"

namespace LIBAMI {

FilesystemExt::FilesystemExt(uint64_t size, Structure structure, unsigned bSize)
: Filesystem(size, structure, bSize) {
    buffer = new uint8_t[bSize];
    offset = 0;
}

FilesystemExt::~FilesystemExt() {
    delete[] buffer;
}

auto FilesystemExt::setDataSource(HardDiskStructure* hdStructure, uint64_t offset) -> void {
    this->structure = hdStructure;
    this->offset = offset;
}

auto FilesystemExt::getBlock(unsigned ref) -> SectorBlock* {
    if (ref >= blockCount)
        return nullptr;

    for(auto block : blocks) {
        if (block->nr == ref)
            return block;
    }

    return getBlock(ref, SectorBlock::Type::EMPTY_BLOCK);
}

auto FilesystemExt::getBlock(unsigned ref, SectorBlock::Type type) -> SectorBlock* {
    if (structure->read(buffer, offset + (uint64_t)bSize * (uint64_t)ref, bSize)) {
        if (type == SectorBlock::Type::EMPTY_BLOCK)
            type = predictType(ref, buffer);

        if (type != SectorBlock::Type::EMPTY_BLOCK) {
            SectorBlock* block = new SectorBlock(*this, type, ref);
            block->importBlock(buffer);
            blocks.push_back(block);
            return block;
        }
    }

    return nullptr;
}

}
