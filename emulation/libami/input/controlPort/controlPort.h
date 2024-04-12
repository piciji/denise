
#pragma once

#include "../../../interface.h"

namespace LIBAMI  {

// interface for all control port connected devices

struct Input;

struct ControlPort {

    ControlPort( Emulator::Interface* interface, Input& input, Emulator::Interface::Device* device = nullptr );

    Emulator::Interface* interface;
    Input& input;
    Emulator::Interface::Device* device;

    static auto create( Emulator::Interface* interface, Input& input, Emulator::Interface::Device* device ) -> ControlPort*;

    virtual auto readButton1( ) -> uint8_t { return 0; }
    virtual auto readDirection( ) -> uint16_t { return 0; }
    virtual auto writeJoytest(uint16_t data) -> void {}

    virtual auto getPotX() -> uint8_t { return 0; }
    virtual auto getPotY() -> uint8_t { return 0; }

    virtual auto observePot(uint8_t& x, uint8_t& y) -> void {}

    virtual auto reset() -> void {}
    virtual auto poll() -> void {}
    virtual auto draw(bool midScreen = false) -> void {}

    virtual auto getCursorPosition( int16_t& x, int16_t& y ) -> bool { return false; }

    virtual auto serialize(Emulator::Serializer& s) -> void;
};

}

