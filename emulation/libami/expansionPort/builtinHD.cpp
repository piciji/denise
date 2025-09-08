
#include "builtinHD.h"
#include "../agnus/agnus.h"
#include "../system/system.h"
#include "../system/firmware.h"
#include "../../tools/buffer.h"
#include "../../tools/error.h"
#include "../../tools/crc32.h"

#pragma warning(disable:4996)
namespace LIBAMI {

BuiltinHD::~BuiltinHD() {
    if (rom)
        delete[] rom;
}

auto BuiltinHD::reset() -> void {
    bool asyncAccessUse = romSize == sizeof(Firmware::mras0RomAsyc);

    if (!rom || (agnus.system->asyncHDDAccess != asyncAccessUse)) {
        if (rom)
            delete[] rom;

        if (agnus.system->asyncHDDAccess) {
            romSize = sizeof(Firmware::mras0RomAsyc);
            rom = new uint8_t[romSize];
            std::memcpy(rom, Firmware::mras0RomAsyc, romSize);
        } else {
            romSize = sizeof(Firmware::mras0Rom);
            rom = new uint8_t[romSize];
            std::memcpy(rom, Firmware::mras0Rom, romSize);
        }
    }

    const std::string deviceName = "denise" + std::to_string(hardDrive.number) + "hf.device";

    while (Emulator::replaceInBuffer(rom, romSize, "virtualhd.device", deviceName)) {}

    if (hardDrive.number == 0)
        fixKick12();

    memPtr = 0;
}

auto BuiltinHD::fixKick12() -> void {
    auto crc32 = Emulator::CRC32(agnus.kickRom, agnus.kickRomSize, ~0).value();

    if (crc32 == Firmware::CRC32::KICK_12_33166) {
        FromU32BE(agnus.kickRom + 19840, 0x426f0004);
        FromU16BE(agnus.kickRom + 19840 + 22, 0);

    } else if (crc32 == Firmware::CRC32::KICK_12_33180) {
        FromU32BE(agnus.kickRom + 19868, 0x426f0004);
        FromU16BE(agnus.kickRom + 19868 + 22, 0);
    }
}

auto BuiltinHD::readAutoConf(uint32_t addr)->uint8_t {
    addr &= 0xffff;
    if (addr & 1)
        return 0xff;

    switch (addr) {
        // type and size
        case 0:
        case 2: {
            uint8_t res = type() | 1;
            return addr == 0 ? res & 0xf0 : res << 4;
        }
        // product
        case 4: return board.encNibble(5); // hi
        case 6: return board.encNibble(2); // lo
        // flags
        case 8:
        case 0xa: return board.encNibble(0);
        // reserved
        case 0xc:
        case 0xe: return board.encNibble(0);
        // manufacturer Hi byte
        case 0x10: return board.encNibble(0);
        case 0x12: return board.encNibble(5);
        // manufacturer Lo byte
        case 0x14: return board.encNibble(0);
        case 0x16: return board.encNibble(0);
        // serial number 4 bytes
        case 0x18: case 0x1a:
        case 0x1c: case 0x1e:
        case 0x20: case 0x22:
        case 0x24:
            return board.encNibble(3);
        case 0x26:
            return board.encNibble(2 + hardDrive.number);
        // rom vector 2 byte
        case 0x28: return board.encNibble(0);
        case 0x2a: return board.encNibble(0);
        case 0x2c: return board.encNibble(4);
        case 0x2e: return board.encNibble(0);
        // reserved area
        // Interrupt support
        case 0x40:
        case 0x42:
            return 0; // not inverted
    }

    return 0xff;
}

auto BuiltinHD::read(uint32_t addr) -> uint8_t {
    unsigned offset = (addr & 0xffff) - 0x40;
    return offset < romSize ? rom[offset] : 0;
}

auto BuiltinHD::readW(uint32_t addr) -> uint16_t {
    unsigned offset = (addr & 0xffff) - 0x40;

    if (offset == romSize)
        return hardDrive.countPartitions();
    else if (offset == romSize + 2)
        return hardDrive.countFileDrivers();
    else if (offset == romSize + 4)
        return board.agnus.system->diskDrives[0].attached();
    else if (offset == romSize + 6)
        return 0;

    return offset < romSize ? (rom[offset] << 8) | rom[offset + 1] : 0;
}

auto BuiltinHD::writeW(uint32_t addr, uint16_t data) -> void {
    unsigned offset = (addr & 0xffff) - 0x40;

    if (offset == romSize)
        memPtr = (memPtr & 0xffff) | (data << 16);
    else if (offset == romSize + 2)
        memPtr = (memPtr & ~0xffff) | data;
    else if (offset == romSize + 4) {
        switch (data) {
            case 0xfede: handleCmd(); break;
            case 0xfedf: handleInit(); break;
            case 0xfee0: handleResource(); break;
            case 0xfee1: handleInfoReq(); break;
            case 0xfee2: handleInitSeg(); break;
            case 0xfee6: handleCmd(true); break;
            default:
                warn("Invalid Builtin Command: %x", data);
        }
    } else if (offset == romSize + 6) {
        if (data == 107)
            warn("abortIO");
        else if (data == 110)
            warn("expunge");
    }
}

auto BuiltinHD::handleInit() -> void {
    constexpr uint16_t devn_dosName = 0x00;  // APTR  Pointer to DOS file handler name
    constexpr uint16_t devn_unit = 0x08;  // ULONG Unit number
    constexpr uint16_t devn_flags = 0x0C;  // ULONG OpenDevice flags
    constexpr uint16_t devn_sizeBlock = 0x14;  // ULONG # longwords in a block
    constexpr uint16_t devn_secOrg = 0x18;  // ULONG sector origin -- unused
    constexpr uint16_t devn_numHeads = 0x1C;  // ULONG number of surfaces
    constexpr uint16_t devn_secsPerBlk = 0x20;  // ULONG secs per logical block
    constexpr uint16_t devn_blkTrack = 0x24;  // ULONG secs per track
    constexpr uint16_t devn_resBlks = 0x28;  // ULONG reserved blocks -- MUST be at least 1!
    constexpr uint16_t devn_interleave = 0x30;  // ULONG interleave
    constexpr uint16_t devn_lowCyl = 0x34;  // ULONG lower cylinder
    constexpr uint16_t devn_upperCyl = 0x38;  // ULONG upper cylinder
    constexpr uint16_t devn_numBuffers = 0x3C;  // ULONG number of buffers
    constexpr uint16_t devn_memBufType = 0x40;  // ULONG Type of memory for AmigaDOS buffers
    constexpr uint16_t devn_transferSize = 0x44;  // LONG  largest transfer size (largest signed #)
    constexpr uint16_t devn_addMask = 0x48;  // ULONG address mask
    constexpr uint16_t devn_bootPrio = 0x4c;  // ULONG boot priority
    constexpr uint16_t devn_dName = 0x50;  // char[4] DOS file handler name
    constexpr uint16_t devn_bootflags = 0x54;  // boot flags (not part of DOS packet)
    constexpr uint16_t devn_segList = 0x58;  // filesystem segment list (not part of DOS packet)

    // if you want to set up partitioning with tools like HD Install Tool,
    // it's better not to mount the entire hard drive as an unformated drive. This can confuse these tools.
    bool noAutoMount = (board.ident == HDController::Ident::BuiltInRDB) && !hardDrive.structure.hasRDB;
    if (noAutoMount)
        return;

    uint32_t unit = agnus.fakeReadLongWord(memPtr + devn_unit);    

    if (unit >= hardDrive.countPartitions())
        return;

    auto& part = hardDrive.partitions()[unit];
    // auto& geometry = hardDrive.geometry();

    uint32_t namePtr = agnus.fakeReadLongWord(memPtr + devn_dosName);

    int i = 0;
    for (; i < part.name.size(); i++)
        agnus.fakeWriteByte(namePtr + i, part.name[i]);

    for (; i < 31; i++)
        agnus.fakeWriteByte(namePtr + i, 0);

    uint32_t segListBptr = hardDrive.getCopiedFileDriverMemOffset(part.dosType);

    if (!segListBptr) {
        // check if another HD controller has already copied a file driver to memory.
        // otherwise ROM doesn't use it for this controller.
        for (auto& _hd : agnus.system->hardDrives) {
            if (_hd.media->id == hardDrive.media->id)
                continue;

            segListBptr = _hd.getCopiedFileDriverMemOffset(part.dosType);
            if (segListBptr)
                break;
        }
    }

    agnus.fakeWriteLongWord(memPtr + devn_flags, part.flags);
    agnus.fakeWriteLongWord(memPtr + devn_sizeBlock, part.bSize >> 2);
    agnus.fakeWriteLongWord(memPtr + devn_secOrg, 0);
    agnus.fakeWriteLongWord(memPtr + devn_numHeads, part.heads);
    agnus.fakeWriteLongWord(memPtr + devn_secsPerBlk, 1);
    agnus.fakeWriteLongWord(memPtr + devn_blkTrack, part.sectors);
    agnus.fakeWriteLongWord(memPtr + devn_interleave, part.interleave);
    agnus.fakeWriteLongWord(memPtr + devn_resBlks, part.reserved);
    agnus.fakeWriteLongWord(memPtr + devn_lowCyl, part.cylLo);
    agnus.fakeWriteLongWord(memPtr + devn_upperCyl, part.cylHi);
    agnus.fakeWriteLongWord(memPtr + devn_numBuffers, part.numBuffers);
    agnus.fakeWriteLongWord(memPtr + devn_memBufType, part.bufMemType);
    agnus.fakeWriteLongWord(memPtr + devn_transferSize, part.maxTransfer);
    agnus.fakeWriteLongWord(memPtr + devn_addMask, part.mask);
    agnus.fakeWriteLongWord(memPtr + devn_bootPrio, part.bootPrio);
    agnus.fakeWriteLongWord(memPtr + devn_dName, part.dosType);
    agnus.fakeWriteLongWord(memPtr + devn_bootflags, part.bootFlags);
    agnus.fakeWriteLongWord(memPtr + devn_segList, segListBptr);
}

auto BuiltinHD::handleResource() -> void {
    if (!memPtr)
        return;

    uint32_t node = agnus.fakeReadLongWord(memPtr + 0x12);

    for (;;) {
        const auto succ = agnus.fakeReadLongWord(node);
        if (!succ)
            break;

        const uint32_t dosType = agnus.fakeReadLongWord(node + 0x0e);
        const uint32_t dosVersion = agnus.fakeReadLongWord(node + 0x12);
        auto& fileDrivers = hardDrive.fileDrivers();

        for (auto it = fileDrivers.begin(); it != fileDrivers.end();) {
            if (it->dosType == dosType && it->dosVersion <= dosVersion)
                it = fileDrivers.erase(it);
            else
                ++it;
        }

        node = succ;
    }
}

auto BuiltinHD::handleInfoReq() -> void {
    typedef Emulator::Error::Type EType;
    static constexpr uint16_t fsinfo_num = 0x00;
    static constexpr uint16_t fsinfo_dosType = 0x02;
    static constexpr uint16_t fsinfo_version = 0x06;
    static constexpr uint16_t fsinfo_numHunks = 0x0a;
    static constexpr uint16_t fsinfo_hunk = 0x0e;

    try {
        uint16_t pos = agnus.fakeReadWord(memPtr + fsinfo_num);
        if (pos >= hardDrive.countFileDrivers())
            throw Emulator::Error(EType::DRIVER_NOT_FOUND, pos);

        auto& fileDriver = hardDrive.fileDrivers()[pos];
        
        std::vector<uint8_t> code;
        hardDrive.structure.readFileDriver(fileDriver, code);

        ProgramUnit pUnit;
        pUnit.parse(code.data(), code.size());
        
        auto countHunks = pUnit.hunks.size();
        if (!countHunks || countHunks > 3)
            throw Emulator::Error(EType::DRIVER_WRONG_HUNK_COUNT, countHunks);

        agnus.fakeWriteLongWord(memPtr + fsinfo_dosType, fileDriver.dosType);
        agnus.fakeWriteLongWord(memPtr + fsinfo_version, fileDriver.dosVersion);
        agnus.fakeWriteLongWord(memPtr + fsinfo_numHunks, countHunks);

        for (int i = 0; i < countHunks; i++)
            agnus.fakeWriteLongWord(memPtr + fsinfo_hunk + 4 * i, pUnit.hunks[i].flags);

    } catch (Emulator::Error& e) {
        warn("handleInfoReq: %s", e.what());
    }
}

auto BuiltinHD::handleInitSeg() -> void {
    typedef Emulator::Error::Type EType;
    typedef HunkSection::Type HType;
    static constexpr uint16_t fsinitseg_hunk = 0x00;
    static constexpr uint16_t fsinitseg_num = 0x0c;

    try {
        uint32_t pos = agnus.fakeReadLongWord(memPtr + fsinitseg_num);
        if (pos >= hardDrive.countFileDrivers())
            throw Emulator::Error(EType::DRIVER_NOT_FOUND, pos);

        auto& fileDriver = hardDrive.fileDrivers()[pos];

        std::vector<uint8_t> code;
        hardDrive.structure.readFileDriver(fileDriver, code);

        ProgramUnit pUnit;
        pUnit.parse(code.data(), code.size());


        auto countHunks = pUnit.hunks.size();
        if (!countHunks || (countHunks > 3))
            throw Emulator::Error(EType::DRIVER_WRONG_HUNK_COUNT, countHunks);

        std::vector<uint32_t> segPtrs;
        for (int i = 0; i < countHunks; i++) {
            auto segPtr = agnus.fakeReadLongWord(memPtr + fsinitseg_hunk + 4 * i);
            if (!segPtr)
                throw Emulator::Error(EType::HUNK_SEGMENT_INITIALIZATION_FAIL);
            
            segPtrs.push_back(segPtr);
        }

        for (int i = 0; i < countHunks; i++) {
            auto& hunk = pUnit.hunks[i];

            for (auto& section : hunk.sections) {                
                if (section.type == HType::CODE || section.type == HType::DATA) {
                    agnus.fakeWriteLongWord(segPtrs[i], hunk.size() + 8);
                    agnus.fakeWriteLongWord(segPtrs[i] + 4, i == (countHunks - 1) ? 0 : (segPtrs[i + 1] + 4) >> 2);

                    for (unsigned d = 0; d < section.size; d++)
                        agnus.fakeWriteByte(segPtrs[i] + 8 + d, *(code.data() + section.offset + 8 + d));
                }
            }

            for (auto& section : hunk.sections) {
                if (section.type == HType::RELOC32) {
                    if (section.relocDest >= countHunks)
                        throw Emulator::Error(EType::HUNK_BAD_RELOC_TARGET, section.relocDest);

                    for (auto& offset : section.relocs) {
                        auto addr = segPtrs[i] + 8 + offset;
                        auto value = agnus.fakeReadLongWord(addr);
                        agnus.fakeWriteLongWord(addr, value + segPtrs[section.relocDest] + 8);
                    }
                }
            }
        }

        fileDriver.seglistBptr = (segPtrs[0] + 4) >> 2;

    } catch (Emulator::Error& e) {
        warn("handleInitSeg: %s", e.what());
    }
}

auto BuiltinHD::handleCmd(bool delayed) -> void {
    //constexpr uint32_t IO_UNIT = 0x18;
    constexpr uint32_t IO_COMMAND = 0x1C;
    constexpr uint32_t IO_ERROR = 0x1F;
    constexpr uint32_t IO_ACTUAL = 0x20;
    constexpr uint32_t IO_LENGTH = 0x24;
    constexpr uint32_t IO_DATA = 0x28;
    constexpr uint32_t IO_OFFSET = 0x2C;

    constexpr uint16_t scsi_Data = 0x00;
    constexpr uint16_t scsi_Length = 0x04;
    constexpr uint16_t scsi_Actual = 0x08;
    constexpr uint16_t scsi_Command = 0x0c;
    constexpr uint16_t scsi_CmdLength = 0x10;
    constexpr uint16_t scsi_CmdActual = 0x12;
    constexpr uint16_t scsi_Flags = 0x14;
    constexpr uint16_t scsi_Status = 0x15;
    constexpr uint16_t scsi_SenseData = 0x16;
    constexpr uint16_t scsi_SenseLength = 0x1a;
    constexpr uint16_t scsi_SenseActual = 0x1c;
    constexpr uint16_t scsi_SizeOf = 0x1e;

    //constexpr uint16_t devunit_UnitNum = 0x2A;

    constexpr int8_t IOERR_NOCMD = -3;
    constexpr int8_t IOERR_BADLENGTH = -4;
    constexpr int8_t IOERR_BADADDRESS = -5;    

    uint16_t cmd = agnus.fakeReadWord(memPtr + IO_COMMAND);
    uint32_t len = agnus.fakeReadLongWord(memPtr + IO_LENGTH);
    uint32_t addr = agnus.fakeReadLongWord(memPtr + IO_DATA);
    uint32_t offset = agnus.fakeReadLongWord(memPtr + IO_OFFSET);
    bool asyncAccessUse = romSize == sizeof(Firmware::mras0RomAsyc);

    int8_t error = 0;
    uint32_t actual = 0;

    //constexpr int8_t IOERR_UNITBUSY = -6;

    switch((Command)cmd) {
        case Command::Read: {
            error = verify(offset, len, addr);
            if (!delayed && asyncAccessUse)
                break;

            if (!error) {
                actual = len;
                uint8_t* data = hardDrive.read(offset, len);
                if (!data)
                    error = IOERR_BADLENGTH;
                else
                    agnus.fakeWriteByte(addr, data, len);
            }
        } break;
        case Command::Write:
        case Command::Format: {
            error = verify(offset, len, addr);
            if (!delayed && asyncAccessUse)
                break;

            if (!error) {
                actual = len;
                hardDrive.setBuffer(len);
                agnus.fakeRead(addr, hardDrive.buffer, len);
                if (!hardDrive.write(offset, len))
                    error = IOERR_BADLENGTH;
            }
        } break;

        case Command::Reset:
        case Command::Update:
        case Command::Clear:
        case Command::Stop:
        case Command::Start:
        case Command::Flush:
        case Command::Motor:
        case Command::Seek:
        case Command::Remove:
        case Command::ChangeNum:
        case Command::ChangeState:
        case Command::ProtStatus:
        case Command::AddChangeInt:
        case Command::RemChangeInt:
            // warn("not implemented command: %x", cmd);
            break;

        case Command::SCSICMD: {
            if (offset || (len != scsi_SizeOf && len != 0x22)) {
                error = IOERR_BADADDRESS;
            } else {
                scsiCmd sc{};
                sc.scsi_Data = agnus.fakeReadLongWord(addr + scsi_Data);
                sc.scsi_Length = agnus.fakeReadLongWord(addr + scsi_Length);
                sc.scsi_Command = agnus.fakeReadLongWord(addr + scsi_Command);
                sc.scsi_CmdLength = agnus.fakeReadWord(addr + scsi_CmdLength);
                sc.scsi_Flags = agnus.fakeReadByte(addr + scsi_Flags);
                sc.scsi_SenseData = agnus.fakeReadLongWord(addr + scsi_SenseData);
                sc.scsi_SenseLength = agnus.fakeReadWord(addr + scsi_SenseLength);

                scsiCommand(sc);
                agnus.fakeWriteLongWord(addr + scsi_Actual, sc.scsi_Actual);
                agnus.fakeWriteLongWord(addr + scsi_CmdActual, sc.scsi_CmdActual);
                agnus.fakeWriteByte(addr + scsi_Status, sc.scsi_Status);
                agnus.fakeWriteWord(addr + scsi_SenseActual, sc.scsi_SenseActual);

                error = 0;
                actual = sc.scsi_Actual;
            }
        } break;
        default:
            warn("unsupported device command: %x", cmd);
            error = IOERR_NOCMD;
            break;
    }

    agnus.fakeWriteByte(memPtr + IO_ERROR, error);

    if (!error)
        agnus.fakeWriteLongWord(memPtr + IO_ACTUAL, actual);
}

auto BuiltinHD::scsiCommand(scsiCmd& sc) -> void {
    auto& geometry = hardDrive.geometry();

    std::vector<uint8_t> cmd(sc.scsi_CmdLength);
    for (uint32_t i = 0; i < sc.scsi_CmdLength; ++i)
        cmd[i] = agnus.fakeReadByte(sc.scsi_Command + i);

    if (sc.scsi_CmdLength < 6) {
        warn("Invalid SCSI command length %i", sc.scsi_CmdLength);
        sc.scsi_Status = 2; // check condition (TODO: sense data)
        return;
    }

    auto copy_data = [&](uint8_t* data, size_t len) {
        const uint32_t actlen = std::min(sc.scsi_Length, static_cast<uint32_t>(len));

        agnus.fakeWriteByte(sc.scsi_Data, data, actlen);

        sc.scsi_Actual = actlen;
        sc.scsi_CmdActual = sc.scsi_CmdLength;
        sc.scsi_Status = 0;
        sc.scsi_SenseActual = 0;
    };

    if (cmd[0] == 0x25 && sc.scsi_CmdLength == 10) {
        uint8_t data[8] = {0,};
        
        const uint32_t max_block = geometry.blocks() - 1;
        if (cmd[8] & 1) { // pmi
            uint32_t lba = ToU32BE(&cmd[2]);
            lba += geometry.sectors * geometry.heads;
            lba /= geometry.sectors * geometry.heads;
            lba *= geometry.sectors * geometry.heads;
            FromU32BE(&data[0], std::min(max_block, lba));
        } else {
            FromU32BE(&data[0], max_block);
        }
        FromU32BE(&data[4], geometry.bSize);
        copy_data(data, sizeof(data));    

    } else if (cmd[0] == 0x12 && sc.scsi_CmdLength == 6) {
        // INQUIRY
        uint8_t data[36] = {0,};

        auto copy_string = [](uint8_t* d, const char* s, int len) {
            while (*s && len--)
                *d++ = *s++;
            while (len--)
                *d++ = ' ';
        };

        data[2] = 2; // Version
        data[4] = static_cast<uint8_t>(sizeof(data) - 4); // Additional length
        copy_string(&data[8], "Denise", 8); // vendor
        std::string product = "Virtual HD" + std::to_string(hardDrive.number);
        copy_string(&data[16], product.c_str(), 16); // product id
        copy_string(&data[32], "1.0", 4); // revision

        copy_data(data, sizeof(data));

    } else if (cmd[0] == 0x1a && sc.scsi_CmdLength == 6 && (cmd[2] == 3 || cmd[2] == 4)) {
        // MODE SENSE (6) with PC = 0 and Page Code = 3 (Format Parameters page) or 4 (Rigid drive geometry parameters)
        uint8_t data[256];
        memset(data, 0, sizeof(data));
        data[3] = 8;
        FromU32BE(&data[4], geometry.blocks());
        FromU32BE(&data[8], geometry.bSize);

        uint8_t* page = &data[12];

        page[0] = cmd[2]; // page code
        page[1] = 0x16; // page length

        if (cmd[2] == 3) {
            FromU16BE(&page[2], 1); // tracks per zone
            FromU16BE(&page[10], geometry.sectors);
            FromU16BE(&page[12], geometry.bSize); // data bytes per physical sector
            FromU16BE(&page[14], 1); // interleave
            page[20] = 0x80; // Drive type
        } else {
            page[14] = page[2] = static_cast<uint8_t>((geometry.cylinders >> 16) & 0xff);
            page[15] = page[3] = static_cast<uint8_t>((geometry.cylinders >> 8) & 0xff);
            page[16] = page[4] = static_cast<uint8_t>((geometry.cylinders >> 0) & 0xff);
            page[5] = geometry.heads;
            FromU16BE(&page[20], 5400); // rotation speed
        }
        page += page[1] + 2;

        data[0] = static_cast<uint8_t>(page - data - 1);

        copy_data(data, page - data);
    } else if (cmd[0] == 0x37 && sc.scsi_CmdLength == 10) { // READ DEFECT DATA
        uint8_t data[4] = { 0, static_cast<uint8_t>(cmd[1] & 0x1f), 0, 0 };
        copy_data(data, sizeof(data));
    } else if (cmd[0] == 0x2f || cmd[0] == 0x0) { // verify or unit ready
        // pretend there are no errors :-)
        sc.scsi_Actual = 0;
        sc.scsi_CmdActual = sc.scsi_CmdLength;
        sc.scsi_Status = 0;
        sc.scsi_SenseActual = 0;

    } else {
        sc.scsi_Status = /*SCSI_INVALID_COMMAND*/ 0x20; // SCSI status
        sc.scsi_Actual = 0; // sc.scsi_Length;
        sc.scsi_CmdActual = 0; // sc.scsi_CmdLength; // Whole command used
        sc.scsi_SenseActual = 0; // Not used

        std::string s(cmd.begin(), cmd.end());
        warn("unsupported scsi cmd: %s", s.c_str());
    }
}

auto BuiltinHD::verify(unsigned offset, unsigned length, uint32_t addr) -> int8_t {
    //constexpr int8_t IOERR_OPENFAIL = -1;
    //constexpr int8_t IOERR_ABORTED = -2;
    constexpr int8_t IOERR_BADLENGTH = -4;
    constexpr int8_t IOERR_BADADDRESS = -5;
    //constexpr int8_t IOERR_UNITBUSY = -6;
    //constexpr int8_t IOERR_SELFTEST = -7;

    if (length % 512)
        return IOERR_BADLENGTH;

    if (offset % 512)
        return IOERR_BADADDRESS;

    auto geometry = hardDrive.geometry();

    if ((uint64_t)offset + (uint64_t)length > geometry.length())
        return IOERR_BADADDRESS;

    if (!agnus.isMem(addr) || !agnus.isMem(addr + length))
        return IOERR_BADADDRESS;

    return 0;
}

}
