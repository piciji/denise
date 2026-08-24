
#pragma once

namespace LIBC64 {
    
struct Zaxxon : GameCart {

    Zaxxon(System* system) : GameCart(system, false, false) {
        
    }

    auto peekRomL( uint16_t addr ) -> uint8_t override {
        return GameCart::readRomL( addr );
    }

    auto readRomL( uint16_t addr ) -> uint8_t override {

        if (  ((addr >> 12) & 1 ) == 1)
            cRomH = getChip(2);
        else
            cRomH = getChip(1);

        return GameCart::readRomL( addr );
    }

    auto reset(bool softReset = false) -> void override {
        
        cRomL = getChip(0);
        cRomH = getChip(1);
    }

    auto assumeChips( ) -> void override {
    
        Cart::assumeChips( {4096u, 8192u} );
    }
};    
    
}
