
#pragma once

#include "../../cia/new/cia.h"
#include "../../tools/events.h"

namespace LIBAMI {

struct Cpu;
struct Blitter;
struct Copper;

struct Agnus : Emulator::Events<10> {

    Agnus(Cpu& cpu, Blitter& blitter, Copper& copper, Cia& cia1, Cia& cia2);

    enum { Unmapped, CHIP_MEM, SLOW_MEM, KICK_ROM, EXT_ROM, MMIO_CUSTOM, MMIO_CIA, MMIO_RTC };

    enum { EVENT_KBD, EVENT_DMA_POINTER, EVENT_BLITTER, EVENT_REG_UPDATE, EVENT_LEAVE_EMULATION };

    enum { None = 0, Unused = 1, PTR_BLT_A_H, PTR_BLT_A_L, PTR_BLT_B_H, PTR_BLT_B_L,
           PTR_BLT_C_H, PTR_BLT_C_L, PTR_BLT_D_H, PTR_BLT_D_L,
           PTR_REF,
    };

    enum { DMACON = 1 };

    enum { ACT_BLITTER = 1, ACT_COPPER = 2 };

    enum { BUS_FREE, BUS_USAGE_BLITTER, BUS_USAGE_COPPER, BUS_USAGE_CPU, BUS_USAGE_REFRESH, BUS_USAGE_AUDIO };

    enum { PAL, NTSC };

    enum { Trigger_Read, Trigger_CPU, Trigger_Copper, Trigger_Vsync };

    enum Mode { OCS = 1, ECS = 2, AGA = 4 } mode; // AGA not supported at the moment

    Cpu& cpu;
    Blitter& blitter;
    Copper& copper;
    Cia& cia1;
    Cia& cia2;

    Emulator::EventCallback dmaPointerUpdate;
    Emulator::EventCallback regUpdate;
    Emulator::EventCallback leaveEmulation;
    uint32_t actions = 0;

    uint8_t mapper[256] = {0};
    uint8_t busUsage[228];
    uint8_t hPos;
    uint16_t vPos;

    uint8_t* chipMem = nullptr;
    unsigned chipMemMask = 0;
    uint8_t* slowMem = nullptr;
    unsigned slowMemSize = 0;
    uint8_t* kickRom = nullptr;
    unsigned kickRomMask = 0;
    uint8_t* extRom = nullptr;
    unsigned extRomMask = 0;

    bool useRTC = false;
    uint16_t dataBus = 0;
    uint16_t dmaCon;
    uint16_t bplCon0;
    unsigned countWaitCycles;
    unsigned resyncCounter;
    unsigned positionChanges;
    uint32_t rDmaPtr;

    uint8_t eClockPosition;
    bool lol;
    bool lof;
    bool lolToggle;
    bool lofToggle;
    bool ntsc;
    unsigned lines;
    bool initVCounter;
    bool shortLineBefore;

    auto ecsAndHigher() -> bool const { return mode & (Mode::ECS | Mode::AGA); }
    auto ecs() -> bool const { return mode == Mode::ECS; }
    auto aga() -> bool const { return mode == Mode::AGA; }

    auto useSpriteDMA() -> bool const { return (dmaCon & 0x220) == 0x220; }
    auto useBlitterDMA() -> bool const { return (dmaCon & 0x240) == 0x240; }
    auto useCopperDMA() -> bool const { return (dmaCon & 0x280) == 0x280; }
    auto useBitplaneDMA() -> bool const { return (dmaCon & 0x300) == 0x300; }
    auto blitterNasty() -> bool const { return dmaCon & 0x400; }

    auto reset() -> void;
    auto setMemory(unsigned typeId, unsigned size) -> void;
    auto mapMemory() -> void;
    auto setOVL(bool state) -> void;

    auto readByte(uint32_t adr) -> uint8_t;
    auto writeByte(uint32_t adr, uint8_t value) -> void;
    auto readWord(uint32_t adr) -> uint16_t;
    auto writeWord(uint32_t adr, uint16_t value) -> void;
    auto sync(uint16_t cycles) -> void;
    auto dmaCycle() -> void;
    auto addWaitstatesToCPU() -> void;
    auto iackCycle(uint8_t level, uint8_t& vector) -> uint8_t;
    auto resetOut() -> void;
    auto pullResetLine(bool state = true) -> void;

    auto msecToDMACycles(unsigned ms) -> unsigned { return 3550 * ms; } // average for PAL/NTSC, todo: check if more accuracy is needed
    auto usecToDMACycles(unsigned us) -> unsigned { return 3.55f * (float)us + 0.5f; }

    auto writeCustom(uint16_t adr, uint16_t value, uint8_t triggeredBy = Trigger_CPU) -> void;
    template<bool byteAccess = false> auto readCustom(uint16_t adr, bool triggeredByWrite = false) -> uint16_t;

    auto canBlitterUseBus() -> bool;
    auto canCopperUseBus() -> bool;
    auto allocateCopper() -> bool;
    template<uint8_t ptrEvent> auto fetchBlitterDma(uint32_t adr, uint16_t& result) -> bool;
    auto fetchCopperDma(uint32_t adr, uint16_t& result) -> bool;
    auto fetchCopperDmaNoBUSCheck(uint32_t adr, uint16_t& result) -> void;
    auto writeBlitterDma(uint32_t adr, uint16_t value) -> bool;

    template<uint8_t ptrEvent> auto fetchBlitterDmaNoBUSCheck(uint32_t adr, uint16_t& result) -> void;
    auto writeBlitterDmaNoBUSCheck(uint32_t adr, uint16_t value) -> void;

    auto setRefPtr(uint16_t value) -> void;

    auto copperLongGap() -> bool { return shortLineBefore && (hPos == 2); }
    auto POSR(bool vhpos) -> uint16_t;

};

}
