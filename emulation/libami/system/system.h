
#pragma once

#include <cstdint>
#include "../interface.h"
#include "../input/input.h"
#include "../../cia/new/cia.h"
#include "../cpu/m68000.h"
#include "../agnus/agnus.h"
#include "../agnus/blitter.h"
#include "../agnus/copper.h"
#include "../video/denise.h"
#include "../../tools/crop.h"

namespace LIBAMI {

struct System {

    System(Interface* interface);

    Interface* interface;
    Input input;
    Cpu cpu;
    Blitter blitter;
    Copper copper;
    Agnus agnus;
    Denise denise;

    Emulator::Crop<uint16_t> crop;

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
    auto videoRefresh( uint16_t* frame, unsigned width, unsigned height, unsigned linePitch, uint8_t interlace) -> void;
    auto videoMidScreenCallback() -> void;


};

extern System* system;


}