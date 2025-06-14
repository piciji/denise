#include <cstring>
#include "hardDrive.h"
#include "../../tools/buffer.h"
#include "../system/system.h"
#include "../../tools/serializer.h"
#include "filesystemExt.h"
#include "../interface.h"
#include "../../tools/error.h"

#define IDE_MAX_MULTIPLE 128

namespace LIBAMI {

HardDrive::HardDrive(uint8_t number, System* system, Agnus& agnus)
    : number(number), system(system), agnus(agnus), structure(agnus, nullptr) {

    interface = system->interface;
    media = &interface->mediaGroups[Interface::MediaGroupIdHardDisk].media[number];
    structure.media = media;

    connected = false;
    buffer = nullptr;
    bufferSize = 0;
    writeProtected = false;
}

HardDrive::~HardDrive() {
    if (buffer)
        delete[] buffer;
}

auto HardDrive::attach(uint8_t* data, uint64_t size) -> bool {
    detach();

    if (!connected)
        return false;

    if (!structure.attach(data, size))
        return false;

    setBuffer( 256 * geometry().bSize ); // max for non builin devcies

    setUsableFileDrivers();

    status = (uint8_t)State::DRDY | (uint8_t)State::DSC;

    return true;
}

auto HardDrive::detach() -> void {
    structure.detach();
    fileDriversInUse.clear();
}

auto HardDrive::setBuffer(unsigned newSize) -> void {    
    if (!buffer || (newSize > bufferSize)) {
        bufferSize = newSize;
        if (buffer)
            delete[] buffer;

        buffer = new uint8_t[bufferSize];
    }
}

auto HardDrive::setBusy(uint8_t delay, bool finishWithDataRequet) -> void {
    status |= (uint8_t)State::BSY;
    processTimer = delay;
    raiseDRQ = finishWithDataRequet;
    agnus.hardDrivesBusy = true;
}

auto HardDrive::power(bool softReset) -> void {
    processTimer = 0;
    error = 1;
    lba.sector = 1;
    lba.cylLo = 0;
    lba.cylHi = 0;
    lba.devHead = 0;
    sectorCount = 1;
    raiseDRQ = false;
    multiple = 0;
    writeOffset = 0;
    transferOffset = 0;
    transferSize = 0;
    refill.sectors = refill.pos = 0;

    if (!connected || !softReset)
        return;

    structure.reset();
    setUsableFileDrivers();
}

auto HardDrive::setUsableFileDrivers() -> void {
    fileDriversInUse.clear();

    for (auto& driver : structure.fileDrivers) {
        for (auto& partition : structure.partitions) {
            if (partition.dosType == driver.dosType) {
                fileDriversInUse.push_back(driver);
                break;
            }
        }
    }
}

auto HardDrive::getCopiedFileDriverMemOffset(uint32_t dosType) -> uint32_t {
    for (auto& fileDriver : fileDriversInUse) {
        if (fileDriver.dosType == dosType) {
            return fileDriver.seglistBptr;
        }
    }
    return 0;
}

auto HardDrive::writeProtect(bool state) -> void {
    writeProtected = state;
}

auto HardDrive::process() -> bool {
    if (processTimer) {
        if (--processTimer == 0) {
            if (raiseDRQ)
                status |= (uint8_t)State::DRQ;
            status &= ~(uint8_t)State::BSY;
        } else
            return true; // in process
    }
    return false;
}


auto HardDrive::readReg(uint8_t reg, uint8_t cs) -> uint16_t {
    // todo Chip Select (when needed) for ALT status, device control
    if (isBusy())
        return status;

    switch ((Register)reg) {
        case Register::Data:
            return fetchData();
        case Register::Error:
            return error;
        case Register::SectorCount:
            return sectorCount;
        case Register::SectorNumber:
            return lba.sector;
        case Register::CylinderLow:
            return lba.cylLo;
        case Register::CylinderHigh:
            return lba.cylHi;
        case Register::DevHead:
            return lba.devHead;
        default:
        case Register::Status:        
            return status;
    }
    _unreachable
}

auto HardDrive::writeReg(uint8_t reg, uint16_t data, uint8_t cs) -> void {
    // todo Chip Select (when needed) for ALT status, device control
    switch ((Register)reg) {
        case Register::Data:
            writeData(data);
            break;
        case Register::Features:
            warn("IDE: unsupported feature request %i", data);
            break;
        case Register::SectorCount:
            sectorCount = (uint8_t)data;
            break;
        case Register::SectorNumber:
            lba.sector = (uint8_t)data;
            break;
        case Register::CylinderLow:
            lba.cylLo = (uint8_t)data;
            break;
        case Register::CylinderHigh:
            lba.cylHi = (uint8_t)data;
            break;
        case Register::DevHead:
            lba.devHead = (uint8_t)data;
            // todo: access second IDE drive (Master/Slave) on same controller (M-Tec AT500 don't support this)
            //if (lba.devHead & 0x10)
              //  warn("Slave is accessed");

            break;
        case Register::Command:
            setCommand((uint8_t)data);
            break;
    }
}

auto HardDrive::setCommand(uint8_t command) -> void {    
    status &= ~((uint8_t)State::DRQ | (uint8_t)State::ERR);
    error = 0;    
    transferOffset = 0;

    switch ((Command)command) {
        case Command::ReadSector:
        case Command::ReadSectorRetry:
            readSector();
            break;
        case Command::ReadVerifySectorRetry:
        case Command::ReadVerifySector:
            readSector(true);
            break;
        case Command::ReadSectorMultiple:
            readSector(false, true);
            break;
        case Command::WriteSector:
        case Command::WriteSectorRetry:
        case Command::WriteVerify:
            writeSector();
            break;
        case Command::WriteSectorMultiple:
            writeSector(true);
            break;
        case Command::SetMultiple:
            setMultiple();
            break;
        default:
            warn("IDE: unsupported command %x", command);
            // fallthrough
        case Command::Nop:
            fail();
            break;
        case Command::Identify:
            identify();
            break;
    }
}

auto HardDrive::setMultiple() -> void {
    if (sectorCount > (IDE_MAX_MULTIPLE >> (geometry().bSize / 512 - 1)))
        fail();
    else
        multiple = sectorCount;
}

auto HardDrive::writeSector(bool useMultiple) -> void {
    if (useMultiple && !multiple)
        return fail();

    int _gap = (int)maxLba() - (int)getLba();

    if (_gap <= 0) {
        status |= (uint8_t)State::ERR;
        error |= (uint8_t)Error::Idnf;
        setBusy(2, false);
        return;
    }

    int _secToTransfer = getSectorsToTransfer();
    if (_secToTransfer > _gap)
        _secToTransfer = _gap;

    writeOffset = getLba() * geometry().bSize;
    transferSize = _secToTransfer * geometry().bSize;
    refill.sectors = useMultiple ? multiple : 1;
    refill.pos = 0;

    status |= (uint8_t)State::DRQ;
    status &= ~(uint8_t)State::BSY;

    agnus.interface->updateDeviceState(media, true, getPositonForUI(), 0x80 | 1, false);
}

auto HardDrive::writeData(uint16_t data) -> void {
    if (!transferSize)
        return;

    buffer[transferOffset + 1] = data & 0xff;
    buffer[transferOffset] = data >> 8;

    transferOffset += 2;
    refill.pos += 2;

    if (transferOffset == transferSize) {
        status &= ~(uint8_t)State::DRQ;
        setBusy(1, false);

        unsigned lba = getLba();

        if (!writeProtected) {
            if (!structure.write(buffer, (uint64_t)writeOffset, transferSize)) {
                status |= (uint8_t)State::ERR;
                error |= (uint8_t)Error::Idnf;
            }
        }

        transferSize = 0;
        agnus.interface->updateDeviceState(media, true, getPositonForUI(), 0x80, false);

    } else if (refill.pos == (refill.sectors * geometry().bSize)) {
        // This is where the write operation takes place
        // For performance reasons, everything is written in one operation, just like reading.
        // simulate the write only
        refill.pos = 0;
        unsigned secsTodo = (transferSize - transferOffset) / geometry().bSize;
        sectorCount -= secsTodo > refill.sectors ? refill.sectors : secsTodo;
        setBusy(2, true);
    }
}

auto HardDrive::readSector(bool verify, bool useMultiple) -> void {
    unsigned lba = getLba();

    if (useMultiple && !multiple)
        return fail();

    int _gap = (int)maxLba() - (int)lba;

    if (_gap <= 0) {
        status |= (uint8_t)State::ERR;
        error |= (uint8_t)Error::Idnf;
        setBusy(2, false);
        return;
    }

    if (verify) {
        setBusy(2, false);
        return;
    }

    int _secToTransfer = getSectorsToTransfer();

    if (_secToTransfer > _gap)
        _secToTransfer = _gap;

    transferSize = _secToTransfer * geometry().bSize;
    refill.sectors = useMultiple ? multiple : 1;
    refill.pos = 0;
    
    sectorCount -= _secToTransfer > refill.sectors ? refill.sectors : _secToTransfer;

    if (!structure.read(buffer, (uint64_t)lba * (uint64_t)geometry().bSize, transferSize))
        error |= (uint8_t)Error::Idnf;

    if (error & (uint8_t)Error::Idnf) {
        status |= (uint8_t)State::ERR;
        setBusy(2, false);
        return;
    }

    setBusy(1, true);

    uint8_t LED = system->getModel() > 1 ? 1 : 2;

    agnus.interface->updateDeviceState(media, false, getPositonForUI(), 0x80 | LED, false);
}

auto HardDrive::fetchData() -> uint16_t {
    if (!transferSize)
        return 0;

    uint16_t data = buffer[transferOffset + 1] | (buffer[transferOffset] << 8);
    transferOffset += 2;
    refill.pos += 2;

    if (transferOffset == transferSize) {
        status &= ~(uint8_t)State::DRQ;
        transferSize = 0;
        agnus.interface->updateDeviceState(media, false, getPositonForUI(), 0x80, false);

    } else if (refill.pos == (refill.sectors * geometry().bSize) ) {
        // read next chunk from drive to buffer.
        // for performance reasons, everything is readed in one operation.
        refill.pos = 0;
        unsigned secsTodo = (transferSize - transferOffset) / geometry().bSize;
        sectorCount -= secsTodo > refill.sectors ? refill.sectors : secsTodo;
        setBusy(2, true);
    }

    return data;
}

auto HardDrive::fail() -> void {
    status |= (uint8_t)State::ERR;
    error |= (uint8_t)Error::Abrt;
    setBusy(2, false);
}

auto HardDrive::getLba() -> unsigned {
    if (lba.devHead & 0x40) // LBA
        return ((lba.devHead & 15) << 24) | lba.cylHi << 16 | lba.cylLo << 8 | lba.sector;

    // CHS
    unsigned cyl = (lba.cylHi << 8) | lba.cylLo;
    return ((cyl * geometry().heads + (lba.devHead & 15)) * geometry().sectors) + (lba.sector - 1);
}

auto HardDrive::maxLba() -> unsigned {
    return (structure.vhd.inUse ? structure.vhd.size : structure.size) / (uint64_t)geometry().bSize;
}

auto HardDrive::identify() -> void {
    transferSize = 512;
    setBusy(2, true);
    std::memset(buffer, 0, transferSize);

    auto addText = [](uint8_t* buf, const std::string& text, unsigned words) -> void {
        auto _l = text.length();
        int i = 0;

        while (words--) {
            *buf++ = (i + 1 < _l) ? text[i + 1] : ' ';
            *buf++ = (i < _l) ? text[i] : ' ';
            i += 2;
        }
    };

    // based on ATA-2 specs
    FromU16LE(buffer, 0x40);
    FromU16LE(buffer + 2, geometry().cylinders);
    FromU16LE(buffer + 4, 0); // ATA reserved
    FromU16LE(buffer + 6, geometry().heads);
    FromU16LE(buffer + 8, geometry().bSize * geometry().sectors);
    FromU16LE(buffer + 10, geometry().bSize);
    FromU16LE(buffer + 12, geometry().sectors);
    // vendor unique 3 words    
    addText(buffer + 20, "DEN-E26", 10); // serial number 10 words
    FromU16LE(buffer + 40, 3);
    FromU16LE(buffer + 42, IDE_MAX_MULTIPLE); // 128 * 512 = 64kb buffer
    FromU16LE(buffer + 44, 4);
    addText(buffer + 46, "REV 1.0", 4);
    addText(buffer + 54, "DENISE-IDE " + std::to_string(number), 20);
    uint8_t multiple = IDE_MAX_MULTIPLE / (geometry().bSize / 512);
    FromU16LE(buffer + 94, 0xc000 | multiple);
    FromU16LE(buffer + 96, 1); // can perform double word IO
    FromU16LE(buffer + 98, 1 << 9); // LBA supported
    FromU16LE(buffer + 100, 0); // reserved
    FromU16LE(buffer + 102, 0x200); // mode 2: fastest
    FromU16LE(buffer + 104, 0x200); // mode 2: fastest

    FromU16LE(buffer + 106, 1); // means: next four entries are valid
    FromU16LE(buffer + 108, geometry().cylinders);
    FromU16LE(buffer + 110, geometry().heads);
    FromU16LE(buffer + 112, geometry().sectors);
    FromU32LE(buffer + 114, geometry().sectors * geometry().cylinders * geometry().heads);
    FromU16LE(buffer + 118, 0x100 | multiple);
    FromU32LE(buffer + 120, (structure.vhd.inUse ? structure.vhd.size : structure.size) / (uint64_t)geometry().bSize);
    FromU16LE(buffer + 124, 7); // mode 0 - 2
    FromU16LE(buffer + 126, 7); // mode 0 - 2
}

// read/write for virtual builtin controller
auto HardDrive::read(unsigned offset, unsigned length) -> uint8_t* {
    setBuffer(length);

    convertCHS(offset / geometry().bSize);
    uint8_t LED = system->getModel() > 1 ? 1 : 2;
    agnus.interface->updateDeviceState(media, false, getPositonForUI(), 0x80 | 0x40 | LED, false);

    if (structure.read(buffer, offset, length))
        return buffer;   

    return nullptr;
}

auto HardDrive::write(unsigned offset, unsigned length) -> bool {
    if (writeProtected)
        return false;

    convertCHS(offset / geometry().bSize);
    agnus.interface->updateDeviceState(media, true, getPositonForUI(), 0x80 | 0x40 | 1, false);

    if (structure.write(buffer, offset, length))
        return true;

    return false;
}

inline auto HardDrive::getPositonForUI() -> unsigned {
    return (lba.cylHi << 8) | lba.cylLo;
}

auto HardDrive::convertCHS(unsigned _lba) -> void {
    unsigned _cyl = _lba / (geometry().heads * geometry().sectors);

    lba.devHead = (_lba / geometry().sectors) % geometry().heads;
    lba.cylHi = (_cyl >> 8) & 0xff;
    lba.cylLo = _cyl & 0xff;
    lba.sector = (_lba % geometry().sectors) + 1;
}

auto HardDrive::getListing() -> std::vector<Emulator::Interface::Listing> {
    return structure.getListing();
}

auto HardDrive::getPreview(System* system, Emulator::Interface::Media* media, uint8_t* data, uint64_t size) -> std::vector<Emulator::Interface::Listing> {
    HardDiskStructure structure(system->agnus, media);

    if (!structure.attach(data, size))
        return {};

    return structure.getListing();
}

auto HardDrive::serialize(Emulator::Serializer& s, bool light, bool builtIn) -> void {
    s.integer(connected);
    if (!connected && light)
        return;

    s.integer(writeProtected);

    // builtin devices read/write all data immediately
    
    if (!builtIn) {
        s.integer(transferSize);

        if (s.mode() == Emulator::Serializer::Mode::Load)
            setBuffer(transferSize);        

        if (s.mode() == Emulator::Serializer::Mode::Size)
            s.array(buffer, 256 * geometry().bSize); // max for non builtin
        else if (transferSize)
            s.array(buffer, transferSize);

        s.integer(processTimer);
        s.integer(raiseDRQ);

        s.integer(status);
        s.integer(error);
        s.integer(transferOffset);
        s.integer(writeOffset);

        s.integer(sectorCount);
        s.integer(multiple);

        s.integer(refill.sectors);
        s.integer(refill.pos);

        s.integer(lba.sector);
        s.integer(lba.devHead);
    }

    s.integer(lba.cylLo);
    s.integer(lba.cylHi);
}

}
