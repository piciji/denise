
#pragma once

#include "../../interface.h"
#include "diskStructure.h"
#include "../../cia/new/cia.h"

namespace Emulator {
    struct Serializer;
}

namespace LIBAMI {

struct Agnus;
struct DiskStructure;
struct System;
struct Interface;

struct DiskDrive {
    DiskDrive(uint8_t number, System* system, Agnus& agnus, Cia<MOS_8520>& cia);
    ~DiskDrive() {}

    auto attach(uint8_t* data, unsigned size) -> bool;
    auto detach() -> void;
    auto power() -> void;
    auto powerOff() -> void;

    uint8_t number;
    Agnus& agnus;
    System* system;
    Interface* interface;

    Cia<MOS_8520>& cia;
    DiskStructure structure;
    Emulator::Interface::Media* media;
    bool selected;
    bool motor;
    bool connected;
    bool inserted;

    uint8_t idPos;
    uint64_t motorClock;
    unsigned motorSpeed;

    uint64_t dskChangeClock;
    bool dskChange;
    bool written;
    bool driveSound;

    uint8_t cylinder;
    uint8_t side;
    DiskStructure::Track* track;
    unsigned headOffset;
    unsigned refCyclesPerRevolution;

    uint32_t accum;

    uint64_t stepClock; // minimum delay between steps
    uint64_t stepSettleClock; // time to read reliable from next track
    uint8_t nextStep;

    static unsigned rpm;
    static unsigned wobble;
    static unsigned refCyclesPerRevolutionBase;
    static unsigned stepperSeekTimeBase;
    unsigned stepperSeekTime;

    auto writeCiaPortB(uint8_t value, uint8_t oldValue) -> void;
    auto readCiaPortA() -> uint8_t;
    auto setMotor(bool state) -> void;
    auto step(bool dir, bool updTrack) -> void;
    auto getMotorSpeed() -> unsigned;
    auto getId() -> unsigned;
    auto updateTrack() -> void;
    auto progressStepper() -> void;

    auto readADF() -> uint8_t;
    auto writeADF(uint8_t data) -> void;
    auto readEXT(uint8_t& dmaCycles) -> bool;
    auto writeEXT(unsigned dmaCycles, bool bit) -> void;

    auto readBit() -> bool;
    auto writeBit(bool bit) -> void;
    auto updateDeviceState() -> void;
    auto enableSounds(bool state) -> void;

    auto updateRpm() -> void;
    static auto randomizeRpm(unsigned frequency) -> void;
    static auto setSpeed( unsigned rpmScaled ) -> void;
    static auto setWobble( unsigned wobbleScaled ) -> void;
    static auto setStepperSeekTime( unsigned stepperSeekTimeScaled ) -> void;
};

}
