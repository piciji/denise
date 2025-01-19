
#include <cstring>
#include "superCpu.h"
#include "../../system/system.h"
#include "../../traps/traps.h"

#define SCPU_FREQ 20'000'000

// RDY NMOS 6502, 6510: Write cycles don't hold CPU
// RDY CMOS 65C02, 65816: Read and Write cycles hold CPU
// todo: find out if RDY input or clock stretching is used for slow mode and/or VIC BA
// todo: find out if incoming Interrupts are checked in clock stretched IRQ sample cycles (usually the last one)
// hint: RDY repeats cycle and checks for incoming Interrupts (if IRQ sample cycle)

namespace LIBC64 {

SuperCpu::SuperCpu(System* system, Emulator::SystemTimer& sysTimer, CIA::M6526& cia1, CIA::M6526& cia2, SidManager& sidManager, Traps& traps)
: ExpansionPort(system), sysTimer(sysTimer), cia1(cia1), cia2(cia2), sidManager(sidManager), traps(traps) {
    setId( Interface::ExpansionIdSuperCpu );

    media = nullptr;
    rom = nullptr;
    dram = nullptr;
    sram = nullptr;
    dramMask = 0;
    dramSize = 0;
    setRom(nullptr, nullptr, 0);

    applyBuffer = [this]() {
        if ((baFlags & 2) == 0) {
            this->system->ram[writeBuffer.addr] = writeBuffer.value;
            writeBuffer.inProgress = false;
        } else
            this->sysTimer.add(&applyBuffer, 1, Emulator::SystemTimer::UpdateExisting);
    };

    baStart = [this]() {
        if (baFlags)
            baFlags = 2;
    };

    readTable[0] = nullptr;
    sysTimer.registerCallback( { {&applyBuffer, 1}, {&baStart, 1} } );

    jumperJiffyDos = false;
    jumper1Mhz = true;
}

SuperCpu::~SuperCpu() {
    if (dram)
        delete[] dram;
    if (sram)
        delete[] sram;
}

auto SuperCpu::setDram() -> void {
    if ((dramSize >> 20) == 16) {
        if (!dram)
            dram = new uint8_t[dramSize - 512 * 1024];
        std::memset(dram, 0, dramSize - 512 * 1024);
    } else if (dramSize) {
        if (!dram)
            dram = new uint8_t[dramSize];
        std::memset(dram, 0, dramSize);
    }
}

auto SuperCpu::reset(bool softReset) -> void {
    power();
    frequency = vicII->frequency();
    dramMask = dramSize ? (dramSize - 1) : 0;
    setDram();

    switch (dramSize >> 20) {
        case 1: dramPageSize = 9 + 2; break;
        case 4:
        case 8: dramPageSize = 10 + 2; break;
        default: dramPageSize = 11 + 2; break;
    }

    if (!sram)
        sram = new uint8_t[0x20000];

    std::memset(sram, 0, 0x20000);

    if (!readTable[0])
        buildMap();

    optim = 0xc7;
    cycles = 0;
    software1Mhz = false;
    system1Mhz = false;
    fastMode = !jumper1Mhz;
    system->interface->updateDeviceState( media, false, 0, 0x80 | fastMode, true );
    baFlags = 0;
    memConf = 7; // processor port
    memConf |= BOOT_MAP;
    mirrorMemConf = memConf;
    dramAddr = 0xffff;
    writeBuffer.inProgress = false;
    updateMirroring();
    updateSimm(4);

    // calculations based on suprtime.pdf
    // Host clock
    float nsCycle = (float)1'000'000'000 / (float)frequency;
    float nsMemoryCacheLatch = nsCycle / 2.0 + 90.0;
    float nsIoAccess = nsCycle / 2.0 + 105.0;
    float nsCycleWriteLimit = nsCycle - 70.0;

    cycleCacheLatch = (unsigned)((float)SCPU_FREQ * (nsMemoryCacheLatch / nsCycle));
    cycleIoAccess = (unsigned)((float)SCPU_FREQ * (nsIoAccess / nsCycle));
    cycleWriteThroughLimit = (unsigned)((float)SCPU_FREQ * (nsCycleWriteLimit / nsCycle));

    // DOT clock
    float nscycleDot = (float)1'000'000'000 / ((float)frequency * 8.0);
    float nsCycleIoLong = 7.0 * nscycleDot + 24.0;
    cycleIoLong = (unsigned)(20'000'000.0 * (nsCycleIoLong / nsCycle));
}

auto SuperCpu::setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void {
    static uint8_t* dummyRom = nullptr;
    if (dummyRom == nullptr) {
        dummyRom = new uint8_t[1];
        dummyRom[0] = 0xff;
    }
    this->media = media;
    this->rom = rom;

    if (romSize >= (512 * 1024)) romSize = 512 * 1024;
    else if (romSize >= (256 * 1024)) romSize = 256 * 1024;
    else if (romSize >= (128 * 1024)) romSize = 128 * 1024;
    else if (romSize >= (64 * 1024)) romSize = 64 * 1024;
    else {
        this->rom = dummyRom;
        romSize = 1;
    }

    this->romMask = romSize - 1;
}

auto SuperCpu::setRamSize(int id) -> void {
    unsigned _dramSize = dramSize;

    switch (id) {
        default:
        case 0: dramSize = 0; break;
        case 1: dramSize = 1; break;
        case 2: dramSize = 4; break;
        case 3: dramSize = 8; break;
        case 4: dramSize = 16; break;
    }
    dramSize <<= 20;

    if (dram && (_dramSize != dramSize)) {
        delete[] dram;
        dram = nullptr;
    }
}

auto SuperCpu::getRamSize() -> int {
    switch (dramSize >> 20) {
        default:
        case 0: return 0;
        case 1: return 1;
        case 4: return 2;
        case 8: return 3;
        case 16: return 4;
    }
    return 0;
}

#define SC_MAP ((page << 8) | (mem) | (conf << 5))
#define SC_MAP_WR (SC_MAP | (m << 16))
#define SC_NO_MIRROR for (int m = 0; m < 9; m++)

auto SuperCpu::buildMap() -> void {
    const uint8_t mirrors[9][2] = {{0x80, 0xbf}, {0x00, 0x3f}, {0x02, 0x3f}, {0x40, 0x7f}, {0xc0, 0xff},
        {0x04, 0x07}, {0x0, 0x0}, {0x00, 0xff}, {0x02, 0xff} };

    for (int page = 0; page < 256; page++) {
        for (int mem = 0; mem < 8; mem++) { // todo: support EXROM and GAME, if a SuperCPU pass through expansion needs this
            for (int conf = 0; conf < 8; conf++) {
                if (conf == 7) {
                    for (int m = 0; m < 9; m++) {
                        if (page == 0) writeTable[SC_MAP_WR] = &SuperCpu::writeSramAndOrHostRam<MODE_INTERNAL, PAGE_ZERO>;
                        else if (page == 0xff) writeTable[SC_MAP_WR] = &SuperCpu::writeSramAndOrHostRam<MODE_INTERNAL, PAGE_HI>;
                        else writeTable[SC_MAP_WR] = &SuperCpu::writeSramAndOrHostRam<MODE_INTERNAL>;
                    }
                } else {
                    for (int m = 0; m < 9; m++) {
                        if (mirrors[m][1] && (page >= mirrors[m][0] && page <= mirrors[m][1])) {
                            if (page == 0) writeTable[SC_MAP_WR] = &SuperCpu::writeSramAndOrHostRam<MODE_MIRROR, PAGE_ZERO>;
                            else if (page == 0xff) writeTable[SC_MAP_WR] = &SuperCpu::writeSramAndOrHostRam<MODE_MIRROR, PAGE_HI>;
                            else writeTable[SC_MAP_WR] = &SuperCpu::writeSramAndOrHostRam<MODE_MIRROR>;
                        } else {
                            if (page == 0) writeTable[SC_MAP_WR] = &SuperCpu::writeSramAndOrHostRam<MODE_SRAM, PAGE_ZERO>;
                            else if (page == 0xff) writeTable[SC_MAP_WR] = &SuperCpu::writeSramAndOrHostRam<MODE_SRAM, PAGE_HI>;
                            else writeTable[SC_MAP_WR] = &SuperCpu::writeSramAndOrHostRam<MODE_SRAM>;
                        }
                    }
                }

                if (page <= 0x0f) {
                    if (conf == 7)          readTable[SC_MAP] = &SuperCpu::readC64Ram;
                    else                    readTable[SC_MAP] = &SuperCpu::readSramB0;
                } else if (page <= 0x5f) {
                    if (conf == 7)          readTable[SC_MAP] = &SuperCpu::readC64Ram;
                    else if (conf & 2)      readTable[SC_MAP] = &SuperCpu::readSramB1;
                    else                    readTable[SC_MAP] = &SuperCpu::readSramB0;
                } else if (page <= 0x7f) {
                    if (conf == 7)          readTable[SC_MAP] = &SuperCpu::readC64Ram;
                    else                    readTable[SC_MAP] = &SuperCpu::readSramB0;
                } else if (page <= 0x9f) {
                    if (conf == 7)          readTable[SC_MAP] = &SuperCpu::readC64Ram;
                    else if (conf & 4)      readTable[SC_MAP] = &SuperCpu::readRom;
                    else if (conf & 2)      readTable[SC_MAP] = &SuperCpu::readSramB1;
                    else                    readTable[SC_MAP] = &SuperCpu::readSramB0;
                } else if (page <= 0xbf) {
                    if (conf & 4)           readTable[SC_MAP] = &SuperCpu::readRom;
                    else if ((mem & 3) == 3)readTable[SC_MAP] = &SuperCpu::readSramB1;
                    else                    readTable[SC_MAP] = &SuperCpu::readSramB0;
                } else if (page <= 0xcf) {
                    if (conf == 7)          readTable[SC_MAP] = &SuperCpu::readC64Ram;
                    else if (conf & 4)      readTable[SC_MAP] = &SuperCpu::readRom;
                    else                    readTable[SC_MAP] = &SuperCpu::readSramB0;
                } else if (page <= 0xdf) {
                    if ((mem & 3) == 0) {
                        if (conf == 7)      readTable[SC_MAP] = &SuperCpu::readSramChar;
                        else if (conf & 4)  readTable[SC_MAP] = &SuperCpu::readRom;
                        else                readTable[SC_MAP] = &SuperCpu::readSramB0;
                    } else if ((mem & 4) == 0)
                                            readTable[SC_MAP] = &SuperCpu::readSramChar;
                    else {
                        if (page == 0xd0) {
                            readTable[SC_MAP] = &SuperCpu::readIoSCPU;
                            SC_NO_MIRROR writeTable[SC_MAP_WR] = &SuperCpu::writeIoSCPU;
                        } else if (page == 0xd1) {
                            readTable[SC_MAP] = &SuperCpu::readIoVIC;
                            SC_NO_MIRROR writeTable[SC_MAP_WR] = &SuperCpu::writeIoVIC;
                        } else if (page <= 0xd3) {
                            readTable[SC_MAP] = &SuperCpu::readSramB1;
                            SC_NO_MIRROR writeTable[SC_MAP_WR] = &SuperCpu::writeIoStrange;
                        } else if (page <= 0xd5) {
                            readTable[SC_MAP] = &SuperCpu::readIoSID;
                            SC_NO_MIRROR writeTable[SC_MAP_WR] = &SuperCpu::writeIoSID<true>;
                        } else if (page <= 0xd7) {
                            readTable[SC_MAP] = &SuperCpu::readIoSID;
                            SC_NO_MIRROR writeTable[SC_MAP_WR] = &SuperCpu::writeIoSID<false>;
                        } else if (page <= 0xdb) {
                            readTable[SC_MAP] = (conf == 7) ? &SuperCpu::readIoColorRamInternal : &SuperCpu::readSramB1;
                            if (conf == 7) { SC_NO_MIRROR writeTable[SC_MAP_WR] = &SuperCpu::writeIoColorRam<false>; }
                            else { SC_NO_MIRROR writeTable[SC_MAP_WR] = &SuperCpu::writeIoColorRam<true>; }
                        } else if (page == 0xdc) {
                            readTable[SC_MAP] = &SuperCpu::readIoCIA1;
                            SC_NO_MIRROR writeTable[SC_MAP_WR] = &SuperCpu::writeIoCIA1;
                        } else if (page == 0xdd) {
                            readTable[SC_MAP] = &SuperCpu::readIoCIA2;
                            SC_NO_MIRROR writeTable[SC_MAP_WR] = &SuperCpu::writeIoCIA2;
                        } else if (page == 0xde) {
                            readTable[SC_MAP] = &SuperCpu::readIo1;
                            SC_NO_MIRROR writeTable[SC_MAP_WR] = &SuperCpu::writeIo1;
                        } else if (page == 0xdf) {
                            readTable[SC_MAP] = &SuperCpu::readIo2;
                            SC_NO_MIRROR writeTable[SC_MAP_WR] = &SuperCpu::writeIo2;
                        }
                    }
                } else if (page <= 0xff) {
                    if (conf & 4)       readTable[SC_MAP] = &SuperCpu::readRom;
                    else if (mem & 2) {
                        if (conf & 1)   readTable[SC_MAP] = &SuperCpu::readSramKernal;
                        else            readTable[SC_MAP] = &SuperCpu::readSramB1;
                    }
                    else                readTable[SC_MAP] = &SuperCpu::readSramB0;
                }
            }
        }
    }
}

inline auto SuperCpu::readVectorByte(uint16_t addr) -> uint8_t {
    ReadTable ptr = readTable[memConf | (addr & 0xff00)];

    if (ptr == &SuperCpu::readSramB1 || ptr == &SuperCpu::readSramKernal) {
        if ((memConf & (HW_ENABLE | DOS_EXT) ) || !modeE || system1Mhz) {
            clockStretchRom();
            return rom[addr & romMask];
        }
    }
    return (this->*ptr)(addr);
}

inline auto SuperCpu::readByte(uint32_t addr) -> uint8_t {
    if ((addr & 0xff0000) == 0) { // bank 0 SRAM
        return (this->*readTable[memConf | (addr & 0xff00)])(addr);
    } if ((addr & 0xff0000) == 0x10000) { // bank 1 SRAM
        stepCycle();
        return (addr & 0xfffe) ? sram[addr] : sram[addr & 1]; // will be mapped to bank 0

    } if ((addr & 0xf80000) == 0xf80000) { // ROM can't be switched out
        clockStretchRom();
        return rom[addr & romMask];
    }

    if (dram) {
        if ((addr & 0xfe0000) == 0xf60000) {
            if (dramPageSize != dramConfPageSize)
                addr = ((addr >> dramConfPageSize) << dramPageSize) | (addr & ((1 << dramPageSize) - 1));

            addr &= 0x1ffff; // map banks $f6/$f7 to banks 0/1 of DRAM
        } else if (addr < dramConfSize) {
            if (dramPageSize != dramConfPageSize)
                addr = ((addr >> dramConfPageSize) << dramPageSize) | (addr & ((1 << dramPageSize) - 1));

        } else {
            stepCycle();
            return addr >> 16;
        }

        clockStretchDramRead(addr);
        return dram[addr & dramMask];
    }

    stepCycle();
    return addr >> 16;
}

inline auto SuperCpu::writeByte(uint32_t addr, uint8_t value) -> void {
    if ((addr & 0xff0000) == 0) { // SRAM
        (this->*writeTable[mirrorMemConf | (addr & 0xff00)])(addr, value);
        return;
    } if ((addr & 0xff0000) == 0x10000) { // bank 1 SRAM
        stepCycle();
        if (addr & 0xfffe)  sram[addr] = value;
        else                sram[addr & 1] = value;
        return;
    } if ((addr & 0xf80000) == 0xf80000) { // ROM
        clockStretchRom();
        return;
    }

    if (dram) {
        if ((addr & 0xfe0000) == 0xf60000) {
            if ((memConf & HW_ENABLE) == 0)
                return stepCycle();
            if (dramPageSize != dramConfPageSize)
                addr = ((addr >> dramConfPageSize) << dramPageSize) | (addr & ((1 << dramPageSize) - 1));

            addr &= 0x1ffff; // map banks $f6/$f7 to banks 0/1 of DRAM
        } else if (addr < dramConfSize) {
            if (dramPageSize != dramConfPageSize)
                addr = ((addr >> dramConfPageSize) << dramPageSize) | (addr & ((1 << dramPageSize) - 1));
        } else
            return stepCycle();

        clockStretchDramWrite(addr);
        if (dramTracker.enabled())
            dramTracker.remember(addr & dramMask, dram);
        dram[addr & dramMask] = value;
    } else
        stepCycle();
}

inline auto SuperCpu::idleCycle(uint32_t addr) -> void {
    // unlike NMOS/CMOS 6502 the 65816 has VDA/VPA lines to inform BUS participants about idle cycles.
    // this way there is no need to waste cycles for slow memory accesses
    // todo: find out if SuperCPU checks this
    if (fastMode) {
        cycles += frequency;
        if (cycles >= SCPU_FREQ) {
            cycles -= SCPU_FREQ;
            syncStock();
        }
    } else
        // and/or if E-Line of 65816 signals emulation mode?
        readByte(addr);
}

inline auto SuperCpu::trapHandler() -> bool {
    return traps.handler();
}

auto SuperCpu::readC64Ram(uint16_t addr) -> uint8_t {
    clockStretchIORead();
    return system->ram[addr];
}

auto SuperCpu::readSramB0(uint16_t addr) -> uint8_t {
    stepCycle<true>();
    return sram[addr];
}

auto SuperCpu::readSramB1(uint16_t addr) -> uint8_t {
    stepCycle<true>();
    return sram[0x10000 | addr];
}

auto SuperCpu::readSramChar(uint16_t addr) -> uint8_t {
    stepCycle<true>();
    return system->charRom[addr & 0xfff];
}

auto SuperCpu::readSramKernal(uint16_t addr) -> uint8_t {
    stepCycle<true>();
    return sram[0x8000 + addr];
}

auto SuperCpu::readRom(uint16_t addr) -> uint8_t {
    clockStretchRom();
    return rom[addr & romMask];
}

template<uint8_t mode, uint8_t addrArea> auto SuperCpu::writeSramAndOrHostRam(uint16_t addr, uint8_t value) -> void {
    if constexpr (mode == MODE_MIRROR || mode == MODE_INTERNAL) {
        if constexpr (addrArea == PAGE_HI) {
            if (addr == 0xff00) {
                clockStretchIOWriteLong();
                system->ram[addr] = value;
            } else
                clockStretchWriteInternal(addr, value);
        } else
            clockStretchWriteInternal(addr, value);
    } else if constexpr (mode == MODE_SRAM)
        stepCycle();

    if constexpr (mode == MODE_SRAM || mode == MODE_MIRROR)
        sram[addr] = value;

    if constexpr (addrArea == PAGE_ZERO) { // templated for performance reasons: following "if" is checked only when the page is zero.
        if (addr == 1) {
            // emulates port of 6510
            // "6510" is permanently stopped as soon as charen/hiram/loram is set to a pattern that allows expansion
            // to switch between C64 RAM and IO area only via game/exrom.
            // kernal, basic and char ROMs are provided by the SuperCPU. the simulated processor port integrates these, not the C64.
            memConf = (memConf & ~7) | (value & 7);
            mirrorMemConf = (mirrorMemConf & ~7) | (value & 7);
        }
    }
}

auto SuperCpu::readIoSCPU(uint16_t addr) -> uint8_t {
    cycles += frequency; // one more cycle to find out if IO
    if ((addr & 0xfff0) == 0xd0b0) {
        stepCycle<true>();
        uint8_t value = optim & 7;
        switch (addr) {
            case 0xd0b0:    value |= 0x40; break; // version V2
            case 0xd0b2:    value |= (memConf & HW_ENABLE) ? 0x80 : 0x00;
                            value |= system1Mhz ? 0x40 : 0; break;
            case 0xd0b3:
            case 0xd0b4:    value |= optim & 0xc0; break;
            case 0xd0b5:    value |= jumperJiffyDos ? 0x80 : 0;
                            value |= jumper1Mhz ? 0x40 : 0; break;
            case 0xd0b6:    value |= emulationMode() ? 0x80 : 0; break;
            case 0xd0b8:
            case 0xd0b9:    value |= software1Mhz ? 0x80 : 0;
                            value |= fastMode ? 0 : 0x40; break;
            case 0xd0bc:
            case 0xd0bd:
            case 0xd0be:
            case 0xd0bf:    value |= ((memConf & DOS_EXT) ? 0x80 : 0x00); break;
            default:        break;
        }
        return value;
    }
    clockStretchIORead();
    return vicII->readReg( addr & 0xff );
}

auto SuperCpu::readIoVIC(uint16_t addr) -> uint8_t {
    clockStretchIORead();
    return vicII->readReg( addr & 0xff );
}

auto SuperCpu::readIoSID(uint16_t addr) -> uint8_t {
    clockStretchIORead();
    return sidManager.readSidReg(addr);
}

auto SuperCpu::readIoColorRamInternal(uint16_t addr) -> uint8_t {
    clockStretchIORead();
    return vicII->lastReadPhase1();
}

auto SuperCpu::readIoCIA1(uint16_t addr) -> uint8_t {
    clockStretchIORead();
    return cia1.read( addr );
}

auto SuperCpu::readIoCIA2(uint16_t addr) -> uint8_t {
    clockStretchIORead();
    return cia2.read( addr );
}

auto SuperCpu::readIo1(uint16_t addr) -> uint8_t {
    clockStretchIORead();
    uint8_t value;
    if (sidManager.readIo(addr, value))
        return value;

    return ExpansionPort::readIo1(addr);
}

auto SuperCpu::readIo2(uint16_t addr) -> uint8_t {
    clockStretchIORead();
    uint8_t value;
    if (sidManager.readIo(addr, value))
        return value;

    return ExpansionPort::readIo2(addr);
}

auto SuperCpu::writeIoSCPU(uint16_t addr, uint8_t value) -> void {
    cycles += frequency; // one more cycle to find out if IO
    sram[0x10000 | addr] = value;

    if ((addr >= 0xd071 && addr < 0xd080) || (addr >= 0xd0b0 && addr < 0xd0c0)) {
        stepCycle<true>();
        switch (addr) {
            case 0xd072:
                if (!system1Mhz) {
                    system1Mhz = true;
                    updateFastmode();
                } break;
            case 0xd073:
                if (system1Mhz) {
                    system1Mhz = false;
                    updateFastmode();
                } break;
            case 0xd074:
            case 0xd075:
            case 0xd076:
            case 0xd077:
                if (memConf & HW_ENABLE) {
                    optim = addr << 6;
                    updateMirroring();
                } break;
            case 0xd078:
                if (memConf & HW_ENABLE)
                    updateSimm(value);
                break;
            case 0xd07a:
                if (!software1Mhz) {
                    software1Mhz = true;
                    updateFastmode();
                } break;
            case 0xd079:
            case 0xd07b:
                if (software1Mhz) {
                    software1Mhz = false;
                    updateFastmode();
                } break;
            case 0xd07e:
                if((memConf & HW_ENABLE) == 0) {
                    memConf |= HW_ENABLE;
                    mirrorMemConf |= HW_ENABLE;
                    updateFastmode();
                } break;
            case 0xd07d:
            case 0xd07f:
                if (memConf & HW_ENABLE) {
                    memConf &= ~HW_ENABLE;
                    mirrorMemConf &= ~HW_ENABLE;
                    updateFastmode();
                } break;
            case 0xd0b2:
                if (memConf & HW_ENABLE) {
                    system1Mhz = !!(value & 0x40);
                    if ((value & 0x80) == 0) {
                        memConf &= ~HW_ENABLE;
                        mirrorMemConf &= ~HW_ENABLE;
                    }
                    updateFastmode();
                } break;
            case 0xd0b3:
                if (memConf & HW_ENABLE) {
                    optim = (optim & 0x38) | (value & 0xc7);
                    updateMirroring();
                } break;
            case 0xd0b4:
                if (memConf & HW_ENABLE) {
                    optim = (optim & 0x3f) | (value & 0xc0);
                    updateMirroring();
                } break;
            case 0xd0b6:
                if ((memConf & (HW_ENABLE | BOOT_MAP)) == (HW_ENABLE | BOOT_MAP)) {
                    memConf &= ~BOOT_MAP;
                    mirrorMemConf &= ~BOOT_MAP;
                } break;
            case 0xd0b7:
                if ((memConf & (HW_ENABLE | BOOT_MAP)) == HW_ENABLE) {
                    memConf |= BOOT_MAP;
                    mirrorMemConf |= BOOT_MAP;
                } break;
            case 0xd0b8:
                if (memConf & HW_ENABLE) {
                    software1Mhz = value >> 7;
                    updateFastmode();
                } break;
            case 0xd0bc:
                if (memConf & HW_ENABLE) {
                    if (value & 0x80) memConf |= DOS_EXT;
                    else memConf &= ~DOS_EXT;
                    mirrorMemConf = (mirrorMemConf & ~0xff) | (memConf & 0xff);
                } break;
            case 0xd0be:
                if ((memConf & (HW_ENABLE | DOS_EXT) ) == HW_ENABLE ) {
                    memConf |= DOS_EXT;
                    mirrorMemConf |= DOS_EXT;
                } break;
            case 0xd0bd:
            case 0xd0bf:
                if (memConf & DOS_EXT) {
                    memConf &= ~DOS_EXT;
                    mirrorMemConf &= ~DOS_EXT;
                } break;
            default: break;
        }
    } else {
        clockStretchIOWrite();
        vicII->writeReg( addr & 0xff, value );
    }
}

auto SuperCpu::writeIoVIC(uint16_t addr, uint8_t value) -> void {
    clockStretchIOWrite();
    sram[0x10000 | addr] = value;
    vicII->writeReg( addr & 0xff, value );
}

auto SuperCpu::writeIoStrange(uint16_t addr, uint8_t value) -> void {
    stepCycle<true, true>();

    if ((memConf & HW_ENABLE) || (addr == 0xd27e))
        sram[0x10000 | addr] = value;
}

template<bool withSram> auto SuperCpu::writeIoSID(uint16_t addr, uint8_t value) -> void {
    clockStretchIOWrite();
    if constexpr (withSram)
        sram[0x10000 | addr] = value;
    sidManager.writeSidReg(addr, value);
}

template<bool withSram> auto SuperCpu::writeIoColorRam(uint16_t addr, uint8_t value) -> void {
    clockStretchIOWrite();
    if constexpr (withSram)
        sram[0x10000 | addr] = value;
    system->colorRam[ addr & 0x3ff ] = value & 0xf;
}

auto SuperCpu::writeIoCIA1(uint16_t addr, uint8_t value) -> void {
    clockStretchIOWriteCia();
    sram[0x10000 | addr] = value;
    cia1.write( addr, value );
}

auto SuperCpu::writeIoCIA2(uint16_t addr, uint8_t value) -> void {
    clockStretchIOWriteCia();
    sram[0x10000 | addr] = value;
    cia2.write( addr, value );
}

auto SuperCpu::writeIo1(uint16_t addr, uint8_t value) -> void {
    clockStretchIOWrite();
    sidManager.writeIo( addr, value );
    ExpansionPort::writeIo1(addr, value);
}

auto SuperCpu::writeIo2(uint16_t addr, uint8_t value) -> void {
    if (addr == 0xdf01 || addr == 0xdf21)
        clockStretchIOWriteLong();
    else
        clockStretchIOWrite();

    sidManager.writeIo( addr, value );
    ExpansionPort::writeIo2(addr, value);
}

auto SuperCpu::updateFastmode(bool synced) -> void {
    bool _fastMode = fastMode;
    fastMode = !(system1Mhz || software1Mhz || (jumper1Mhz && ((memConf & HW_ENABLE) == 0)));

    if (fastMode != _fastMode) {
        if (synced) {
            if (fastMode)
                cycles = 15'500'000;
            else
                syncStock();
        }
        system->interface->updateDeviceState( media, false, 0, 0x80 | fastMode, true );
    }
}

auto SuperCpu::updateSimm(uint8_t value) -> void {
    switch (value & 7) {
        case 0:
            dramConfPageSize = 9 + 2;
            dramConfSize = 1;
            break;
        case 1:
            dramConfPageSize = 10 + 2;
            dramConfSize = 4;
            break;
        case 2:
            dramConfPageSize = 10 + 2;
            dramConfSize = 8;
            break;
        case 3:
            dramConfPageSize = 10 + 2;
            dramConfSize = 16;
            break;
        default:
            dramConfPageSize = 11 + 2;
            dramConfSize = 16;
            break;
    }

    dramConfSize <<= 20;
    dramRowMask = ~((1 << dramConfPageSize) - 1);
}

auto SuperCpu::updateMirroring() -> void {
    unsigned x = 7; // no optimization
    if ((optim & 0xc4) == 0) x = 0;
    else if ((optim & 0xc5) == 4) x = 1;
    else if ((optim & 0xc5) == 5) x = 2;
    else if ((optim & 0xc4) == 0x40) x = 3;
    else if ((optim & 0xc4) == 0x44) x = 4;
    else if ((optim & 0xc4) == 0x80) x = 5;
    else if ((optim & 0xc4) == 0x84) x = 6;
    //else if ((optim & 0xc1) == 0xc0) x = 7; // default
    else if ((optim & 0xc1) == 0xc1) x = 8;

    mirrorMemConf = (mirrorMemConf & 0xffff) | (x << 16);
}

template<bool checkBA, bool IsWrite> auto SuperCpu::stepCycle() -> void {
    if (fastMode) {
        cycles += frequency;
        if (cycles >= SCPU_FREQ) {
            cycles -= SCPU_FREQ;
            syncStock();
        }
    } else {
        syncStock();
        if constexpr (checkBA) {
            if constexpr (IsWrite)
                while (baFlags & 2) syncStock();
            else
                while (baFlags) syncStock();
        }
    }
}

auto SuperCpu::clockStretchRom() -> void {
    if (fastMode) {
        cycles += frequency * 4;
        if (cycles >= SCPU_FREQ) {
            cycles -= SCPU_FREQ;
            syncStock();
        }
    } else
        syncStock();
}

auto SuperCpu::clockStretchDramRead(uint32_t& addr) -> void {
    if (fastMode) {
        if (!((dramAddr ^ addr) & ~3)) { // same column (four bytes each)
            cycles += frequency; // maximum speed like SRAM
        } else if ((dramAddr ^ addr) & dramRowMask) { // different row
            cycles += (frequency * 3) + (frequency >> 1); // 3.5 cycles
            dramAddr = addr;
        } else if (!(((dramAddr + 4) ^ addr) & ~3)) { // same row, next column
            cycles += frequency; // maximum speed like SRAM
            dramAddr = addr;
        } else { // same row but column is not the next one (non-sequential)
            cycles += frequency * 2;
            dramAddr = addr;
        }

        if (cycles >= SCPU_FREQ) {
            cycles -= SCPU_FREQ;
            syncStock();
        }
    } else
        syncStock();
}

auto SuperCpu::clockStretchDramWrite(uint32_t& addr) -> void {
    if (fastMode) {
        if ((dramAddr ^ addr) & dramRowMask)
            cycles += frequency * 3; // different row
        else
            cycles += frequency;

        dramAddr = addr;

        if (cycles >= SCPU_FREQ) {
            cycles -= SCPU_FREQ;
            syncStock();
        }
    } else
        syncStock();
}

auto SuperCpu::clockStretchIORead() -> void {
    if (fastMode) {
        if (writeBuffer.inProgress)
            waitForWriteBuffer();
        else if (cycles > cycleWriteThroughLimit)
            syncStock();

        syncStock();
        while (baFlags & 2)
            syncStock();
        cycles = cycleIoAccess;
    } else {
        syncStock();
        while (baFlags)
            syncStock();
    }
}

auto SuperCpu::clockStretchIOWrite() -> void {
    if (fastMode) {
        if (writeBuffer.inProgress)
            waitForWriteBuffer();
        else if (cycles > cycleWriteThroughLimit)
            syncStock();

        cycles = cycleIoAccess;
    }

    syncStock();
    while (baFlags & 2)
        syncStock();
}

auto SuperCpu::clockStretchIOWriteLong() -> void {
    if (fastMode) {
        if (writeBuffer.inProgress)
            waitForWriteBuffer();
        else if (cycles > cycleWriteThroughLimit) // too late, can't sync to the following host clock
            syncStock();

        cycles = cycleIoLong;
    }

    syncStock();
    while (baFlags & 2)
        syncStock();
}

auto SuperCpu::clockStretchIOWriteCia() -> void {
    if (fastMode) {
        if (writeBuffer.inProgress)
            waitForWriteBuffer();
        else if (cycles > cycleWriteThroughLimit)
            syncStock();

        syncStock();
        cycles = cycleIoLong;

        syncStock();
        while (baFlags)
            syncStock();
    } else
        syncStock();
}

auto SuperCpu::waitForWriteBuffer() -> void {
    do {
        if ((baFlags & 2) == 0) { // SuperCPU can use the first three BA cycles, 6510 can't because RDY is ignored during "Write" cycles
            if (sysTimer.delay( &applyBuffer ) <= 1) {
                system->ram[writeBuffer.addr] = writeBuffer.value;
                writeBuffer.inProgress = false;
                sysTimer.remove(&applyBuffer);
                break;
            }
        }
        syncStock();
    } while(true);

    cycles = cycleCacheLatch;
}

auto SuperCpu::clockStretchWriteInternal(uint16_t addr, uint8_t value) -> void {
    if (fastMode) {
        if (writeBuffer.inProgress) {
            waitForWriteBuffer();
        } else {
            cycles += frequency;
            if (cycles >= SCPU_FREQ) {
                cycles -= SCPU_FREQ;
                syncStock();
            }
        }

        writeBuffer.addr = addr;
        writeBuffer.value = value;
        writeBuffer.inProgress = true;
        // change will be applied in the next C64 cycle, if happens up to ~70 ns before C64 cycles ends.
        // cycle 1: not this cycle
        // extra cycle: it happens one cycle later if the request is made very late and can no longer be clocked in time in the following host cycle
        // cycle 2: write here, but read/write is expected to happen after VIC, SID, CIA processes
        // cycle 3: do the "write" at the end of cycle before
        sysTimer.add( &applyBuffer, cycles > cycleWriteThroughLimit ? 4 : 3);
    } else {
        syncStock();
        system->ram[addr] = value;
    }
}

auto SuperCpu::observeIrq(bool state) -> void {
    setIrqLineLow(state);
}

auto SuperCpu::observeNmi(bool state) -> void {
    setNmiLineLow(state);
}

auto SuperCpu::observeRdy(bool state) -> void {
    if (state) {
        baFlags = 1;
        sysTimer.add( &baStart, 4, Emulator::SystemTimer::UpdateExisting);
    } else
        baFlags = 0;
}

inline auto SuperCpu::syncStock() -> void {
    sysTimer.process();
    cia1.clock();
    vicII->clock();
    cia2.clock();
}

auto SuperCpu::setJumper(unsigned jumperId, bool state) -> void {
    if (jumperId == 0) {
        jumper1Mhz = !state;
        updateFastmode(false);
        cycles = 0;
    }
    else if (jumperId == 1)
        jumperJiffyDos = state;
}

auto SuperCpu::getJumper(unsigned jumperId) -> bool {
    if (jumperId == 0) return !jumper1Mhz;
    if (jumperId == 1) return jumperJiffyDos;
    return false;
}

auto SuperCpu::serialize(Emulator::Serializer& s) -> void {
    bool light = s.lightUsage();
    unsigned _dramSize = dramSize;
    s.integer(dramSize);

    if ( s.mode() == Emulator::Serializer::Mode::Load ) {
        if (light)
            dramTracker.applyAndDisable(dram);
        else {
            if (!dram || (_dramSize != dramSize)) {
                if (dram) {
                    delete[] dram;
                    dram = nullptr;
                }
                setDram();
            }
        }
    } else {
        if (light)
            dramTracker.enable();
    }

    if (!light)
        s.array( dram, dramSize - ( ((dramSize >> 20) == 16) ? (512 * 1024) : 0) );

    s.array( sram, 128 * 1024 );
    s.integer(optim);
    s.integer(dramMask);
    s.integer(dramPageSize);
    s.integer(romMask);
    s.integer(frequency);
    s.integer(cycles);
    s.integer(jumperJiffyDos);
    s.integer(jumper1Mhz);
    s.integer(software1Mhz);
    s.integer(system1Mhz);
    s.integer(fastMode);
    s.integer(baFlags);
    s.integer(memConf);
    s.integer(mirrorMemConf);
    s.integer(dramAddr);
    s.integer(dramRowMask);
    s.integer(dramConfSize);
    s.integer(dramConfPageSize);
    s.integer(cycleCacheLatch);
    s.integer(cycleIoAccess);
    s.integer(cycleIoLong);
    s.integer(cycleWriteThroughLimit);
    s.integer(writeBuffer.addr);
    s.integer(writeBuffer.value);
    s.integer(writeBuffer.inProgress);

    // 65816 CPU
    s.integer(pc);
    s.integer(a);
    s.integer(x);
    s.integer(y);
    s.integer(this->s);
    s.integer(d);
    s.integer(pbr);
    s.integer(dbr);
    s.integer((uint8_t&)p);
    s.integer(modeE);
    s.integer(control);
    s.integer(lines);
}

}
