
#pragma once

namespace LIBC64 {
    
struct Cart16k : GameCart {

    Cart16k(System* system) : GameCart(system, false, false) {
        
    }

    auto assumeChips( ) -> void override {
    
        Cart::assumeChips( {16384} );
    }
};    
    
}
