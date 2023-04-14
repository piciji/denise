
#pragma once

#include "../expansionPort.h"
#include "../../../tools/memchangetracker.h"

namespace LIBC64 {
    
struct GeoRam : ExpansionPort {  
	
	GeoRam();
	~GeoRam();
	
	Emulator::Interface::Media* media;
	unsigned dumpSize = 0;
    uint8_t* dump = nullptr;
	uint8_t blockOf16k;
	uint8_t page;
	unsigned maxBlocks;

	unsigned size = 0; // in kb
	uint8_t* data = nullptr;
	
	bool dirty = false;
	bool writeProtect;

    MemChangeTracker<uint32_t, uint8_t> memChangeTracker;
	
	auto writeIo1( uint16_t addr, uint8_t value ) -> void;
	auto writeIo2( uint16_t addr, uint8_t value ) -> void;
	auto readIo1( uint16_t addr ) -> uint8_t;
	auto prepareRam(unsigned size) -> void;
	
	auto isBootable( ) -> bool;
	
	auto setRam(  Emulator::Interface::Media* media, uint8_t* dump, unsigned dumpSize ) -> void;
	auto unsetRam() -> void;
	auto injectRam( ) -> void;
	auto reset(bool softReset = false) -> void;
	
	auto write() -> void;

	static auto createImage(unsigned& imageSize, uint8_t id) -> uint8_t*;

	auto serialize(Emulator::Serializer& s) -> void;

	auto setWriteProtect(bool state) -> void;
	auto isWriteProtected() -> bool;

};

extern GeoRam* geoRam;
}
