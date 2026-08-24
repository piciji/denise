
#pragma once

#include <cstdlib>
#include "serializer.h"
#include "rand.h"

namespace Emulator {
	
// generate signal from AC voltage of power supply	
// tod source for cia chips	
	
struct PowerSupply {
	
	unsigned ticksPerSecond;
	unsigned powerFrequency;
	unsigned baseTicks;
    Rand randomizer;
    unsigned deviation;
	
	auto init( unsigned ticksPerSecond, unsigned powerFrequency ) -> void {
		
		this->ticksPerSecond = ticksPerSecond;
		this->powerFrequency = powerFrequency;
		// ideal tick count for each impulse
		this->baseTicks = static_cast<float>(ticksPerSecond) / static_cast<float>(powerFrequency) + 0.5f;
        deviation = 0;
	    randomizer.initXorShift();
	}	
	
	auto nextTickCount() -> unsigned {

        unsigned useTicks = baseTicks;

        if (deviation == 0) {
            deviation = randomizer.xorShift() & 15;
            useTicks += deviation;
        } else {
            useTicks -= deviation;
            deviation = 0;
        }

        return useTicks;
	}

    auto serialize(Serializer& s) -> void {
        
        s.integer( ticksPerSecond );
        s.integer( powerFrequency );
        s.integer( baseTicks );
        s.integer( deviation );
	    s.integer( randomizer.xorShift32 );
    }
};
	
}
