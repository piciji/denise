
#pragma once

#include "base.h"

namespace AudioRecord {

    struct WavWriter : BaseWriter {

	unsigned chunkPos = 0;
	
    auto init(std::string& path, unsigned sampleRate, bool useFloat) -> bool;
    
    auto write(uint8_t* buf, unsigned size) -> void;
    
    auto finish() -> void;

    auto writeChunk(unsigned value, uint8_t size, unsigned offset = 0) -> void;
}; 

}
