
#include "controlPort.h"

namespace LIBAMI {

struct CD32Pad : ControlPort {
    uint16_t out = 0;
    uint8_t position = 8;
    bool serialMode = false;

    CD32Pad(Emulator::Interface* interface, Input& input, Emulator::Interface::Device* device)
    : ControlPort(interface, input, device) {}

    auto readCia() -> uint8_t {

        return interface->inputPoll(device->id, 4) & 1;
    }

    auto writeCia(bool state) -> void {
        if (!state && serialMode && position)
            position--;
    }

    auto readDirection() -> uint16_t {
        out &= ~0x303;

        // 0 0 -> no press
        // 1 0 -> both press
        // 0 1 -> Bit 1 press
        // 1 1 -> Bit 0 press

        if (interface->inputPoll(device->id, 2) & 1) out |= 0x300; // Left
        if (interface->inputPoll(device->id, 3) & 1) out |= 0x3; // Right

        if (interface->inputPoll(device->id, 0) & 1) out ^= 0x100; // Up
        if (interface->inputPoll(device->id, 1) & 1) out ^= 0x1; // Down

        return out;
    }

    auto writeJoytest(uint16_t data) -> void {
        out &= 0x303;
        out |= data & 0xfcfc;
    }

    auto writePot(uint8_t& x, uint8_t& y) -> void {
        bool _serialMode = serialMode;
        serialMode = x <= 100;
        if (_serialMode && !serialMode)
            position = 8;
    }

    auto observePot(uint8_t& x, uint8_t& y) -> void {
        uint16_t inputId = 5; // second button

        if (serialMode) {            
            switch (position) {
                case 8: inputId = 5; break; // blue
                case 7: inputId = 4; break; // red
                case 6: inputId = 7; break; // yellow
                case 5: inputId = 6; break; // green
                case 4: inputId = 9; break; // FFW
                case 3: inputId = 8; break; // RWD
                case 2: inputId = 10; break; // Play
                case 0:
                    y = 0;
                    // fallthrough
                case 1:
                default:
                    return;
            }
        }

        y = (interface->inputPoll(device->id, inputId) & 1) ? 0 : 0xff;
    }

    auto serialize(Emulator::Serializer& s) -> void {

        s.integer(out);
        s.integer(position);
        s.integer(serialMode);
    }

    auto reset() -> void {
        out = 0;
        position = 8;
        serialMode = false;
    }
};

}
