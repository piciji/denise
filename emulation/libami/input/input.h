
#pragma once

#define ID_PORT_1 0
#define ID_PORT_2 1

#include "../../tools/serializer.h"
#include "../../interface.h"
#include "keyboard/keyboard.h"

namespace LIBAMI {

struct ControlPort;

struct Input {

    Input(Emulator::Interface* interface);

    Emulator::Interface* interface;
    ControlPort* controlPort1 = nullptr;
    ControlPort* controlPort2 = nullptr;
    Keyboard keyboard;
  //  Cia::Lines* lines = nullptr;
    uint8_t potMask;

    struct Jit {
        bool enable = false;
        bool allow = false;
        bool midscreen = false;
    } jit;

    auto connectControlport( Emulator::Interface::Connector* connector, Emulator::Interface::Device* device ) -> void;
    auto getConnectedDevice( Emulator::Interface::Connector* connector ) -> Emulator::Interface::Device*;
    auto getCursorPosition( Emulator::Interface::Device* device, int16_t& x, int16_t& y ) -> bool;

    auto readCiaPortA(  ) -> uint8_t;

    auto poll() -> void;

    auto reset() -> void;

    auto jitPoll() -> void;

    auto readPotX() -> uint8_t;
    auto readPotY() -> uint8_t;

    auto drawCursor(bool midScreen = false) -> void;
    auto serialize(Emulator::Serializer& s) -> void;

    auto enableJit(bool state) -> void;
    auto allowJit() -> void;
};

}
