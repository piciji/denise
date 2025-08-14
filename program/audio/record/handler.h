
#pragma once

#include <string>
#include "../../../emulation/interface.h"

namespace AudioRecord {

struct BaseWriter;
    
struct Handler {
		
    enum Type { MP3, WAV } type;

    BaseWriter* baseWriter = nullptr;
	
	unsigned startTime = 0;
	
	unsigned timeLimit = 0;
	
	unsigned sampleRate = 44100;
	
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
