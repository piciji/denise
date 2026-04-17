
#include "hardDiskStructure.h"
#include "../system/system.h"
#include "../agnus/agnus.h"
#include "filesystemExt.h"
#include "../../tools/buffer.h"
#include "../../tools/error.h"
#include "../../tools/crc32.h"
#include "../../tools/uuid.h"
#include "../system/firmware.h"
#include <cmath>
#include <ctime>

namespace LIBAMI {

HardDiskStructure::HardDiskStructure(Agnus& agnus, Emulator::Interface::Media* media)
: agnus(agnus), media(media) {
    size = 0;
    data = nullptr;
    hasRDB = false;
    vhd.inUse = false;
    vhd.header = nullptr;
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
    detectVHD();
    detectGeometry();
    detectPartitions();
    detectFileDrivers();
}

auto HardDiskStructure::detectVHD() -> void {
    uint8_t header[512];
    uint8_t footer[512];

    vhd.inUse = false;

    if (!readDirect(header, 0, 512))
        return;

    uint32_t val = ToU32BE(header + 8);
    if ((val & 3) != 2)
        return;

    val = ToU32BE(header + 0xc);
    if ((val >> 16) != 1)
        return;

    val = ToU32BE(header + 0x3c);
    if (val != 2 && val != 3) // allow fixed or dynamic
        return;

    vhd.dynamic = val == 3;

    val = ToU32BE(header + 0x40);
    if (!val) // checksum
        return;

    if (getVHDCrc(header, 0x40) != val)
        return;

    if (!readDirect(footer, size - 512, 512))
        return;
    
    if (std::memcmp(footer, header, 512)) // footer and header are redundant
        return;

    vhd.size = (uint64_t)(ToU32BE(header + 0x30)) << 32;
    vhd.size |= (uint64_t)(ToU32BE(header + 0x34));

    if (vhd.dynamic) {
        vhd.bamOffset = ToU32BE(header + 0x14); // offset to dynamic disk header

        if (!vhd.bamOffset || (vhd.bamOffset >= size))
            return;

        if (!readDirect(header, vhd.bamOffset, 512))
            return;

        val = ToU32BE(header + 0x24);

        if (getVHDCrc(header, 0x24) != val)
            return;

        val = ToU32BE(header + 0x18);
        if ((val >> 16) != 1) // check version
            return;

        vhd.bamOffset = ToU32BE(header + 0x14); // offset to BAM, 512 + 1024
        vhd.blockSize = ToU32BE(header + 0x20);        
        vhd.bamSize = ((uint32_t)((vhd.size + (uint64_t)vhd.blockSize - 1) / (uint64_t)vhd.blockSize) * 4 + 511) & ~511;
        // one bit for each 512 byte sector, a block size up to 2 MB fits in one 512 byte bitmap sector
        vhd.bitmapSize = ((vhd.blockSize / (8 * 512)) + 511) & ~511; // align to 512 byte sector boundary

        vhd.header = new uint8_t[vhd.bamOffset + vhd.bamSize];

        if (!readDirect(vhd.header, 0, vhd.bamOffset + vhd.bamSize))
            return;

        vhd.bitmapSectorOffset = 0;
    }

    vhd.inUse = true;
}

auto HardDiskStructure::detach() -> void {
    partitions.clear();
    fileDrivers.clear();
    data = nullptr;
    size = 0;
    vhd.inUse = false;

    if (vhd.header) {
        delete[] vhd.header;
        vhd.header = nullptr;
    }    
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
    if (read(blockBuffer, 512ull * (uint64_t)ref, 512))
        return &blockBuffer[0];

    return nullptr;
}

inline auto HardDiskStructure::writeDirect(uint8_t* buffer, uint64_t offset, unsigned length) -> bool {
    if ((offset + (uint64_t)length) > size)
        return false;

    if (data) {
        std::memcpy(data + (uint32_t)offset, buffer, length);
        return true;

    } else if (agnus.interface->writeMedia(media, buffer, length, offset) == length)
        return true;

    return false;
}

auto HardDiskStructure::write(uint8_t* buffer, uint64_t offset, unsigned length) -> bool {
    if (!vhd.inUse)
        return writeDirect(buffer, offset, length);

    if (!vhd.dynamic)
        return writeDirect(buffer, offset + 512ull, length);

    // dynamic VHD    
    if (offset & 511)
        return false;

    if (length & 511)
        return false;

    uint8_t* ptr = buffer;

    while (length) {
        uint32_t bamOffset = (uint32_t)(offset / (uint64_t)vhd.blockSize) * 4 + vhd.bamOffset;
        uint32_t blockOffset = ToU32BE(vhd.header + bamOffset);

        if (blockOffset == 0xffffffff) {
            if (!expandVHD(bamOffset))
                return false;
            
            continue; // now try again this sector

        } else {
            unsigned sectorInBlock = (uint32_t)(offset / 512ull) % (vhd.blockSize / 512);
            unsigned bitmapPos = sectorInBlock / 8; // one bit each sector

            uint64_t bitmapSectorOffset = (uint64_t)blockOffset * 512ull + (uint64_t)(bitmapPos & ~511);

            if (vhd.bitmapSectorOffset != bitmapSectorOffset) {
                if (!readDirect(vhd.bitmapSector, bitmapSectorOffset, 512))
                    return false;

                vhd.bitmapSectorOffset = bitmapSectorOffset;
            }

            uint64_t sectorOffset = (uint64_t)blockOffset * 512ull + (uint64_t)vhd.bitmapSize + (uint64_t)sectorInBlock * 512ull;

            if (!writeDirect(ptr, sectorOffset, 512))
                return false;

            if (!(vhd.bitmapSector[bitmapPos & 511] & (1 << (7 - (sectorInBlock & 7))))) { // check if allocated
                vhd.bitmapSector[bitmapPos & 511] |= (1 << (7 - (sectorInBlock & 7)));

                if (!writeDirect(vhd.bitmapSector, bitmapSectorOffset, 512))
                    return false;
            }
        }

        length -= 512;
        offset += 512ull;
        ptr += 512;
    }

    return true;
}

auto HardDiskStructure::expandVHD(unsigned bamOffset) -> bool {
    if (this->data) // compressed VHD will not be expanded
        return false;

    unsigned length = vhd.blockSize + vhd.bitmapSize + 512;
    uint8_t* buffer = new uint8_t[length];
    std::memset(buffer, 0, length - 512);
    std::memcpy(buffer + length - 512, vhd.header, 512);

    uint64_t footerOffset = size - 512ull;
    size += (uint64_t)(length - 512);

    bool result = writeDirect(buffer, footerOffset, length);
    delete[] buffer;

    if (!result) {
        size = footerOffset + 512ull;
        return false;
    }

    uint8_t* ptr = vhd.header + bamOffset;
    uint32_t blockOffset = (uint32_t)(footerOffset / 512ull);

    FromU32BE(ptr, blockOffset);

    return writeDirect(vhd.header + vhd.bamOffset, vhd.bamOffset, vhd.bamSize);
}

inline auto HardDiskStructure::readDirect(uint8_t* buffer, uint64_t offset, unsigned length) -> bool {
    if ((offset + (uint64_t)length) > size)
        return false;

    if (data) {
        std::memcpy(buffer, data + (uint32_t)offset, length);
        return true;
                  
    } else if (agnus.interface->readMedia(media, buffer, length, offset) == length)
        return true;

    return false;
}

auto HardDiskStructure::read(uint8_t* buffer, uint64_t offset, unsigned length) -> bool {
    if (!vhd.inUse)
        return readDirect(buffer, offset, length);

    if (!vhd.dynamic)
        return readDirect(buffer, offset + 512ull, length);

    // dynamic VHD    
    if (offset & 511)
        return false;

    if (length & 511)
        return false;

    uint8_t* ptr = buffer;

    while(length) {
        uint32_t bamOffset = (uint32_t)(offset / (uint64_t)vhd.blockSize) * 4 + vhd.bamOffset;
        uint32_t blockOffset = ToU32BE(vhd.header + bamOffset);

        if (blockOffset == 0xffffffff) {
            std::memset(ptr, 0, 512);

        } else {
            unsigned sectorInBlock = (uint32_t)(offset / 512ull) % (vhd.blockSize / 512);
            unsigned bitmapPos = sectorInBlock / 8; // one bit each sector

            uint64_t bitmapSectorOffset = (uint64_t)blockOffset * 512ull + (uint64_t)(bitmapPos & ~511);

            if (vhd.bitmapSectorOffset != bitmapSectorOffset) {                
                 if (!readDirect(vhd.bitmapSector, bitmapSectorOffset, 512))
                     return false;

                 vhd.bitmapSectorOffset = bitmapSectorOffset;
            }
            
            if (vhd.bitmapSector[bitmapPos & 511] & (1 << (7 - (sectorInBlock & 7)))) { // check if allocated
                uint64_t sectorOffset = (uint64_t)blockOffset * 512ull + (uint64_t)vhd.bitmapSize + (uint64_t)sectorInBlock * 512ull;

                if (!readDirect(ptr, sectorOffset, 512))
                    return false;

            } else
                std::memset(ptr, 0, 512);
        }

        length -= 512;
        offset += 512ull;
        ptr += 512;
    }

    return true;
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

    if (vhd.inUse) {
        uint8_t header[512];

        if (readDirect(header, 0, 512)) {
            geometry.cylinders = header[0x38] << 8;
            geometry.cylinders |= header[0x39];
            geometry.heads = header[0x3a];
            geometry.sectors = header[0x3b];

            if (geometry.heads && geometry.cylinders && geometry.sectors)
                return;
        }
    }

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
        partition.offset = (uint64_t)partition.cylLo * (uint64_t)partition.heads * (uint64_t)partition.sectors * (uint64_t)partition.bSize;

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

    if (hasRDB)
        listing.push_back({ {'R','D','B'} });

    for (auto& partition : partitions) {
        std::vector<Emulator::Interface::Listing> _listing;

        if (((partition.dosType >> 8) == 0x504453) || ((partition.dosType >> 8) == 0x504653)) { // PFS or PDS
            uint8_t buf[32];
            static std::vector<uint16_t> _preLabel = {'L','a','b','e','l',':',' '};
            if (read(buf, partition.offset + (uint64_t)partition.bSize * (uint64_t)2 + (uint64_t)20, 32)) {
                _listing.push_back( {} );
                for (unsigned i = 0; i < std::min((unsigned)buf[0], 32u); i++)
                    _listing[0].line.push_back( buf[i + 1] );
                Filesystem::combine(_listing[0].line, _preLabel);
            }
        } else {
            FilesystemExt fsExt(partition.size, (Filesystem::Structure)partition.dosType, partition.bSize);
            fsExt.setDataSource(this, partition.offset);
            _listing = fsExt.getDirectory(true);
        }
         
        std::vector<uint16_t> out;
        std::string _name = partition.name;
        _name = " - " + _name;
        out.resize(_name.size());
        for (unsigned i = 0; i < _name.size(); i++)
            out[i] = _name[i];

        if (_listing.size()) {
            if (!listing.empty())
                listing.push_back({  });

            Filesystem::combine(_listing[0].line, out, true);

            Filesystem::combine(listing, _listing, true);
        } else {
            out.erase( out.begin(), out.begin() + 3 );
            listing.push_back({ out, {} });
        }
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
    uint8_t* buffer = new uint8_t[geometry.bSize];

    for (auto& seg : fileDriver.segList) {
        auto offset = seg * geometry.bSize;

        if (!read(buffer, offset, geometry.bSize)) {
            delete[] buffer;
            throw Emulator::Error(EType::HDD_BAD_FILE_OFFSET, offset);
        }

        std::memcpy(code.data() + codeOffset, buffer + 20, bytesPerBlock);

        codeOffset += bytesPerBlock;
    }

    delete[] buffer;
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

auto HardDiskStructure::create(System* system, uint64_t _size, bool vhd) -> Emulator::Interface::Data {
    if (!vhd)
        return {nullptr, 0}; // let UI create a header less HDF

    // create dynamic VHD
    HardDiskStructure hs(system->agnus, nullptr);
    hs.size = _size;

    uint32_t blockSize = 512 * 1024;
    uint32_t batSize = (uint32_t)((_size + (uint64_t)blockSize - 1) / (uint64_t)blockSize);

    uint32_t maxEntries = batSize;
    batSize *= 4;
    // BAT is always extended to sector boundary
    batSize += 511;
    batSize &= ~511;

    unsigned bufSize = 512 + 1024 + batSize + 512;
    uint8_t* buffer = new uint8_t[bufSize];
    std::memset(buffer, 0, bufSize);

    std::memcpy(buffer, "conectix", 8);
    buffer[0x0b] = 2; // features
    buffer[0x0d] = 1; // version 0x10000
    buffer[0x16] = 2; // data offset 0x200 (512) -> points to header for dynamic disks

    auto tm = std::time(nullptr) - 946684800; // 1.1.2000 12:00:00 - 1.1.1970 00:00:00
    FromU32BE(buffer + 0x18, tm);

    std::memcpy(buffer + 0x1c, "vpc ", 4); // creator application

    buffer[0x21] = 5; // creator version (Virtual PC 2004)
    std::memcpy(buffer + 0x24, "Wi2k", 4); // creator host os
    // original and current size
    buffer[0x28] = buffer[0x30] = (uint8_t)(_size >> 56);
    buffer[0x29] = buffer[0x31] = (uint8_t)(_size >> 48);
    buffer[0x2a] = buffer[0x32] = (uint8_t)(_size >> 40);
    buffer[0x2b] = buffer[0x33] = (uint8_t)(_size >> 32);
    buffer[0x2c] = buffer[0x34] = (uint8_t)(_size >> 24);
    buffer[0x2d] = buffer[0x35] = (uint8_t)(_size >> 16);
    buffer[0x2e] = buffer[0x36] = (uint8_t)(_size >> 8);
    buffer[0x2f] = buffer[0x37] = (uint8_t)(_size >> 0);

    hs.predictGeometrie();

    buffer[0x38] = hs.geometry.cylinders >> 8;
    buffer[0x39] = hs.geometry.cylinders;
    buffer[0x3a] = hs.geometry.heads;
    buffer[0x3b] = hs.geometry.sectors;
    buffer[0x3f] = 3; // dynamic VHD

    // GUID needed for "differencing hard disks" to find out parent <> child relationship
    Emulator::UUID uuid;
    auto guid = uuid.get();
    std::memcpy(buffer + 0x44, (uint8_t*)guid.data(), guid.size());

    auto crc = getVHDCrc(buffer);
    FromU32BE(buffer + 0x40, crc);

    // 512 byte footer and header are the same
    std::memcpy(buffer + 512 + 1024 + batSize, buffer, 512);

    // header for dynamic VHD
    uint8_t* ptr = buffer + 512;
    std::memcpy(ptr, "cxsparse", 8);
    std::memset(ptr + 8, 0xff, 8); // unused

    ptr[0x16] = 6; // 0x600 header + sparse
    ptr[0x19] = 1; // version 0x1000

    FromU32BE(ptr + 0x1c, maxEntries);
    FromU32BE(ptr + 0x20, blockSize);

    crc = getVHDCrc(ptr);
    FromU32BE(ptr + 0x24, crc);

    std::memset(buffer + 512 + 1024, 0xff, maxEntries * 4);

    return { buffer, bufSize };
}

auto HardDiskStructure::getVHDCrc(uint8_t* buf, unsigned crcPosToExclude) -> uint32_t {
    uint32_t sum = 0;

    for (int i = 0; i < 512; i++) {
        if ( (crcPosToExclude != ~0) && (i >= crcPosToExclude && i < crcPosToExclude + 4))
            continue;

        sum += buf[i];
    }

    return ~sum;
}

}
