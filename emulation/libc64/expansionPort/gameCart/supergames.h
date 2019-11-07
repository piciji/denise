
#pragma once

namespace LIBC64 {
    
struct SuperGames : GameCart {
    
    SuperGames() : GameCart(false, false) {
        writeProtect = false;
    }       

    bool writeProtect = false; 	
	
    auto writeIo2( uint16_t addr, uint8_t value ) -> void {
        
        if (writeProtect)
            return;       
				
		system->changeExpansionPortMemoryMode( !!(value & 4), !!(value & 4) );
        
        for( auto& chip : chips ) {
            if (chip.bank == (value & 3) ) {
                cRomL = &chip;
				cRomH = &chip;
                break;
            }            
        }
        
		writeProtect = !!(value & 8);
    }
    
    auto reset() -> void {
        cRomL = getChip(0);
        cRomH = getChip(0);

		writeProtect = false;
    }

    auto assumeChips( ) -> void {
    
        Cart::assumeChips( {16384} );
    }    
    
    auto serializeStep2(Emulator::Serializer& s) -> void {
    
        Cart::serializeStep2( s );

        s.integer(writeProtect);   
    }
};
    
}
