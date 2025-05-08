
#include "filesystemExt.h"
#include "../../interface.h"

namespace LIBAMI {

FilesystemExt::FilesystemExt(uint64_t size, Structure structure, unsigned bSize)
: Filesystem(size, structure, bSize) {
    buffer = new uint8_t[bSize];
    media = nullptr;
    interface = nullptr;
    offset = 0;
}

FilesystemExt::~FilesystemExt() {
    delete[] buffer;
}

auto FilesystemExt::setDataSource(Emulator::Interface* interface, Emulator::Interface::Media* media, unsigned offset) -> void {
    this->media = media;
    this->interface = interface;
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
    if (interface && interface->readMedia(media, buffer, bSize, offset + bSize * ref) == bSize) {
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
