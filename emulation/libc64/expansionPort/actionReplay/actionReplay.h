
#pragma once

#include "../../interface.h"
#include "../cart/cart.h"

namespace LIBC64 {
    
struct ActionReplay : Cart {
    
    ActionReplay(bool game = true, bool exrom = false);

    unsigned cyclesTillFreeze = 0;
    bool freezeArmed = false;
    
    auto hasFreezeButton() -> bool { return true; }
    
    auto freeze() -> void;
    
    auto cycleLo() -> void;
    auto cycleHi() -> void;
    
    auto create( Interface::CartridgeId cartridgeId ) -> Cart*;
    
    virtual auto serializeStep2(Emulator::Serializer& s) -> void;
    auto assign(Cart* cart) -> void;
    
    virtual auto didFreeze() -> void {}
};    
    
extern ActionReplay* actionReplay;   

}