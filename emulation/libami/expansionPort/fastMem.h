
#pragma once

#include <cstdint>
#include "expansionPort.h"

namespace LIBAMI {

struct Agnus;

struct FastMemExpansion : ExpansionPort {
    FastMemExpansion(Agnus& agnus) : ExpansionPort(agnus) { }

    auto type() -> const uint8_t { return (uint8_t)ExpansionPort::Type::ZORROII | (uint8_t)ExpansionPort::Type::RAM; }

    auto pages() const -> unsigned;

    auto readAutoConf(uint32_t addr) -> uint8_t;

    auto reset(bool softReset) -> void;

    auto serialize(Emulator::Serializer& s, bool light = false) -> void;
};

}
