
#pragma once
#include <cstdint>
#include <cstring>
#include <functional>
#include "../../interface.h"
#include "../../tools/DLLoader.h"

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

    enum Type { ADF, EXT2, EXT, IPF, Unknown = -1 } type = Unknown;
    enum {  CAPSInit, CAPSExit, CAPSAddImage, CAPSRemImage,
            CAPSLockImageMemory, CAPSUnlockImage, CAPSLoadImage,
            CAPSGetVersionInfo, CAPSGetImageInfo,
            CAPSLockTrack, CAPSUnlockAllTracks,
            CAPS_METHOD_COUNT};

    std::function<unsigned (uint8_t*, unsigned, unsigned)> write = [](uint8_t* buffer, unsigned length, unsigned offset){ return 0; };
    std::function<unsigned (uint8_t*&)> readAssigned = [](uint8_t*& buffer){ return 0; };
    std::function<unsigned (uint8_t*, unsigned)> writeAssigned = [](uint8_t* buffer, unsigned length){ return 0; };

    struct Track {
        int pos;
        uint8_t* data = nullptr;
        unsigned length = 0;
        unsigned bits = 0;
        uint8_t options = 0; // Bit 0: written, Bit 1: IPF Weak Bit Track
        uint16_t* cellWidth = nullptr; // flux formats like IPF
        int32_t overlap = -1;
    };

    Agnus& agnus;
    bool hd;
    uint8_t trackCount;
    Track tracks[ LIBAMI_MAX_TRACKS ];

    bool writeProtected = true;
    unsigned serializationSize = 0;
    bool virtualCreated = false;
    static Emulator::DLLoader dlLoader;
    int capsImageId;

    auto attach(uint8_t* data, unsigned size) -> bool;
    auto detach() -> void;
    auto analyze(uint8_t*& data, unsigned& size) -> bool;
    auto analyzeEXT(uint8_t* data, unsigned size) -> bool;
    auto analyzeEXT2(uint8_t* data, unsigned size) -> bool;
    auto analyzeADF(uint8_t* data, unsigned size) -> bool;
    auto analyzeDMS(uint8_t*& data, unsigned& size) -> bool;
    auto analyzeIPF(uint8_t* data, unsigned size) -> bool;
    auto analyzeEXE(uint8_t*& data, unsigned& size) -> bool;

    auto prepareADF(uint8_t* data, unsigned size) -> void;
    auto prepareEXT(uint8_t* data, unsigned size) -> void;
    auto prepareEXT2(uint8_t* data, unsigned size) -> void;
    auto createEXT2(unsigned size) -> uint8_t*;
    auto prepareIPF(uint8_t* data, unsigned size) -> void;

    static auto initIPF() -> bool;
    static auto destroyIPF() -> void;
    auto removeIPF() -> void;
    auto unloadIPF() -> void;
    auto loadNextRevIPF(Track& track) -> void;
    auto deleteTimingIPF(Track& track) -> void;
    auto addTimingIPF(Track& track, unsigned tiLength, uint32_t* tiData) -> void;

    auto storeWrittenTracks(Emulator::Interface::Media* media) -> void;

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
    auto initTrack(Track& track, unsigned newLength = 0, unsigned bits = 0, uint8_t initVal = 0) -> void;

    auto getADFCreationImageSize() -> unsigned;
    auto getEXT2CreationImageSize() -> unsigned;

    auto markAppendedADFTracks() -> void;
    auto updateTrackCount() -> void;

    auto updateSerializationSize() -> void;
    auto applyAssignedSave() -> void;
    auto updateWrittenTracks(uint8_t* data, unsigned size) -> void;
    auto setStandardTiming(Track& track) -> bool;

    static auto create(System* system, Type type, std::string name, bool hd, bool ffs, bool bootable) -> Emulator::Interface::Data;
    static auto getPreview(System* system, Emulator::Interface::Media* media, uint8_t* data, unsigned size) -> std::vector<Emulator::Interface::Listing>;

};

}

