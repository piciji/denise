
#pragma once

namespace LIBC64 {
    
struct Zaxxon : GameCart {

    Zaxxon() : GameCart(true, true) {
        
    }

    auto readRomL( uint16_t addr ) -> uint8_t {

        if (  ((addr >> 12) & 1 ) == 1)
            cRomH = &chips[2];
        else
            cRomH = &chips[1];

        return GameCart::readRomL( addr );
    }

    auto init() -> void {
        cRomL = &chips[0];
        cRomH = &chips[1];
    }

    auto assumeChips( ) -> void {
    
        GameCart::assumeChips( {4096u, 8192u} );
    }
};    
    
}
