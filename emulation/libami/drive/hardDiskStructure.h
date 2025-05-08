#pragma once

#include "../../interface.h"
#include "hunk.h"

namespace LIBAMI {

struct System;

struct Agnus;

struct Interface;

struct HardDiskStructure {

    HardDiskStructure(Agnus& agnus, Emulator::Interface::Media* media);

    Agnus& agnus;

    uint8_t* data;

    uint64_t size;

    bool hasRDB;

    Emulator::Interface::Media* media;

    uint8_t buffer[512];

    struct Partition {
        std::string name;
        uint32_t cylLo;
        uint32_t cylHi;
        uint32_t heads;
        uint32_t bSize; // logical block size of file system partition
        uint32_t sectors;
        uint32_t dosType;
        uint32_t reserved;
        uint32_t flags;
        uint32_t bootFlags;
        uint32_t interleave;
        uint32_t numBuffers;
        uint32_t bufMemType;
        uint32_t maxTransfer;
        uint32_t mask;
        uint32_t bootPrio;

        unsigned blocks;
        unsigned offset;
        uint64_t size;
    };
    std::vector<Partition> partitions;

    struct FileDriver {
        uint32_t dosType;
        uint32_t dosVersion;
        uint32_t patchFlags;
        std::vector<unsigned> segList;        
        uint32_t seglistBptr;
    };    
    std::vector<FileDriver> fileDrivers;

    struct Geometry {
        unsigned cylinders;
        unsigned sectors;
        unsigned heads;
        unsigned bSize; // physical block size? (identified from HD, written to RDB)

        unsigned tracks() const { return heads * cylinders; }
        unsigned blocks() const { return tracks() * sectors; }
        uint64_t length() const { return (uint64_t)blocks() * (uint64_t)bSize; }
    } geometry;

    auto buildHdfFromBinaries(const std::string& name, std::vector<Emulator::Interface::Item>& files) -> Emulator::Interface::Data;
    static auto buildHardDisk(System* system, const std::string& name, std::vector<Emulator::Interface::Item>& files) -> Emulator::Interface::Data;

    auto getRDB() -> uint8_t*;

    auto getBlock(unsigned ref) -> uint8_t*;

    auto detectGeometry() -> void;

    auto detectPartitions() -> void;

    auto detectFileDrivers() -> void;

    auto addPartition(uint8_t* data) -> void;

    auto addFileDriver(uint8_t* data) -> void;

    auto predictGeometrie() -> void;

    auto attach(uint8_t* data, uint64_t size) -> bool;

    auto detach() -> void;

    auto reset() -> void;

    auto getListing() -> std::vector<Emulator::Interface::Listing>;

    auto readFileDriver(FileDriver& fileDriver, std::vector<uint8_t>& code) -> void;

    auto makePartitionUnique(std::string& name, unsigned suffix = 0) -> void;
};

}
