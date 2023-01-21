
#pragma once

#include "m68000/m68000.h"
#include <string>

namespace Emulator {
    struct Serializer;
}

namespace LIBAMI {

struct Agnus;

struct Cpu : M68FAMILY::M68000 {
    Cpu(Agnus& agnus);

    unsigned logCounter = 0;

    auto serialize(Emulator::Serializer& s) -> void;

    auto getIrd() -> uint16_t;
    auto getPC() -> uint32_t;
    auto getA(uint8_t pos) -> uint32_t;
    auto getD(uint8_t pos) -> uint32_t;
    auto getStatus() -> uint16_t;

    auto logState() -> void;
};

}
