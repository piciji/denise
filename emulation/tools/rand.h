
#pragma once

namespace Emulator {

struct Rand {
    
    uint32_t xorShift32 = 0x1234abcd;
    
    auto initXorShift( uint32_t state ) -> void {
        xorShift32 = state;
    }
    
    // Xorshift random number generators are a class of pseudorandom number generators.
    // it's a fast way too generate random numbers.
    // Note: it's no real randomness
    auto xorShift() -> uint32_t {
        
        xorShift32 ^= (xorShift32 << 13);
        
        xorShift32 ^= (xorShift32 >> 17);
        
        return xorShift32 ^= (xorShift32 << 5);
    }

};

}