
#pragma once
#include <cstdint>
#include <cstring>
#include <functional>
#include "../../interface.h"

namespace Emulator {
struct Serializer;
}

namespace LIBAMI {

struct System;
struct Agnus;

#define LIBAMI_FLOPPY_REVOLUTION_LENGTH_PAL 101339 // bits per revolution
#define LIBAMI_FLOPPY_REVOLUTION_LENGTH_NTSC 102272 // bits per revolution
#define LIBAMI_MAX_TRACKS (84 * 2)

struct DiskStructure {
    DiskStructure(Agnus& agnus);
    ~DiskStructure();

    enum Type { ADF, EXT, Unknown = -1 } type = Unknown;

    std::function<unsigned (uint8_t*, unsigned, unsigned)> write = [](uint8_t* buffer, unsigned length, unsigned offset){ return 0; };

    struct Track {
        uint8_t* data = nullptr;
        unsigned length = 0;
        unsigned bits = 0;
        uint8_t written = 0; // MSB: if set and track has changed, the entire image must be rewritten
    };

    Agnus& agnus;
    bool hd;
    uint8_t trackCount;
    Track tracks[ LIBAMI_MAX_TRACKS ];

    uint8_t* rawData = nullptr;
    unsigned rawSize = 0;
    bool writeProtected = true;
    unsigned serializationSize = 0;

    auto attach(uint8_t* data, unsigned size) -> bool;
    auto detach() -> void;
    auto analyze(uint8_t* data, unsigned size) -> bool;
    auto analyzeEXT(uint8_t* data, unsigned size) -> bool;
    auto analyzeADF(uint8_t* data, unsigned size) -> bool;

    auto prepareADF(uint8_t* data, unsigned size) -> void;
    auto prepareEXT(uint8_t* data, unsigned size) -> void;
    auto createEXT(unsigned size) -> uint8_t*;

    auto storeWrittenTracks() -> void;

    auto getListing() -> std::vector<Emulator::Interface::Listing>;

    auto serialize(Emulator::Serializer& s, bool written) -> void;

    auto encodeTrack(Track& track, unsigned trackNr, uint8_t* userData) -> void;
    auto decodeTrack(Track& track, uint8_t* userData) -> void;
    auto shiftData(uint8_t* dst, uint8_t* src, unsigned size, uint8_t shift) -> void;

    auto addClockBits( uint16_t* raw, unsigned words) -> void;
    auto separateOddEven(uint8_t* dst, uint8_t* src, unsigned size) -> void;
    auto joinOddEven(uint8_t* dst, uint8_t* src, unsigned size) -> void;

    auto getTrackBitLength() -> unsigned;
    auto getTrackByteLength() -> unsigned;
    auto initTrack(Track& track, unsigned newLength = 0, unsigned bits = 0) -> void;

    auto getADFCreationImageSize() -> unsigned;
    auto getEXTCreationImageSize() -> unsigned;

    auto markAppendedADFTracks() -> void;
    auto EXTImageNeedsCompleteRebuild() -> bool;

    auto updateSerializationSize() -> void;

    static auto create(System* system, Type type, std::string name, bool hd, bool ffs, bool bootable) -> Emulator::Interface::Data;
    static auto getPreview(System* system, uint8_t* data, unsigned size) -> std::vector<Emulator::Interface::Listing>;

};

}

