
#pragma once

#include "cart.h"
#include "../system/system.h"

namespace LIBC64 {
    
struct FunplayMapper : Mapper {    
	
    auto write( bool io1, uint16_t addr, uint8_t value ) -> void {
        
        if (!io1 )
            return;       				
		
		if ((value & 0xc6) == 0x86) {
			system->changeExpansionPortMemoryMode( true, true );
		} else if ((value & 0xc6) == 0) {
			system->changeExpansionPortMemoryMode( false, true );
		}
				   
		// linear bank order [0 - 15] in memory can be determined:
		// value = ((value >> 3) & 7) | ((value & 1) << 3);
		// chip header contains translated bank number
		
        for( auto& chip : cart->chips ) {
            if (chip.bank == (value & 0x39)) {
                cart->cRomL = &chip;
                break;
            }            
        }
    }
    
    auto init() -> void {
        cart->cRomL = &cart->chips[0];
        cart->cRomH = nullptr;
    }

};
    
}
