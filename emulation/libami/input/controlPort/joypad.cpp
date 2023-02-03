
#include "controlPort.h"

namespace LIBAMI {

struct Joypad : ControlPort {

    Joypad( Emulator::Interface* interface, Emulator::Interface::Device* device ) : ControlPort( interface, device ) {}

    auto readButton1( ) -> uint8_t {

        return interface->inputPoll( device->id, 4 );
    }

    auto readDirection( ) -> uint16_t {
        uint16_t out = 0;

        // 0 0 -> no press
        // 1 0 -> both press
        // 0 1 -> Bit 1 press
        // 1 1 -> Bit 0 press

        if (interface->inputPoll( device->id, 2 )) out |= 0x300; // Left
        if (interface->inputPoll( device->id, 3 )) out |= 0x3; // Right

        if (interface->inputPoll( device->id, 0 )) out ^= 0x100; // Up
        if (interface->inputPoll( device->id, 1 )) out ^= 0x1; // Down

        return out;
    }

    auto useJitPolling() -> bool {
        return true;
    }

    auto getPotY() -> uint8_t {
        return interface->inputPoll( device->id, 5 ) ? 0 : 0xff;
    }
};

}