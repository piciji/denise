
#pragma once

#include "../../interface.h"
#include "../cart/cart.h"

namespace LIBC64 {
    
struct GameCart : Cart {
    
    GameCart(System* system, bool game = true, bool exrom = true);
    
    auto create( Interface::CartridgeId cartridgeId, unsigned _size ) -> Cart*;
    auto assign(Cart* cart) -> void;

    virtual auto setWriteProtect(bool state) -> void {}
    virtual auto isWriteProtected() -> bool { return false; }

    virtual auto isBootable( ) -> bool {
        return rom ? true : false;
    }
};

}
