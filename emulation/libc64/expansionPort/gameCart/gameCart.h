
#pragma once

#include "../../interface.h"
#include "../cart/cart.h"

namespace LIBC64 {
    
struct GameCart : Cart {
    
    GameCart(bool game = true, bool exrom = true);
    
    auto create( Interface::CartridgeId cartridgeId ) -> Cart*;   
    auto assign(Cart* cart) -> void;

    virtual auto setWriteProtect(bool state) -> void {}
    virtual auto isWriteProtected() -> bool { return false; }
	
	static auto createImage(unsigned& imageSize, uint8_t id) -> uint8_t*;

    auto isBootable( ) -> bool {    
        return rom ? true : false;
    }
};    
    
extern GameCart* gameCart;   

}