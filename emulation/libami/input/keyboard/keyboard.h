
#pragma once

namespace Emulator {
    struct Serializer;
}

#include "../../../interface.h"

namespace LIBAMI {

struct Agnus;

struct Keyboard {

    using Callback = std::function<void(uint8_t id)>;

    enum {  };

    Callback callback;

    Keyboard(Emulator::Interface* interface, Agnus& agnus);
    Agnus& agnus;
    Emulator::Interface* interface;
    Emulator::Interface::Device* device = nullptr;

    auto poll() -> void;

    auto reset() -> void;
    auto setDevice( Emulator::Interface::Device* device ) -> void;

    auto serialize( Emulator::Serializer& s ) -> void;
};

}
