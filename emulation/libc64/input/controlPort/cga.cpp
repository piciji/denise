
#include "controlPort.h"

namespace LIBC64 {

struct CGA : Joypad {

    CGA(System* system, Interface::Device* device)
    : Joypad( system, device ) {}

    auto connectUserPort() -> bool { return true; }
    

    auto readUserPort() -> uint8_t {
        uint8_t res = 0xff;

        if (system->cia2.lines.iob & 0x80) {
            if (interface->inputPoll(device->id, 6) & 1) res &= ~1;
            if (interface->inputPoll(device->id, 7) & 1) res &= ~2;
            if (interface->inputPoll(device->id, 8) & 1) res &= ~4;
            if (interface->inputPoll(device->id, 9) & 1) res &= ~8;

            if (interface->inputPoll(device->id, 10) & 1) res &= ~16;
            if (interface->inputPoll(device->id, 15) & 1) res &= ~32;

        } else {
            if (interface->inputPoll(device->id, 11) & 1) res &= ~1;
            if (interface->inputPoll(device->id, 12) & 1) res &= ~2;
            if (interface->inputPoll(device->id, 13) & 1) res &= ~4;
            if (interface->inputPoll(device->id, 14) & 1) res &= ~8;

            if (interface->inputPoll(device->id, 10) & 1) res &= ~16;
            if (interface->inputPoll(device->id, 15) & 1) res &= ~32;
        }

        return res;
    }

};

}
