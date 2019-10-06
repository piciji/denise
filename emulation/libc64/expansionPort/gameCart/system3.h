
#pragma once

namespace LIBC64 {
    
struct System3 : GameCart {
    
    System3() : GameCart(false, true) {
        
    }

    auto writeIo1( uint16_t addr, uint8_t value ) -> void {

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
   
        cRomL = &chips[0];

        return ExpansionPort::readIo1( addr );
    }


    auto init() -> void {
        cRomL = &chips[0];
    }

};    
    
}
