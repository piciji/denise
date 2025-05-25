
#include "hardDiskStructure.h"
#include "../system/system.h"
#include "../agnus/agnus.h"
#include "filesystemExt.h"
#include "../../tools/buffer.h"
#include "../../tools/error.h"
#include "../../tools/crc32.h"
#include "../system/firmware.h"
#include <cmath>

namespace LIBAMI {

HardDiskStructure::HardDiskStructure(Agnus& agnus, Emulator::Interface::Media* media)
: agnus(agnus), media(media) {
    size = 0;
    data = nullptr;
    hasRDB = false;
}

auto HardDiskStructure::attach(uint8_t* data, uint64_t size) -> bool {
    this->data = data;
    this->size = size;
    
    if (!size)
        return false;

    reset();

    return true;
}

auto HardDiskStructure::reset() -> void {
    detectGeometry();
    detectPartitions();
    detectFileDrivers();
}

auto HardDiskStructure::detach() -> void {
    partitions.clear();
    fileDrivers.clear();
    data = nullptr;
    size = 0;
}

auto HardDiskStructure::getRDB() -> uint8_t* {
    for (int i = 0; i < 16; i++) {
        uint8_t* ptr = getBlock(i);
        if (ptr) {
            if (std::memcmp(ptr, "RDSK", 4) == 0)
                return ptr;
        }
    }
    return nullptr;
}

auto HardDiskStructure::getBlock(unsigned ref) -> uint8_t* {
    if (ref == ~0)
        return nullptr;

    // don't fetch logical filesystem blocks here, hence size does not necessarily have to be 512 for this. 
    if ((512ull * (uint64_t)(ref + 1)) <= size) {
        if (this->data) {
            if ((512ull * (uint64_t)ref + 512ull) <= size)
                return this->data + 512 * ref;
        } else if (agnus.interface->readMedia(media, buffer, 512, 512 * ref) == 512)
            return &buffer[0];
    }

    return nullptr;
}

auto HardDiskStructure::detectGeometry() -> void {
    auto rdb = getRDB();
    hasRDB = rdb != nullptr;

    if (rdb) {        
        geometry.bSize = ToU32BE(rdb + 16);
        geometry.cylinders = ToU32BE(rdb + 64);
        geometry.sectors = ToU32BE(rdb + 68);
        geometry.heads = ToU32BE(rdb + 72);

        if (!geometry.bSize || !geometry.heads || !geometry.cylinders || !geometry.sectors)
            predictGeometrie();
    } else
        predictGeometrie();
}

auto HardDiskStructure::predictGeometrie() -> void {
    geometry.bSize = 512;
    geometry.sectors = 32;

    unsigned numBlocks = size / (uint64_t)geometry.bSize;
    double _inMB = (double)numBlocks / (double)(2 * 1024);

    float intpart;
    float fractpart = std::modf(_inMB, &intpart);

    unsigned inMB = (unsigned)intpart;
    if (inMB <= 2048 && fractpart != 0.0) {
        geometry.heads = 1;

    } else {        
        if (inMB <= 512)
            geometry.heads = 4;
        else if (inMB <= 1024)
            geometry.heads = 8;
        else if (inMB <= 2048)
            geometry.heads = 16;
        else { // because of 63 sectors a few kb are wasted.
            geometry.heads = 16;
            geometry.sectors = 63;
        }
    }

    geometry.cylinders = (unsigned)(numBlocks / (geometry.heads * geometry.sectors));
}

auto HardDiskStructure::detectPartitions() -> void {
    partitions.clear();

    if (hasRDB) {
        auto block = getRDB();
        if (block) {
            block = getBlock(ToU32BE(block + 28));

            if (block && (std::memcmp(block, "PART", 4) == 0)) {
                addPartition(block);

                for (unsigned i = 0; i < 16; i++) {
                    block = getBlock(ToU32BE(block + 16));
                    if (!block || (std::memcmp(block, "PART", 4) != 0))
                        break;

                    addPartition(block);
                }
            }
        }
    }
    
    if (!partitions.size()) {
        Partition partition;
        partition.name = "DH" + std::to_string(media->id);
        makePartitionUnique(partition.name);
        partition.cylLo = 0;
        partition.cylHi = geometry.cylinders ? geometry.cylinders - 1 : 0;
        partition.heads = geometry.heads;
        partition.bSize = geometry.bSize;
        partition.sectors = geometry.sectors;
        partition.dosType = 0x444f5300; // DOS/0 (OFS)
        partition.reserved = 2;
        partition.flags = 0;
        partition.bootFlags = 1; // bootable and auto mount
        partition.interleave = 0;
        partition.numBuffers = 30;
        partition.bufMemType = 0;
        partition.maxTransfer = 0x7ffe;
        partition.mask = 0xfffffffe;
        partition.bootPrio = 0;
        partitions.push_back(partition);
    }

    for (auto& partition : partitions) {
        auto c = partition.cylHi - partition.cylLo + 1;
        auto h = partition.heads;
        auto s = partition.sectors;

        partition.blocks = c * h * s;
        partition.size = (uint64_t)partition.blocks * (uint64_t)partition.bSize;
        partition.offset = partition.cylLo * partition.heads * partition.sectors * partition.bSize;

        inform("HD %i, partition %s size %llu", media->id, partition.name.c_str(), (unsigned long long)partition.size);
    }
}

auto HardDiskStructure::makePartitionUnique(std::string& name, unsigned suffix) -> void {
    std::string test = name;
    if (suffix)
        test += "_" + std::to_string(suffix);
    else {
        bool isPreview = true;
        for (auto& hd : agnus.system->hardDrives) {
            if (&hd.structure == this)
                isPreview = false;
        }
        if (isPreview)
            return;
    }

    for (auto& hd : agnus.system->hardDrives) {
        if (!hd.connected || (media->id < hd.media->id))
            continue;

        for (auto& p : hd.structure.partitions) {
            if (p.name == test)
                return makePartitionUnique(name, suffix + 1);
        }
    }

    if (suffix)
        name = test;
}

auto HardDiskStructure::addPartition(uint8_t* data) -> void {
    Partition partition;
    unsigned length = data[36];
    if (length > 31)
        length = 31;
    partition.name.assign(data + 37, data + 37 + length);
    
    makePartitionUnique(partition.name);
    partition.bootFlags = ToU32BE(data + 20);
    partition.flags = ToU32BE(data + 32);
    partition.bSize = ToU32BE(data + 132) << 2;
    partition.heads = ToU32BE(data + 140);
    partition.sectors = ToU32BE(data + 148);
    partition.reserved = ToU32BE(data + 152);
    partition.interleave = ToU32BE(data + 160);
    partition.cylLo = ToU32BE(data + 164);
    partition.cylHi = ToU32BE(data + 168);
    partition.numBuffers = ToU32BE(data + 172);
    partition.bufMemType = ToU32BE(data + 176);
    partition.maxTransfer = ToU32BE(data + 180);
    partition.mask = ToU32BE(data + 184);
    partition.bootPrio = ToU32BE(data + 188);
    partition.dosType = ToU32BE(data + 192);
    partitions.push_back(partition);
}

auto HardDiskStructure::detectFileDrivers() -> void {
    fileDrivers.clear();

    if (!hasRDB)
        return;

    auto block = getRDB();
    if (!block)
        return;

    block = getBlock(ToU32BE(block + 32));
    if (!block || (std::memcmp(block, "FSHD", 4) != 0))
        return;

    unsigned nextRef = ToU32BE(block + 16);
    addFileDriver(block); // overwrites block buffer, hence fetch nextRef before

    for (unsigned i = 0; i < 16; i++) {
        block = getBlock(nextRef);

        if (!block || (std::memcmp(block, "FSHD", 4) != 0))
            break;

        nextRef = ToU32BE(block + 16);
        addFileDriver(block);
    }
}

auto HardDiskStructure::addFileDriver(uint8_t* data) -> void {
    fileDrivers.push_back({});
    auto& driver = fileDrivers.back();

    driver.dosType = ToU32BE(data + 32);
    driver.dosVersion = ToU32BE(data + 36);
    driver.patchFlags = ToU32BE(data + 40);
    driver.seglistBptr = 0;

    uint8_t* lsegBlock;
    unsigned lsegRef = ToU32BE(data + 72);

    for (unsigned i = 0; i < 1024; i++) {
        auto lsegBlock = getBlock(lsegRef);

        if (!lsegBlock || (std::memcmp(lsegBlock, "LSEG", 4) != 0))
            break;

        driver.segList.push_back(lsegRef);

        lsegRef = ToU32BE(lsegBlock + 16);
    }
}

auto HardDiskStructure::getListing() -> std::vector<Emulator::Interface::Listing> {
    std::vector<Emulator::Interface::Listing> listing;
    Filesystem* fs = nullptr;

    if (hasRDB)
        listing.push_back({ {'R','D','B'} });

    for (auto& partition : partitions) {
        if (this->data == nullptr) {
            FilesystemExt* fsExt = new FilesystemExt(partition.size, (Filesystem::Structure)partition.dosType, partition.bSize);
            fsExt->setDataSource(agnus.interface, media, partition.offset);
            fs = fsExt;
        } else {
            fs = new Filesystem(partition.size, (Filesystem::Structure)partition.dosType, partition.bSize);
            if (!fs->importMedia(data + partition.offset, partition.size)) {

            }
        }

        std::vector<uint16_t> out;
        std::string _name = partition.name;
        _name = " - " + _name;
        out.resize(_name.size());
        for (unsigned i = 0; i < _name.size(); i++)
            out[i] = _name[i];

        auto _listing = fs->getDirectory(true);

        if (_listing.size()) {
            if (!listing.empty())
                listing.push_back({  });

            Filesystem::combine(_listing[0].line, out, true);

            Filesystem::combine(listing, _listing, true);
        }
        delete fs;
    }

    return listing;
}

auto HardDiskStructure::readFileDriver(FileDriver& fileDriver, std::vector<uint8_t>& code) -> void {
    typedef Emulator::Error::Type EType;

    if ((geometry.bSize <= 20) || !fileDriver.segList.size())
        throw Emulator::Error(EType::DRIVER_NO_SEGLIST);

    auto bytesPerBlock = geometry.bSize - 20;
    unsigned codeSize = fileDriver.segList.size() * bytesPerBlock;

    unsigned codeOffset = 0;
    code.resize(codeSize);

    for (auto& seg : fileDriver.segList) {
        auto offset = seg * geometry.bSize + 20;

        if (this->data) {
            if ((uint64_t)offset + (uint64_t)bytesPerBlock <= size)
                std::memcpy(code.data() + codeOffset, data + offset, bytesPerBlock);
            else
                throw Emulator::Error(EType::HDD_BAD_FILE_OFFSET, offset);

        }
        else if (agnus.interface->readMedia(media, code.data() + codeOffset, bytesPerBlock, offset) != bytesPerBlock)
            throw Emulator::Error(EType::HDD_BAD_FILE_OFFSET, offset);

        codeOffset += bytesPerBlock;
    }
}

auto HardDiskStructure::buildHdfFromBinaries(const std::string& name, std::vector<Emulator::Interface::Item>& files) -> Emulator::Interface::Data {
    unsigned rawSize;
    uint8_t* rawData;
    std::string test;

    if (!files.size())
        return { nullptr, 0 };

    std::function<bool(Filesystem& fs, Emulator::Interface::Item&)>
    addItem = [&](Filesystem& fs, Emulator::Interface::Item& item) -> bool {
        if (item.isGroup) {
            if (!fs.createDir(item.name))
                return false;

            if (!fs.changeDir(item.name))
                return false;

            for (auto child : item.childs) {
                if (!addItem(fs, *child))
                    return false;
            }

            if (!fs.changeDir(".."))
                return false;
        } else {
            if (!fs.createFile(item.name, item.data.ptr, item.data.size))
                return false;
        }

        return true;
    };

    unsigned _size = 1 * 1024 * 1024;
    for (auto& item : files) {
        if (!item.isGroup)
            _size += item.data.size;
    }

    _size += 1024 * 1024 - 1;
    _size = _size / (1024 * 1024);
    _size *= 1024 * 1024;

    auto crc32 = Emulator::CRC32(agnus.kickRom, agnus.kickRomSize, ~0).value();

    bool ofs = crc32 == Firmware::CRC32::KICK_07_27003_BETA || crc32 == Firmware::CRC32::KICK_10_30_NTSC || crc32 == Firmware::CRC32::KICK_11_31034_NTSC
        || crc32 == Firmware::CRC32::KICK_11_31034_PAL || crc32 == Firmware::CRC32::KICK_12_33166 || crc32 == Firmware::CRC32::KICK_12_33180
        || crc32 == Firmware::CRC32::KICK_12_33180_GUARDIAN_HACK || crc32 == Firmware::CRC32::KICK_12_33180_MRAS_HACK || crc32 == Firmware::CRC32::KICK_121_34004
        || crc32 == Firmware::CRC32::KICK_13_34005_A500 || crc32 == Firmware::CRC32::KICK_13_34005_A3000 || crc32 == Firmware::CRC32::KICK_13_34005_GUARDIAN_HACK;

    Filesystem fs(_size, ofs ? Filesystem::Structure::OFS : Filesystem::Structure::FFS);
    fs.format(!name.empty() ? name : "Volume", true);

    for (auto& item : files) {
        if (item.parent)
            continue;

        if (!addItem(fs, item))
            return { nullptr, 0 };
    }

    fs.calculateChecksums();

    rawSize = fs.volSize();
    rawData = new uint8_t[rawSize];

    if (!fs.exportMedia(rawData, rawSize)) {
        delete[] rawData;
    } else
        return { rawData, rawSize };

    return { nullptr, 0 };
}

auto HardDiskStructure::buildHardDisk(System* system, const std::string& name, std::vector<Emulator::Interface::Item>& files) -> Emulator::Interface::Data {
    HardDiskStructure hardDisk(system->agnus, nullptr);
    return hardDisk.buildHdfFromBinaries(name, files);
}

}
