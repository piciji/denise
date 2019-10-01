
#pragma once

namespace LIBC64 {
    
struct Cart16k : GameCart {

    Cart16k() : GameCart(false, false) {
        
    }

    auto assumeChips( ) -> void {
    
        GameCart::assumeChips( {16384} );
    }
};    
    
}
