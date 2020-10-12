
#pragma once

#include "base.h"

namespace CIA {
    
struct M6526 : Base {   
	
	M6526( uint8_t model, Emulator::SystemTimer* events = nullptr ) : Base( model, events ) {}
	/**
	 * impulse on tod line occured
	 */
    auto tod() -> void;
	
	auto read(unsigned pos) -> uint8_t;
	auto write(unsigned pos, uint8_t value) -> void;
	auto reset() -> void;
    auto serialize(Emulator::Serializer& s) -> void;
        
protected:        
	bool todLatched;
	bool todActive;
	uint32_t todLatch;
	uint32_t alarm;
	uint32_t todc;
	unsigned tickCounter;
};

}