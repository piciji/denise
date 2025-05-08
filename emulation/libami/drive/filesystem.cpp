
#include "filesystem.h"
#include "sectorBlock.h"

namespace LIBAMI {

Filesystem::Filesystem(uint64_t size, Structure structure, unsigned bSize) {
    this->structure = structure;
    this->bSize = bSize;
    this->blockCount = (size + (uint64_t)(bSize - 1)) / (uint64_t)bSize;
    this->rootBlock = nullptr;
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
// "importMedia", "predictType" can only be used reliably if the file system is used for
// read only purposes like "directory listing" or is filled in one go directly after "formatting".

// TODO when needed
// When writing to already filled file systems, we need to process the header blocks for two reasons.
// 1. "predictType" cannot reliably distinguish FFS from the rest.
// 2. For very large harddisk images, it would simply take too long to search for a free block using "allocateEmptyBlock."

auto Filesystem::importMedia(uint8_t* data, unsigned size) -> bool {
    if ((size % bSize) != 0)
        return false;

    if ((blockCount * bSize) != size)
        return false;

    clear();
    blocks.assign( blockCount, nullptr );

    rootBlock = nullptr;
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
        auto bmRef = rootBlock->getBitmapBlock(i);

        if (bmRef && bmRef < blockCount) {
            if (blocks[bmRef]) delete blocks[bmRef];
            blocks[bmRef] = new SectorBlock(*this, SectorBlock::Type::BITMAP_BLOCK, bmRef);
            blocks[bmRef]->importBlock( data + bmRef * bSize );
            bmBlockRefs.push_back(bmRef);
        }
    }

    auto extRef = rootBlock->getBitmapExtBlock();
    while(extRef && extRef < blockCount) {
        if (blocks[extRef]) delete blocks[extRef];
        blocks[extRef] = new SectorBlock(*this, SectorBlock::Type::BITMAP_EXT_BLOCK, extRef);
        blocks[extRef]->importBlock( data + extRef * bSize );
        for(unsigned i = 0; i < countBitmapPointersEachExtBlock(); i++) {
            auto bmRef = blocks[extRef]->getBitmapBlock(i);
            if (bmRef && bmRef < blockCount) {
                if (blocks[bmRef]) delete blocks[bmRef];
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
    unsigned type = testBlock.read( 0 );
    unsigned subType = testBlock.read( -4 );
    testBlock.data = nullptr;

    if (type == 2 && subType == 1) return SectorBlock::Type::ROOT_BLOCK;
    if (type == 2 && subType == 2) return SectorBlock::Type::DIR_BLOCK;
    if (type == 2 && subType == (unsigned)-3) return SectorBlock::Type::FILE_HEADER_BLOCK;

    // no data block detection here (wouldnt work reliable)
    // don't use this to append files to already filled file systems
    return SectorBlock::Type::EMPTY_BLOCK;
}

auto Filesystem::currentDir() -> SectorBlock* {
    if (cd) {
        if (cd->type == SectorBlock::Type::ROOT_BLOCK || cd->type == SectorBlock::Type::DIR_BLOCK)
            return cd;
    }

    if (!rootBlock)
        rootBlock = blocks[getRootBlockRef()];
    cd = rootBlock;

    return cd;
}

auto Filesystem::placeNewBlock(SectorBlock::Type type) -> SectorBlock* {
    auto block = allocateEmptyBlock();
    if (block) {
        unsigned nr = block->nr;
        delete block;
        block = new SectorBlock(*this, type, nr);
        blocks[nr] = block;
    }
    return block;
}

auto Filesystem::allocateEmptyBlock() -> SectorBlock* {
    for (int i = getRootBlockRef() + 1; i < blockCount; i++) {
        if (blocks[i]->type == SectorBlock::Type::EMPTY_BLOCK) {
            markBlockAsAllocated(i);
            return blocks[i];
        }
    }
    for (int i = getRootBlockRef() - 1; i >= 0; i--) {
        if (blocks[i]->type == SectorBlock::Type::EMPTY_BLOCK) {
            markBlockAsAllocated(i);
            return blocks[i];
        }
    }
    return nullptr;
}

auto Filesystem::createBase(const std::string& name, bool isDir, uint8_t* data, unsigned size) -> SectorBlock* {
    SectorBlock* block = placeNewBlock(isDir ? SectorBlock::Type::DIR_BLOCK : SectorBlock::Type::FILE_HEADER_BLOCK);
    if (!block)
        return nullptr;

    block->setName(name);
    SectorBlock* cd = currentDir();
    block->setParentDir( cd->nr );

    unsigned hash = block->getHash() % cd->tableEntries();
    unsigned hashRef = cd->getHashTable(hash);
    if (!hashRef) {
        cd->setHashTable(hash, block->nr);
    } else {
        auto last = getLastChainBlock( getHashTableBlock(hashRef) );
        if (last)
            last->setHashChain( block->nr );
    }

    if (data && !isDir) {
        if (!writeData(block, data, size))
            return nullptr;
    }

    return block;
}

auto Filesystem::getLastChainBlock(SectorBlock* block) -> SectorBlock* {
    std::set<unsigned> sanityCheck;

    while (block && sanityCheck.find(block->nr) == sanityCheck.end()) {
        SectorBlock* next = getHashTableBlock(block->getHashChain());
        if (!next)
            return block;

        sanityCheck.insert(block->nr);
        block = next;
    }
    return nullptr;
}


auto Filesystem::seek(std::string name) -> SectorBlock* {
    std::set<unsigned> sanityCheck;

    SectorBlock* cd = currentDir();

    unsigned hash = SectorBlock::calcHash(name) % cd->tableEntries();
    unsigned hashRef = cd->getHashTable(hash);

    while (hashRef && sanityCheck.find(hashRef) == sanityCheck.end())  {
        SectorBlock* block = getHashTableBlock(hashRef);
        if (!block)
            break;

        if (block->getName() == name)
            return block;

        sanityCheck.insert(hashRef);
        hashRef = block->getHashChain();
    }

    return nullptr;
}

auto Filesystem::changeDir(const std::string& name) -> SectorBlock* {
    currentDir();

    if (name == "/") {
        cd = rootBlock;
        return cd;
    }

    if (name == "..") {
        cd = getBlock(cd->getParentDir());
        return currentDir();
    }

    auto subdir = seek(name);
    if (!subdir)
        return cd;

    cd = getBlock(subdir->nr);
    return currentDir();
}

auto Filesystem::writeData(SectorBlock* fhBlock, uint8_t* data, unsigned size) -> bool {
    if(fhBlock->type != SectorBlock::Type::FILE_HEADER_BLOCK)
        return false;

    unsigned dataBlocks = requiredDataBlocks(size);
    unsigned extBlocks = requiredExtensionBlocks(size);
    unsigned freeBlocks = countFreeBlocks();

    if (freeBlocks < (dataBlocks + extBlocks))
        return false;

    SectorBlock* parent = fhBlock;
    for (int i = 0; i < extBlocks; i++) {
        SectorBlock* block = placeNewBlock(SectorBlock::Type::EXTENSION_BLOCK);
        if (!block)
            return false;
        block->setFileHeader(fhBlock->nr);
        parent->setNextExtension(block->nr);
        parent = block;
    }

    parent = nullptr;
    for (int i = 1; i <= dataBlocks; i++) {
        SectorBlock* block = placeNewBlock(isOFS() ? SectorBlock::Type::DATA_BLOCK_OFS : SectorBlock::Type::DATA_BLOCK_FFS);
        if (!block)
            return false;

        block->setFileHeader(fhBlock->nr);
        block->setDataSeqNum(i);
        if (parent)
            parent->setNextData(block->nr);

        parent = block;

        if (!referenceDataBlock(fhBlock, block))
            return false;

        unsigned todo;
        if (isOFS()) {
            todo = std::min(bSize - 24, size);
            std::memcpy(block->data + 24, data, todo);
            block->setSize(todo);
        } else {
            todo = std::min(bSize, size);
            std::memcpy(block->data, data, todo);
        }

        fhBlock->setSize(fhBlock->getSize() + todo);

        data += todo;
        size -= todo;

        if (!size)
            break;
    }

    return true;
}

auto Filesystem::referenceDataBlock(SectorBlock* fhBlock, SectorBlock* dataBlock) -> bool {
    unsigned tablePos = fhBlock->getHighSeq();

    if (tablePos < fhBlock->tableEntries()) {
        if (!tablePos)
            fhBlock->setFirstData(dataBlock->nr);

        fhBlock->setDataTable(tablePos, dataBlock->nr);
        fhBlock->setHighSeq(tablePos + 1);
        return true;
    }

    std::set<SectorBlock*> sanityCheck;
    SectorBlock* extBlock = fhBlock;

    while(nullptr != (extBlock = getExtensionBlock(extBlock->getNextExtension()))) {
        if (sanityCheck.find(extBlock) != sanityCheck.end())
            return false;

        tablePos = extBlock->getHighSeq();

        if (tablePos < extBlock->tableEntries()) {
            extBlock->setDataTable(tablePos, dataBlock->nr);
            extBlock->setHighSeq(tablePos + 1);
            return true;
        }
        sanityCheck.insert( extBlock );
    }
    return false;
}

auto Filesystem::getDirectory(bool noFilesInSubdirs) -> std::vector<Emulator::Interface::Listing> {
    std::stack<SectorBlock*> dir;
    std::vector<SectorBlock*> sanityCheck; // prevent endless iterations
    std::vector<Emulator::Interface::Listing> listing;
    if (!rootBlock) {
        rootBlock = getBlock( getRootBlockRef() );
        if (!rootBlock)
            return listing;
    }

    static std::vector<uint16_t> _preLabel = {'L','a','b','e','l',':',' '};
    auto label = rootBlock->getNameRaw();
    combine(label, _preLabel);
    listing.push_back({rootBlock->nr, label, {}});

    rootBlock->depth = -1;
    traverse( rootBlock, dir );

    while(dir.size()) {
        auto block = dir.top();
        dir.pop();

        if (!find(sanityCheck, block)) {
            sanityCheck.push_back( block );

            if (block->type == SectorBlock::Type::DIR_BLOCK) {
                listing.push_back({ block->nr, block->getNameRaw(true), getPathRaw(block) });
                traverse( block, dir );
            } else if (!noFilesInSubdirs) {
                listing.push_back({ block->nr, block->getNameRaw(true), getPathRaw(block) });
            }
        }
    }
    return listing;
}

auto Filesystem::getPathRaw(SectorBlock* block) -> std::vector<uint16_t> {
    std::vector<uint16_t> out;
    std::vector<SectorBlock*> sanityCheck; // prevent endless iterations

    while(block) {
        if (!getHashTableBlock(block->nr))
            break;

        if (find(sanityCheck, block))
            break;
        sanityCheck.push_back( block );

        auto name = block->getNameRaw();

        if (out.size()) {
            name.push_back('/');
            combine(out, name);
        } else
            out = name;

        block = getBlock( block->getParentDir() );
    }

    return out;
}

auto Filesystem::traverse( SectorBlock* from, std::stack<SectorBlock*>& result ) -> void {
    std::vector<SectorBlock*> sanityCheck; // prevent endless iterations
    if (!from)
        return;

    for (int i = from->tableEntries(); i >= 0; i--) {
        for(SectorBlock* block = getHashTableBlock( from->getHashTable(i) ); block; block = getHashTableBlock(block->getHashChain())) {
            if (find(sanityCheck, block))
                break;

            block->depth = from->depth + 1;
            result.push( block );
            sanityCheck.push_back( block );
        }
    }
}

auto Filesystem::format(std::string name, bool bootblock) -> void {
    clear();
    blocks.assign( blockCount, nullptr );
    unsigned rootBlockRef = getRootBlockRef();

    blocks[0] = new SectorBlock(*this, SectorBlock::Type::BOOT_BLOCK, 0);
    blocks[1] = new SectorBlock(*this, SectorBlock::Type::BOOT_BLOCK, 1);
    blocks[rootBlockRef] = new SectorBlock(*this, SectorBlock::Type::ROOT_BLOCK, rootBlockRef);
    rootBlock = blocks[rootBlockRef];

    unsigned countBitmapBlocks = (blockCount + countBitsEachBitmapBlock() - 1) / countBitsEachBitmapBlock();
    for (unsigned i = 0; i < countBitmapBlocks; i++) {
        unsigned bmBlockRef = rootBlockRef + 1 + i;
        bmBlockRefs.push_back(bmBlockRef);
        blocks[bmBlockRef] = new SectorBlock(*this, SectorBlock::Type::BITMAP_BLOCK, bmBlockRef);
    }

    SectorBlock* block = rootBlock;
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

    auto block = blocks[0];
    if (block) {
        block->data[10] = 0x3;  // root block: 880, for HD block sizes too ???
        block->data[11] = 0x70;
        std::memcpy( block->data + 12, structure == Structure::OFS ? os13 : os20, structure == Structure::OFS ? sizeof(os13) : sizeof(os20) );
    }
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

auto Filesystem::countFreeBlocks() -> unsigned {
    unsigned count = 0;

    for (int i = 0; i < blockCount; i++) {
        if (isFree(i))
            count++;
    }
    return count;
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
    unsigned byte = ref >> 3;
    unsigned bit = ref & 7;

    // reverse byte order in long word chunks
    switch (byte & 3) {
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
    if (!rootBlock)
        rootBlock = blocks[getRootBlockRef()];

    auto block = rootBlock;

    for(auto& blockRef : bmBlockRefs) {
        if (i < 25) {
            block->setBitmapBlock(i, blockRef);
            i++;
        } else {
            if (j == countBitmapPointersEachExtBlock()) {
                j = 0;
                block = getBitmapExtBlock( block->getBitmapExtBlock() );
                if (!block)
                    return;
            }

            block->setBitmapBlock(j, blockRef);
            j++;
        }
    }
}

auto Filesystem::requiredDataBlocks(unsigned fileSize) -> unsigned {
    unsigned bytesPerBlock = bSize - (isOFS() ? 24 : 0);
    return (fileSize + bytesPerBlock - 1) / bytesPerBlock;
}

auto Filesystem::requiredExtensionBlocks(unsigned fileSize) -> unsigned {
    unsigned blocks = requiredDataBlocks(fileSize);
    unsigned refs = (bSize / 4) - 56;

    if (blocks <= refs)
        return 0;

    return (blocks - 1) / refs;
}

auto Filesystem::getBitmapExtBlock(unsigned ref) -> SectorBlock* {
    if ((ref >= blockCount) || (blocks[ref]->type != SectorBlock::Type::BITMAP_EXT_BLOCK))
        return nullptr;

    return blocks[ref];
}

auto Filesystem::getBitmapBlock(unsigned ref) -> SectorBlock* {
    if ((ref >= blockCount) || (blocks[ref]->type != SectorBlock::Type::BITMAP_BLOCK))
        return nullptr;

    return blocks[ref];
}

auto Filesystem::getExtensionBlock(unsigned ref) -> SectorBlock* {
    if ((ref >= blockCount) || (blocks[ref]->type != SectorBlock::Type::EXTENSION_BLOCK))
        return nullptr;

    return blocks[ref];
}

auto Filesystem::getHashTableBlock(unsigned ref) -> SectorBlock* {
    auto block = getBlock(ref);

    if (!block || (block->type != SectorBlock::Type::FILE_HEADER_BLOCK && block->type != SectorBlock::Type::DIR_BLOCK))
        return nullptr;

    return block;
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
