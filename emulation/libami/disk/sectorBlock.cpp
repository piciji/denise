
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

}

auto SectorBlock::write32(int offset, uint32_t value) -> void {
    if (offset >= 0)
        Emulator::copyIntToBufferBigEndian<uint32_t>(data + offset, value);
    else
        Emulator::copyIntToBufferBigEndian<uint32_t>(data + bSize + offset, value);
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
