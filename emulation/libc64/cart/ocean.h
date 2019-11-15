
#pragma once

#include "cart.h"

namespace LIBC64 {
    
struct OceanMapper : Mapper {
        
    auto write( bool io1, uint16_t addr, uint8_t value ) -> void {
        
        if (!io1 )
            return;
        
        value &= 63;
        value %= cart->chips.size();
        
        for( auto& chip : cart->chips ) {
            if (chip.bank == value ) {
                cart->cRomL = &chip;
                cart->cRomH = &chip;
                break;
            }            
        }        
    }
    
    auto init() -> void {
        cart->cRomL = &cart->chips[0];
        cart->cRomH = &cart->chips[0];
    }

};
    
}
