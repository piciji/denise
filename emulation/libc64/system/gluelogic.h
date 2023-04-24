
#pragma once

#include <cstdint>
#include <functional>

namespace Emulator {
    struct Serializer;
    struct SystemTimer;
}

namespace LIBC64 {

struct System;

struct GlueLogic {
    
    enum Type : uint8_t { Discrete = 0, CustomIC = 1 } type = Type::Discrete;
    
    using Callback = std::function<void ()>;

    Emulator::SystemTimer& sysTimer;
    System* system;

    Callback updateVbank;
    
    uint8_t vbankBefore;
    
    GlueLogic(System* system, Emulator::SystemTimer& sysTimer);
    
    auto serialize(Emulator::Serializer& s) -> void;
    
    auto setType( Type type ) -> void;
    
    auto setVBank( uint8_t vbank, bool ddrChange ) -> void;
    
    auto reset() -> void;
};
    
}
