
#pragma once

#include "cart.h"
#include "../system/system.h"

namespace LIBC64 {
    
struct SuperGamesMapper : Mapper {
        
	bool writeProtect;
	
    auto write( bool io1, uint16_t addr, uint8_t value ) -> void {
        
        if (writeProtect || io1 )
            return;       
				
		system->changeExpansionPortMemoryMode( !!(value & 4), !!(value & 4) );
        
        for( auto& chip : cart->chips ) {
            if (chip.bank == (value & 3) ) {
                cart->cRomL = &chip;
				cart->cRomH = &chip;
                break;
            }            
        }
        
		writeProtect = !!(value & 8);
    }
    
    auto init() -> void {
        cart->cRomL = &cart->chips[0];
        cart->cRomH = &cart->chips[0];
		writeProtect = false;
    }

    auto serialize(Emulator::Serializer& s) -> void {
        
        s.integer( writeProtect );
    }
};
    
}
