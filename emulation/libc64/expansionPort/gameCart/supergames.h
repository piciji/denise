
#pragma once

namespace LIBC64 {
    
struct SuperGames : GameCart {
    
    SuperGames(System* system) : GameCart(system, false, false) {
        writeProtect = false;
    }       

    bool writeProtect = false;
	bool mafiosino = false; // fastloader
	
	auto isBootable( ) -> bool override {
		return !mafiosino;
	}
	
    auto writeIo2( uint16_t addr, uint8_t value ) -> void {
        
        if (writeProtect)
            return;       
				
        exRom = !!(value & 4);
        game = !!(value & 4);
        
		system->changeExpansionPortMemoryMode( exRom, game );
        
        for( auto& chip : chips ) {
            if (chip.bank == (value & 3) ) {
                cRomL = &chip;
				cRomH = &chip;
                break;
            }
        }
        
		writeProtect = !!(value & 8);
    }
    
    auto reset(bool softReset = false) -> void {
        cRomL = getChip(0);
        cRomH = getChip(0);

		writeProtect = false;
		
		uint8_t mafioPattern[] = { 0x4d, 0x41, 0x46, 0x49, 0x4f };
		uint8_t* mafioPatternLast = mafioPattern + sizeof(mafioPattern);
		uint8_t* dataLast = data + std::min<unsigned>(2048 << 2, size);

		uint8_t* result = std::search(data, dataLast, mafioPattern, mafioPatternLast);
		
		mafiosino = result != dataLast;
    }

    auto assumeChips( ) -> void {
    
        Cart::assumeChips( {16384} );
    }    
    
    auto serializeSwitchedIn(Emulator::Serializer& s) -> void {
    
        Cart::serialize( s );

        s.integer(writeProtect);
		
		s.integer(mafiosino);
    }
};
    
}
