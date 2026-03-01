
#pragma once

#include <cstdint>
#include "../../tools/serializer.h"

namespace LIBAMI {

struct Agnus;

struct ExpansionPort {

    ExpansionPort(Agnus& agnus);

    Agnus& agnus;    

    enum class Type { ZORROII = 0xc0, RAM = 0x20, ROM_VECTOR = 0x10, SUB_CARD = 0x8 };

    enum class BoardState { AutoConf, Configured, ShutUp } boardState = BoardState::AutoConf;

    uint32_t baseAdr = 0;
    
    auto writeAutoConf(uint32_t addr, uint8_t data) -> void;

    auto writeAutoConfW(uint32_t addr, uint16_t data) -> void;

    auto encNibble(uint8_t val) -> uint8_t { return ~(val << 4) | 0xf; }

    virtual auto readAutoConf(uint32_t addr) -> uint8_t { return 0xff; }

    virtual auto read(uint32_t addr) -> uint8_t { return 0xff; }

    virtual auto peekW(uint32_t addr) -> uint16_t { return 0xffff; }

    virtual auto readW(uint32_t addr) -> uint16_t { return 0xffff; }

    virtual auto write(uint32_t addr, uint8_t data) -> void {}

    virtual auto writeW(uint32_t addr, uint16_t data) -> void {}

    virtual auto reset(bool softReset) -> void;

    auto firstPage() const -> unsigned { return baseAdr >> 16; }

    auto lastPage() const -> unsigned { return firstPage() + pages() - 1; }

    auto getSizeBits() -> uint8_t;

    auto add() -> void;

    virtual auto canBeShutUp() -> bool { return true; }

    // const 'before' means method don't change object, const 'after' means return value is constant
    virtual auto pages() const -> unsigned { return 0; };    

    virtual auto serialize(Emulator::Serializer& s, bool light = false) -> void {        
        s.integer((uint8_t&)boardState);
        s.integer(baseAdr);        
    }
};

}
