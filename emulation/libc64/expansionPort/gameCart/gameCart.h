
#pragma once

#include "../../interface.h"
#include "../cart/cart.h"

namespace LIBC64 {
    
struct GameCart : Cart {
    
    GameCart(System* system, bool game = true, bool exrom = true);
    
    auto create( Interface::CartridgeId cartridgeId, unsigned _size ) -> GameCart*;
    auto assign(GameCart* cart) -> void;

    auto setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void override;

    auto build(Interface::CartridgeId cartridgeId, uint8_t* _rom, unsigned _romSize) -> GameCart*;

    auto serialize(Emulator::Serializer& s) -> void override;

    virtual auto serializeSwitchedIn(Emulator::Serializer& s) -> void;

    auto isBootable( ) -> bool override {
        return rom ? true : false;
    }
};

}
