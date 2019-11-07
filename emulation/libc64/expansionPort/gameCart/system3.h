
#pragma once

namespace LIBC64 {
    
struct System3 : GameCart {
    
    System3() : GameCart(true, false) {
        
    }

    auto writeIo1( uint16_t addr, uint8_t value ) -> void {
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

    auto readIo1( uint16_t addr ) -> uint8_t {
   
        cRomL = getChip(0);

        return ExpansionPort::readIo1( addr );
    }


    auto reset() -> void {        
        cRomL = getChip(0);
        cRomH = nullptr;
    }

};    
    
}
