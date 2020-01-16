#pragma once

#include "../../interface.h"
#include "../cart/freezer.h"

namespace LIBC64 {
    
struct RetroReplay : Freezer {
    
    RetroReplay(bool game = true, bool exrom = true);
    
    auto create( Interface::CartridgeId cartridgeId ) -> Cart*;
    
    auto assign(Cart* cart) -> void;
    
    auto writeIo1( uint16_t addr, uint8_t value ) -> void;
    
    bool flashJumper;
    bool bankJumper;
    bool enabled;
    uint8_t bank;
    bool frozen;
    bool ramMode;
    bool active;
};    
    
extern RetroReplay* retroReplay;   

}