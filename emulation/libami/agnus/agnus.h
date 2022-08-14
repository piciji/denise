
#pragma once

#include "../../cia/new/cia.h"
#include "../cpu/m68000.h"

namespace LIBAMI {

struct Cpu;

struct Agnus {

    Agnus(Cpu& cpu, Cia& cia1, Cia& cia2);

    enum { Unmapped, CHIP_MEM, SLOW_MEM, KICK_ROM, EXT_ROM, MMIO_CUSTOM, MMIO_CIA, MMIO_RTC };

    Cpu& cpu;
    Cia& cia1;
    Cia& cia2;

    uint8_t mapper[256] = {0};

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

    unsigned cpuCycles;
    uint8_t eClockPosition;

    auto reset() -> void;
    auto setMemory(unsigned typeId, unsigned size) -> void;
    auto mapMemory() -> void;
    auto setOVL(bool state) -> void;

    auto readByte(uint32_t adr) -> uint8_t;
    auto writeByte(uint32_t adr, uint8_t value) -> void;
    auto readWord(uint32_t adr) -> uint16_t;
    auto writeWord(uint32_t adr, uint16_t value) -> void;
    auto sync(uint16_t cycles) -> void;
    auto iackCycle(uint8_t level, uint8_t& vector) -> uint8_t;
    auto resetOut() -> void;

};

}
