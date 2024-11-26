
#pragma once

namespace LIBC64 {
    
struct Ocean : GameCart {
        
    Ocean(System* system) : GameCart(system, false, false) {
        
    }
    
    auto writeIo1( uint16_t addr, uint8_t value ) -> void {
        
        if (!getChip(0))
            return;
        
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
    
    auto reset(bool softReset = false) -> void {
        
        cRomL = getChip(0);
        cRomH = getChip(0);
    }

};
    
}
