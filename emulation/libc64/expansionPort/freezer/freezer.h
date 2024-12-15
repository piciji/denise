
#pragma once

#include "../../interface.h"
#include "../cart/freezeButton.h"

namespace LIBC64 {
    
struct Freezer : FreezeButton {

    Freezer(System* system, bool game = true, bool exrom = true);
    
    auto create( Interface::CartridgeId cartridgeId, unsigned _size ) -> Cart*;
    
    auto assign(Cart* cart) -> void;

    virtual auto bootSpeed() -> float { return 0.5; }
            
};

}
