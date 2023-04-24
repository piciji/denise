
#pragma once

namespace LIBC64 {
    
struct Funplay : GameCart {    
	
    Funplay(System* system) : GameCart(system, false, false) {
        
    }
    
    auto writeIo1( uint16_t addr, uint8_t value ) -> void {
        
		if ((value & 0xc6) == 0x86) {
			system->changeExpansionPortMemoryMode( exRom = true, game = true );
		} else if ((value & 0xc6) == 0) {
			system->changeExpansionPortMemoryMode( exRom = false, game = true );
		}
				   
		// linear bank order [0 - 15] in memory can be determined:
		// value = ((value >> 3) & 7) | ((value & 1) << 3);
		// chip header contains translated bank number
		
        for( auto& chip : chips ) {
            if (chip.bank == (value & 0x39)) {
                cRomL = &chip;
                break;
            }            
        }
    }
    
    auto assumeChips( ) -> void {
    
        Cart::assumeChips( );
        
        for (auto& chip : chips) {
            
            if (chip.bank > 7)                
                chip.bank = chips[ (chip.bank - 8) ].bank + 1;
            else
                chip.bank <<= 3;
        }
    }
    
    auto reset(bool softReset = false) -> void {
        
        cRomL = getChip(0);
        
        cRomH = nullptr;
    }

};
    
}
