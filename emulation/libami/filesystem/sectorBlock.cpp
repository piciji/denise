
#include "sectorBlock.h"
#include "../../tools/buffer.h"


namespace LIBAMI {

SectorBlock::SectorBlock(Type type, unsigned nr, bool ffs, unsigned size) {
    this->type = type;
    this->nr = nr;
    this->ffs = ffs;
    this->bSize = (size > MAX_AMI_BLOCK_SIZE) ? MAX_AMI_BLOCK_SIZE : size;
    init();
}

auto SectorBlock::init() -> void {

    switch(type) {
        case ROOT_BLOCK: {
            write32(0, 2);
            write32(12, (bSize >> 2) - 56);
            write32(-200, (uint32_t) -1);
            time_t unixTS = std::time(nullptr);
            writeDate(unixTS, -28); // creation date
            writeDate(unixTS, -92); // modification date
            write32(-4, 1); // ST_ROOT

        } break;
        case BOOT_BLOCK:
            data[0] = 'D';
            data[1] = 'O';
            data[2] = 'S';
            data[3] = (uint8_t)ffs;
            break;
    }
}

auto SectorBlock::setName(std::string name) -> void {

    switch(type) {
        case ROOT_BLOCK:
            if (name == "") name = "empty";
            writeName(-80, name, 30);
            break;
    }
}

auto SectorBlock::setBitmapBlockPtr(unsigned pos, unsigned value) -> void {
    switch(type) {
        case ROOT_BLOCK:
            if (pos < 25)
                write32((pos - 49) << 2, value);
            break;
    }
}

auto SectorBlock::setBitmapExtBlock(unsigned value) -> void {
    switch(type) {
        case ROOT_BLOCK:
            write32(-96, value);
            break;
    }
}


auto SectorBlock::write32(int offset, uint32_t value) -> void {
    Emulator::copyIntToBufferBigEndian<uint32_t>( getAdrPtr(offset), value);
}

inline auto SectorBlock::getAdrPtr(int offset) -> uint8_t* {
    if (offset >= 0)
        return data + offset;
    return data + bSize + offset;
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

}
