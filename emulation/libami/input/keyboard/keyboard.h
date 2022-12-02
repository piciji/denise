
#pragma once

namespace Emulator {
    struct Serializer;
}

#include "../../../interface.h"
#include "../../../tools/circularBuffer.h"

#define MOS_8520 1
template<uint8_t model>
struct Cia;

namespace LIBAMI {

struct Agnus;

struct Keyboard {

    using EventCallback = std::function<void(uint8_t, uint16_t)>;
    CircularBuffer<uint8_t> queue;
    bool keyState[128];
    static const uint8_t keymap[96];

    enum State { KBD_Send, KBD_Selftest, KBD_Wait_For_Timeout, KBD_Initiate, KBD_Terminate, KBD_Hardreset,
            KBD_Transfer, KBD_Transfer1, KBD_Transfer2,
            KBD_Lost_Sync_Init, KBD_Lost_Sync_Transmit
    } state, memState;

    EventCallback callback;

    Keyboard(Emulator::Interface* interface, Agnus& agnus, Cia<MOS_8520>& cia);
    Agnus& agnus;
    Cia<MOS_8520>& cia;
    uint64_t handshakeClock;
    uint8_t shiftOut;
    uint8_t shiftPos;
    uint8_t curCode;
    bool overflow;
    bool capsLock;
    bool hardReset;

    Emulator::Interface* interface;
    Emulator::Interface::Device* device = nullptr;

    auto reset() -> void;
    auto setDevice( Emulator::Interface::Device* device ) -> void;
    auto sendKeyChange(bool pressed, Emulator::Interface::Device::Input* input) -> void;
    auto handshake(bool spLine) -> void;
    auto sendCode(uint8_t code) -> void;
    auto resync() -> void;

    auto serialize( Emulator::Serializer& s ) -> void;
};

}
