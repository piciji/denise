
#pragma once

#include "m68000/m68000.h"
#include <string>

namespace Emulator {
    struct Serializer;
}

namespace LIBAMI {

struct Agnus;
struct DebuggerSnapshot;

struct Cpu : M68FAMILY::M68000 {
    Cpu(Agnus& agnus);

    auto serialize(Emulator::Serializer& s) -> void;

    auto updateSnapshot(DebuggerSnapshot& snap) -> void;
};

}
