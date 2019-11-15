
#pragma once

#include "../../tools/listing.h"
#include "../../tools/serializer.h"
#include "../interface.h"

namespace LIBC64 {		

struct Prg {
	
	// loaded file
	uint8_t* data = nullptr;
	unsigned size = 0;
    uint8_t* headlinePtr = nullptr;    
	
	// prg chunk
	struct Chunk {
        unsigned id;
		uint8_t* data;
		unsigned dataOffset;
		unsigned size;			
		unsigned offset; // offset in c64 memory	
		unsigned end;
        uint8_t* namePtr;
	};
	
	std::vector<Chunk> chunks;
	Chunk* useChunk = nullptr;

	std::vector<Emulator::Interface::Listing> listings;
	
	auto select( unsigned pos ) -> bool;    
	auto unset() -> void;	
	auto set( uint8_t* data, unsigned size ) -> void;    
    auto serialize(Emulator::Serializer& s) -> void;
	auto inject( ) -> void;	
	auto getMemory(unsigned& prgSize) -> uint8_t*;	
	auto createListing() -> void;	
	auto getListing() -> std::vector<Emulator::Interface::Listing>;	
	auto prepareT64( ) -> void;	
	auto prepareP00() -> void;	
	auto preparePrg() -> void;	
	auto isP00() -> bool;	
	auto isT64() -> bool;	
	auto isPrg() -> bool;	
	auto saneSize() -> void;		
	auto find( std::vector<uint8_t> match, unsigned offset = 0 ) -> bool;
};

extern Prg* prg;
} 