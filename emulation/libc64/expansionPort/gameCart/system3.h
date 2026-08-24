
#pragma once

namespace LIBC64 {
    
struct System3 : GameCart {
    
    System3(System* system) : GameCart(system, true, false) {
        
    }

    auto writeIo1( uint16_t addr, uint8_t value ) -> void override {
        if (!getChip(0))
            return;

        addr &= 63;
        addr %= chips.size();

        for( auto& chip : chips ) {
            if (chip.bank == addr ) {
                cRomL = &chip;
                break;
            }                
        }            
    }

    auto peekIo1( uint16_t addr ) -> uint8_t override {
        return ExpansionPort::readIo1( addr );
    }

    auto readIo1( uint16_t addr ) -> uint8_t override {
   
        cRomL = getChip(0);

        return ExpansionPort::readIo1( addr );
    }


    auto reset(bool softReset = false) -> void override {
        cRomL = getChip(0);
        cRomH = nullptr;
    }

};    
    
}
