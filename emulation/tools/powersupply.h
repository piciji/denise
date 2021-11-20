
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

    unsigned waitDelay;
    int aberration;
	
	auto init( unsigned ticksPerSecond, unsigned powerFrequency ) -> void {
		
		this->ticksPerSecond = ticksPerSecond;
		this->powerFrequency = powerFrequency;
		// ideal tick count for each impulse
		this->baseTicks = ticksPerSecond / powerFrequency;

        waitDelay = 0;
        aberration = 0;
	}	
	
	auto nextTickCount() -> unsigned {

        unsigned useTicks = baseTicks;

        if (waitDelay == 0) {

            unsigned _rand = rand();
            waitDelay = _rand & 7;

            if (aberration == 0) {
                aberration = (_rand >> 3) & 3;
                useTicks += aberration;
            } else {
                useTicks -= aberration;
                aberration = 0;
            }

        } else {
            waitDelay--;
        }

        return useTicks;
	}

    auto serialize(Serializer& s) -> void {
        
        s.integer( ticksPerSecond );
        s.integer( powerFrequency );
        s.integer( baseTicks );
        s.integer( aberration );
        s.integer( waitDelay );
    }
};
	
}
