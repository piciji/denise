
#pragma once

#include "../../tools/listing.h"
#include "../../interface.h"

namespace LIBC64 {

struct Tape;

struct TapeStructure {

    TapeStructure(Tape& tape);
    ~TapeStructure();

    std::vector<Emulator::Interface::Listing> listings;
    Tape& tape;

    struct FileEntry {
        uint8_t* header = nullptr;
        unsigned headerSize = 0;

        uint8_t type;
        bool turoTape;
        uint16_t startAddr;
        uint16_t endAddr;
        int number = -1;

        unsigned offset = 0;
        unsigned dataOffset = 0;
        unsigned endOffset = 0;

        uint8_t* buffer = nullptr;
        unsigned size = 0;
    };
    std::vector<FileEntry> fileEntries;
    FileEntry* curFileEntry = nullptr;

    uint8_t version;
    uint8_t* rawData = nullptr;
    unsigned rawSize;
    unsigned curPos;
    unsigned allocatedSize;

    auto setData(uint8_t* data, unsigned size) -> void;
    auto analyzeFile() -> int;
    auto fetchPulse( ) -> int;
    auto getBit() -> int;
    auto getByte() -> int;
    auto getTTByte() -> int;

    auto readCbmHeader(FileEntry& fileEntry) -> bool;
    auto readCbmBlock(uint8_t* buffer, unsigned& size, bool forceSecondPassEvenFirstPassIsErrorFree) -> bool;
    auto readCbmBlock(uint8_t* buffer, unsigned& size, std::vector<unsigned>& errors, uint8_t& pass) -> int;
    auto nextFile(FileEntry& fileEntry) -> bool;

    auto readCurFile() -> FileEntry*;
    auto readFile(FileEntry& fileEntry) -> bool;
    auto readTTBlock(uint8_t* buffer, unsigned size, bool header = false) -> bool;
    auto readTTHeader(FileEntry& fileEntry) -> bool;
    auto getListing( ) -> std::vector<Emulator::Interface::Listing>&;
    auto setFile( unsigned fileNumber ) -> bool;
    auto setPosition( unsigned pos ) -> void;
    auto readForward( uint8_t& byte ) -> bool;

    auto readTTFile(FileEntry& fileEntry) -> bool;
    auto readCbmFile(FileEntry& fileEntry) -> bool;
    auto readCbmFilePrg(FileEntry& fileEntry) -> bool;
    auto readCbmFileSeq(FileEntry& fileEntry) -> bool;

    auto getCurFile() -> FileEntry*;
    auto clearBuffer() -> void;

    auto jumpOverCbmFile(bool seq) -> bool;
    auto jumpOverCbmGap() -> bool;
    auto jumpOverTTGap() -> bool;
};

}
