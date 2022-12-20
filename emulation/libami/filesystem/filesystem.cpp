
#include "filesystem.h"
#include "sectorBlock.h"

namespace LIBAMI {

Filesystem::Filesystem(Structure structure, unsigned size, unsigned bSize) {
    this->structure = structure;
    this->bSize = bSize;
    this->blockCount = (size + (bSize - 1)) / bSize;
}

Filesystem::~Filesystem() {
    clear();
}

auto Filesystem::exportMedia(uint8_t* data, unsigned size) -> bool {
    if ((size % bSize) != 0)
        return false;

    if ((blockCount * bSize) != size)
        return false;

    for(auto block : blocks)
        block->exportBlock( data + block->nr * bSize );

    return true;
}

auto Filesystem::importMedia(uint8_t* data, unsigned size) -> bool {
    if ((size % bSize) != 0)
        return false;

    if ((blockCount * bSize) != size)
        return false;

    clear();
    blocks.assign( blockCount, nullptr );

    SectorBlock* rootBlock = nullptr;
    for (unsigned i = 0; i < blockCount; i++) {
        uint8_t* ptr = data + i * bSize;
        auto type = predictType(i, ptr);
        auto block = new SectorBlock(*this, type, i);
        block->importBlock( ptr );
        if (block->type == SectorBlock::Type::ROOT_BLOCK)
            rootBlock = block;
        blocks[i] = block;
    }

    if (!rootBlock)
        rootBlock = blocks[getRootBlockRef()];

    for(unsigned i = 0; i < 25; i++) {
        auto bmRef = rootBlock->getBitmapBlockPtr(i);

        if (bmRef && bmRef < blockCount) {
            if (blocks[bmRef]) delete[] blocks[bmRef];
            blocks[bmRef] = new SectorBlock(*this, SectorBlock::Type::BITMAP_BLOCK, bmRef);
            blocks[bmRef]->importBlock( data + bmRef * bSize );
            bmBlockRefs.push_back(bmRef);
        }
    }

    auto extRef = rootBlock->getBitmapExtBlock();
    while(extRef && extRef < blockCount) {
        if (blocks[extRef]) delete[] blocks[extRef];
        blocks[extRef] = new SectorBlock(*this, SectorBlock::Type::BITMAP_EXT_BLOCK, extRef);
        blocks[extRef]->importBlock( data + extRef * bSize );
        for(unsigned i = 0; i < countBitmapPointersEachExtBlock(); i++) {
            auto bmRef = blocks[extRef]->getBitmapBlockPtr(i);
            if (bmRef && bmRef < blockCount) {
                if (blocks[bmRef]) delete[] blocks[bmRef];
                blocks[bmRef] = new SectorBlock(*this, SectorBlock::Type::BITMAP_BLOCK, bmRef);
                blocks[bmRef]->importBlock(data + bmRef * bSize);
                bmBlockRefs.push_back(bmRef);
            }
        }
        extRef = blocks[extRef]->getBitmapExtBlock();
    }
    return true;
}

auto Filesystem::predictType(unsigned ref, uint8_t* buffer) -> SectorBlock::Type {
    if (ref == 0 || ref == 1) return SectorBlock::Type::BOOT_BLOCK;

    SectorBlock testBlock(*this, SectorBlock::Type::EMPTY_BLOCK, ~0);
    testBlock.data = buffer;
    unsigned type = testBlock.read32( 0 );
    unsigned subType = testBlock.read32( -4 );

    if (type == 2 && subType == 1) return SectorBlock::Type::ROOT_BLOCK;
    if (type == 2 && subType == 2) return SectorBlock::Type::DIR_BLOCK;
    if (type == 2 && subType == (unsigned)-3) return SectorBlock::Type::FILE_HEADER_BLOCK;

    // todo data blocks
    return SectorBlock::Type::EMPTY_BLOCK;
}

auto Filesystem::getDirectory() -> Emulator::Interface::Listing& {
    std::vector<SectorBlock*> storage;
    Emulator::Interface::Listing listing;
    traverseDir( getBlock( getRootBlockRef() ), storage, &listing);
    return listing;
}

auto Filesystem::traverseDir( SectorBlock* from, std::vector<SectorBlock*>& storage, Emulator::Interface::Listing* listing ) -> void {
    if (!from)
        return;

    for (int i = from->hashTableEntries(); i >= 0; i--) {
        for(SectorBlock* block = getHashChainBlock( from->getHashRef(i) ); block; block = getBlock(block->getHashChainRef())) {
            if (find(storage, block))
                break;
            storage.push_back( block );

            listing->childs.push_back({block->nr});

            if (block->type == SectorBlock::Type::DIR_BLOCK)
                traverseDir(block, storage, &listing->childs.back());
        }
    }
}

auto Filesystem::format(std::string name, bool bootblock) -> void {
    clear();
    blocks.assign( blockCount, nullptr );
    unsigned rootBlockRef = getRootBlockRef();

    blocks[0] = new SectorBlock(*this, SectorBlock::Type::BOOT_BLOCK, 0);
    blocks[1] = new SectorBlock(*this, SectorBlock::Type::BOOT_BLOCK, 1);
    blocks[rootBlockRef] = new SectorBlock(*this, SectorBlock::Type::BOOT_BLOCK, rootBlockRef);

    unsigned countBitmapBlocks = (blockCount + countBitsEachBitmapBlock() - 1) / countBitsEachBitmapBlock();
    for (unsigned i = 0; i < countBitmapBlocks; i++) {
        unsigned bmBlockRef = rootBlockRef + 1 + i;
        bmBlockRefs.push_back(bmBlockRef);
        blocks[bmBlockRef] = new SectorBlock(*this, SectorBlock::Type::BITMAP_BLOCK, bmBlockRef);
    }

    SectorBlock* block = blocks[rootBlockRef];
    block->setName( name );

    if (countBitmapBlocks > 25) {
        countBitmapBlocks -= 25;
        unsigned countExtBlocks = (countBitmapBlocks + countBitmapPointersEachExtBlock() - 1) / countBitmapPointersEachExtBlock();
        for (unsigned i = 0; i < countExtBlocks; i++) {
            unsigned bmExtBlockRef = rootBlockRef + 1 + 25 + i;
            blocks[bmExtBlockRef] = new SectorBlock(*this, SectorBlock::Type::BITMAP_EXT_BLOCK, bmExtBlockRef);
            block->setBitmapExtBlock(bmExtBlockRef);
            block = blocks[bmExtBlockRef];
        }
    }

    referenceBitmaps();

    for(unsigned i = 0; i < blockCount; i++) {
        block = blocks[i];
        if (!block) {
            blocks[i] = new SectorBlock(*this, SectorBlock::Type::EMPTY_BLOCK, i);
            markBlockAsFree(i);
        }
    }

    if (bootblock)
        addBootblock();

    calculateChecksums();
}

auto Filesystem::addBootblock() -> void {
    static uint8_t os13[] = {
        0x43, 0xfa, 0x00, 0x18, 0x4e, 0xae, 0xff, 0xa0, 0x4a, 0x80, 0x67, 0x0a,
        0x20, 0x40, 0x20, 0x68, 0x00, 0x16, 0x70, 0x00, 0x4e, 0x75, 0x70, 0xff,
        0x60, 0xfa, 0x64, 0x6f, 0x73, 0x2e, 0x6c, 0x69, 0x62, 0x72, 0x61, 0x72,
        0x79 };

    static uint8_t os20[] = {
        0x43, 0xfa, 0x00, 0x3e, 0x70, 0x25, 0x4e, 0xae, 0xfd, 0xd8, 0x4a, 0x80,
        0x67, 0x0c, 0x22, 0x40, 0x08, 0xe9, 0x00, 0x06, 0x00, 0x22, 0x4e, 0xae,
        0xfe, 0x62, 0x43, 0xfa, 0x00, 0x18, 0x4e, 0xae, 0xff, 0xa0, 0x4a, 0x80,
        0x67, 0x0a, 0x20, 0x40, 0x20, 0x68, 0x00, 0x16, 0x70, 0x00, 0x4e, 0x75,
        0x70, 0xff, 0x4e, 0x75, 0x64, 0x6f, 0x73, 0x2e, 0x6c, 0x69, 0x62, 0x72,
        0x61, 0x72, 0x79, 0x00, 0x65, 0x78, 0x70, 0x61, 0x6e, 0x73, 0x69, 0x6f,
        0x6e, 0x2e, 0x6c, 0x69, 0x62, 0x72, 0x61, 0x72, 0x79 };

    if (blocks[0])
        std::memcpy( blocks[0]->data + 12, structure == Structure::OFS ? os13 : os20, structure == Structure::OFS ? sizeof(os13) : sizeof(os20) );
}

auto Filesystem::calculateChecksums() -> void {
    for(auto block : blocks)
        block->calcChecksum();
}

auto Filesystem::markBlockAsFree(unsigned ref) -> void {
    accessBitmapAllocation(ref, 1);
}

auto Filesystem::markBlockAsAllocated(unsigned ref) -> void {
    accessBitmapAllocation(ref, 0);
}

auto Filesystem::accessBitmapAllocation(unsigned ref, int update) -> bool {
    if (ref < 2)
        return false; // allocated

    ref -= 2;

    unsigned bitmapPos = ref / countBitsEachBitmapBlock();

    SectorBlock* block = nullptr;

    if (bitmapPos < bmBlockRefs.size())
        block = getBitmapBlock( bmBlockRefs[bitmapPos] );

    if (!block)
        return false;

    ref %= countBitsEachBitmapBlock();
    unsigned byte = ref / 8;
    unsigned bit = ref % 8;

    // reverse byte order in long word chunks
    switch (byte % 4) {
        case 0: byte += 3; break;
        case 1: byte += 1; break;
        case 2: byte -= 1; break;
        case 3: byte -= 3; break;
    }

    byte += 4; // skip checksum
    if (byte >= bSize)
        return false;

    if (update == 1)
        block->data[byte] |= (1 << bit);
    else if (update == 0)
        block->data[byte] &= ~(1 << bit);

    return block->data[byte] & (1 << bit);
}

auto Filesystem::referenceBitmaps() -> void {
    unsigned i = 0;
    unsigned j = countBitmapPointersEachExtBlock();
    auto block = blocks[getRootBlockRef()]; // no sanity checking needed

    for(auto& blockRef : bmBlockRefs) {
        if (i < 25) {
            block->setBitmapBlockPtr(i, blockRef);
            i++;
        } else {
            if (j == countBitmapPointersEachExtBlock()) {
                j = 0;
                block = getBmExtBlock( block->getBitmapExtBlock() ); // sanity checking is usefull here
                if (!block)
                    return;
            }

            block->setBitmapBlockPtr(j, blockRef);
            j++;
        }
    }
}

auto Filesystem::getBmExtBlock(unsigned ref) -> SectorBlock* {
    if ((ref >= blockCount) || (blocks[ref]->type != SectorBlock::Type::BITMAP_EXT_BLOCK))
        return nullptr;

    return blocks[ref];
}

auto Filesystem::getBitmapBlock(unsigned ref) -> SectorBlock* {
    if ((ref >= blockCount) || (blocks[ref]->type != SectorBlock::Type::BITMAP_BLOCK))
        return nullptr;

    return blocks[ref];
}

auto Filesystem::getHashChainBlock(unsigned ref) -> SectorBlock* {
    if ((ref >= blockCount) || (blocks[ref]->type != SectorBlock::Type::FILE_HEADER_BLOCK && blocks[ref]->type != SectorBlock::Type::DIR_BLOCK))
        return nullptr;

    return blocks[ref];
}

auto Filesystem::getBlock(unsigned ref) -> SectorBlock* {
    if (ref >= blockCount)
        return nullptr;

    return blocks[ref];
}

auto Filesystem::clear() -> void {
    for(auto& block : blocks) {
        if (block)
            delete block;
    }
    blocks.clear();
    bmBlockRefs.clear();
}

}