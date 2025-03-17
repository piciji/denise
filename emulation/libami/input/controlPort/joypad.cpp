
#include "controlPort.h"

namespace LIBAMI {

struct Joypad : ControlPort {
    uint16_t out = 0;

    Joypad( Emulator::Interface* interface, Input& input, Emulator::Interface::Device* device )
    : ControlPort( interface, input, device ) {}

    auto readCia() -> uint8_t {

        return interface->inputPoll( device->id, 4 ) & 1;
    }

    auto readDirection( ) -> uint16_t {
        out &= ~0x303;

        // 0 0 -> no press
        // 1 0 -> both press
        // 0 1 -> Bit 1 press
        // 1 1 -> Bit 0 press

        if (interface->inputPoll( device->id, 2 ) & 1) out |= 0x300; // Left
        if (interface->inputPoll( device->id, 3 ) & 1) out |= 0x3; // Right

        if (interface->inputPoll( device->id, 0 ) & 1) out ^= 0x100; // Up
        if (interface->inputPoll( device->id, 1 ) & 1) out ^= 0x1; // Down

        return out;
    }

    auto writeJoytest(uint16_t data) -> void {
        out &= 0x303;
        out |= data & 0xfcfc;
    }

    auto observePot(uint8_t& x, uint8_t& y) -> void {
        // PotY = Pin9 = Button 2
        // PotX = Pin5 = Button 3

        // with pullup resistor
        y = (interface->inputPoll( device->id, 5 ) & 1) ? 0 : 0xff;
        x = (interface->inputPoll( device->id, 6 ) & 1) ? 0 : 0xff;
    }

    auto serialize(Emulator::Serializer& s) -> void {

        s.integer(out);
    }

    auto reset() -> void {
        out = 0;
    }
};

}