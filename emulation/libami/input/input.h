
#pragma once

#define ID_PORT_1 0
#define ID_PORT_2 1

#include "../../tools/serializer.h"
#include "../../interface.h"

#include "keyboard/keyboard.h"
#include "../../cia/new/cia.h"

template<uint8_t model>
struct Cia;

namespace LIBAMI {

struct System;
struct Interface;
struct ControlPort;
struct Agnus;

struct Input {
    Input(System* system, Agnus& agnus, Cia<MOS_8520>& cia1);

    System* system;
    Interface* interface;
    Agnus& agnus;
    Cia<MOS_8520>& cia1;
    ControlPort* controlPort1 = nullptr;
    ControlPort* controlPort2 = nullptr;
    Keyboard keyboard;

    enum SamplingMode { Static_Sampling = 0, Restricted_Dynamic_Sampling = 1, Dynamic_Sampling = 2 };

    struct Sampling {
        SamplingMode mode = Dynamic_Sampling;
        bool allow = false;
        uint8_t midscreen = 0;
        bool externalKeyEvent = false;
    } sampling;

    auto connectControlport( Emulator::Interface::Connector* connector, Emulator::Interface::Device* device ) -> void;
    auto getConnectedDevice( Emulator::Interface::Connector* connector ) -> Emulator::Interface::Device*;
    auto getCursorPosition( Emulator::Interface::Device* device, int16_t& x, int16_t& y ) -> bool;

    auto readCia() -> uint8_t;
    auto writeCiaPort1(bool state) -> void;
    auto writeCiaPort2(bool state) -> void;

    auto readParallelportCIA1B(uint8_t& res) -> void;
    auto readParallelportCIA2A(uint8_t& res) -> void;

    auto readDenisePortA() -> uint16_t;
    auto readDenisePortB() -> uint16_t;
    auto writeDeniseJoytest(uint16_t data) -> void;

    auto observePot(uint8_t& x0, uint8_t& y0, uint8_t& x1, uint8_t& y1) -> void;
    auto writePot(uint8_t& x0, uint8_t& y0, uint8_t& x1, uint8_t& y1) -> void;
    auto observePotPort1(uint8_t& x0, uint8_t& y0) -> void;
    auto observePotPort2(uint8_t& x1, uint8_t& y1) -> void;

    auto initFrame() -> void;

    auto reset() -> void;

    auto jitPoll() -> void;

    auto emergencyPoll() -> void;

    auto drawCursor(bool midScreen = false) -> void;
    auto serialize(Emulator::Serializer& s) -> void;

    auto setSampling(uint8_t mode) -> void;
    auto updateSampling() -> void;
    inline auto externalKeyEvent() -> bool const { return sampling.externalKeyEvent; }
};

}
