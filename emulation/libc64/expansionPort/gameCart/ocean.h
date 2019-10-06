
#pragma once

namespace LIBC64 {
    
struct Ocean : GameCart {
        
    Ocean() : GameCart(false, false) {
        
    }
    
    auto writeIo1( uint16_t addr, uint8_t value ) -> void {
        
        value &= 63;
        value %= chips.size();
        
        for( auto& chip : chips ) {
            if (chip.bank == value ) {
                cRomL = &chip;
                cRomH = &chip;
                break;
            }            
        }        
    }
    
    auto init() -> void {
        cRomL = &chips[0];
        cRomH = &chips[0];
    }

};
    
}
