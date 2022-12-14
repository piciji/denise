
#include "disk.h"
#include "sectorBlock.h"

#define LIBAMI_FLOPPY_REVOLUTION_LENGTH_PAL 101339 //bits per revolution
#define LIBAMI_FLOPPY_REVOLUTION_LENGTH_NTSC 102272 //bits per revolution

namespace LIBAMI {

auto Disk::attach(uint8_t* data, unsigned size) -> bool {
    rawData = data;
    rawSize = size;

    if (size < 32)
        return false;

    type = (std::memcmp(data, "RAW", 3) == 0) ? Type::RAW : Type::ADF;

    (type == Type::RAW) ? readRawHeader() : readAdfHeader();

    return true;
}

auto Disk::readRawHeader() -> void {
    trackCount = rawData[4];
    hd = rawData[5];
    unsigned offset = 32 + 166;
    uint8_t* ptr = rawData;
    ptr += 32;

    for(unsigned i = 0; i < trackCount; i++) {
        Track& track = tracks[i];
        track.offset = offset;
        track.formatted = *ptr++ ? true : false;
        offset += getRawTrackSize(hd);
    }
}

auto Disk::readAdfHeader() -> void {
    uint8_t sectors;
    trackCount = 0;
    hd = false;

    if ( rawSize > (160 * 11 * 512 + 511) ) {
        for (unsigned i = 80; i <= 83; i++) {
            if (rawSize == i * 22 * 512 * 2) { // HD
                hd = true;
                trackCount = rawSize / (512 * (sectors = 22));
                break;
            } if (rawSize == i * 11 * 512 * 2) { // >80 cyl DD
                trackCount = rawSize / (512 * (sectors = 11));
                break;
            }
        }
        if (trackCount == 0) {
            trackCount = rawSize / (512 * (sectors = 22));
            hd = true;
        }
    } else {
        trackCount = rawSize / (512 * (sectors = 11));
    }

    for(unsigned i = 0; i < trackCount; i++) {
        Track& track = tracks[i];
        track.offset = i * 512 * sectors;
        track.formatted = true;
    }
}

auto Disk::create( Type type, bool hd, std::string name, bool ffs ) -> Emulator::Interface::Data {
    unsigned size = getImageSize(type, hd);

    uint8_t* data = new uint8_t[size];
    std::memset(data, 0, size);

    if (type == ADF) {
        SectorBlock bootBlock(SectorBlock::Type::BOOT_BLOCK, 0, ffs);

        SectorBlock rootBlock(SectorBlock::Type::ROOT_BLOCK, size / 2);
        rootBlock.setName(name);

        std::strcpy ((char*)data, "DOS");
        data[3] = ffs ? 1 : 0;
        writeRootblock(data + _size / 2, _size / 1024, name, hd);
    } else { //RAW
        unsigned rawTrackSize = getRawTrackSize( hd );
        strcpy((char*)data, "RAW");
        data[4] = 166;
        data[5] = hd ? 1 : 0;
        data[6] = rawTrackSize >> 8;
        data[7] = rawTrackSize & 0xff;

        if ( ffs || (name != "") ) {
            u8* target = data + 32 + 166;
            strcpy ((char*)target, "DOS");
            target[3] = ffs ? 1 : 0;

            target += 80 * rawTrackSize;
            writeRootblock(target, 80 * 11 * (hd ? 2 : 1), name, hd);
        }
    }
}

auto Disk::writeRootblock(uint8_t* data, int blocksize, std::string name, bool hd) -> void {



    data[316+2] = (blocksize + 1) >> 8;
    data[316+3] = (blocksize + 1) & 0xff;



    memcpy (data + 472, data + 420, 3 * 4);
    memcpy (data + 484, data + 420, 3 * 4);
    writeDiskChecksum (data, data + 20);
    /* bitmap block */
    memset (data + 512 + 4, 0xff, 2 * blocksize / 8);
    data[512 + (!hd ? 0x72 : 0xdc) ] = 0x3f;

    writeDiskChecksum (data + 512, data + 512);
}

auto Disk::getImageSize(Type type, bool hd) -> unsigned {
    unsigned size = 0;

    if (type == RAW) {
        size = 32 + 166 + 83 * 2 * getRawTrackSize( hd );
    } else if (type == ADF) {
        size = 11 * 512 * 80 * 2;
        if (hd) size <<= 1;
    }
    return size;
}

auto Disk::getRawTrackSize( bool hd ) -> unsigned {
    static unsigned _size = LIBAMI_FLOPPY_REVOLUTION_LENGTH_NTSC / 8;
    return _size << hd;
}

}
