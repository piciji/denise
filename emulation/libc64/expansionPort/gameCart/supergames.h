
#pragma once

namespace LIBC64 {
    
struct SuperGames : GameCart {
    
    SuperGames() : GameCart(false, false) {
        
    }
        
	bool writeProtect;
	
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
        cRomL = &chips[0];
        cRomH = &chips[0];
		writeProtect = false;
    }

    auto assumeChips( ) -> void {
    
        GameCart::assumeChips( {16384} );
    }
    
    auto serialize(Emulator::Serializer& s) -> void {
        
        s.integer( writeProtect );
        
        GameCart::serialize( s );
    }
    
};
    
}
