
#pragma once

#include "../../tools/listing.h"
#include "../system/system.h"

namespace LIBC64 {

struct Tape;

struct TapeStructure {

    TapeStructure(Tape* tape);
    ~TapeStructure();

    std::vector<Emulator::Interface::Listing> listings;
    Tape* tape = nullptr;

    struct FileEntry {
        uint8_t name[16];
        uint8_t type;
        bool turoTape;
        uint16_t startAddr;
        uint16_t endAddr;
        int number = -1;
    };

    uint8_t version;
    uint8_t* rawData = nullptr;
    unsigned rawSize;
    unsigned fetchPos;
    unsigned fetchSize;
    unsigned curPos;
    uint8_t* fetchData;

    auto setData(uint8_t* data, unsigned size) -> void;
    auto analyzeFile() -> int;
    auto fetchPulse( ) -> int;
    auto getBit() -> int;
    auto getByte() -> int;
    auto getTTByte() -> int;
    auto findCbm() -> bool;
    auto skipCbm() -> bool;
    auto skipCbmFile(bool seq) -> bool;
    auto readCbmHeader(FileEntry& fileEntry) -> bool;
    auto readCbmBlock(uint8_t* buffer, unsigned size) -> bool;
    auto readCbmBlock(uint8_t* buffer, unsigned& size, std::vector<unsigned>& errors, uint8_t& pass) -> int;
    auto nextFile(FileEntry& fileEntry) -> bool;
    auto skipTT() -> bool;
    auto skipTTFile() -> bool;

    auto readTTBlock(uint8_t* buffer, unsigned size, bool header = false) -> bool;
    auto readTTHeader(FileEntry& fileEntry) -> bool;
    auto getListing( ) -> std::vector<Emulator::Interface::Listing>&;
    auto getFilePosition( unsigned fileNumber ) -> unsigned;
    auto setPosition( unsigned pos ) -> void;
    auto readForward( uint8_t& byte ) -> bool;
};

}
