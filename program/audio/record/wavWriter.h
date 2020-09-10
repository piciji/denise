
#pragma once

#include "../../../guikit/api.h"

namespace AudioRecord {

struct WavWriter {

	GUIKIT::File file;
	
	unsigned chunkPos = 0;
	
    auto create(std::string path) -> bool;
    
    auto writeHeader(unsigned sampleRate, bool useFloat) -> void;
    
    auto writeChunk( unsigned value, uint8_t size, unsigned offset = 0 ) -> void;

    auto write(uint8_t* buf, unsigned size) -> void;
    
    auto flush() -> void;
    
    auto finish() -> void;        
}; 

}
