
#include "prg.h"

#include "../system/keyBuffer.h"
#include <cstring>

namespace LIBC64 {
	
std::vector<Prg*> prgs;    
    
auto Prg::select( unsigned pos ) -> bool {

	auto _size = listings.size();
	
    if (_size <= pos) {
		if (_size == 1 && pos == 1) // fallback for PRG, which has no header
			pos = 0;
		else
			return false;
	}
	
    auto id = listings[ pos ].id;

    useChunk = &chunks[ id ];

    saneSize();

    if ( system->kernalBootComplete ) {
        inject();

    } else {
        KeyBuffer::Action action;
        action.mode = KeyBuffer::Mode::WaitDelay;
        action.delay = 0;        
        action.callbackId = 2;
        action.callback = [this]() { inject(); };
        system->keyBuffer->add( action );

        action.callback = nullptr;
        action.callbackId = 0;
        action.mode = KeyBuffer::Mode::Input;
        action.buffer = {'R', 'U', 'N', '\r'};    
        system->keyBuffer->add( action );            
    }

    return system->kernalBootComplete;
}

auto Prg::unset() -> void {
    useChunk = nullptr;
    chunks.clear();
    headlinePtr = nullptr;
}

auto Prg::set( uint8_t* data, unsigned size ) -> void {
    this->data = data;
    this->size = size;		
    unset();

    if ( !data || !size )
        return;

    if ( isP00() )		
        prepareP00();			
    else if ( isT64() )
        prepareT64();			
    else if ( isPrg() )
        preparePrg();										
}

auto Prg::serialize(Emulator::Serializer& s) -> void {

    int chunkId = useChunk ? useChunk->id : -1;

    s.integer( chunkId );

    if ( s.mode() == Emulator::Serializer::Mode::Load ) {

        useChunk = ( (chunkId >= 0) && (chunkId < chunks.size()) ) ? &chunks[chunkId] : nullptr;          
    }
}

auto Prg::inject( ) -> void {

    if (!useChunk)
        return;

    uint8_t* ram = system->ram;
    // put prg file in c64 memory
    for (unsigned i = 0; i < useChunk->size; i++)
        ram[ useChunk->offset + i ] = *(useChunk->data + i);

    uint16_t end = useChunk->offset + useChunk->size;
    // now we need to simulate the kernal load
    ram[0x2b] = ram[0xac] = ram[0x2b];
    ram[0x2c] = ram[0xad] = ram[0x2c];
    ram[0x2d] = ram[0x2f] = ram[0x31] = ram[0xae] = end & 0xff;
    ram[0x2e] = ram[0x30] = ram[0x32] = ram[0xaf] = end >> 8;				
}

auto Prg::getMemory(unsigned& prgSize) -> uint8_t* {
    uint8_t* ram = system->ram;

    unsigned _start = ram[0x2b] | (ram[0x2c] << 8);
    unsigned _end = ram[0x2d] | (ram[0x2e] << 8);    

    if (_end <= _start)
        return nullptr;

    if (_end > (64 * 1024))
        return nullptr;

    _end -= _start;

    prgSize = _end + 2;

    uint8_t* prgData = new uint8_t[ prgSize ];

    prgData[0] = ram[0x2b];
    prgData[1] = ram[0x2c];

    for( unsigned i = 0; i < _end; i++ )
        prgData[ i + 2 ] = ram[_start + i];

    return prgData;
}

auto Prg::createListing() -> void {        
    Emulator::C64Listing listing;
    listing.convertToScreencode = system->interface->convertToScreencode;

    unsigned id = 0;

    if (headlinePtr)
        listings.push_back( { id, listing.buildHeadline( headlinePtr ), listing.decodeToScreencode( {'R','U','N', ' ', '"', '*', '"'} ) } );

    for (auto& chunk : chunks) {
        unsigned listingSize = (chunk.size / 254) + ( (chunk.size % 254) ? 1 : 0);
        listings.push_back( { id++, listing.buildListing( chunk.namePtr, listingSize, 0x82 ), listing.decodeToScreencode( buildLoadCommand( listing.loader ) ) } );
    }
}

auto Prg::buildLoadCommand( std::vector<uint8_t> loadPath ) -> std::vector<uint8_t> {
    
	if (!loadPath.size())
		loadPath.insert( loadPath.begin(), '*' );
	
	loadPath.insert( loadPath.begin(), { 'R', 'U', 'N', ' ', '"' } );    		
	
	loadPath.insert( loadPath.end(), '"' );
	
	return loadPath;
}

auto Prg::getListing() -> std::vector<Emulator::Interface::Listing> {

    listings.clear();

    createListing();

    return listings;
}

auto Prg::prepareT64( ) -> void {
    uint8_t* ptr = data;
    ptr += 32; 

    unsigned maxNumber = ptr[2] | (ptr[3] << 8);
    unsigned id = 0;

    headlinePtr = ptr + 8;

    ptr += 32;

    std::vector<uint32_t> dataOffsets;

    for( unsigned i = 0; i < maxNumber; i++ ) {

        uint32_t dataOffset = ptr[0x8] | (ptr[0x9] << 8) | (ptr[0xa] << 16) | (ptr[0xb] << 24);			
        dataOffsets.push_back( dataOffset );

        if (ptr[0] != 0 ) {
            Chunk chunk;
            chunk.id = id++;
            chunk.data = data + dataOffset;
            chunk.dataOffset = dataOffset;
            chunk.offset = ptr[0x2] | (ptr[0x3] << 8);
            chunk.end = ptr[0x4] | (ptr[0x5] << 8);
            chunk.namePtr = ptr + 0x10;				
            chunks.push_back( chunk );
        }

        ptr += 32;
    }
    // sort offset ascending
    std::sort(dataOffsets.begin(), dataOffsets.end(), [ ](uint32_t a, uint32_t b) {
        return a < b;
    });
    // remove doubles
    dataOffsets.erase( std::unique( dataOffsets.begin(), dataOffsets.end() ), dataOffsets.end() );

    // calc sizes
    for( auto& chunk : chunks ) {

        for( unsigned i = 0; i < dataOffsets.size(); i++ ) {

            if ( chunk.dataOffset == dataOffsets[i] ) {

                if ( (i + 1) == dataOffsets.size() ) { // last value
                    // faulty end addresses
                    if (chunk.end == 0xc3c6) 
                        // beware: dirmaster adds zeros at the end, so total file size is not correct
                        chunk.size = size - dataOffsets[i];
                    else
                        chunk.size = chunk.end - chunk.offset;
                } else
                    chunk.size = dataOffsets[i+1] - dataOffsets[i];

                break;
            }
        }
    }
}

auto Prg::prepareP00() -> void {
    // 26 byte header ahead
    Chunk chunk;
    chunk.id = 0;
    chunk.data = data + 26 + 2;
    chunk.size = size - 26 - 2;
    chunk.offset = data[0x1a] | (data[0x1b] << 8);		
    chunk.namePtr = data + 8;

    chunks.push_back( chunk );	
}

auto Prg::preparePrg() -> void {	
    Chunk chunk;
    chunk.id = 0;
    chunk.data = data + 2;
    chunk.size = size - 2;
    chunk.offset = data[0] | (data[1] << 8);
    chunk.namePtr = nullptr;

    chunks.push_back( chunk );		
}

auto Prg::isP00() -> bool {
    if (size < 28)
        return false;
    // C64File 
    return find( {0x43, 0x36, 0x34, 0x46, 0x69, 0x6c, 0x65, 0x00} );
}

auto Prg::isT64() -> bool {
    if (size < 64)
        return false;
    // C64 tape image or C64S tape image or C64 tape file or C64S tape file
    return find( {0x43, 0x36, 0x34, 0x20, 0x74, 0x61, 0x70, 0x65, 0x20, 0x69, 0x6d, 0x61, 0x67, 0x65} )
        || find( {0x43, 0x36, 0x34, 0x53, 0x20, 0x74, 0x61, 0x70, 0x65, 0x20, 0x69, 0x6d, 0x61, 0x67, 0x65} )
        || find( {0x43, 0x36, 0x34, 0x20, 0x74, 0x61, 0x70, 0x65, 0x20, 0x66, 0x69, 0x6c, 0x65} )
        || find( {0x43, 0x36, 0x34, 0x53, 0x20, 0x74, 0x61, 0x70, 0x65, 0x20, 0x66, 0x69, 0x6c, 0x65} );
}

auto Prg::isPrg() -> bool {
    // can not check that much
    return size > 2; // first 2 bytes are offset
}

auto Prg::saneSize() -> void {
    // crop to c64 memory limit
    if ( useChunk->size > ( (64 * 1024) - useChunk->offset) )
        useChunk->size = (64 * 1024) - useChunk->offset;
}

auto Prg::find( std::vector<uint8_t> match, unsigned offset ) -> bool {

    for( auto& _char : match )	
        if ( _char != *(data + offset++) )
            return false;

    return true;
}

auto Prg::getInstance(Emulator::Interface::Media* media) -> Prg* {
    
    for(auto prg : prgs) {
        if (prg->media == media )
            return prg;
    }
    
    return nullptr;
}

}
