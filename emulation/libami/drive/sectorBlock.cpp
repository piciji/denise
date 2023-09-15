
#include "sectorBlock.h"
#include "filesystem.h"
#include "../../tools/buffer.h"

namespace LIBAMI {

SectorBlock::~SectorBlock() {
    if (data)
        delete[] data;
}

SectorBlock::SectorBlock(Filesystem& filesystem, Type type, unsigned nr) : filesystem(filesystem) {
    this->type = type;
    this->nr = nr;
    this->depth = 0;

    if (type != Type::EMPTY_BLOCK) {
        data = new uint8_t[filesystem.bSize];
        std::memset(data, 0, bSize());
        init();
    }
}

auto SectorBlock::init() -> void {
    switch(type) {
        case ROOT_BLOCK: {
            write(0, 2);
            write(12, (bSize() >> 2) - 56);
            write(-200, (uint32_t) -1);
            time_t unixTS = std::time(nullptr);
            writeDate(unixTS, -28); // creation date
            writeDate(unixTS, -92); // modification date
            write(-4, 1); // ST_ROOT

        } break;
        case BOOT_BLOCK:
            if (nr == 0) {
                data[0] = 'D';
                data[1] = 'O';
                data[2] = 'S';
                data[3] = filesystem.structure == Filesystem::Structure::FFS ? 1 : 0;
            }
            break;
        case DIR_BLOCK:
            write(0, 2);
            write(4, nr);
            write(-4, 2);
            writeDate(std::time(nullptr), -92); // access date
            break;
        case FILE_HEADER_BLOCK:
            write(0, 2);
            write(4, nr);
            write(-4, (uint32_t)-3);
            writeDate(std::time(nullptr), -92); // access date
            break;
        case EXTENSION_BLOCK:
            write(0, 16);
            write(4, nr);
            write(-4, (uint32_t)-3);
            break;
        case DATA_BLOCK_OFS:
            write(0, 8);
            break;
    }
}

auto SectorBlock::setName(std::string name) -> void {
    switch(type) {
        case ROOT_BLOCK:
        case DIR_BLOCK:
        case FILE_HEADER_BLOCK:
            if (name == "")
                name = "empty";
            writeName(-80, name, 30);
            break;
    }
}

auto SectorBlock::getName() -> std::string {
    switch(type) {
        case ROOT_BLOCK:
        case DIR_BLOCK:
        case FILE_HEADER_BLOCK:
            return readName(-80, 30);
        default:
            return "";
    }
}

auto SectorBlock::getNameRaw(bool indentByDepth) -> std::vector<uint16_t> {
    switch(type) {
        case ROOT_BLOCK:
        case DIR_BLOCK:
        case FILE_HEADER_BLOCK:
            return readNameRaw(-80, 30, indentByDepth);
        default:
            return {};
    }
}

auto SectorBlock::getHash() -> unsigned {
    switch(type) {
        case DIR_BLOCK:
        case FILE_HEADER_BLOCK:
            return calcHash( readName(-80, 30) );

        default:
            return 0;
    }
}

auto SectorBlock::setBitmapBlock(unsigned pos, unsigned value) -> void {
    switch(type) {
        case ROOT_BLOCK:
            if (pos < 25)
                write((pos - 49) << 2, value);
            break;
        case BITMAP_EXT_BLOCK:
            write(pos << 2, value);
            break;
    }
}

auto SectorBlock::getBitmapBlock(unsigned pos) -> unsigned {
    switch(type) {
        case ROOT_BLOCK:
            if (pos < 25)
                return read((pos - 49) << 2);
        default:
            return 0;
        case BITMAP_EXT_BLOCK:
            return read(pos << 2);
    }
}

auto SectorBlock::setBitmapExtBlock(unsigned value) -> void {
    switch(type) {
        case ROOT_BLOCK:
            write(-96, value);
            break;
        case BITMAP_EXT_BLOCK:
            write(-4, value);
            break;
    }
}

auto SectorBlock::getBitmapExtBlock() -> unsigned {
    switch(type) {
        case ROOT_BLOCK:
            return read(-96);
        case BITMAP_EXT_BLOCK:
            return read(-4);

        default:
            return 0;
    }
}

auto SectorBlock::setParentDir(unsigned pos) -> void {
    switch(type) {
        case FILE_HEADER_BLOCK:
        case DIR_BLOCK:
            write(-12, pos);
            break;
    }
}

auto SectorBlock::getParentDir() -> unsigned {
    switch(type) {
        case FILE_HEADER_BLOCK:
        case DIR_BLOCK:
            return read(-12);

        default:
            return 0;
    }
}

auto SectorBlock::getHashTable(unsigned pos) -> unsigned {
    switch (type) {
        case DIR_BLOCK:
        case ROOT_BLOCK:
            if (pos < tableEntries())
                return read( (6 + pos) << 2 );
        default:
            return 0;
    }
}

auto SectorBlock::setHashTable(unsigned pos, unsigned nr) -> void {
    switch (type) {
        case DIR_BLOCK:
        case ROOT_BLOCK:
            if (pos < tableEntries())
                write( (6 + pos) << 2, nr );
            break;
    }
}

auto SectorBlock::getHashChain() -> unsigned {
    switch (type) {
        case DIR_BLOCK:
        case FILE_HEADER_BLOCK:
            return read(-16);

        default:
            return 0;
    }
}

auto SectorBlock::setHashChain(unsigned nr) -> void {
    switch (type) {
        case DIR_BLOCK:
        case FILE_HEADER_BLOCK:
            write(-16, nr);
            break;
    }
}

auto SectorBlock::getFileHeader() -> unsigned {
    switch (type) {
        case EXTENSION_BLOCK:
            return read(-12);
        case DATA_BLOCK_OFS:
            return read(4);

        default:
            return 0;
    }
}

auto SectorBlock::setFileHeader(unsigned nr) -> void {
    switch (type) {
        case EXTENSION_BLOCK:
            write(-12, nr);
            break;
        case DATA_BLOCK_OFS:
            write(4, nr);
            break;
    }
}

auto SectorBlock::getNextExtension() -> unsigned {
    switch (type) {
        case FILE_HEADER_BLOCK:
        case EXTENSION_BLOCK:
            return read(-8);

        default:
            return 0;
    }
}

auto SectorBlock::setNextExtension(unsigned nr) -> void {
    switch (type) {
        case FILE_HEADER_BLOCK:
        case EXTENSION_BLOCK:
            write(-8, nr);
            break;

        default:
            break;
    }
}

auto SectorBlock::getSize() -> unsigned {
    switch (type) {
        case DATA_BLOCK_OFS:
            return read(12);
        case FILE_HEADER_BLOCK:
            return read(-188);
        default:
            return 0;
    }
}

auto SectorBlock::setSize(unsigned value) -> void {
    switch (type) {
        case DATA_BLOCK_OFS:
            write(12, value);
            break;
        case FILE_HEADER_BLOCK:
            write(-188, value);
            break;
        default:
            break;
    }
}

auto SectorBlock::getDataSeqNum() -> unsigned {
    switch (type) {
        case DATA_BLOCK_OFS:
            return read(8);

        default:
            return 0;
    }
}

auto SectorBlock::setDataSeqNum(unsigned value) -> void {
    switch (type) {
        case DATA_BLOCK_OFS:
            write(8, value);
            break;

        default:
            break;
    }
}

auto SectorBlock::getNextData() -> unsigned {
    switch (type) {
        case DATA_BLOCK_OFS:
            return read(16);

        default:
            return 0;
    }
}

auto SectorBlock::setNextData(unsigned value) -> void {
    switch (type) {
        case DATA_BLOCK_OFS:
            write(16, value);
            break;

        default:
            break;
    }
}

auto SectorBlock::getHighSeq() -> unsigned {
    switch (type) {
        case FILE_HEADER_BLOCK:
        case EXTENSION_BLOCK:
            return read(8);

        default:
            return 0;
    }
}

auto SectorBlock::setHighSeq(unsigned value) -> void {
    switch (type) {
        case FILE_HEADER_BLOCK:
        case EXTENSION_BLOCK:
            write(8, value);
            break;

        default:
            break;
    }
}

auto SectorBlock::getFirstData() -> unsigned {
    switch (type) {
        case FILE_HEADER_BLOCK:
            return read(16);

        default:
            return 0;
    }
}

auto SectorBlock::setFirstData(unsigned value) -> void {
    switch (type) {
        case FILE_HEADER_BLOCK:
            write(16, value);
            break;

        default:
            break;
    }
}

auto SectorBlock::getDataTable(int pos) -> unsigned {
    switch (type) {
        case FILE_HEADER_BLOCK:
        case EXTENSION_BLOCK:
            return read( -204 - (pos << 2) );

        default:
            return 0;
    }
}

auto SectorBlock::setDataTable(int pos, unsigned value) -> void {
    switch (type) {
        case FILE_HEADER_BLOCK:
        case EXTENSION_BLOCK:
            write( -204 - (pos << 2), value);
            break;
    }
}

auto SectorBlock::getChecksumOffset() -> int {
    switch(type) {
        case BOOT_BLOCK:
            if (nr == 0)
                return 4;
            break;

        case ROOT_BLOCK:
        case DIR_BLOCK:
        case FILE_HEADER_BLOCK:
        case EXTENSION_BLOCK:
        case DATA_BLOCK_OFS:
            return 20;

        case BITMAP_BLOCK:
            return 0;
    }
    return -1;
}

auto SectorBlock::calcChecksum() -> unsigned {
    uint32_t result = 0;
    uint32_t precsum;
    int offset = getChecksumOffset();
    if (offset < 0 || offset >= bSize())
        return 0;

    if (type == BOOT_BLOCK) {
        write(offset, 0);

        for (unsigned i = 0; i < bSize(); i += 4) {
            precsum = result;
            result += read(i);
            if (result < precsum) result++;
        }

        auto second = filesystem.blocks[1];
        if (second) {
            for (unsigned i = 0; i < bSize(); i += 4) {
                precsum = result;
                result += second->read(i);
                if (result < precsum) result++;
            }
        }
        if (result == read(0)) // no Bootblock
            return 0;

        result = ~result;
    } else {
        write(offset, 0);
        for (unsigned i = 0; i < bSize(); i += 4)
            result += read(i);

        result = ~result;
        result += 1;
    }

    write( offset, result );
    return result;
}

auto SectorBlock::write(int offset, uint32_t value) -> void {
    Emulator::copyIntToBufferBigEndian<uint32_t>( getAdrPtr(offset), value);
}

auto SectorBlock::read(int offset) -> unsigned {
    return Emulator::copyBufferToIntBigEndian<uint32_t>( getAdrPtr(offset) );
}

inline auto SectorBlock::getAdrPtr(int offset) -> uint8_t* {
    if (offset >= 0)
        return data + offset;
    return data + bSize() + offset;
}

auto SectorBlock::bSize() -> unsigned {
    return filesystem.bSize;
}

auto SectorBlock::exportBlock(uint8_t* data) -> void {
    if (type == Type::EMPTY_BLOCK)
        std::memset(data, 0, bSize());
    else
        std::memcpy(data, this->data, bSize());
}

auto SectorBlock::importBlock(uint8_t* data) -> void {
    if (this->data)
        std::memcpy(this->data, data, bSize());
}

auto SectorBlock::tableEntries() -> unsigned {
    switch (type) {
        case ROOT_BLOCK:
        case DIR_BLOCK:
        case EXTENSION_BLOCK:
        case FILE_HEADER_BLOCK:
            return (bSize() / 4) - 56;

        default:
            return 0;
    }
}

auto SectorBlock::readName(int offset, uint8_t allocatedSize) -> std::string {
    std::string out;
    uint8_t* ptr = getAdrPtr(offset);
    uint8_t size = *ptr++;
    uint8_t strSize = std::min(allocatedSize, size);
    out.assign(ptr, ptr + strSize);

    std::replace( out.begin(), out.end(), ':', '_');
    std::replace( out.begin(), out.end(), '/', '_');

    return out;
}

auto SectorBlock::readNameRaw(int offset, uint8_t allocatedSize, bool indentByDepth) -> std::vector<uint16_t> {
    uint8_t* ptr = getAdrPtr(offset);
    uint8_t size = *ptr++;
    uint8_t strSize = std::min(allocatedSize, size);
    std::vector<uint16_t> out;

    if (indentByDepth) {
        int _depth = depth;
        if (_depth < 0) _depth = 0;
        out.resize(strSize + _depth);
        for (unsigned i = 0; i < _depth; i++)
            out[i] = ' ';

        for(unsigned i = 0; i < strSize; i++)
            out[i + _depth] = *ptr++;

        if (type == Type::DIR_BLOCK)
            out.push_back(0x5c);

    } else {
        out.resize(strSize);
        for(unsigned i = 0; i < strSize; i++)
            out[i] = *ptr++;
    }

    return out;
}

auto SectorBlock::writeName(int offset, std::string name, uint8_t allocatedSize) -> void {
    auto str = name.c_str();
    uint8_t* ptr = getAdrPtr(offset);
    uint8_t strSize = std::min(allocatedSize, (uint8_t)std::strlen(str));
    *ptr++ = strSize;
    std::memset(ptr, 0, allocatedSize);

    for (unsigned i = 0; i < strSize; i++) {
        if (str[i] == ':' || str[i] == '/')
            *ptr++ = '_';
        else
            *ptr++ = str[i];
    }
}

auto SectorBlock::writeDate(time_t unixTS, int offset) -> void {
    static const unsigned secPerDay = 24 * 60 * 60;
    static const time_t unixToAmigaDiff = (8 * 365 + 2) * secPerDay;

    unixTS -= unixToAmigaDiff;

    unsigned days = unixTS / secPerDay;
    unixTS -= days * secPerDay;
    unsigned mins = unixTS / 60;
    unixTS -= mins * 60;
    unsigned ticks = unixTS * 50;

    write(offset, days);
    write(offset + 4, mins);
    write(offset + 8, ticks);
}

auto SectorBlock::capital(char c) -> char {
    return (c >= 'a' && c <= 'z') ? c - ('a' - 'A') : c;
}

auto SectorBlock::calcHash(const std::string& str) -> uint32_t {
    unsigned length = str.size();
    uint32_t result = length;

    for (int i = 0; i < length; i++) {
        char c = capital(str[i]);
        result = (result * 13 + (uint32_t)c) & 0x7FF;
    }

    return result;
}

}
