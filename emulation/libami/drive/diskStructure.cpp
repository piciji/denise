
#include "diskStructure.h"
#include "filesystem.h"
#include "../../tools/buffer.h"
#include "../agnus/agnus.h"
#include "../system/system.h"
#include "adf.cpp"
#include "ext.cpp"
#include "ext2.cpp"
#include "dms.cpp"
#include "ipf.cpp"
#include "exe.cpp"

namespace LIBAMI {

DiskStructure::DiskStructure(Agnus& agnus) : agnus(agnus) {
    writeProtected = true;
    capsImageId = -1;
    for(unsigned i = 0; i < LIBAMI_MAX_TRACKS; i++) {
        tracks[i].pos = i;
    }
}

DiskStructure::~DiskStructure() {
    for(unsigned i = 0; i < LIBAMI_MAX_TRACKS; i++) {
        Track& track = tracks[i];
        if (track.data)
            delete[] track.data;
        if (track.cellWidth)
            delete[] track.cellWidth;
    }
    removeIPF();
}

auto DiskStructure::attach(uint8_t* data, unsigned size) -> bool {
    if (!analyze(data, size))
        return false;

    switch(type) {
        case Type::ADF:
            prepareADF(data, size);
            if (virtualCreated) // virtual created: DMS, EXE
                delete[] data;
            break;
        case Type::EXT:
            prepareEXT(data, size);
            break;
        case Type::EXT2:
            prepareEXT2(data, size);
            break;
        case Type::IPF:
            prepareIPF(data, size);
            break;
        default:
            return false;
    }

    applyAssignedSave();
    return true;
}

auto DiskStructure::detach() -> void {
    if (type == Type::IPF)
        unloadIPF();

    type = Type::Unknown;
    writeProtected = false;
    hd = false;
    virtualCreated = false;
}

auto DiskStructure::analyze(uint8_t*& data, unsigned& size) -> bool {
    if (!data || !size)
        return false;

    if (analyzeEXT(data, size))
        return true;

    if (analyzeEXT2(data, size))
        return true;

    if (analyzeDMS(data, size))
        return true;

    if (analyzeIPF(data, size))
        return true;

    if (analyzeADF(data, size))
        return true;

    if (analyzeEXE(data, size))
        return true;

    return false;
}

auto DiskStructure::storeWrittenTracks(Emulator::Interface::Media* media) -> void {
    uint8_t* buffer;
    unsigned sectors = hd ? 22 : 11;

    if (!agnus.interface->questionToWrite(media))
        return;

    if ((type == Type::ADF) && !virtualCreated) {
        markAppendedADFTracks();
        buffer = new uint8_t[sectors * 512];

        for(unsigned i = 0; i < LIBAMI_MAX_TRACKS; i++) {
            Track& track = tracks[i];
            if (track.options & 1) {
                std::memset(buffer, 0, sectors * 512);
                decodeTrack(track, buffer);
                if (!write(buffer, sectors * 512, sectors * 512 * i)) {
                    delete[] buffer;
                    goto ExtraSaveImage; // ADF is compressed or was openend read only
                }
                track.options &= ~1;
            }
        }
        delete[] buffer;
        return;
    }

ExtraSaveImage:
    updateTrackCount();
    unsigned extSize = getEXT2CreationImageSize();
    uint8_t* extData = createEXT2(extSize);
    writeAssigned( extData, extSize );
    delete[] extData;
}

auto DiskStructure::applyAssignedSave() -> void {
    uint8_t* buffer = nullptr;
    auto length = readAssigned(buffer);

    if (!length || !buffer)
        return;

    auto _type = type;

    if (analyzeEXT2(buffer, length)) {
        updateWrittenTracks(buffer, length);
        if (_type == Type::IPF)
            type = Type::IPF;
        else if (_type == Type::ADF)
            type = Type::ADF;
            // when use EXT2 dont forget to increase bits to be byte aligned
    }

    delete[] buffer;
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
    if (type == Type::Unknown)
        return {};

    unsigned trackSize = (hd ? 22 : 11) * 512;
    unsigned size = trackCount * trackSize;
    uint8_t* data = new uint8_t[size];

    for(unsigned i = 0; i < trackCount; i++) {
        Track& track = tracks[i];
        decodeTrack(track, data + i * trackSize);
    }

    Filesystem fs(size);
    if (fs.importMedia( data, size )) {
        delete[] data;
        return fs.getDirectory();
    }

    delete[] data;
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
    track.options = 0;
    deleteTimingIPF(track);
}

auto DiskStructure::setStandardTiming(Track& track) -> bool {
    if (type == DiskStructure::ADF || type == DiskStructure::Unknown)
        return false;

    unsigned standardBitLength = getTrackBitLength();
    deleteTimingIPF(track);
    track.options &= ~2; // disable possible multi rev behaviour

    if (standardBitLength != track.bits) {
        track.bits = standardBitLength;
        track.length = getTrackByteLength();
        return true;
    }
    return false;
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
        if (!(track.options & 1))
            continue;

        serializationSize += 4 + 4 + 4 + track.length;
    }

    agnus.system->serializationSize += serializationSize;
}

auto DiskStructure::serialize(Emulator::Serializer& s, bool written) -> void {
    s.integer( (int&)type );
    s.integer( hd );
    s.integer( trackCount );
    s.integer( writeProtected );
    s.integer( virtualCreated );
    s.integer( serializationSize );

    if (!written || (s.mode() == Emulator::Serializer::Mode::Size))
        return;

    for (unsigned i = 0; i < LIBAMI_MAX_TRACKS; i++) {
        Track& track = tracks[i];
        s.integer(track.options);

        if (!(track.options & 1))
            continue;

        unsigned _trackLength = track.length;
        s.integer(track.length);
        s.integer(track.bits);

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
