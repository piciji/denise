#include "../../system/system.h"
#include "geoRam.h"

namespace LIBC64 {  

GeoRam::GeoRam(System* system) : ExpansionPort(system), system(system) {
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
    unsigned _addr = (blockOf16k << 14) + (page << 8) + (addr & 0xff);

    if (memChangeTracker.enabled())
        memChangeTracker.remember(_addr, data);

	data[_addr] = value;
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

auto GeoRam::setRamSize(int id) -> void {
    switch (id) {
        default:
        case 0: prepareRam( 64 ); break;
        case 1: prepareRam( 128 ); break;
        case 2: prepareRam( 256 ); break;
        case 3: prepareRam( 512 ); break;
        case 4: prepareRam( 1024 ); break;
        case 5: prepareRam( 2048 ); break;
        case 6: prepareRam( 4096 ); break;
    }
}

auto GeoRam::getRamSize() -> int {
    switch (size) {
        case 64 * 1024: return 0;
        case 128 * 1024: return 1;
        case 256 * 1024: return 2;
        case 512 * 1024: return 3;
        case 1024 * 1024: return 4;
        case 2048 * 1024: return 5;
        case 4096 * 1024: return 6;
    }
    return 0;
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

auto GeoRam::reset(bool softReset) -> void {

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
    unsigned _size = size;
    bool light = s.lightUsage();

    s.integer(_size);
    if (s.mode() == Emulator::Serializer::Mode::Load) {
        if (light)
            memChangeTracker.applyAndDisable(data);
        else
            prepareRam(_size >> 10); // when size mismatches, recreate
    } else {
        if (light)
            memChangeTracker.enable();
    }

    if (!light)
		s.array(data, size);

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