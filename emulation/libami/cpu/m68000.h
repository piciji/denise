
#pragma once

#include "../agnus/agnus.h"
#include "m68000/m68000.h"

#include "../../tools/serializer.h"

namespace LIBAMI {

struct Cpu : M68FAMILY::M68000 {
    Cpu(Agnus& agnus);

    auto serialize(Emulator::Serializer& s) -> void;
};

}
