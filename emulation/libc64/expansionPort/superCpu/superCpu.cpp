
#include <cstring>
#include "superCpu.h"
#include "../../system/system.h"
#include "../reu/reu.h"
#include "../../traps/traps.h"

#define SCPU_FREQ 20'000'000

// RDY NMOS 6502, 6510: Write cycles don't hold CPU
// RDY CMOS 65C02, 65816: Read and Write cycles hold CPU
// todo: find out if RDY input or clock stretching is used for slow mode and/or VIC BA
// todo: find out if incoming Interrupts are checked in clock stretched IRQ sample cycles (usually the last one)
// hint: RDY repeats cycle and checks for incoming Interrupts (in case of IRQ sample cycle)

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
            if (writeBuffer.colram)
                this->system->colorRam[writeBuffer.addr] = writeBuffer.value;
            else
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
    jumperDramBoost = false;
    jumper1Mhz = false;
    dma = true;

    watchPoints.expressionCallback = [this](const std::string& input, int& pos) {
        return parseExpressionValue(input, pos);
    };
    watchPointsWrite.expressionCallback = [this](const std::string& input, int& pos) {
        return parseExpressionValue(input, pos);
    };
    breakPoints.expressionCallback = [this](const std::string& input, int& pos) {
        return parseExpressionValue(input, pos);
    };
    exceptionPoints.expressionCallback = [this](const std::string& input, int& pos) {
        return parseExpressionValue(input, pos);
    };
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
    if (expander)
        expander->reset(softReset);
    power();
    frequencyDRAM = frequency = vicII->frequency();
    if (jumperDramBoost)
        frequencyDRAM >>= 3;
    dramMask = dramSize ? (dramSize - 1) : 0;
    randomizer.initXorShift(0x1234abcd);
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

    // Host clock
    float nsCycle = 1'000'000'000.0 / (float)frequency;
    float nsMemoryCacheLatch = nsCycle / 2.0 + 90.0 - 65.0;
    float nsIoAccess = nsCycle / 2.0 + 105.0 - 80.0;
    float nsCycleWriteLimit = nsCycle - 70.0 - 31.0;

    cycleCacheLatch = (unsigned)((float)SCPU_FREQ * (nsMemoryCacheLatch / nsCycle));
    cycleIoAccess = (unsigned)((float)SCPU_FREQ * (nsIoAccess / nsCycle));
    cycleWriteThroughLimit = (unsigned)((float)SCPU_FREQ * (nsCycleWriteLimit / nsCycle));

    // DOT clock
    float nscycleDot = 1'000'000'000.0 / ((float)frequency * 8.0);
    float nsCycleIoLong = 7.0 * nscycleDot - 86.0;
    cycleIoLong = (unsigned)((float)SCPU_FREQ * (nsCycleIoLong / nsCycle));

    memChange = nullptr;
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
                        if (page == 0) {
                            writeTable[SC_MAP_WR] = &SuperCpu::writeSramAndOrHostRam<false, MODE_INTERNAL, PAGE_ZERO>;
                            writeTableReu[SC_MAP_WR] = &SuperCpu::writeSramAndOrHostRam<true, MODE_INTERNAL, PAGE_ZERO>;
                        } else if (page == 0xff) {
                            writeTable[SC_MAP_WR] = &SuperCpu::writeSramAndOrHostRam<false, MODE_INTERNAL, PAGE_HI>;
                            writeTableReu[SC_MAP_WR] = &SuperCpu::writeSramAndOrHostRam<true, MODE_INTERNAL, PAGE_HI>;
                        } else {
                            writeTable[SC_MAP_WR] = &SuperCpu::writeSramAndOrHostRam<false, MODE_INTERNAL, PAGE_REGULAR>;
                            writeTableReu[SC_MAP_WR] = &SuperCpu::writeSramAndOrHostRam<true, MODE_INTERNAL, PAGE_REGULAR>;
                        }
                    }
                } else {
                    for (int m = 0; m < 9; m++) {
                        if (mirrors[m][1] && (page >= mirrors[m][0] && page <= mirrors[m][1])) {
                            if (page == 0) {
                                writeTable[SC_MAP_WR] = &SuperCpu::writeSramAndOrHostRam<false, MODE_MIRROR, PAGE_ZERO>;
                                writeTableReu[SC_MAP_WR] = &SuperCpu::writeSramAndOrHostRam<true, MODE_MIRROR, PAGE_ZERO>;
                            } else if (page == 0xff) {
                                writeTable[SC_MAP_WR] = &SuperCpu::writeSramAndOrHostRam<false, MODE_MIRROR, PAGE_HI>;
                                writeTableReu[SC_MAP_WR] = &SuperCpu::writeSramAndOrHostRam<true, MODE_MIRROR, PAGE_HI>;
                            } else {
                                writeTable[SC_MAP_WR] = &SuperCpu::writeSramAndOrHostRam<false, MODE_MIRROR, PAGE_REGULAR>;
                                writeTableReu[SC_MAP_WR] = &SuperCpu::writeSramAndOrHostRam<true, MODE_MIRROR, PAGE_REGULAR>;
                            }
                        } else {
                            if (page == 0) {
                                writeTable[SC_MAP_WR] = &SuperCpu::writeSramAndOrHostRam<false, MODE_SRAM, PAGE_ZERO>;
                                writeTableReu[SC_MAP_WR] = &SuperCpu::writeSramAndOrHostRam<true, MODE_SRAM, PAGE_ZERO>;
                            } else if (page == 0xff) {
                                writeTable[SC_MAP_WR] = &SuperCpu::writeSramAndOrHostRam<false, MODE_SRAM, PAGE_HI>;
                                writeTableReu[SC_MAP_WR] = &SuperCpu::writeSramAndOrHostRam<true, MODE_SRAM, PAGE_HI>;
                            } else {
                                writeTable[SC_MAP_WR] = &SuperCpu::writeSramAndOrHostRam<false, MODE_SRAM, PAGE_REGULAR>;
                                writeTableReu[SC_MAP_WR] = &SuperCpu::writeSramAndOrHostRam<true, MODE_SRAM, PAGE_REGULAR>;
                            }
                        }
                    }
                }

                if (page <= 0x0f) {
                    if (conf == 7)          {
                        readTable[SC_MAP] = &SuperCpu::readC64Ram;
                        peekTable[SC_MAP] = &SuperCpu::readC64Ram<true>;
                        readTableReu[SC_MAP] = &SuperCpu::readC64Ram<true>;
                    } else {
                        readTable[SC_MAP] = &SuperCpu::readSramB0;
                        peekTable[SC_MAP] = &SuperCpu::readSramB0<true>;
                        readTableReu[SC_MAP] = &SuperCpu::readSramB0<true>;
                    }
                } else if (page <= 0x5f) {
                    if (conf == 7) {
                        readTable[SC_MAP] = &SuperCpu::readC64Ram;
                        peekTable[SC_MAP] = &SuperCpu::readC64Ram<true>;
                        readTableReu[SC_MAP] = &SuperCpu::readC64Ram<true>;
                    } else if (conf & 2) {
                        readTable[SC_MAP] = &SuperCpu::readSramB1;
                        peekTable[SC_MAP] = &SuperCpu::readSramB1<true>;
                        readTableReu[SC_MAP] = &SuperCpu::readSramB1<true>;
                    } else {
                        readTable[SC_MAP] = &SuperCpu::readSramB0;
                        peekTable[SC_MAP] = &SuperCpu::readSramB0<true>;
                        readTableReu[SC_MAP] = &SuperCpu::readSramB0<true>;
                    }
                } else if (page <= 0x7f) {
                    if (conf == 7) {
                        readTable[SC_MAP] = &SuperCpu::readC64Ram;
                        peekTable[SC_MAP] = &SuperCpu::readC64Ram<true>;
                        readTableReu[SC_MAP] = &SuperCpu::readC64Ram<true>;
                    } else {
                        readTable[SC_MAP] = &SuperCpu::readSramB0;
                        peekTable[SC_MAP] = &SuperCpu::readSramB0<true>;
                        readTableReu[SC_MAP] = &SuperCpu::readSramB0<true>;
                    }
                } else if (page <= 0x9f) {
                    if (conf == 7) {
                        readTable[SC_MAP] = &SuperCpu::readC64Ram;
                        peekTable[SC_MAP] = &SuperCpu::readC64Ram<true>;
                        readTableReu[SC_MAP] = &SuperCpu::readC64Ram<true>;
                    } else if (conf & 4) {
                        readTable[SC_MAP] = &SuperCpu::readRom;
                        peekTable[SC_MAP] = &SuperCpu::readRom<true>;
                        readTableReu[SC_MAP] = &SuperCpu::readRom<true>;
                    } else if (conf & 2) {
                        readTable[SC_MAP] = &SuperCpu::readSramB1;
                        peekTable[SC_MAP] = &SuperCpu::readSramB1<true>;
                        readTableReu[SC_MAP] = &SuperCpu::readSramB1<true>;
                    } else {
                        readTable[SC_MAP] = &SuperCpu::readSramB0;
                        peekTable[SC_MAP] = &SuperCpu::readSramB0<true>;
                        readTableReu[SC_MAP] = &SuperCpu::readSramB0<true>;
                    }
                } else if (page <= 0xbf) {
                    if (conf & 4) {
                        readTable[SC_MAP] = &SuperCpu::readRom;
                        peekTable[SC_MAP] = &SuperCpu::readRom<true>;
                        readTableReu[SC_MAP] = &SuperCpu::readRom<true>;
                    } else if ((mem & 3) == 3) {
                        readTable[SC_MAP] = &SuperCpu::readSramB1;
                        peekTable[SC_MAP] = &SuperCpu::readSramB1<true>;
                        readTableReu[SC_MAP] = &SuperCpu::readSramB1<true>;
                    } else {
                        readTable[SC_MAP] = &SuperCpu::readSramB0;
                        peekTable[SC_MAP] = &SuperCpu::readSramB0<true>;
                        readTableReu[SC_MAP] = &SuperCpu::readSramB0<true>;
                    }
                } else if (page <= 0xcf) {
                    if (conf == 7) {
                        readTable[SC_MAP] = &SuperCpu::readC64Ram;
                        peekTable[SC_MAP] = &SuperCpu::readC64Ram<true>;
                        readTableReu[SC_MAP] = &SuperCpu::readC64Ram<true>;
                    } else if (conf & 4) {
                        readTable[SC_MAP] = &SuperCpu::readRom;
                        peekTable[SC_MAP] = &SuperCpu::readRom<true>;
                        readTableReu[SC_MAP] = &SuperCpu::readRom<true>;
                    } else {
                        readTable[SC_MAP] = &SuperCpu::readSramB0;
                        peekTable[SC_MAP] = &SuperCpu::readSramB0<true>;
                        readTableReu[SC_MAP] = &SuperCpu::readSramB0<true>;
                    }
                } else if (page <= 0xdf) {
                    if ((mem & 3) == 0) {
                        if (conf == 7) {
                            readTable[SC_MAP] = &SuperCpu::readSramChar;
                            peekTable[SC_MAP] = &SuperCpu::readSramChar<true>;
                            readTableReu[SC_MAP] = &SuperCpu::readSramChar<true>;
                        } else if (conf & 4) {
                            readTable[SC_MAP] = &SuperCpu::readRom;
                            peekTable[SC_MAP] = &SuperCpu::readRom<true>;
                            readTableReu[SC_MAP] = &SuperCpu::readRom<true>;
                        } else {
                            readTable[SC_MAP] = &SuperCpu::readSramB0;
                            peekTable[SC_MAP] = &SuperCpu::readSramB0<true>;
                            readTableReu[SC_MAP] = &SuperCpu::readSramB0<true>;
                        }
                    } else if ((mem & 4) == 0) {
                        readTable[SC_MAP] = &SuperCpu::readSramChar;
                        peekTable[SC_MAP] = &SuperCpu::readSramChar<true>;
                        readTableReu[SC_MAP] = &SuperCpu::readSramChar<true>;
                    } else {
                        if (page == 0xd0) {
                            readTable[SC_MAP] = &SuperCpu::readIoSCPU;
                            peekTable[SC_MAP] = &SuperCpu::peekIoSCPU;
                            readTableReu[SC_MAP] = &SuperCpu::readIoSCPU<true>;
                            SC_NO_MIRROR {
                                writeTable[SC_MAP_WR] = &SuperCpu::writeIoSCPU;
                                writeTableReu[SC_MAP_WR] = &SuperCpu::writeIoSCPU<true>;
                            }
                        } else if (page == 0xd1) {
                            readTable[SC_MAP] = &SuperCpu::readIoVIC;
                            peekTable[SC_MAP] = &SuperCpu::peekIoVIC;
                            readTableReu[SC_MAP] = &SuperCpu::readIoVIC<true>;
                            SC_NO_MIRROR {
                                writeTable[SC_MAP_WR] = &SuperCpu::writeIoVIC;
                                writeTableReu[SC_MAP_WR] = &SuperCpu::writeIoVIC<true>;
                            }
                        } else if (page <= 0xd3) {
                            readTable[SC_MAP] = &SuperCpu::readSramB1;
                            peekTable[SC_MAP] = &SuperCpu::readSramB1<true>;
                            readTableReu[SC_MAP] = &SuperCpu::readSramB1<true>;
                            SC_NO_MIRROR {
                                writeTable[SC_MAP_WR] = &SuperCpu::writeIoStrange;
                                writeTableReu[SC_MAP_WR] = &SuperCpu::writeIoStrange<true>;
                            }
                        } else if (page <= 0xd5) {
                            readTable[SC_MAP] = &SuperCpu::readIoSID;
                            peekTable[SC_MAP] = &SuperCpu::peekIoSID;
                            readTableReu[SC_MAP] = &SuperCpu::readIoSID<true>;
                            SC_NO_MIRROR {
                                writeTable[SC_MAP_WR] = &SuperCpu::writeIoSID<true>;
                                writeTableReu[SC_MAP_WR] = &SuperCpu::writeIoSID<true, true>;
                            }
                        } else if (page <= 0xd7) {
                            readTable[SC_MAP] = &SuperCpu::readIoSID;
                            peekTable[SC_MAP] = &SuperCpu::peekIoSID;
                            readTableReu[SC_MAP] = &SuperCpu::readIoSID<true>;
                            SC_NO_MIRROR {
                                writeTable[SC_MAP_WR] = &SuperCpu::writeIoSID<false>;
                                writeTableReu[SC_MAP_WR] = &SuperCpu::writeIoSID<false, true>;
                            }
                        } else if (page <= 0xdb) {
                            if (conf == 7) {
                                readTable[SC_MAP] = &SuperCpu::readIoColorRamInternal;
                                peekTable[SC_MAP] = &SuperCpu::readIoColorRamInternal<true>;
                                readTableReu[SC_MAP] = &SuperCpu::readIoColorRamInternal<true>;
                                SC_NO_MIRROR {
                                    writeTable[SC_MAP_WR] = &SuperCpu::writeIoColorRam<false>;
                                    writeTableReu[SC_MAP_WR] = &SuperCpu::writeIoColorRam<false, true>;
                                }
                            } else {
                                readTable[SC_MAP] = &SuperCpu::readSramB1;
                                peekTable[SC_MAP] = &SuperCpu::readSramB1<true>;
                                readTableReu[SC_MAP] = &SuperCpu::readSramB1<true>;
                                SC_NO_MIRROR {
                                    writeTable[SC_MAP_WR] = &SuperCpu::writeIoColorRam<true>;
                                    writeTableReu[SC_MAP_WR] = &SuperCpu::writeIoColorRam<true, true>;
                                }
                            }
                        } else if (page == 0xdc) {
                            readTable[SC_MAP] = &SuperCpu::readIoCIA1;
                            peekTable[SC_MAP] = &SuperCpu::peekIoCIA1;
                            readTableReu[SC_MAP] = &SuperCpu::readIoCIA1<true>;
                            SC_NO_MIRROR {
                                writeTable[SC_MAP_WR] = &SuperCpu::writeIoCIA1;
                                writeTableReu[SC_MAP_WR] = &SuperCpu::writeIoCIA1<true>;
                            }
                        } else if (page == 0xdd) {
                            readTable[SC_MAP] = &SuperCpu::readIoCIA2;
                            peekTable[SC_MAP] = &SuperCpu::peekIoCIA2;
                            readTableReu[SC_MAP] = &SuperCpu::readIoCIA2<true>;
                            SC_NO_MIRROR {
                                writeTable[SC_MAP_WR] = &SuperCpu::writeIoCIA2;
                                writeTableReu[SC_MAP_WR] = &SuperCpu::writeIoCIA2<true>;
                            }
                        } else if (page == 0xde) {
                            readTable[SC_MAP] = &SuperCpu::readIo1;
                            peekTable[SC_MAP] = &SuperCpu::peekIo1;
                            readTableReu[SC_MAP] = &SuperCpu::readIo1<true>;
                            SC_NO_MIRROR {
                                writeTable[SC_MAP_WR] = &SuperCpu::writeIo1;
                                writeTableReu[SC_MAP_WR] = &SuperCpu::writeIo1<true>;
                            }
                        } else if (page == 0xdf) {
                            readTable[SC_MAP] = &SuperCpu::readIo2;
                            peekTable[SC_MAP] = &SuperCpu::peekIo2;
                            readTableReu[SC_MAP] = &SuperCpu::readIo2<true>;
                            SC_NO_MIRROR {
                                writeTable[SC_MAP_WR] = &SuperCpu::writeIo2;
                                writeTableReu[SC_MAP_WR] = &SuperCpu::writeIo2<true>;
                            }
                        }
                    }
                } else if (page <= 0xff) {
                    if (conf & 4) {
                        readTable[SC_MAP] = &SuperCpu::readRom;
                        peekTable[SC_MAP] = &SuperCpu::readRom<true>;
                        readTableReu[SC_MAP] = &SuperCpu::readRom<true>;
                    } else if (mem & 2) {
                        if (conf & 1) {
                            readTable[SC_MAP] = &SuperCpu::readSramKernal;
                            peekTable[SC_MAP] = &SuperCpu::readSramKernal<true>;
                            readTableReu[SC_MAP] = &SuperCpu::readSramKernal<true>;
                        } else {
                            readTable[SC_MAP] = &SuperCpu::readSramB1;
                            peekTable[SC_MAP] = &SuperCpu::readSramB1<true>;
                            readTableReu[SC_MAP] = &SuperCpu::readSramB1<true>;
                        }
                    } else {
                        readTable[SC_MAP] = &SuperCpu::readSramB0;
                        peekTable[SC_MAP] = &SuperCpu::readSramB0<true>;
                        readTableReu[SC_MAP] = &SuperCpu::readSramB0<true>;
                    }
                }
            }
        }
    }
}

inline auto SuperCpu::readVectorByte(uint16_t addr) -> uint8_t {
    ReadTable ptr = readTable[memConf | (addr & 0xff00)];

    if (ptr == &SuperCpu::readSramB1<false> || ptr == &SuperCpu::readSramKernal<false>) {
        if ((memConf & (HW_ENABLE | DOS_EXT) ) || !emulationMode() || system1Mhz) {
            clockStretchRom();
            return rom[addr & romMask];
        }
    }
    return (this->*ptr)(addr);
}

auto SuperCpu::executeKernalRom() -> bool {
    if (pc & 0xff0000)
        return false;

    ReadTable ptr = readTable[memConf | (pc & 0xff00)];
    if (ptr == &SuperCpu::readSramKernal<false>)
        return true;

    if (((pc >> 8) >= 0xe0) && ptr == &SuperCpu::readSramB1<false>)
        return true;
    return false;
}

auto SuperCpu::executeHostRam() -> bool {
    if (pc & 0xff0000)
        return false;

    ReadTable ptr = readTable[memConf | (pc & 0xff00)];
    return ptr == &SuperCpu::readSramB0<false> || ptr == &SuperCpu::readC64Ram<false>;
}

auto SuperCpu::readByteReu(uint16_t addr) -> uint8_t {
    return (this->*readTableReu[memConf | (addr & 0xff00)])(addr);
}

auto SuperCpu::writeByteReu(uint16_t addr, uint8_t value) -> void {
    (this->*writeTableReu[mirrorMemConf | (addr & 0xff00)])(addr, value);
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

auto SuperCpu::peekByte(uint32_t addr) -> uint8_t {
    if ((addr & 0xff0000) == 0) { // bank 0 SRAM
        return (this->*peekTable[memConf | (addr & 0xff00)])(addr);
    } if ((addr & 0xff0000) == 0x10000) { // bank 1 SRAM
        return (addr & 0xfffe) ? sram[addr] : sram[addr & 1]; // will be mapped to bank 0

    } if ((addr & 0xf80000) == 0xf80000) { // ROM can't be switched out
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
            return addr >> 16;
        }

        return dram[addr & dramMask];
    }

    return addr >> 16;
}

auto SuperCpu::memoryDumpPage(uint8_t page, uint8_t* dump) -> void {
    page <<= 4;
    for (unsigned addr = page << 8; addr <= ((page << 8) | 0xfff); addr++ )
        *dump++ = (this->*peekTable[memConf | (addr & 0xff00)])(addr);
}

auto SuperCpu::memoryDumpBank(uint8_t bank, uint8_t* dump) -> void {
    if (bank == 0) {
        for (unsigned addr = 0; addr < 0x10000; addr++)
            *dump++ = (this->*peekTable[memConf | (addr & 0xff00)])(addr);

    } else if (bank == 1) {
        *dump++ = sram[0];
        *dump++ = sram[1];
        for (unsigned addr = (1 << 16) | 2; addr <= ((1 << 16) | 0xffff); addr++)
            *dump++ = sram[addr];

    } else if ((bank & 0xf8) == 0xf8) { // f8 - ff (512k max)
        for (unsigned addr = (bank << 16); addr <= ((bank << 16) | 0xffff); addr++)
            *dump++ = rom[addr & romMask];

    } else if (dram) {
        bool _mismatch = dramPageSize != dramConfPageSize;
        unsigned _addr;

        if ((bank & 0xfe) == 0xf6) {
            for (unsigned addr = (bank << 16); addr <= ((bank << 16) | 0xffff); addr++) {
                _addr = addr;
                if (_mismatch)
                    _addr = ((_addr >> dramConfPageSize) << dramPageSize) | (_addr & ((1 << dramPageSize) - 1));

                _addr &= 0x1ffff; // map banks $f6/$f7 to banks 0/1 of DRAM

                *dump++ = dram[_addr & dramMask];
            }
        } else if (((bank << 16) | 0xffff) < dramConfSize) {
            for (unsigned addr = (bank << 16); addr <= ((bank << 16) | 0xffff); addr++) {
                _addr = addr;
                if (_mismatch)
                    _addr = ((_addr >> dramConfPageSize) << dramPageSize) | (_addr & ((1 << dramPageSize) - 1));

                *dump++ = dram[_addr & dramMask];
            }
        } else {
            std::memset(dump, bank, 0x10000);
        }
    } else {
        std::memset(dump, bank, 0x10000);
    }
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
        clockStretchRom<true>();
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
        if (memChange)
            memChange->remember(addr & dramMask);
        dram[addr & dramMask] = value;
    } else
        stepCycle();
}

inline auto SuperCpu::idleCycle(uint32_t addr) -> void {
    // unlike NMOS/CMOS 6502 the 65816 has VDA/VPA lines to inform BUS participants about idle cycles.
    // this way there is no need to waste cycles for slow memory accesses
    // todo: find out if SuperCPU checks this
    // if (fastMode) {
    //     cycles += frequency;
    //     if (cycles >= SCPU_FREQ) {
    //         cycles -= SCPU_FREQ;
    //         syncStock();
    //     }
    // } else
        // and/or if E-Line of 65816 signals emulation mode?
        readByte(addr);
}

inline auto SuperCpu::trapHandler() -> bool {
    return traps.handler();
}

template<bool peekAccess> auto SuperCpu::readC64Ram(uint16_t addr) -> uint8_t {
    if constexpr (!peekAccess)
        clockStretchIORead();        
    return system->ram[addr];
}

template<bool peekAccess> auto SuperCpu::readSramB0(uint16_t addr) -> uint8_t {
    if constexpr (!peekAccess)
        stepCycle<true>();
    return sram[addr];
}

template<bool peekAccess> auto SuperCpu::readSramB1(uint16_t addr) -> uint8_t {
    if constexpr (!peekAccess)
        stepCycle<true>();
    return sram[0x10000 | addr];
}

template<bool peekAccess> auto SuperCpu::readSramChar(uint16_t addr) -> uint8_t {
    if constexpr (!peekAccess)
        stepCycle<true>();
    return system->charRom[addr & 0xfff];
}

template<bool peekAccess> auto SuperCpu::readSramKernal(uint16_t addr) -> uint8_t {
    if constexpr (!peekAccess)
        stepCycle<true>();
    return sram[0x8000 + addr];
}

auto SuperCpu::peekKernal(uint16_t addr) -> uint8_t {
    return memConf & HW_ENABLE ? readSramKernal<true>( addr ) : readSramB1<true>( addr );
}

template<bool peekAccess> auto SuperCpu::readRom(uint16_t addr) -> uint8_t {
    if constexpr (!peekAccess)
        clockStretchRom();    
    return rom[addr & romMask];
}

template<bool reuAccess, uint8_t mode, uint8_t addrArea> auto SuperCpu::writeSramAndOrHostRam(uint16_t addr, uint8_t value) -> void {
    if constexpr (mode == MODE_MIRROR || mode == MODE_INTERNAL) {
        if constexpr (addrArea == PAGE_HI) {
            if (addr == 0xff00) {
                if constexpr (reuAccess) {
                    system->ram[addr] = value;
                } else {
                    clockStretchIOWriteLong();
                    system->ram[addr] = value;
                    if (fastMode)
                        syncStockIO();

                    Reu* reu = dynamic_cast<Reu*>(expander);
                    if (reu) {
                        if (reu->waitForStart) {
                            reu->setDma();
                            reu->waitForStart = false;
                            do {
                                syncStock();
                                reu->clockSCPU();
                            } while (reu->dma);
                        }
                    }
                }
            } else {
                if constexpr (reuAccess)
                    system->ram[addr] = value;
                else
                    clockStretchWriteInternal(addr, value);
            }
        } else {
            if constexpr (reuAccess) {
                system->ram[addr] = value;
            } else
                clockStretchWriteInternal(addr, value);
        }
    } else if constexpr (!reuAccess && (mode == MODE_SRAM))
        stepCycle();

    if constexpr (mode == MODE_SRAM || mode == MODE_MIRROR)
        sram[addr] = value;

    if constexpr (!reuAccess && (addrArea == PAGE_ZERO)) { // templated for performance reasons: following "if" is checked only when the page is zero.
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

auto SuperCpu::peekIoSCPU(uint16_t addr) -> uint8_t {
    if ((addr & 0xfff0) == 0xd0b0) {
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

    return vicII->peekReg( addr & 0xff );
}

template<bool reuAccess> auto SuperCpu::readIoSCPU(uint16_t addr) -> uint8_t {
    if ((addr & 0xfff0) == 0xd0b0) {
        if constexpr (!reuAccess)
            stepCycle<true, true>();

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
    if constexpr (!reuAccess)
        clockStretchIORead();
    return vicII->readReg( addr & 0xff );
}

template<bool reuAccess> auto SuperCpu::readIoVIC(uint16_t addr) -> uint8_t {
    if constexpr (!reuAccess)
        clockStretchIORead();
    return vicII->readReg( addr & 0xff );
}

auto SuperCpu::peekIoVIC(uint16_t addr) -> uint8_t {
    return vicII->peekReg( addr & 0xff );
}

template<bool reuAccess> auto SuperCpu::readIoSID(uint16_t addr) -> uint8_t {
    if constexpr (!reuAccess)
        clockStretchIORead();
    return sidManager.readSidReg(addr);
}

auto SuperCpu::peekIoSID(uint16_t addr) -> uint8_t {
    return sidManager.peekSidReg(addr);
}

template<bool peekAccess> auto SuperCpu::readIoColorRamInternal(uint16_t addr) -> uint8_t {
    if constexpr (!peekAccess)
        clockStretchIORead();
    return vicII->lastReadPhase1();
}

template<bool reuAccess> auto SuperCpu::readIoCIA1(uint16_t addr) -> uint8_t {
    if constexpr (reuAccess)
        return cia1.read(addr);

    bool postSync = clockStretchIOReadCIA();
    uint8_t out = cia1.read( addr );
    if (postSync)
        syncStockIO();
    return out;
}

auto SuperCpu::peekIoCIA1(uint16_t addr) -> uint8_t {
    return cia1.peek(addr);
}

template<bool reuAccess> auto SuperCpu::readIoCIA2(uint16_t addr) -> uint8_t {
    if constexpr (reuAccess)
        return cia2.read(addr);

    bool postSync = clockStretchIOReadCIA();
    uint8_t out = cia2.read( addr );
    if (postSync)
        syncStockIO();
    return out;
}

auto SuperCpu::peekIoCIA2(uint16_t addr) -> uint8_t {
    return cia2.peek(addr);
}

template<bool reuAccess> auto SuperCpu::readIo1(uint16_t addr) -> uint8_t {
    if constexpr (!reuAccess)
        clockStretchIORead();
    uint8_t value;
    if (sidManager.readIo(addr, value))
        return value;

    return ExpansionPort::readIo1(addr);
}

 auto SuperCpu::peekIo1(uint16_t addr) -> uint8_t {
    uint8_t value;
    if (sidManager.peekIo(addr, value))
        return value;

    return ExpansionPort::peekIo1(addr);
}

template<bool reuAccess> auto SuperCpu::readIo2(uint16_t addr) -> uint8_t {
    if constexpr (!reuAccess)
        clockStretchIORead();
    uint8_t value;
    if (sidManager.readIo(addr, value))
        return value;

    if (expander)
        return expander->readIo2(addr);

    return ExpansionPort::readIo2(addr);
}

auto SuperCpu::peekIo2(uint16_t addr) -> uint8_t {
    uint8_t value;
    if (sidManager.peekIo(addr, value))
        return value;

    if (expander)
        return expander->peekIo2(addr);

    return ExpansionPort::peekIo2(addr);
}

template<bool reuAccess> auto SuperCpu::writeIoSCPU(uint16_t addr, uint8_t value) -> void {
    sram[0x10000 | addr] = value;

    if ((addr >= 0xd071 && addr < 0xd080) || (addr >= 0xd0b0 && addr < 0xd0c0)) {
        if constexpr (!reuAccess)
            stepCycle<false, true>();

        switch (addr) {
            case 0xd072:
                if (!system1Mhz) {
                    system1Mhz = true;
                    updateFastmode<reuAccess>();
                } break;
            case 0xd073:
                if (system1Mhz) {
                    system1Mhz = false;
                    updateFastmode<reuAccess>();
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
                    updateFastmode<reuAccess>();
                } break;
            case 0xd079:
            case 0xd07b:
                if (software1Mhz) {
                    software1Mhz = false;
                    updateFastmode<reuAccess>();
                } break;
            case 0xd07e:
                if((memConf & HW_ENABLE) == 0) {
                    memConf |= HW_ENABLE;
                    mirrorMemConf |= HW_ENABLE;
                    updateFastmode<reuAccess>();
                } break;
            case 0xd07d:
            case 0xd07f:
                if (memConf & HW_ENABLE) {
                    memConf &= ~HW_ENABLE;
                    mirrorMemConf &= ~HW_ENABLE;
                    updateFastmode<reuAccess>();
                } break;
            case 0xd0b2:
                if (memConf & HW_ENABLE) {
                    system1Mhz = !!(value & 0x40);
                    if ((value & 0x80) == 0) {
                        memConf &= ~HW_ENABLE;
                        mirrorMemConf &= ~HW_ENABLE;
                    }
                    updateFastmode<reuAccess>();
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
                    updateFastmode<reuAccess>();
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
        if constexpr (reuAccess) {
            vicII->writeReg(addr & 0xff, value);
        } else {
            clockStretchIOWrite();
            vicII->writeReg( addr & 0xff, value );
            if (fastMode)
                syncStockIO();
        }
    }
}

template<bool reuAccess> auto SuperCpu::writeIoVIC(uint16_t addr, uint8_t value) -> void {
    if constexpr (reuAccess) {
        sram[0x10000 | addr] = value;
        vicII->writeReg(addr & 0xff, value);
    } else {
        clockStretchIOWrite();
        sram[0x10000 | addr] = value;
        vicII->writeReg( addr & 0xff, value );
        if (fastMode)
            syncStockIO();
    }
}

template<bool reuAccess> auto SuperCpu::writeIoStrange(uint16_t addr, uint8_t value) -> void {
    if constexpr (!reuAccess)
        stepCycle();
    if ((memConf & HW_ENABLE) || (addr == 0xd27e))
        sram[0x10000 | addr] = value;
}

template<bool withSram, bool reuAccess> auto SuperCpu::writeIoSID(uint16_t addr, uint8_t value) -> void {
    if constexpr (withSram)
        sram[0x10000 | addr] = value;

    if constexpr (reuAccess) {
        sidManager.writeSidReg(addr, value);
    } else {
        clockStretchIOWrite();
        sidManager.writeSidReg(addr, value);
        if (fastMode)
            syncStockIO();
    }
}

template<bool withSram, bool reuAccess> auto SuperCpu::writeIoColorRam(uint16_t addr, uint8_t value) -> void {
    if constexpr (reuAccess) {
        system->colorRam[addr & 0x3ff] = value & 0xf;
    } else
        clockStretchWriteInternal<true>(addr & 0x3ff, value & 0xf);
    
    if constexpr (withSram)
        sram[0x10000 | addr] = value;
}

template<bool reuAccess> auto SuperCpu::writeIoCIA1(uint16_t addr, uint8_t value) -> void {
    if constexpr (!reuAccess)
        clockStretchIOWriteCia();
    sram[0x10000 | addr] = value;
    cia1.write( addr, value );

    if constexpr (!reuAccess) {
        if (fastMode)
            syncStockIO();
    }
}

template<bool reuAccess> auto SuperCpu::writeIoCIA2(uint16_t addr, uint8_t value) -> void {
    if constexpr (!reuAccess)
        clockStretchIOWriteCia();
    sram[0x10000 | addr] = value;
    cia2.write( addr, value );

    if constexpr (!reuAccess) {
        if (fastMode)
            syncStockIO();
    }
}

template<bool reuAccess> auto SuperCpu::writeIo1(uint16_t addr, uint8_t value) -> void {
    if constexpr (!reuAccess)
        clockStretchIOWrite();
    sidManager.writeIo( addr, value );
    ExpansionPort::writeIo1(addr, value);

    if constexpr (!reuAccess) {
        if (fastMode)
            syncStockIO();
    }
}

template<bool reuAccess> auto SuperCpu::writeIo2(uint16_t addr, uint8_t value) -> void {
    if constexpr (reuAccess) {
        sidManager.writeIo(addr, value);
        ExpansionPort::writeIo2(addr, value);

    } else {
        if (addr == 0xdf01 || addr == 0xdf21)
            clockStretchIOWriteLong();
        else
            clockStretchIOWrite();

        sidManager.writeIo( addr, value );

        if (expander) {
            expander->writeIo2(addr, value);

            Reu* reu = dynamic_cast<Reu*>(expander);
            if (reu) {
                if (sysTimer.has(&reu->setDma)) {
                    do {
                        syncStock();                        
                        reu->clockSCPU();
                    } while (reu->dma);
                }
            }
        } else {
            ExpansionPort::writeIo2(addr, value);

            if (fastMode)
                syncStockIO();
        }
    }
}

template<bool reuAccess> auto SuperCpu::updateFastmode(bool synced) -> void {
    bool _fastMode = fastMode;
    fastMode = !(system1Mhz || software1Mhz || (jumper1Mhz && ((memConf & HW_ENABLE) == 0)));

    if (fastMode != _fastMode) {
        if constexpr(!reuAccess) {
            if (synced) {
                if (!fastMode)
                    syncStock();
                else
                    cycles = 16'600'000;
            }
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

template<bool checkBA, bool hardwareReg> auto SuperCpu::stepCycle() -> void {
    if (fastMode) {
        if constexpr(hardwareReg)
            cycles += frequency * 2;
        else
            cycles += frequency;        

        if (cycles >= SCPU_FREQ) {
            cycles -= SCPU_FREQ;
            syncStock();
        }
    } else {
        syncStock();
        if constexpr (checkBA) {
            while (baFlags) syncStock();
        }
    }
}

template<bool IsWrite> auto SuperCpu::clockStretchRom() -> void {
    if (fastMode) {
        cycles += frequency * 4;
        if (cycles >= SCPU_FREQ) {
            cycles -= SCPU_FREQ;
            syncStock();
        }
    } else {
        syncStock();
        if constexpr(!IsWrite) {
            while (baFlags)
                syncStock();
        }
    }
}

auto SuperCpu::clockStretchDramRead(uint32_t& addr) -> void {
    if (fastMode) {
        if (!((dramAddr ^ addr) & ~3)) { // same column (four bytes each)
            cycles += frequencyDRAM; // maximum speed like SRAM
        } else if ((dramAddr ^ addr) & dramRowMask) { // different row
            cycles += (frequencyDRAM * 3) + (frequencyDRAM >> 1); // 3.5 cycles
            dramAddr = addr;
        } else if (!(((dramAddr + 4) ^ addr) & ~3)) { // same row, next column
            cycles += frequencyDRAM; // maximum speed like SRAM
            dramAddr = addr;
        } else { // same row but column is not the next one (non-sequential)
            cycles += frequencyDRAM * 2;
            dramAddr = addr;
        }

        if (cycles >= SCPU_FREQ) {
            cycles -= SCPU_FREQ;
            syncStock();
        }
    } else {
        syncStock();
        while (baFlags)
            syncStock();
    }
}

auto SuperCpu::clockStretchDramWrite(uint32_t& addr) -> void {
    if (fastMode) {
        if ((dramAddr ^ addr) & dramRowMask)
            cycles += frequencyDRAM * 3; // different row
        else
            cycles += frequencyDRAM * 2;

        dramAddr = addr;

        if (cycles >= SCPU_FREQ) {
            cycles -= SCPU_FREQ;
            syncStock();
        }
    } else {
        syncStock();
    }
}

auto SuperCpu::clockStretchIORead() -> void {    
    while (baFlags & 2)
        syncStock();

    if (fastMode) {
        if (writeBuffer.inProgress)
            waitForWriteBuffer();
        else if (cycles > cycleWriteThroughLimit)
            syncStock();

        syncStockIO();

        cycles = cycleIoAccess - (randomizer.xorShift() & 0x1ffff);
    } else
        syncStockIO();
}

auto SuperCpu::clockStretchIOReadCIA() -> bool {
    bool postSync = false;

    while (baFlags & 2)
        syncStock();

    if (fastMode) {
        if (writeBuffer.inProgress)
            waitForWriteBuffer();
        else if (cycles > cycleWriteThroughLimit)
            syncStock();
        else
            postSync = cycles < 9'000'000; // fixme

        if (!postSync)
            syncStockIO();

        cycles = cycleIoAccess - (randomizer.xorShift() & 0x1ffff);
    } else
        syncStockIO();

    return postSync;
}

auto SuperCpu::clockStretchIOWrite() -> void {
    if (fastMode) {
        if (writeBuffer.inProgress)
            waitForWriteBuffer();
        else if (cycles > cycleWriteThroughLimit) {
            syncStock();
        }

        cycles = cycleIoAccess - (randomizer.xorShift() & 0x1ffff);
    } else
        syncStockIO();

    while (baFlags & 2)
        syncStock();    
}

auto SuperCpu::clockStretchIOWriteLong() -> void {
    if (fastMode) {
        if (writeBuffer.inProgress)
            waitForWriteBuffer();
        else if (cycles > cycleWriteThroughLimit) // too late, can't sync to the current host clock
            syncStock();

        cycles = cycleIoLong;
        
    } else
        syncStockIO();

    while (baFlags & 2)
        syncStock();    
}

auto SuperCpu::clockStretchIOWriteCia() -> void {
    if (fastMode) {
        if (writeBuffer.inProgress)
            waitForWriteBuffer();
        else if (cycles > cycleWriteThroughLimit)
            syncStock();

        syncStockIO();
        cycles = cycleIoLong;

        while (baFlags)
             syncStock();
    } else
        syncStockIO();
}

auto SuperCpu::waitForWriteBuffer() -> void {
    do {
        if ((baFlags & 2) == 0) { // SuperCPU can use the first three BA cycles, 6510 can't because RDY is ignored during "Write" cycles
            if (sysTimer.delay( &applyBuffer ) <= 1) {
                if (writeBuffer.colram)
                    system->colorRam[writeBuffer.addr] = writeBuffer.value;
                else
                    system->ram[writeBuffer.addr] = writeBuffer.value;
                writeBuffer.inProgress = false;
                sysTimer.remove(&applyBuffer);
                break;
            }
        }
        syncStock();
    } while(true);    
}

template<bool inColRam> auto SuperCpu::clockStretchWriteInternal(uint16_t addr, uint8_t value) -> void {
    if (fastMode) {
        if (writeBuffer.inProgress) {
            waitForWriteBuffer();
            cycles = cycleCacheLatch;
        } else {
            cycles += frequency;
            if (cycles >= SCPU_FREQ) {
                cycles -= SCPU_FREQ;
                syncStock();
            }
        }

        writeBuffer.colram = inColRam;
        writeBuffer.addr = addr;
        writeBuffer.value = value;
        writeBuffer.inProgress = true;
        sysTimer.add( &applyBuffer, cycles > cycleWriteThroughLimit ? 3 : 2);
    } else {
        syncStock();
        if constexpr(inColRam)
            system->colorRam[ addr & 0x3ff ] = value & 0xf;
        else
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
        sysTimer.add( &baStart, 3, Emulator::SystemTimer::UpdateExisting);
    } else
        baFlags = 0;
}

inline auto SuperCpu::syncStockIO() -> void {
    ioOnHost = true;
    syncStock();
    ioOnHost = false;
}

inline auto SuperCpu::syncStock() -> void {
    sysTimer.process();
    cia1.clock();
    vicII->clockMaybeLogged();
    cia2.clock();
    if (system->secondDriveCable.cycleSyncing)
	    system->iecBus.syncDrivesEachCycle();
}

auto SuperCpu::setJumper(unsigned jumperId, bool state) -> void {
    if (jumperId == 0) {
        jumper1Mhz = !state;
        updateFastmode(false);
        cycles = 0;
    }
    else if (jumperId == 1)
        jumperJiffyDos = state;
    else if (jumperId == 2) {
        jumperDramBoost = state;
        frequencyDRAM = frequency;
        if (state)
            frequencyDRAM >>= 3;
    }
}

auto SuperCpu::getJumper(unsigned jumperId) -> bool {
    if (jumperId == 0) return !jumper1Mhz;
    if (jumperId == 1) return jumperJiffyDos;
    if (jumperId == 2) return jumperDramBoost;
    return false;
}

auto SuperCpu::setExpander(ExpansionPort* expander) -> void {
    this->expander = expander;

    if (expander == system->reu)
        setId(Interface::ExpansionIdSuperCpuReu);
    else
        setId(Interface::ExpansionIdSuperCpu);
}

auto SuperCpu::serialize(Emulator::Serializer& s) -> void {
    bool memUsage = s.memUsage();
    unsigned _dramSize = dramSize;
    s.integer(dramSize);

    if ( s.mode() == Emulator::Serializer::Mode::Load ) {
        if (memUsage && memChange) {
            memChange->apply();
            memChange = nullptr;
        } else {
            if (!dram || (_dramSize != dramSize)) {
                if (dram) {
                    delete[] dram;
                    dram = nullptr;
                }
                setDram();
            }
        }
    } else {
        if (memUsage && memChange)
            memChange->reset(dram);
    }

    s.array( sram, 128 * 1024 );
    s.integer(optim);
    s.integer(dramMask);
    s.integer(dramPageSize);
    s.integer(romMask);
    s.integer(frequency);
    s.integer(frequencyDRAM);
    s.integer(cycles);
    s.integer(jumperJiffyDos);
    s.integer(jumper1Mhz);
    s.integer(jumperDramBoost);
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
    s.integer(writeBuffer.colram);
    s.integer(randomizer.xorShift32);

    if (!memUsage) {
        s.array(dram, dramSize - (((dramSize >> 20) == 16) ? (512 * 1024) : 0));
        updateFastmode(false);
    }

    // 65816 CPU
    s.integer(pc);
    s.integer(a);
    s.integer(x);
    s.integer(y);
    s.integer(this->s);
    s.integer(d);
    s.integer(pbr);
    s.integer(dbr);
    uint8_t _p = this->p;
    s.integer(_p);
    this->p = _p;
    s.integer(modeE);
    s.integer(control);
    s.integer(lines);

    if (expander)
        expander->serialize(s);
}

auto SuperCpu::getSizeNotConsideredForMemorySerialization() -> unsigned {
    unsigned _size = dramSize - (((dramSize >> 20) == 16) ? (512 * 1024) : 0);

    if (expander)
        return expander->getSizeNotConsideredForMemorySerialization() + _size;
    return _size;
}

auto SuperCpu::updateSnapshot(DebuggerSnapshot& snap) -> void {
    snap.pc = pc;
    snap.pcEdge = pcEdge;
    snap.regA = a;
    snap.regX = x;
    snap.regY = y;
    snap.regS = s;
    snap.pbr = pbr;
    snap.dbr = dbr;
    snap.regD = d;
    snap.flags = p;
    snap.modeE = modeE;
}

auto SuperCpu::updateMemorySnapshot(DebuggerSnapshot& snap) -> void {
    snap.mode = (memConf & 7) | 0x18;

    for (int b = 0; b <= 0xff; b++ ) {
        if (b < 2)                                          snap.mapperSCPU[b] = 1; // sram
        else if ((b & 0xf8) == 0xf8)                        snap.mapperSCPU[b] = 2; // rom
        else if (dram) {
            if ((b & 0xfe) == 0xf6)                         snap.mapperSCPU[b] = 3; // dram
            else if (((b << 16) | 0xffff) < dramConfSize)   snap.mapperSCPU[b] = 3; // dram
            else                                            snap.mapperSCPU[b] = 0; // unmapped
        } else                                              snap.mapperSCPU[b] = 0; // unmapped
    }

    for (int p = 0; p <= 0xf; p++ ) {
        uint16_t i = (p << 12) | memConf;
        if (readTable[i] == &SuperCpu::readC64Ram<false>)                   snap.mapper[p] = 1;
        else if (readTable[i] == &SuperCpu::readIoSCPU<false>)              snap.mapper[p] = 2;
        else if (readTable[i] == &SuperCpu::readSramChar<false>)            snap.mapper[p] = 3;
        else if (readTable[i] == &SuperCpu::readSramKernal<false>)          snap.mapper[p] = 4;

        else if (readTable[i] == &SuperCpu::readSramB0<false>)              snap.mapper[p] = 10;
        else if (readTable[i] == &SuperCpu::readSramB1<false>)              snap.mapper[p] = 11;
        else if (readTable[i] == &SuperCpu::readRom<false>)                 snap.mapper[p] = 12;

        else                                                                snap.mapper[p] = 0;
    }
}

auto SuperCpu::editMemory(uint32_t addr, uint8_t value) -> void {
    if ((addr & 0xff0000) == 0) { // SRAM
        (this->*writeTableReu[mirrorMemConf | (addr & 0xff00)])(addr, value);

        if (readTable[(addr & 0xff00) | memConf] == &SuperCpu::readSramKernal<false>) {
            sram[0x8000 + addr] = value;
        } else if (readTable[(addr & 0xff00) | memConf] == &SuperCpu::readSramB1<false>) {
            sram[0x10000 | addr] = value;
        }
        return;
    } if ((addr & 0xff0000) == 0x10000) { // bank 1 SRAM
        if (addr & 0xfffe)  sram[addr] = value;
        else                sram[addr & 1] = value;
        return;
    } if ((addr & 0xf80000) == 0xf80000) { // ROM
        return;
    }

    if (dram) {
        if ((addr & 0xfe0000) == 0xf60000) {
            if (dramPageSize != dramConfPageSize)
                addr = ((addr >> dramConfPageSize) << dramPageSize) | (addr & ((1 << dramPageSize) - 1));

            addr &= 0x1ffff; // map banks $f6/$f7 to banks 0/1 of DRAM
        } else if (addr < dramConfSize) {
            if (dramPageSize != dramConfPageSize)
                addr = ((addr >> dramConfPageSize) << dramPageSize) | (addr & ((1 << dramPageSize) - 1));
        }

        dram[addr & dramMask] = value;
    }
}

auto SuperCpu::parseExpressionValue(const std::string& input, int& pos) -> uint32_t {
    for (auto& cond : DebuggerSnapshot::breakConditionsSCPU) {
        std::string token = cond.ident;
        if (input.compare(pos, token.size(), token) == 0) {
            pos += token.size();

            switch (cond.vector) {
                default: return 0;
                case 0: return vicII->getVcounter();
                case 1: return vicII->getCycle();
                case 2: return pc;
                case 3: return x;
                case 4: return y;
                case 5: return a;
                case 6: return s;
                case 7: return d;
                case 8: return p;
                case 9: return dbr;
                case 10: return pbr;
                case 11: return modeE;
                case 12: return !!(lines & RDY_LINE);
                case 13: return !!(lines & IRQ_LINE);
                case 14: return !!(lines & NMI_TRANSITION);

                case 20: return p.c;
                case 21: return p.z;
                case 22: return p.i;
                case 23: return p.d;
                case 24: return p.x;
                case 25: return p.m;
                case 26: return p.v;
                case 27: return p.n;

                case 100:
                case 101:
                case 102:
                case 103:
                case 104:
                case 105: {
                    int radix = 10;
                    if (input.compare(pos, 1, "$") == 0) {
                        radix = 16;
                        pos++;
                    }
                    const char* start = input.c_str() + pos;
                    char* end;
                    uint32_t value = std::strtoul(start, &end, radix);
                    if (start != end) {
                        pos += (end - start);
                        if (cond.vector == 101)
                            return peekByte( value );
                        return system->peekMemoryByIdent( value, cond.vector );
                    }
                    return 0;
                }
            }
        }
    }
    return 0;
}

auto SuperCpu::setWatchpointCondition(DebuggerAction action, unsigned ident, unsigned hitCount, unsigned hitCountMode, const std::string& expression, unsigned expressionMode) -> bool {
    bool expressionError = false;

    if (!expression.empty()) {
        ExpressionParser parser;
        parser.setExpression( expression );
        parser.callback = [this](const std::string& input, int& pos) {
            return parseExpressionValue(input, pos);
        };

        try {
            parser.parse();
        } catch (ExpressionParseError& e) {
            expressionError = true;
        }
    }

    switch (action) {
        case DebuggerAction::Breakpoint:
            breakPoints.setBreakpointCondition( ident, hitCount, hitCountMode, expressionError ? "" : expression, expressionMode );
            break;
        case DebuggerAction::Watchpoint:
            watchPoints.setBreakpointCondition( ident, hitCount, hitCountMode, expressionError ? "" : expression, expressionMode );
            break;
        case DebuggerAction::WatchpointWrite:
            watchPointsWrite.setBreakpointCondition( ident, hitCount, hitCountMode, expressionError ? "" : expression, expressionMode );
            break;
        case DebuggerAction::ExceptionPoint:
            exceptionPoints.setBreakpointCondition( ident, hitCount, hitCountMode, expressionError ? "" : expression, expressionMode );
            break;
        default: break;
    }

    return !expressionError;
}

auto SuperCpu::debuggerAdd(DebuggerAction action, unsigned ident, uint32_t addr, uint32_t addrTo) -> void {
    switch (action) {
        case DebuggerAction::Breakpoint:        breakPoints.add( ident, addr, addrTo ); break;
        case DebuggerAction::Watchpoint:        watchPoints.add( ident, addr, addrTo ); break;
        case DebuggerAction::WatchpointWrite:   watchPointsWrite.add(ident,  addr, addrTo ); break;
        case DebuggerAction::ExceptionPoint:    exceptionPoints.add( ident, addr, addr ); break;
        case DebuggerAction::History:           historyHandler.enable(); break;
        case DebuggerAction::ModifiedCode:      modifiedCode.add( addr, addrTo ); break;
        default:
            break;
    }
}

auto SuperCpu::debuggerRemove(DebuggerAction action, unsigned ident) -> void {
    switch (action) {
        case DebuggerAction::Breakpoint:        breakPoints.remove( ident ); break;
        case DebuggerAction::Watchpoint:        watchPoints.remove( ident ); break;
        case DebuggerAction::WatchpointWrite:   watchPointsWrite.remove( ident ); break;
        case DebuggerAction::ExceptionPoint:    exceptionPoints.remove( ident ); break;
        case DebuggerAction::History:           historyHandler.disable( ); break;
        default:
            break;
    }
}

auto SuperCpu::debuggerRemove(DebuggerAction action) -> void {
    switch (action) {
        case DebuggerAction::Breakpoint:        breakPoints.removeAll(); break;
        case DebuggerAction::Watchpoint:        watchPoints.removeAll(); break;
        case DebuggerAction::WatchpointWrite:   watchPointsWrite.removeAll(); break;
        case DebuggerAction::ExceptionPoint:    exceptionPoints.removeAll(); break;
        case DebuggerAction::History:           historyHandler.disable(); break;
        case DebuggerAction::ModifiedCode:      modifiedCode.disable(); break;
        default:
            break;
    }
}

auto SuperCpu::debugPointReached(int source, Emulator::WatchPoints& wp, unsigned addr) -> void {
    Emulator::Interface::DebuggerAction action;

    switch (source) {
        case WatchPoint: action = DebuggerAction::Watchpoint; break;
        case WatchPointWrite: action = DebuggerAction::WatchpointWrite; break;
        case ExceptionPoint: action = DebuggerAction::ExceptionPoint; break;
        case BreakPoint: action = DebuggerAction::Breakpoint; break;
        case SoftStop: action = DebuggerAction::Softstop; break;
        default: return;
    }
    system->debugPointReached(DebuggerTheme::SCPU, action, wp, addr);
}

}
