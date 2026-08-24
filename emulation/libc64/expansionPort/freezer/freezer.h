
#pragma once

#include "../../interface.h"
#include "../cart/freezeButton.h"

namespace LIBC64 {
    
struct Freezer : FreezeButton {

    Freezer(System* system, bool game = true, bool exrom = true);
    
    auto create( Interface::CartridgeId cartridgeId, unsigned _size ) -> Freezer*;
    auto assign(Freezer* cart) -> void;

    auto setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void override;

    auto build(Interface::CartridgeId cartridgeId, uint8_t* _rom, unsigned _romSize) -> Freezer*;

    auto serialize(Emulator::Serializer& s) -> void override;

    virtual auto serializeSwitchedIn(Emulator::Serializer& s) -> void;

    auto bootSpeed() -> float override { return 0.5; }
            
};

}
