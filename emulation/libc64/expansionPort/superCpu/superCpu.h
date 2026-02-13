
#pragma once

#include "../expansionPort.h"
#include "../../../wdc/65816/w65816.h"
//#include "../../../wdc65816alt/my65816.h"
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

    uint8_t* sram;
    uint8_t* dram;
    uint8_t* rom;

    uint8_t optim;
    unsigned dramMask;
    unsigned dramSize;
    unsigned dramPageSize;
    unsigned romMask;
    unsigned frequency;
    unsigned frequencyDRAM;
    unsigned cycles;
    bool jumperJiffyDos;
    bool jumper1Mhz;
    bool jumperDramBoost;

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
    bool ioOnHost; // SuperCPU switch GAME/EXROM (indirectly emulated)
    Emulator::Rand randomizer;

    typedef uint8_t (SuperCpu::*ReadTable)(uint16_t);
    typedef void (SuperCpu::*WriteTable)(uint16_t, uint8_t);
    ReadTable readTable[0x10000] = {nullptr};
    ReadTable readTableReu[0x10000] = { nullptr };
    ReadTable peekTable[0x10000] = {nullptr};
    WriteTable writeTable[0x90000] = {nullptr};
    WriteTable writeTableReu[0x90000] = { nullptr };

    struct {
        uint16_t addr;
        uint8_t value;
        bool colram;
        bool inProgress;
    } writeBuffer;

    template<bool peekAccess = false> auto readC64Ram(uint16_t addr) -> uint8_t;
    template<bool peekAccess = false> auto readSramB0(uint16_t addr) -> uint8_t;
    template<bool peekAccess = false> auto readSramB1(uint16_t addr) -> uint8_t;
    template<bool peekAccess = false> auto readSramChar(uint16_t addr) -> uint8_t;
    template<bool peekAccess = false> auto readSramKernal(uint16_t addr) -> uint8_t;
    template<bool peekAccess = false> auto readRom(uint16_t addr) -> uint8_t;

    template<bool reuAccess, uint8_t mode, uint8_t addrArea> auto writeSramAndOrHostRam(uint16_t addr, uint8_t value) -> void;

    template<bool reuAccess = false> auto readIoSCPU(uint16_t addr) -> uint8_t;
    auto peekIoSCPU(uint16_t addr) -> uint8_t;
    template<bool reuAccess = false> auto readIoVIC(uint16_t addr) -> uint8_t;
    auto peekIoVIC(uint16_t addr) -> uint8_t;
    template<bool reuAccess = false> auto readIoSID(uint16_t addr) -> uint8_t;
    auto peekIoSID(uint16_t addr) -> uint8_t;
    template<bool peekAccess = false> auto readIoColorRamInternal(uint16_t addr) -> uint8_t;
    template<bool reuAccess = false> auto readIoCIA1(uint16_t addr) -> uint8_t;
    auto peekIoCIA1(uint16_t addr) -> uint8_t;
    template<bool reuAccess = false> auto readIoCIA2(uint16_t addr) -> uint8_t;
    auto peekIoCIA2(uint16_t addr) -> uint8_t;
    template<bool reuAccess = false> auto readIo1(uint16_t addr) -> uint8_t;
    auto peekIo1(uint16_t addr) -> uint8_t;
    template<bool reuAccess = false> auto readIo2(uint16_t addr) -> uint8_t;
    auto peekIo2(uint16_t addr) -> uint8_t;

    template<bool reuAccess = false> auto writeIoSCPU(uint16_t addr, uint8_t value) -> void;
    template<bool reuAccess = false> auto writeIoVIC(uint16_t addr, uint8_t value) -> void;
    template<bool reuAccess = false> auto writeIoStrange(uint16_t addr, uint8_t value) -> void;
    template<bool withSram, bool reuAccess = false> auto writeIoSID(uint16_t addr, uint8_t value) -> void;
    template<bool withSram, bool reuAccess = false> auto writeIoColorRam(uint16_t addr, uint8_t value) -> void;
    template<bool reuAccess = false> auto writeIoCIA1(uint16_t addr, uint8_t value) -> void;
    template<bool reuAccess = false> auto writeIoCIA2(uint16_t addr, uint8_t value) -> void;
    template<bool reuAccess = false> auto writeIo1(uint16_t addr, uint8_t value) -> void;
    template<bool reuAccess = false> auto writeIo2(uint16_t addr, uint8_t value) -> void;

    template<bool checkBA = false, bool hardwareReg = false> auto stepCycle() -> void;

    template<bool reuAccess = false> auto updateFastmode(bool synced = true) -> void;
    auto updateMirroring() -> void;
    auto updateSimm(uint8_t value) -> void;

    auto clock() -> void { process(); }
    auto reset(bool softReset = false) -> void;

    auto setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void;
    auto setRamSize(int id) -> void;
    auto getRamSize() -> int;
    auto setDram() -> void;

    auto readByte(uint32_t addr) -> uint8_t;
    auto editMemory(uint32_t addr, uint8_t value) -> void;
    auto readByteReu(uint16_t addr) -> uint8_t;
    auto peekByte(uint32_t addr) -> uint8_t;
    auto readVectorByte(uint16_t addr) -> uint8_t;
    auto writeByte(uint32_t addr, uint8_t value) -> void;
    auto writeByteReu(uint16_t addr, uint8_t value) -> void;
    auto idleCycle(uint32_t addr) -> void;
    auto memoryDumpBank(uint8_t bank, uint8_t* dump) -> void;
    auto memoryDumpPage(uint8_t page, uint8_t* dump) -> void;

    auto trapHandler() -> bool;

    template<bool IsWrite = false> auto clockStretchRom() -> void;
    auto clockStretchDramRead(uint32_t& addr) -> void;
    auto clockStretchDramWrite(uint32_t& addr) -> void;
    auto clockStretchIORead() -> void;
    auto clockStretchIOReadCIA() -> bool;
    auto clockStretchIOWrite() -> void;
    auto clockStretchIOWriteLong() -> void;
    auto clockStretchIOWriteCia() -> void;
    auto syncStock() -> void;
    auto syncStockIO() -> void;
    auto waitForWriteBuffer() -> void;
    template<bool inColRam = false> auto clockStretchWriteInternal(uint16_t addr, uint8_t value) -> void;

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

    auto setExpander(ExpansionPort* expander) -> void;
    auto getSizeNotConsideredForMemorySerialization() -> unsigned;

    // SuperCPU can change HOST memory map onyl by GAME/EXROM.
    // 6510 runs only for a short time to set PPORT to a meaningfull setting.
    // hence 6510 is permanently halted and can't change PPORT anymore.
    // PPORT has setting to make it possible to map IO in only by GAME/EXROM.
    // all PPORT related stuff is simulated by SuperCPU: switch in ROMS, RAM, ...
    // means: GAME/EXROM of pass through expansion is not the same as C64.
    // C64 GAME/EXROM is only used to toggle between all RAM or IO
    // ROML, ROMH, Ultimax for pass through expansion is simulated by SuperCPU and never reach C64
    auto hasIoOnHost() -> bool { return ioOnHost; } 

    // some helper for traps
    auto getPC() -> uint16_t { return pc; }
    auto setPC(uint16_t _pc) -> void { pc = _pc; }
    auto setRegP_I(bool state) -> void { p.i = state; }
    auto setRegP_C(bool state) -> void { p.c = state; }
    auto setRegP_N(bool state) -> void { p.n = state; }
    auto setRegP_Z(bool state) -> void { p.z = state; }
    auto setRegA(uint8_t data) -> void { a = (a & 0xff00) | data; }

    auto updateSnapshot(DebuggerSnapshot& snap) -> void;
    auto updateMemorySnapshot(DebuggerSnapshot& snap) -> void;
    auto debugPointReached(DebuggerAction action, unsigned addr) -> void;
};

}
