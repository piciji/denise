
#pragma once

#include "../../tools/listing.h"
#include "../interface.h"

namespace Emulator {
    struct Serializer;
}

namespace LIBC64 {		

struct Prg {
    Prg(System* system);
    ~Prg();

    System* system;
    Emulator::Interface::Media* media = nullptr;
    
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
    auto updateLinkedList( uint16_t addr, uint16_t endAddr, bool dryRun = false) -> bool;
	static auto getMemory(unsigned& prgSize, uint8_t* ram) -> uint8_t*;
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
	auto buildLoadCommand( std::vector<uint8_t> loadPath ) -> std::vector<uint8_t>;
};

}
