
#pragma once

#include "cart.h"

namespace LIBC64 {
    
    struct System3Mapper : Mapper {
        
        auto write( bool io1, uint16_t addr, uint8_t value ) -> void {
            
            if (!io1)
                return;
            
            addr &= 63;
            addr %= cart->chips.size();
            
            for( auto& chip : cart->chips ) {
                if (chip.bank == addr ) {
                    cart->cRomL = &chip;
                    break;
                }                
            }            
        }
		
		auto read( bool io1, uint16_t addr ) -> uint8_t {
			if (io1) {
				cart->cRomL = &cart->chips[0];
			}
                
			return Mapper::read( io1, addr );
		}
        
        auto init() -> void {
            cart->cRomL = &cart->chips[0];
        }

    };    
    
}
