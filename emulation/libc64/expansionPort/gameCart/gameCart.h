
#pragma once

#include "../../interface.h"
#include "../cart/cart.h"

namespace LIBC64 {
    
struct GameCart : Cart {
    
    GameCart(bool game = true, bool exrom = false);        

    auto create( Interface::CartridgeId cartridgeId ) -> Cart*;   
    auto assign(Cart* cart) -> void;
};    
    
extern GameCart* gameCart;   

}