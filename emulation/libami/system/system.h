
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
#include "../drive/hardDrive.h"
#include "rtc.h"
#include "../../tools/crop.h"
#include "../../tools/history.h"
#include "../../tools/serializer.h"
#include "debuggerSnapshot.h"

namespace LIBAMI {

struct Interface;

struct System {

    System(Interface* interface);
    ~System();

    Interface* interface;
    Input input;
    Cpu cpu;
    Agnus agnus;
    Denise denise;
    Paula paula;
    RTC rtc;
    DiskDrive diskDrives[4];
    HardDrive hardDrives[4];
    bool ntsc;
    bool firmwareChanged;
    bool asyncHDDAccess = false;

    Emulator::Crop<uint16_t> crop;

    Cia<MOS_8520> cia1;
    Cia<MOS_8520> cia2;
    History<uint32_t, uint16_t, 3> history;

    enum Dongle { DongleNone = 0, DongleRoboCop3, DongleBat2, DongleCricketCaptain, DongleLeaderBoard, DongleRugbyCoach, DongleStrikerManager };
    struct {
        Dongle type;
        uint8_t control;
        int64_t clock;
        auto connected() -> bool { return type != DongleNone; }
    } dongle;

    struct {
        unsigned config = 0;
        unsigned frameCounter;
        bool renderNext;
    } warp;

    struct {
        unsigned frames = 0;
        unsigned pos = 0;
        bool performance = false;
        bool preventJit = true;
        bool active = false;
        MemState<uint32_t, uint16_t, 3> memState;
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
    unsigned serializationSizeLight;
    Emulator::Serializer serializer;

    auto setFirmware(unsigned typeId, uint8_t* data, unsigned size, bool allowPatching) -> void;
    auto power(bool softReset = false, bool resetInstruction = false) -> void;
    auto powerOff() -> void;
    auto run() -> void;
    auto informAboutKeyUpdate() -> void;
    auto videoRefresh( uint16_t* frame, unsigned width, unsigned height, unsigned linePitch, uint8_t options) -> void;
    auto audioRefresh(int16_t left, int16_t right) -> void;
    auto setModel(uint8_t model) -> void;
    auto getModel() -> uint8_t;
    auto updateStats() -> void;
    auto updateDriveSounds() -> void;
    auto setFloppySounds(bool state) -> void;
    auto setRTC(bool state) -> void;
    auto useRTC() -> bool;

    auto setRegion( int region ) -> void;
    auto setResampleQuality( int value ) -> void;
    auto setWarpMode( unsigned config ) -> void;
    auto setRunAhead(unsigned frames) -> void;
    auto runAheadPreventJit() -> bool { return runAhead.preventJit && runAhead.frames; }
    auto allowRunAhead() -> const bool { return !warp.config && runAhead.frames && !agnus.resetFromKeyboard
        && !cpu.inDebugMode() && agnus.womLocked() && !agnus.debugger.dmaLog; }
    auto hintSlowSpeed(bool state) -> void;

    auto calcSerializationSize() -> void;
    auto serialize(unsigned& size) -> uint8_t*;
    auto serializeLight(MemState<uint32_t, uint16_t, 3>& memState, bool fromHistory = false) -> void;
    auto unserializeLight(MemState<uint32_t, uint16_t, 3>& memState, bool fromHistory = false) -> void;
    auto checkSerialization(uint8_t* data, unsigned size) -> bool;
    auto unserialize(uint8_t* data, unsigned size) -> bool;
    auto serializeAll(Emulator::Serializer& s) -> void;
    auto serialize(Emulator::Serializer& s) -> void;

    auto setChipmem(unsigned value) -> void;
    auto getChipmem() -> unsigned;
    auto setSlowmem(unsigned value) -> void;
    auto getSlowmem() -> unsigned;
    auto setFastmem(unsigned value) -> void;
    auto getFastmem() -> unsigned;

    auto setDiskDrivesEnabled( uint8_t count ) -> void;
    auto getDiskDrivesEnabled() -> uint8_t;
    auto setHardDrivesEnabled(uint8_t count) -> void;
    auto getHardDrivesEnabled() -> uint8_t;

    auto hintObserverMotorChange(bool state) -> void;
    auto observeInputFetches() -> void;
    auto informAboutStateChange() -> void;

    auto isDisplayFrame() -> const bool { return !runAhead.pos; }
    auto isProcessFrame() -> const bool { return !runAhead.active || (runAhead.frames == runAhead.pos); }

    template<bool CIA2> auto dongleCiaWrite(Cia<MOS_8520>::Lines* lines) -> void;
    template<bool CIA2> auto dongleCiaRead(Cia<MOS_8520>::Lines* lines, uint8_t& val) -> void;
    template<bool portB> auto dongleJoydat(uint16_t& val) -> void;
    auto donglePotGo(uint16_t& val) -> void;

    auto setOverclock(unsigned factor) -> void;
    auto getOverclock() -> unsigned;

    auto setHDDAsync(bool state) -> void;
    auto getHDDAsync() -> bool;
    auto cropFrame( Emulator::Interface::CropType type, Emulator::Interface::Crop crop ) -> void;

    auto debuggerAdd(Emulator::Interface::DebuggerChip chip, Emulator::Interface::DebuggerAction action, unsigned addr, unsigned addrTo) -> void;
    auto debuggerRemove(Emulator::Interface::DebuggerChip chip, Emulator::Interface::DebuggerAction action, unsigned addr) -> void;
    auto debuggerEnable(Emulator::Interface::DebuggerChip chip, Emulator::Interface::DebuggerAction action, unsigned addr) -> void;
    auto debuggerDisable(Emulator::Interface::DebuggerChip chip, Emulator::Interface::DebuggerAction action, unsigned addr) -> void;
    auto updateDebuggerSnapshot(DebuggerSnapshot& snap) -> void;
    auto updateCiaDebuggerSnapshot(DebuggerSnapshot& snap) -> void;
};


}