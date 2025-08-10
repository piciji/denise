
#pragma once

#include <string>
#include "../../../emulation/interface.h"

namespace AudioRecord {

struct WavWriter;    
    
struct Handler {
		
	WavWriter* wavWriter = nullptr;
	
	unsigned startTime = 0;
	
	unsigned timeLimit = 0;
	
	unsigned sampleRate = 44100;
	
	unsigned framesFlush;
	
	unsigned framesTimeCheck;
    
    bool useFloat;
		
    auto start( Emulator::Interface* emulator, std::string& errorText ) -> bool;
    
    auto setTimeLimit() -> void;
    
    auto run(Emulator::Interface* emulator = nullptr) -> bool;
    
    auto write( uint8_t* buf, unsigned frames ) -> void;
    
    auto checkTime() -> void;
    
    auto finish(bool timeup = false) -> void;

    auto toggle(Emulator::Interface* emulator, std::string& errorText) -> bool;
};

}
