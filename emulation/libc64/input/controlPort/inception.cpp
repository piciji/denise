
#include "controlPort.h"

namespace LIBC64 {

    struct Inception : ControlPort {

        Inception(System* system, Interface::Device* device)
            : ControlPort(system, device) {
        }

        uint8_t position = 0;
        uint8_t clock = 0x10;

        auto read() -> uint8_t {
            uint8_t res = 0xf0;

            if ((system->input.lines->ddra & 0xf) != 0)
                return 0xff;

            switch (position) {
                case 0:
                default:
                    res = 0xff;
                    break;
                case 1:
                    if (interface->inputPoll(device->id, 4) & 1) res |= 1;
                    if (interface->inputPoll(device->id, 5) & 1) res |= 8;
                    break;
                case 2:
                    if (interface->inputPoll(device->id, 0) & 1) res |= 1;
                    if (interface->inputPoll(device->id, 1) & 1) res |= 2;
                    if (interface->inputPoll(device->id, 2) & 1) res |= 4;
                    if (interface->inputPoll(device->id, 3) & 1) res |= 8;
                    break;
                case 3:
                    if (interface->inputPoll(device->id, 10) & 1) res |= 1;
                    if (interface->inputPoll(device->id, 11) & 1) res |= 8;
                    break;
                case 4:
                    if (interface->inputPoll(device->id, 6) & 1) res |= 1;
                    if (interface->inputPoll(device->id, 7) & 1) res |= 2;
                    if (interface->inputPoll(device->id, 8) & 1) res |= 4;
                    if (interface->inputPoll(device->id, 9) & 1) res |= 8;
                    break;
                case 5:
                    if (interface->inputPoll(device->id, 16) & 1) res |= 1;
                    if (interface->inputPoll(device->id, 17) & 1) res |= 8;
                    break;
                case 6:
                    if (interface->inputPoll(device->id, 12) & 1) res |= 1;
                    if (interface->inputPoll(device->id, 13) & 1) res |= 2;
                    if (interface->inputPoll(device->id, 14) & 1) res |= 4;
                    if (interface->inputPoll(device->id, 15) & 1) res |= 8;
                    break;
                case 7:
                    if (interface->inputPoll(device->id, 22) & 1) res |= 1;
                    if (interface->inputPoll(device->id, 23) & 1) res |= 8;
                    break;
                case 8:
                    if (interface->inputPoll(device->id, 18) & 1) res |= 1;
                    if (interface->inputPoll(device->id, 19) & 1) res |= 2;
                    if (interface->inputPoll(device->id, 20) & 1) res |= 4;
                    if (interface->inputPoll(device->id, 21) & 1) res |= 8;
                    break;
                case 9:
                    if (interface->inputPoll(device->id, 28) & 1) res |= 1;
                    if (interface->inputPoll(device->id, 29) & 1) res |= 8;
                    break;
                case 10:
                    if (interface->inputPoll(device->id, 24) & 1) res |= 1;
                    if (interface->inputPoll(device->id, 25) & 1) res |= 2;
                    if (interface->inputPoll(device->id, 26) & 1) res |= 4;
                    if (interface->inputPoll(device->id, 27) & 1) res |= 8;
                    break;
                case 11:
                    if (interface->inputPoll(device->id, 34) & 1) res |= 1;
                    if (interface->inputPoll(device->id, 35) & 1) res |= 8;
                    break;
                case 12:
                    if (interface->inputPoll(device->id, 30) & 1) res |= 1;
                    if (interface->inputPoll(device->id, 31) & 1) res |= 2;
                    if (interface->inputPoll(device->id, 32) & 1) res |= 4;
                    if (interface->inputPoll(device->id, 33) & 1) res |= 8;
                    break;
                case 13:
                    if (interface->inputPoll(device->id, 40) & 1) res |= 1;
                    if (interface->inputPoll(device->id, 41) & 1) res |= 8;
                    break;
                case 14:
                    if (interface->inputPoll(device->id, 36) & 1) res |= 1;
                    if (interface->inputPoll(device->id, 37) & 1) res |= 2;
                    if (interface->inputPoll(device->id, 38) & 1) res |= 4;
                    if (interface->inputPoll(device->id, 39) & 1) res |= 8;
                    break;
                case 15:
                    if (interface->inputPoll(device->id, 46) & 1) res |= 1;
                    if (interface->inputPoll(device->id, 47) & 1) res |= 8;
                    break;
                case 16:
                    if (interface->inputPoll(device->id, 42) & 1) res |= 1;
                    if (interface->inputPoll(device->id, 43) & 1) res |= 2;
                    if (interface->inputPoll(device->id, 44) & 1) res |= 4;
                    if (interface->inputPoll(device->id, 45) & 1) res |= 8;
                    break;
            }

            return res;
        }

        auto write(uint8_t value) -> void {
            uint8_t _clock = value & 0x10;

            if (((value & 0x1f) == 0) )
                position = 0;
            else {
                if (clock != _clock) {
                    if (position < 16)
                        position++;
                    else
                        position = 0;
                }
            }

            clock = _clock;
        }

        auto reset() -> void {
            position = 0;
            clock = 0x10;
        }

        auto serialize(Emulator::Serializer& s) -> void {
            s.integer(position);
            s.integer(clock);
        }
    };

}
