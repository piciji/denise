
#pragma once

#include "m68000/m68000.h"
#include "../../tools/serializer.h"
#include "../system/system.h"

namespace LIBAMI {

struct M68000 : M68FAMILY::M68000 {
    M68000();

//    auto sync(uint16_t cycles) -> void;
//
//    auto readByte(uint32_t adr) -> uint8_t;
//    auto readWord(uint32_t adr) -> uint16_t;
//    auto writeByte(uint32_t adr, uint8_t data) -> void;
//    auto writeWord(uint32_t adr, uint16_t data) -> void;

    auto IackCycle(uint8_t level, uint8_t& vector) -> uint8_t;

    auto serialize(Emulator::Serializer& s) -> void;
};

extern M68000* cpu;

}
