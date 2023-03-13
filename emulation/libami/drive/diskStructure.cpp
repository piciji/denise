
#include "diskStructure.h"
#include "filesystem.h"
#include "../../tools/buffer.h"
#include "../agnus/agnus.h"
#include "../system/system.h"
#include "adf.cpp"
#include "ext.cpp"
#include "ext2.cpp"

namespace LIBAMI {

DiskStructure::DiskStructure(Agnus& agnus) : agnus(agnus) {
    writeProtected = true;
}

DiskStructure::~DiskStructure() {
    for(unsigned i = 0; i < LIBAMI_MAX_TRACKS; i++) {
        Track& track = tracks[i];
        if (track.data)
            delete[] track.data;
    }
}

auto DiskStructure::attach(uint8_t* data, unsigned size) -> bool {
    if (!analyze(data, size))
        return false;

    switch(type) {
        case Type::ADF:
            prepareADF(data, size);
            break;
        case Type::EXT:
            prepareEXT(data, size);
            break;
        case Type::EXT2:
            prepareEXT2(data, size);
            break;
        default:
            return false;
    }

    rawData = data;
    rawSize = size;

    return true;
}

auto DiskStructure::detach() -> void {
    rawData = nullptr;
    rawSize = 0;
    type = Type::Unknown;
    writeProtected = false;
    hd = false;
}

auto DiskStructure::analyze(uint8_t* data, unsigned size) -> bool {
    if (analyzeEXT(data, size))
        return true;

    if (analyzeEXT2(data, size))
        return true;

    if (analyzeADF(data, size))
        return true;

    return false;
}

auto DiskStructure::storeWrittenTracks() -> void {
    if (type == Type::EXT || type == Type::EXT2) {
        if (EXT2ImageNeedsCompleteRebuild()) {
            unsigned extSize = getEXT2CreationImageSize();
            uint8_t* extData = createEXT2(extSize);
            write( extData, extSize, 0 );
            delete[] extData;
            return;
        }
    } else if (type == Type::ADF)
        markAppendedADFTracks();

    unsigned sectors = hd ? 22 : 11;
    unsigned trackLength = 0;
    uint8_t buffer[sectors * 512];

    for(unsigned i = 0; i < LIBAMI_MAX_TRACKS; i++) {
        Track& track = tracks[i];
        if (track.written & 1) {
            if (type == Type::ADF) {
                std::memset(buffer, 0, sectors * 512);
                decodeTrack(track, buffer);
                write(buffer, sectors * 512, sectors * 512 * i);
            } else if (type == Type::EXT) {
                write(track.data, track.length, 8 + trackCount * 4 + trackLength);
            } else if (type == Type::EXT2) {
                write(track.data, track.length, 12 + trackCount * 12 + trackLength);
            }
            track.written = 0;
        }
        trackLength += track.storage;
    }
}

auto DiskStructure::create( System* system, Type type, std::string name, bool hd, bool ffs, bool bootable ) -> Emulator::Interface::Data {
    DiskStructure disk(system->agnus);
    disk.hd = hd;
    unsigned size = disk.getADFCreationImageSize();

    uint8_t* data = new uint8_t[size];
    std::memset(data, 0, size);

    Filesystem fs(size, ffs ? Filesystem::Structure::FFS : Filesystem::Structure::OFS);
    fs.format(name, bootable);
    fs.exportMedia(data, size);

    // we do not want to encourage the user to create EXT1.
    if (type == EXT2) {
        disk.attach(data, size);

        unsigned extSize = disk.getEXT2CreationImageSize();
        uint8_t* extData = disk.createEXT2(extSize);
        delete[] data;

        return {extData, extSize};
    }

    return {data, size};
}

auto DiskStructure::getListing() -> std::vector<Emulator::Interface::Listing> {
    uint8_t* data = rawData;
    unsigned size = rawSize;

    if (!data)
        return {};

    if (type == Type::EXT || type == Type::EXT2) {
        unsigned trackSize = (hd ? 22 : 11) * 512;
        size = trackCount * trackSize;
        data = new uint8_t[size];

        for(unsigned i = 0; i < trackCount; i++) {
            Track& track = tracks[i];
            decodeTrack(track, data + i * trackSize);
        }
    }

    Filesystem fs(size);
    if (fs.importMedia( data, size )) {
        if (type == Type::EXT || type == Type::EXT2)
            delete[] data;

        return fs.getDirectory();
    }
    return {};
}

auto DiskStructure::getPreview(System* system, uint8_t* data, unsigned size) -> std::vector<Emulator::Interface::Listing> {
    DiskStructure disk(system->agnus);

    if (!disk.attach( data, size ))
        return {};

    return disk.getListing();
}

auto DiskStructure::initTrack(Track& track, unsigned newLength, unsigned bits, uint8_t initVal) -> void {
    if (!newLength)
        newLength = getTrackByteLength();

    if (!track.data) {
        track.data = new uint8_t[newLength];
    } else if (newLength != track.length) {
        delete[] track.data;
        track.data = new uint8_t[newLength];
    }

    std::memset( track.data, initVal, newLength );
    track.length = newLength;
    track.bits = bits == 0 ? getTrackBitLength() : bits;
    track.written = 0;
}

auto DiskStructure::getTrackBitLength() -> unsigned {
    return (agnus.ntsc ? LIBAMI_FLOPPY_REVOLUTION_LENGTH_NTSC : LIBAMI_FLOPPY_REVOLUTION_LENGTH_PAL) << hd;
}

auto DiskStructure::getTrackByteLength() -> unsigned {
    return (getTrackBitLength() + 7) / 8;
}

auto DiskStructure::updateSerializationSize() -> void {
    if (serializationSize)
        agnus.system->serializationSize -= serializationSize;

    serializationSize = 0;
    for (unsigned i = 0; i < LIBAMI_MAX_TRACKS; i++) {
        Track& track = tracks[i];
        serializationSize += 1;
        if (!(track.written & 1))
            continue;

        serializationSize += 4 + 4 + 4 + track.length;
    }

    agnus.system->serializationSize += serializationSize;
}

auto DiskStructure::serialize(Emulator::Serializer& s, bool written) -> void {
    s.integer( (int&)type );
    s.integer( hd );
    s.integer( trackCount );
    s.integer( rawSize );
    s.integer( writeProtected );
    s.integer( serializationSize );

    if (!written || (s.mode() == Emulator::Serializer::Mode::Size))
        return;

    for (unsigned i = 0; i < LIBAMI_MAX_TRACKS; i++) {
        Track& track = tracks[i];
        s.integer(track.written);

        if (!(track.written & 1))
            continue;

        unsigned _trackLength = track.length;
        s.integer(track.length);
        s.integer(track.bits);
        s.integer(track.storage);

        if (s.mode() == Emulator::Serializer::Mode::Load) {
            if (_trackLength != track.length) {
                if (track.data)
                    delete[] track.data;

                track.data = nullptr;

                if (track.length)
                    track.data = new uint8_t[track.length];
            }
        }

        if (track.length)
            s.array(track.data, track.length);
    }
}

}
