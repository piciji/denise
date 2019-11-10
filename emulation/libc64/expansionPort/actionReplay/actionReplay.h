
#pragma once

#include "../../interface.h"
#include "../cart/freezer.h"

namespace LIBC64 {
    
struct ActionReplay : Freezer {
    
    ActionReplay(bool game = true, bool exrom = true);
    
    auto create( Interface::CartridgeId cartridgeId ) -> Cart*;
    
    auto assign(Cart* cart) -> void;
            
};    
    
extern ActionReplay* actionReplay;   

}