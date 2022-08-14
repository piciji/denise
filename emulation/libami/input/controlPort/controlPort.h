
#pragma once

#include "../../../interface.h"

namespace LIBAMI  {

// interface for all control port connected devices

struct ControlPort {

    ControlPort( Emulator::Interface* interface, Emulator::Interface::Device* device = nullptr );

    Emulator::Interface* interface;
    Emulator::Interface::Device* device;

    static auto create( Emulator::Interface* interface, Emulator::Interface::Device* device ) -> ControlPort*;

    virtual auto readFire( ) -> uint8_t { return 1; }
    virtual auto write( uint8_t value ) -> void {}

    virtual auto getPotX() -> uint8_t { return 0xff; }
    virtual auto getPotY() -> uint8_t { return 0xff; }

    virtual auto reset() -> void {}
    virtual auto poll() -> void {}
    virtual auto draw(bool midScreen = false) -> void {}

    virtual auto getCursorPosition( int16_t& x, int16_t& y ) -> bool { return false; }

    virtual auto serialize(Emulator::Serializer& s) -> void;

    virtual auto useJitPolling() -> bool { return true; }
};

}

