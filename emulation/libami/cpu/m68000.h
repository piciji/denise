
#pragma once

#include "m68000/m68000.h"

namespace Emulator {
    struct Serializer;
}

namespace LIBAMI {

struct Agnus;

struct Cpu : M68FAMILY::M68000 {
    Cpu(Agnus& agnus);

    auto serialize(Emulator::Serializer& s) -> void;
};

}
