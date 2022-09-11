
#pragma once

#include "../../cia/new/cia.h"
#include "../../tools/events.h"

namespace LIBAMI {

struct Cpu;
struct Blitter;

struct Agnus : Emulator::Events<10> {

    Agnus(Cpu& cpu, Blitter& blitter, Cia& cia1, Cia& cia2);

    enum { Unmapped, CHIP_MEM, SLOW_MEM, KICK_ROM, EXT_ROM, MMIO_CUSTOM, MMIO_CIA, MMIO_RTC };

    enum { EVENT_KBD, EVENT_DMA_POINTER, EVENT_BLITTER };

    enum { None = 0, Unused = 1, PTR_BLT_A_H, PTR_BLT_A_L, PTR_BLT_B_H, PTR_BLT_B_L, PTR_BLT_C_H, PTR_BLT_C_L, PTR_BLT_D_H, PTR_BLT_D_L };

    enum { ACT_BLITTER = 1 };

    enum { BUS_FREE, BUS_USAGE_BLITTER, BUS_USAGE_CPU };

    Cpu& cpu;
    Blitter& blitter;
    Cia& cia1;
    Cia& cia2;

    Emulator::EventCallback dmaPointerUpdate;
    uint32_t actions = 0;

    uint8_t mapper[256] = {0};
    uint8_t busUsage[228];
    uint8_t hPos;

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
    unsigned countWaitCycles;

    unsigned cpuCycles;
    uint8_t eClockPosition;

    bool useBlitterDMA() const { return (dmaCon & 0x240) == 0x240; }
    bool blitterNasty() const  { return dmaCon & 0x400; }

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

    auto writeCustom(uint16_t adr, uint16_t value) -> void;

    auto canBlitterUseBus() -> bool;
    template<uint8_t ptrEvent> auto fetchBlitterDma(uint32_t adr, uint16_t& result) -> bool;
    auto writeBlitterDma(uint32_t adr, uint16_t value) -> bool;

    template<uint8_t ptrEvent> auto fetchBlitterDmaNoBUSCheck(uint32_t adr, uint16_t& result) -> void;
    auto writeBlitterDmaNoBUSCheck(uint32_t adr, uint16_t value) -> void;

};

}
