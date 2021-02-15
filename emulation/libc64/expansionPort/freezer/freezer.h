
#pragma once

#include "../../interface.h"
#include "../cart/freezeButton.h"

namespace LIBC64 {
    
struct Freezer : FreezeButton {

    Freezer(bool game = true, bool exrom = true);
    
    auto create( Interface::CartridgeId cartridgeId ) -> Cart*;
    
    auto assign(Cart* cart) -> void;
            
};    
    
extern Freezer* freezer;

}