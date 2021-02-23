#include "../../system/system.h"
#include "geoRam.h"

namespace LIBC64 {  
	
GeoRam* geoRam = nullptr;	
	
GeoRam::GeoRam() : ExpansionPort() {
	setId( Interface::ExpansionIdGeoRam );
	prepareRam( 64 );
}	

GeoRam::~GeoRam() {
	if (data)
		delete[] data;
}

auto GeoRam::isBootable( ) -> bool {
    
    return false;
}

auto GeoRam::writeIo1( uint16_t addr, uint8_t value ) -> void {
	
	data[(blockOf16k << 14) + (page << 8) + (addr & 0xff)] = value;
	dirty = true;
}

auto GeoRam::writeIo2( uint16_t addr, uint8_t value ) -> void {
	
	if ((addr & 1) == 1) {
		blockOf16k = value & (maxBlocks - 1);
	}
	else if ((addr & 1) == 0) {
		page = value & 63;
	}
}

auto GeoRam::readIo1( uint16_t addr ) -> uint8_t {
	
	return data[(blockOf16k << 14) + (page << 8) + (addr & 0xff)];
}

auto GeoRam::prepareRam(unsigned size) -> void {    
	
	unsigned sizeInKb = size;

    size <<= 10;	
	
	maxBlocks = sizeInKb / 16;

	if (data && (size == this->size))
		return;

	if (data)
		delete[] data;

	this->size = size;

	data = new uint8_t[size];   
}

auto GeoRam::setRam( Emulator::Interface::Media* media, uint8_t* dump, unsigned dumpSize ) -> void {

	if ((this->dump == nullptr) && (dump == nullptr))
		return;

	if (this->dump && (dump == nullptr))
		write(); // unset
	
	this->media = media;
    this->dump = dump;
    this->dumpSize = dumpSize;       
}

auto GeoRam::unsetRam() -> void {
    this->dump = nullptr;
    this->dumpSize = 0;
}

auto GeoRam::injectRam( ) -> void {

    if (!dump || dumpSize == 0)
        return;
    
    std::memcpy(data, dump, std::min(dumpSize, size) );
}

auto GeoRam::reset() -> void {

	std::memset(data, 0, size);
	
	injectRam( );
	
	blockOf16k = 0;
	
	page = 0;
	
	dirty = false;
}

auto GeoRam::write() -> void {
	bool _dirty = dirty;
	dirty = false;
	
    if (!media || !media->guid || !_dirty || writeProtect )
        return;
        
    if (!system->interface->questionToWrite(media))
        return;    
    
    system->interface->truncateMedia( media );
	
	system->interface->writeMedia(media, data, size, 0);		
}

auto GeoRam::createImage(unsigned& imageSize, uint8_t id) -> uint8_t* {
	imageSize = 64 * 1024;
	uint8_t* buffer = new uint8_t[ imageSize ];
	std::memset(buffer, 0xff, imageSize);
	return buffer;
}

auto GeoRam::serialize(Emulator::Serializer& s) -> void {
	
	//if (!s.lightUsage()) {
		unsigned _size = size;

		s.integer(_size);
		if (s.mode() == Emulator::Serializer::Mode::Load) {
			// when size mismatches, recreate
			prepareRam(_size >> 10);
		}

		s.array(data, size);
	//}
	s.integer( blockOf16k );
	s.integer( page );
	s.integer( dirty );
	s.integer( writeProtect );
	
	ExpansionPort::serialize( s );
}

auto GeoRam::setWriteProtect(bool state) -> void {
	writeProtect = state;
}

auto GeoRam::isWriteProtected() -> bool {
	return writeProtect;
}

}