
#pragma once

#include "cart.h"

namespace LIBC64 {
    
    struct ZaxxonMapper : Mapper {
        
		auto readRomL( uint16_t addr ) -> void {
			
			if (  ((addr >> 12) & 1 ) == 1)
				cart->cRomH = &cart->chips[2];
			else
				cart->cRomH = &cart->chips[1];
		}
		
        auto init() -> void {
            cart->cRomL = &cart->chips[0];
			cart->cRomH = &cart->chips[1];
        }

    };    
    
}
