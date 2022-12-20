
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

    if (type != Type::EMPTY_BLOCK)
        data = new uint8_t[filesystem.bSize];

    init();
}

auto SectorBlock::init() -> void {
    switch(type) {
        case ROOT_BLOCK: {
            write32(0, 2);
            write32(12, (bSize() >> 2) - 56);
            write32(-200, (uint32_t) -1);
            time_t unixTS = std::time(nullptr);
            writeDate(unixTS, -28); // creation date
            writeDate(unixTS, -92); // modification date
            write32(-4, 1); // ST_ROOT

        } break;
        case BOOT_BLOCK:
            if (nr == 0) {
                data[0] = 'D';
                data[1] = 'O';
                data[2] = 'S';
                data[3] = filesystem.structure == Filesystem::Structure::FFS ? 1 : 0;
                write32(8, 880); // for HD too
            }
            break;
        case DIR_BLOCK:
            write32(0, 2);
            write32(4, nr);
            write32(-4, 2);
            writeDate(std::time(nullptr), -92); // access date
            break;
        case FILE_HEADER_BLOCK:
            write32(0, 2);
            write32(4, nr);
            write32(-4, (uint32_t)-3);
            writeDate(std::time(nullptr), -92); // access date
            break;
    }
}

auto SectorBlock::setName(std::string name) -> void {

    switch(type) {
        case ROOT_BLOCK:
        case DIR_BLOCK:
        case FILE_HEADER_BLOCK:
            if (name == "") name = "empty";
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

auto SectorBlock::setBitmapBlockPtr(unsigned pos, unsigned value) -> void {
    switch(type) {
        case ROOT_BLOCK:
            if (pos < 25)
                write32((pos - 49) << 2, value);
            break;
        case BITMAP_EXT_BLOCK:
            write32(pos << 2, value);
            break;
    }
}

auto SectorBlock::getBitmapBlockPtr(unsigned pos) -> unsigned {
    switch(type) {
        case ROOT_BLOCK:
            if (pos < 25)
                return read32((pos - 49) << 2);
            return 0;
        case BITMAP_EXT_BLOCK:
            return read32(pos << 2);

        default:
            return 0;
    }
}

auto SectorBlock::setBitmapExtBlock(unsigned value) -> void {
    switch(type) {
        case ROOT_BLOCK:
            write32(-96, value);
            break;
        case BITMAP_EXT_BLOCK:
            write32(-4, value);
            break;
    }
}

auto SectorBlock::getBitmapExtBlock() -> unsigned {
    switch(type) {
        case ROOT_BLOCK:
            return read32(-96);
        case BITMAP_EXT_BLOCK:
            return read32(-4);

        default:
            return 0;
    }
}

auto SectorBlock::getParentDir() -> unsigned {
    switch(type) {
        case FILE_HEADER_BLOCK:
        case DIR_BLOCK:
            return read32(-12);

        default:
            return 0;
    }
}

auto SectorBlock::getHashRef(unsigned pos) -> unsigned {
    switch (type) {
        case DIR_BLOCK:
            if (pos < hashTableEntries())
                return read32( (6 + pos) << 2 );
            return 0;
        default:
            return 0;
    }
}

auto SectorBlock::getHashChainRef() -> unsigned {
    switch (type) {
        case DIR_BLOCK:
        case FILE_HEADER_BLOCK:
            return read32(-16);

        default:
            return 0;
    }
}

auto SectorBlock::getChecksumOffset() -> int {
    switch(type) {
        case BOOT_BLOCK:
            return 4;

        case ROOT_BLOCK:
        case DIR_BLOCK:
        case FILE_HEADER_BLOCK:
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
        if (nr == 0)
            write32(offset, 0);
        else
            result = read32(offset);

        for (unsigned i = 0; i < bSize(); i += 4) {
            precsum = result;
            result += read32(i);
            if (result < precsum) result++;
        }

    } else {
        write32(offset, 0);
        for (unsigned i = 0; i < bSize(); i += 4)
            result += read32(i);
    }

    result = ~result;
    write32( offset, result );
    return result;
}

auto SectorBlock::write32(int offset, uint32_t value) -> void {
    Emulator::copyIntToBufferBigEndian<uint32_t>( getAdrPtr(offset), value);
}

auto SectorBlock::read32(int offset) -> unsigned {
    return Emulator::copyBufferToIntBigEndian<uint32_t>( getAdrPtr(offset) );
}

inline auto SectorBlock::getAdrPtr(int offset) -> uint8_t* {
    if (offset >= 0)
        return data + offset;
    return data + bSize() + offset;
}

auto SectorBlock::readName(int offset, uint8_t allocatedSize) -> std::string {
    std::string out;
    uint8_t* ptr = getAdrPtr(offset);
    uint8_t size = *ptr++;
    uint8_t strSize = std::min(allocatedSize, size);
    out.assign(ptr, ptr + strSize);
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

    write32(offset, days);
    write32(offset + 4, mins);
    write32(offset + 8, ticks);
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

auto SectorBlock::hashTableEntries() -> unsigned {
    switch (type) {
        case ROOT_BLOCK:
        case DIR_BLOCK:
            return 72;

        default:
            return 0;
    }
}

}
