
#pragma once

#include <cstdint>
#include "../interface.h"
#include "../../tools/systimer.h"
#include "../../cia/new/cia.h"

namespace LIBAMI {

struct Input;

struct System {

    System(Interface* interface);

    enum { Unmapped, CHIP_MEM, SLOW_MEM, KICK_ROM, EXT_ROM, MMIO_CUSTOM, MMIO_CIA, MMIO_RTC };

    Interface* interface;
    Input* input;

    struct {
        unsigned config = 0;
        unsigned frameCounter;
        bool renderNext;
    } fastForward;

    struct {
        unsigned frames = 0;
        unsigned pos = 0;
        bool performance = false;
        bool preventJit = true;
        Emulator::MemSerializer serializer;
    } runAhead;

    struct {
        bool requestFloppy;
        bool useFloppy;
    } driveSounds;

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
    bool leaveEmulation = false;
    bool powerOn = false;
    uint16_t dataBus = 0;

    unsigned cpuCycles;
    uint8_t eClockPosition;

    auto power(bool softReset = false) -> void;
    auto powerOff() -> void;
    auto run() -> void;

    auto setMemory(unsigned typeId, unsigned size) -> void;
    auto setFirmware(unsigned typeId, uint8_t* data, unsigned size) -> void;
    auto mapMemory() -> void;
    auto setOVL(bool state) -> void;

    auto readMemory(uint32_t adr) -> uint8_t;
    auto writeMemory(uint32_t adr, uint8_t value) -> void;
    auto readMemory16(uint32_t adr) -> uint16_t;
    auto writeMemory16(uint32_t adr, uint16_t value) -> void;

    auto doDMA( unsigned cycles) -> void;
};

extern System* system;
extern Cia* cia1;
extern Cia* cia2;
extern Emulator::SystemTimer sysTimerCia;

}