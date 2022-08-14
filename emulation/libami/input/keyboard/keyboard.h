
#pragma once

namespace Emulator {
    struct Serializer;
}

#include "../../../interface.h"

namespace LIBAMI {

struct Keyboard {

    Emulator::Interface::Device* device = nullptr;

    auto poll() -> void;

    auto reset() -> void;
    auto setDevice( Emulator::Interface::Device* device ) -> void;

    auto serialize( Emulator::Serializer& s ) -> void;
};

}
