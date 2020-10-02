
#pragma once

#include <cstdlib>
#include "serializer.h"

namespace Emulator {
	
// generate signal from AC voltage of power supply	
// tod source for cia chips	
	
struct PowerSupply {
	
	unsigned ticksPerSecond;
	unsigned powerFrequency;
	unsigned baseTicks;
	unsigned powerCounter;
	unsigned actualTicks;
	
	auto init( unsigned ticksPerSecond, unsigned powerFrequency ) -> void {
		
		this->ticksPerSecond = ticksPerSecond;
		this->powerFrequency = powerFrequency;
		// ideal tick count for each impulse
		this->baseTicks = ticksPerSecond / powerFrequency;		
		
		powerCounter = 0;
		actualTicks = 0;		
	}	
	
	auto nextTickCount() -> unsigned {		
		
		// ideal count of ticks so far within this second
		unsigned idealTicks = (ticksPerSecond * powerCounter) / powerFrequency;
		// real value is fluctuating from ideal value
		unsigned useTicks = baseTicks;
		
		// make sure real value doesn't go away too far from ideal value
		if (actualTicks < idealTicks)
			useTicks += random();			
		else
			useTicks -= random();
				
		// last impulse for this second
		if (++powerCounter == powerFrequency) {
			// use the remaining ticks
			useTicks = ticksPerSecond - actualTicks;
			powerCounter = 0;
			actualTicks = 0;
			
		} else
			// real value
			actualTicks += useTicks;
		
		return useTicks;
	}
	
	auto random() -> unsigned {
		// 0 - 3
		return rand() % 4;
	}
    
    auto serialize(Serializer& s) -> void {
        
        s.integer( ticksPerSecond );
        s.integer( powerFrequency );
        s.integer( baseTicks );
        s.integer( powerCounter );
        s.integer( actualTicks );
    }
};
	
}
