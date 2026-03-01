#pragma once

#include <cstdint>
#include "../../interface.h"
#include "hardDiskStructure.h"

namespace Emulator {
    struct Serializer;
}

namespace LIBAMI {

struct System;
struct Agnus;
struct Interface;

struct HardDrive {

    enum class Register { Data = 0, Error = 1, Features = 1, SectorCount = 2,
        SectorNumber = 3, CylinderLow = 4, CylinderHigh = 5, DevHead = 6,
        Status = 7, Command = 7 };

    enum class State { ERR = 1, IDX = 2, CORR = 4, DRQ = 8, DSC = 0x10, DWF = 0x20, DRDY = 0x40, BSY = 0x80 };

    enum class Error { Abrt = 4, Idnf = 0x10 };

    enum class Command { Nop = 0x00, Identify = 0xec,
        ReadSectorRetry = 0x20, ReadSector = 0x21, ReadVerifySectorRetry = 0x40, ReadVerifySector = 0x41,
        WriteSectorRetry = 0x30, WriteSector = 0x31, WriteVerify = 0x3c,
        SetMultiple = 0xc6, ReadSectorMultiple = 0xc4, WriteSectorMultiple = 0xc5,
    };

    HardDiskStructure structure;
    uint8_t number;
    Agnus& agnus;
    System* system;
    Interface* interface;
    Emulator::Interface::Media* media;
    bool connected;
    uint8_t processTimer; // in hsync counts
    bool raiseDRQ;

    bool writeProtected;
    uint8_t* buffer;
    unsigned bufferSize;
    uint8_t status;
    uint8_t error;
    unsigned transferSize;
    unsigned transferOffset;
    unsigned writeOffset;

    uint8_t sectorCount;
    uint8_t multiple;

    struct {
        uint8_t sectors;
        unsigned pos;
    } refill;

    struct {
        uint8_t sector;
        uint8_t cylLo;
        uint8_t cylHi;
        uint8_t devHead;
    } lba;

    std::vector<HardDiskStructure::FileDriver> fileDriversInUse;

    HardDrive(uint8_t number, System* system, Agnus& agnus);
    ~HardDrive();

    auto attach(uint8_t* data, uint64_t size) -> bool;
    auto attached() -> bool { return structure.size != 0; }

    auto detach() -> void;

    auto power(bool softReset) -> void;

    auto peekReg(uint8_t reg, uint8_t cs = 0) -> uint16_t;
    auto readReg(uint8_t reg, uint8_t cs = 0) -> uint16_t;
    auto writeReg(uint8_t reg, uint16_t data, uint8_t cs = 0) -> void;
    auto fail() -> void;

    auto setCommand(uint8_t command) -> void;
    auto identify() -> void;
    auto setMultiple() -> void;
    auto readSector(bool verify = false, bool useMultiple = false) -> void;
    auto writeSector(bool useMultiple = false) -> void;

    auto read(unsigned offset, unsigned length) -> uint8_t*;
    auto write(unsigned offset, unsigned length) -> bool;

    auto isBusy() const -> bool { return status & (uint8_t)State::BSY; }

    auto process() -> bool;

    auto fetchData() -> uint16_t;
    auto writeData(uint16_t data) -> void;

    auto writeProtect(bool state) -> void;    

    auto serialize(Emulator::Serializer& s, bool light = false, bool builtIn = true) -> void;

    auto getLba() -> unsigned;
    auto maxLba() -> unsigned;
    auto getSectorsToTransfer() -> unsigned { return !sectorCount ? 256 : sectorCount; }
    auto setBuffer(unsigned newSize) -> void;
    auto setBusy(uint8_t delay, bool finishWithDataRequet) -> void;
    auto getPositonForUI() -> unsigned;
    auto convertCHS(unsigned _lba) -> void;

    auto countPartitions() -> uint16_t { return structure.partitions.size(); }
    auto countFileDrivers() -> uint16_t { return fileDriversInUse.size(); }
    auto partitions() -> std::vector<HardDiskStructure::Partition>& { return structure.partitions; }
    auto fileDrivers() -> std::vector<HardDiskStructure::FileDriver>& { return fileDriversInUse; }
    auto geometry() -> HardDiskStructure::Geometry& { return structure.geometry; }

    auto setUsableFileDrivers() -> void;
    auto getCopiedFileDriverMemOffset(uint32_t dosType) -> uint32_t;

    auto getListing() -> std::vector<Emulator::Interface::Listing>;
    static auto getPreview(System* system, Emulator::Interface::Media* media, uint8_t* data, uint64_t size) -> std::vector<Emulator::Interface::Listing>;
};

}
