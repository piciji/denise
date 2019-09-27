
#pragma once

#include "../m6502/m6502.h"

namespace MOS65FAMILY {

struct M6510 : M6502 {
    
    M6510() {}
    
    auto reset() -> void;

    auto updateIoLines( uint8_t pullup, uint8_t pulldown = 0 ) -> void;
      
protected:
	
	inline auto busRead( uint16_t addr ) -> uint8_t;
	inline auto busWrite( uint16_t addr, uint8_t data ) -> void;
    inline auto busWatch() -> uint8_t;

	inline auto updateLines() -> void;
	inline auto chargeUndefinedBits( uint8_t newDdr ) -> void;
	inline auto advanceCounter() -> void;
	
};

}