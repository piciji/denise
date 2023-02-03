
#pragma once

#include <cstdint>
#include "../input/input.h"
#include "../../cia/new/cia.h"
#include "../cpu/m68000.h"
//#include "../altCpu/cpu.h"
#include "../agnus/agnus.h"
#include "../video/denise.h"
#include "../paula/paula.h"
#include "../drive/diskDrive.h"
#include "../../tools/crop.h"

namespace LIBAMI {

struct Interface;

struct System {

    System(Interface* interface);

    Interface* interface;
    Input input;
    Cpu cpu;
    Agnus agnus;
    Denise denise;
    Paula paula;
    DiskDrive diskDrives[4];

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
        bool stateChange = false;
        bool motor;
        uint8_t inputFetches = 0;
    } observer;

    bool requestFloppySound = false;

    bool leaveEmulation = false;
    bool powerOn = false;
    unsigned serializationSize;
    Emulator::Serializer serializer;

    auto setFirmware(unsigned typeId, uint8_t* data, unsigned size) -> void;
    auto power(bool softReset = false, bool resetInstruction = false) -> void;
    auto powerOff() -> void;
    auto run() -> void;
    auto informAboutKeyUpdate() -> void;
    auto videoRefresh( uint16_t* frame, unsigned width, unsigned height, unsigned linePitch, uint8_t interlace) -> void;
    auto videoMidScreenCallback(uint8_t interlace) -> void;
    auto audioRefresh(int16_t left, int16_t right) -> void;
    auto setModel(uint8_t model) -> void;
    auto getModel() -> uint8_t;
    auto updateStats() -> void;
    auto updateDriveSounds() -> void;
    auto setFloppySounds(bool state) -> void;

    auto setRegion( int region ) -> void;
    auto setResampleQuality( int value ) -> void;
    auto setFastForward( unsigned config ) -> void;
    auto setRunAhead(unsigned frames) -> void;
    auto hintSlowSpeed(bool state) -> void;

    auto calcSerializationSize() -> void;
    auto serialize(unsigned& size) -> uint8_t*;
    auto serializeLight() -> void;
    auto unserializeLight() -> void;
    auto checkSerialization(uint8_t* data, unsigned size) -> bool;
    auto unserialize(uint8_t* data, unsigned size) -> bool;
    auto serializeAll(Emulator::Serializer& s) -> void;

    auto setChipmem(unsigned value) -> void;
    auto getChipmem() -> unsigned;
    auto setSlowmem(unsigned value) -> void;
    auto getSlowmem() -> unsigned;

    auto setDrivesEnabled( uint8_t count ) -> void;
    auto getDrivesEnabled() -> uint8_t;

    auto hintObserverMotorChange(bool state) -> void;
    auto informAboutStateChange() -> void;

    auto displayFrame() -> const bool { return !runAhead.pos; }
};


}