
#pragma once

#include "../expansionPort.h"
#include "../../../wdc/65816/w65816.h"
//#include "../../../wdc65816alt/my65816.h"
#include "../../../tools/memchangetracker.h"
#include "../../../interface.h"

namespace CIA {
    struct M6526;
}

namespace LIBC64 {

struct SuperCpu : ExpansionPort, WDCFAMILY::W65816 {
    SuperCpu(System* system, Emulator::SystemTimer& sysTimer, CIA::M6526& cia1, CIA::M6526& cia2, SidManager& sidManager, Traps& traps);
    ~SuperCpu();

    enum { MODE_SRAM, MODE_MIRROR, MODE_INTERNAL };
    enum { PAGE_REGULAR, PAGE_ZERO, PAGE_HI };
    enum { HW_ENABLE = 0x20, DOS_EXT = 0x40, BOOT_MAP = 0x80 };

    using Callback = std::function<void ()>;
    Emulator::Interface::Media* media;
    Emulator::SystemTimer& sysTimer;
    CIA::M6526& cia1;
    CIA::M6526& cia2;
    SidManager& sidManager;
    Traps& traps;

    MemChangeTracker<uint32_t, uint8_t> dramTracker;

    uint8_t* sram;
    uint8_t* dram;
    uint8_t* rom;

    uint8_t optim;
    unsigned dramMask;
    unsigned dramSize;
    unsigned dramPageSize;
    unsigned romMask;
    unsigned frequency;
    unsigned cycles;
    bool jumperJiffyDos;
    bool jumper1Mhz;

    bool software1Mhz;
    bool system1Mhz;
    bool fastMode;

    int baFlags;
    unsigned memConf;
    unsigned mirrorMemConf;

    uint32_t dramAddr;
    uint32_t dramRowMask;
    unsigned dramConfSize;
    unsigned dramConfPageSize;

    Callback applyBuffer;
    Callback baStart;

    unsigned cycleCacheLatch;
    unsigned cycleIoAccess;
    unsigned cycleIoLong;
    unsigned cycleWriteThroughLimit;

    typedef uint8_t (SuperCpu::*ReadTable)(uint16_t);
    typedef void (SuperCpu::*WriteTable)(uint16_t, uint8_t);
    ReadTable readTable[0x10000] = {nullptr};
    WriteTable writeTable[0x90000] = {nullptr};

    struct {
        uint16_t addr;
        uint8_t value;
        bool inProgress;
    } writeBuffer;

    auto readC64Ram(uint16_t addr) -> uint8_t;
    auto readSramB0(uint16_t addr) -> uint8_t;
    auto readSramB1(uint16_t addr) -> uint8_t;
    auto readSramChar(uint16_t addr) -> uint8_t;
    auto readSramKernal(uint16_t addr) -> uint8_t;
    auto readRom(uint16_t addr) -> uint8_t;

    template<uint8_t mode, uint8_t addrArea = PAGE_REGULAR> auto writeSramAndOrHostRam(uint16_t addr, uint8_t value) -> void;

    auto readIoSCPU(uint16_t addr) -> uint8_t;
    auto readIoVIC(uint16_t addr) -> uint8_t;
    auto readIoSID(uint16_t addr) -> uint8_t;
    auto readIoColorRamInternal(uint16_t addr) -> uint8_t;
    auto readIoCIA1(uint16_t addr) -> uint8_t;
    auto readIoCIA2(uint16_t addr) -> uint8_t;
    auto readIo1(uint16_t addr) -> uint8_t;
    auto readIo2(uint16_t addr) -> uint8_t;

    auto writeIoSCPU(uint16_t addr, uint8_t value) -> void;
    auto writeIoVIC(uint16_t addr, uint8_t value) -> void;
    auto writeIoStrange(uint16_t addr, uint8_t value) -> void;
    template<bool withSram> auto writeIoSID(uint16_t addr, uint8_t value) -> void;
    template<bool withSram> auto writeIoColorRam(uint16_t addr, uint8_t value) -> void;
    auto writeIoCIA1(uint16_t addr, uint8_t value) -> void;
    auto writeIoCIA2(uint16_t addr, uint8_t value) -> void;
    auto writeIo1(uint16_t addr, uint8_t value) -> void;
    auto writeIo2(uint16_t addr, uint8_t value) -> void;

    template<bool checkBA = false, bool IsWrite = false> auto stepCycle() -> void;

    auto updateFastmode(bool synced = true) -> void;
    auto updateMirroring() -> void;
    auto updateSimm(uint8_t value) -> void;

    auto clock() -> void { process(); }
    auto reset(bool softReset = false) -> void;

    auto setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void;
    auto setRamSize(int id) -> void;
    auto getRamSize() -> int;
    auto setDram() -> void;

    auto readByte(uint32_t addr) -> uint8_t;
    auto readVectorByte(uint16_t addr) -> uint8_t;
    auto writeByte(uint32_t addr, uint8_t value) -> void;
    auto idleCycle(uint32_t addr) -> void;

    auto trapHandler() -> bool;

    auto clockStretchRom() -> void;
    auto clockStretchDramRead(uint32_t& addr) -> void;
    auto clockStretchDramWrite(uint32_t& addr) -> void;
    auto clockStretchIORead() -> void;
    auto clockStretchIOWrite() -> void;
    auto clockStretchIOWriteLong() -> void;
    auto clockStretchIOWriteCia() -> void;
    auto syncStock() -> void;
    auto waitForWriteBuffer() -> void;
    auto clockStretchWriteInternal(uint16_t addr, uint8_t value) -> void;

    auto observeRdy(bool state) -> void;
    auto observeIrq(bool state) -> void;
    auto observeNmi(bool state) -> void;
    auto haltMainCpu() -> bool { return true; }

    auto setJumper(unsigned jumperId, bool state) -> void;
    auto getJumper(unsigned jumperId) -> bool;

    auto buildMap() -> void;
    auto bootSpeed() -> float { return romMask > (65 * 1024) ? (jumper1Mhz ? 11.0f : 4.0f) : 1.0f; }

    auto serialize(Emulator::Serializer& s) -> void;
    auto executeKernalRom() -> bool;
    auto executeHostRam() -> bool;

    // some helper for traps
    auto getPC() -> uint16_t { return pc; }
    auto setPC(uint16_t _pc) -> void { pc = _pc; }
    auto setRegP_I(bool state) -> void { p.i = state; }
    auto setRegP_C(bool state) -> void { p.c = state; }
    auto setRegP_N(bool state) -> void { p.n = state; }
    auto setRegP_Z(bool state) -> void { p.z = state; }
    auto setRegA(uint8_t data) -> void { a = (a & 0xff00) | data; }
};

}
