
#pragma once
#include <cstdint>
#include <cstring>
#include "../../interface.h"

namespace Emulator {
    struct Serializer;
}

namespace LIBAMI {

struct Agnus;

/**
    speed: floppy speed is the same for PAL/NTSC: 0,2 sec per revolution (full track rotation)
    access: differs between PAL/NTSC. a bitcell access is connected to system clock and takes roughly 2 us
    PAL: 7.09379 MHz, NTSC: 7.15909 MHz
    PAL bitcell time:  140.96837 ns (1 cycle) * 14 (nearest integral) = 1.97356 us
    NTSC bitcell time:  139.68256 ns (1 cycle) * 14 (nearest integral) = 1.95556 us
    PAL: 0.2 s / 1.97356 us = 101339 bits = 12668 bytes per revolution
    NTSC: 0.2 s / 1.95556 us = 102272 bits = 12784 bytes per revolution

    all above is valid only for reading standard amiga written disks or writing disks.
    a sophisticated device for generating original disks can write bits with a variable bitcell
    width, so timing can change each bitcell (not emulated at the moment)

    precompensation is uninteresting for emulation, beacuse it doesn't change bitcell width but
    advance or delays the flux transition within the bit cell in order to optimize the distance
    between 2 adjacent transitions
*/

#define LIBAMI_FLOPPY_REVOLUTION_LENGTH_PAL 101339 // bits per revolution
#define LIBAMI_FLOPPY_REVOLUTION_LENGTH_NTSC 102272 // bits per revolution
#define LIBAMI_MAX_TRACKS (84 * 2)

struct Disk {
    Disk(Agnus& agnus);
    ~Disk();

    enum Type { ADF, EXT } type;

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

    auto attach(uint8_t* data, unsigned size) -> bool;
    auto analyze(uint8_t* data, unsigned size) -> bool;
    auto analyzeEXT(uint8_t* data, unsigned size) -> bool;
    auto analyzeADF(uint8_t* data, unsigned size) -> bool;

    auto prepareADF(uint8_t* data, unsigned size) -> void;
    auto prepareEXT(uint8_t* data, unsigned size) -> void;
    auto createEXT(unsigned size) -> uint8_t*;

    auto storeWrittenTracks() -> void;

    auto getListing() -> std::vector<Emulator::Interface::Listing>&;

    auto serialize(Emulator::Serializer& s, bool written) -> void;

    auto encodeTrack(Track& track, unsigned trackNr, uint8_t* userData) -> void;
    auto decodeTrack(Track& track, uint8_t* userData) -> void;

    auto addClockBits( uint16_t* raw, unsigned words) -> void;
    auto separateOddEven(uint8_t* dst, uint8_t src[], unsigned size) -> void;
    auto joinOddEven(uint8_t* dst, uint8_t* src, unsigned size) -> void;

    auto getTrackBitLength() -> unsigned;
    auto getTrackByteLength() -> unsigned;
    auto initTrack(Track& track, unsigned newLength) -> void;

    auto getADFCreationImageSize() -> unsigned;
    auto getEXTCreationImageSize() -> unsigned;

    auto markAppendedADFTracks() -> void;
    auto EXTImageNeedsCompleteRebuild() -> bool;

    static auto create( Type type, std::string name, bool hd, bool ffs, bool bootable ) -> Emulator::Interface::Data;


};

}

