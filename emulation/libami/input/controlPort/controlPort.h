
#pragma once

#include "../../../interface.h"

namespace LIBAMI  {

// interface for all control port connected devices

struct ControlPort {

    ControlPort( Emulator::Interface* interface, Emulator::Interface::Device* device = nullptr );

    Emulator::Interface* interface;
    Emulator::Interface::Device* device;

    static auto create( Emulator::Interface* interface, Emulator::Interface::Device* device ) -> ControlPort*;

    auto readButton1( ) -> uint8_t { return 1; }
    auto readDirection( ) -> uint16_t { return 0; }

    auto getPotX() -> uint8_t { return 0xff; }
    auto getPotY() -> uint8_t { return 0xff; }

    auto reset() -> void {}
    auto poll() -> void {}
    auto draw(bool midScreen = false) -> void {}

    auto getCursorPosition( int16_t& x, int16_t& y ) -> bool { return false; }

    auto serialize(Emulator::Serializer& s) -> void;

    auto useJitPolling() -> bool { return true; }
};

}

