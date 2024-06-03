
#include "controlPort.h"

namespace LIBAMI {

    struct FourPlayerAdapter : Joypad {

        FourPlayerAdapter( Emulator::Interface* interface, Input& input, Emulator::Interface::Device* device )
        : Joypad( interface, input, device ) {}

        auto readParallelFromCIA1B(uint8_t& res) -> void {
            if (interface->inputPoll( device->id, 6 ) & 1) res &= ~1;
            if (interface->inputPoll( device->id, 7 ) & 1) res &= ~2;
            if (interface->inputPoll( device->id, 8 ) & 1) res &= ~4;
            if (interface->inputPoll( device->id, 9 ) & 1) res &= ~8;

            if (interface->inputPoll( device->id, 12 ) & 1) res &= ~16;
            if (interface->inputPoll( device->id, 13 ) & 1) res &= ~32;
            if (interface->inputPoll( device->id, 14 ) & 1) res &= ~64;
            if (interface->inputPoll( device->id, 15 ) & 1) res &= ~128;
        }

        auto readParallelFromCIA2A(uint8_t& res) -> void {
            if (interface->inputPoll( device->id, 10 ) & 1) res &= ~4;
            if (interface->inputPoll( device->id, 16 ) & 1) res &= ~1;

            if (interface->inputPoll( device->id, 11 ) & 1) res &= ~2;
            else if (interface->inputPoll( device->id, 17 ) & 1) res &= ~2;
        }

    };

}