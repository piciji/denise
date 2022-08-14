
#pragma once

#include <cstdint>
#include "../interface.h"
#include "../input/input.h"
#include "../../cia/new/cia.h"
#include "../cpu/m68000.h"
#include "../agnus/agnus.h"

namespace LIBAMI {

struct System {

    System(Interface* interface);

    Interface* interface;
    Input input;
    Cpu cpu;
    Agnus agnus;
    Cia cia1;
    Cia cia2;

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

    bool leaveEmulation = false;
    bool powerOn = false;

    auto setFirmware(unsigned typeId, uint8_t* data, unsigned size) -> void;
    auto power(bool softReset = false) -> void;
    auto powerOff() -> void;
    auto run() -> void;


};

extern System* system;


}