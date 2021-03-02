
#pragma once

namespace LIBC64 {
    
struct Zaxxon : GameCart {

    Zaxxon() : GameCart(false, false) {
        
    }

    auto readRomL( uint16_t addr ) -> uint8_t {

        if (  ((addr >> 12) & 1 ) == 1)
            cRomH = getChip(2);
        else
            cRomH = getChip(1);

        return GameCart::readRomL( addr );
    }

    auto reset(bool softReset = false) -> void {
        
        cRomL = getChip(0);
        cRomH = getChip(1);
    }

    auto assumeChips( ) -> void {
    
        Cart::assumeChips( {4096u, 8192u} );
    }
};    
    
}
