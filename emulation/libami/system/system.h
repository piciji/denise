
#pragma once

#include <cstdint>
#include "../interface.h"
#include "../input/input.h"
#include "../../cia/new/cia.h"
#include "../cpu/m68000.h"
#include "../agnus/agnus.h"
#include "../agnus/blitter.h"
#include "../agnus/copper.h"

namespace LIBAMI {

struct System {

    System(Interface* interface);

    Interface* interface;
    Input input;
    Cpu cpu;
    Blitter blitter;
    Copper copper;
    Agnus agnus;
    Cia<MOS_8520> cia1;
    Cia<MOS_8520> cia2;

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
    auto power(bool softReset = false, bool resetInstruction = false) -> void;
    auto powerOff() -> void;
    auto run() -> void;
    auto informAboutKeyUpdate() -> void;


};

extern System* system;


}