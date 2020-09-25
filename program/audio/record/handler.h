
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
		
    auto record( Emulator::Interface* emulator, std::string& errorText ) -> bool;
    
    auto setTimeLimit() -> void;
    
    auto run() -> bool;
    
    auto write( uint8_t* buf, unsigned frames ) -> void;
    
    auto checkTime() -> void;
    
    auto finish() -> void;
};

}
