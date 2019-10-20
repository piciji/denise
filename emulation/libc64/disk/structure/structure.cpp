
#include "structure.h"
#include "../../system/system.h"
#include "d64.cpp"
#include "g64.cpp"
#include "../../../tools/petcii.h"
#include "../../../tools/listing.h"
#include "../../system/keyBuffer.h"

namespace LIBC64 {
    
const unsigned Structure1541::MAX_TRACKS = MAX_TRACKS_1541;    
const unsigned Structure1541::TYPICAL_TRACKS = 35; 
const unsigned Structure1541::TYPICAL_SIZE = 174848;  // for 35 tracks in cbm dos
const uint8_t Structure1541::SECTORS_IN_SPEEDZONE[4] = { 17, 18, 19, 21 };
const unsigned Structure1541::BYTES_IN_SPEEDZONE[4] = { 6250, 6666, 7142, 7692 };
const uint8_t Structure1541::GAPS_IN_SPEEDZONE[4] = { 9, 12, 17, 8 };
    
Structure1541::Structure1541() {
    
    errorMap = nullptr;
    errorMapSize = 0;
    
    for( unsigned i = 0; i < (MAX_TRACKS * 2); i++ ) {
        gcrTrack[i].data = nullptr;
        gcrTrack[i].size = 0;
        gcrTrack[i].bits = 0;
    }
}   

Structure1541::~Structure1541() {
    
    clearTrackData();   
}

auto Structure1541::attach( uint8_t* data, unsigned size ) -> bool {
    rawData = data;
    rawSize = size;
    
    if ( !analyze() )
        return false;
    
    prepare();
    
    return true;
}

auto Structure1541::detach() -> void {    
    rawData = nullptr;
    rawSize = 0;
    
    clearTrackData(); 
}

auto Structure1541::clearTrackData() -> void {
    for (unsigned i = 0; i < (MAX_TRACKS * 2); i++) {
        auto trackPtr = &gcrTrack[i];
        
        if (trackPtr->data)
            delete[] trackPtr->data;

        trackPtr->data = nullptr;
        trackPtr->size = 0;
        trackPtr->bits = 0;
    }
    
    if (errorMap)
        delete[] errorMap;

    errorMap = nullptr;
    errorMapSize = 0;
}
    
auto Structure1541::speedzone( uint8_t track ) -> uint8_t {
    // speedzone: 0 - 3, depends on track sector count
    return (track < 31) + (track < 25) + (track < 18);
}   

auto Structure1541::countSectors( uint8_t track ) -> uint8_t {
    
    return SECTORS_IN_SPEEDZONE[ speedzone( track ) ];
}

auto Structure1541::countBytes( uint8_t track ) -> unsigned {
    
    return BYTES_IN_SPEEDZONE[ speedzone( track ) ];
}

auto Structure1541::gapSize( uint8_t track ) -> unsigned {
    
    return GAPS_IN_SPEEDZONE[ speedzone( track ) ];
}

auto Structure1541::countSectors( uint8_t track, uint8_t sector ) -> int {
    
    int sectors = 0;
    
    if (track > MAX_TRACKS)
        return -1;
    
    if (sector >= countSectors( track ) )
        return -2;
    
    for (uint8_t i = 1; i < track; i++)
        sectors += countSectors( i );
    
    sectors += sector;
    
    return sectors;
}

auto Structure1541::analyze() -> bool {        
    
    type = Type::Unknown;
    
    if (!rawData || !rawSize)
        return false;
    
    if ( analyzeD64() )
        return true;
    
    if ( analyzeG64() )
        return true;            
    
    return false;
}

auto Structure1541::prepare() -> void {
    
    switch( type ) {
        case Type::D64:
            prepareD64();
            break;
        case Type::G64:
            prepareG64();
            break;
    }            
}

auto Structure1541::imageSize( Type newType ) -> unsigned {
    
    switch( newType ) {
        case Type::D64:
            return imageSizeD64();
        case Type::G64:
            return imageSizeG64();
    }
    
    return 0;
}

auto Structure1541::createListing( ) -> void {
    
    if (!rawData || (type == Type::Unknown))
        return;
    
    if (tracks < 18)
        return;
    
    Emulator::C64Listing listing;
    listing.convertToScreencode = system->interface->convertToScreencode;
        
    unsigned id = 0;
    
    uint8_t buffer[256];    
    
    decodeSector( &gcrTrack[17 * 2], buffer, 0 );
    
    listings.push_back( { id++, listing.buildHeadline( buffer + 0x90, buffer + 0xa5, buffer + 0xa2 ) } );
    loader.push_back( {'*'} );
        
    decodeSector( &gcrTrack[17 * 2], buffer, 1 );
    
    uint8_t* ptr = &buffer[0];
    
    unsigned entry = 0;
    
    while(1) {
        
        unsigned listingSize = *(ptr + 0x1f) * 256 + *(ptr + 0x1e);        
        
        if ( *(ptr + 0x2) != 0 ) {       
            listings.push_back( { id++, listing.buildListing( ptr + 0x5, listingSize, *(ptr + 0x2) ) } );
            loader.push_back( listing.loader );
        }
        
        ptr += 0x20;
        entry++;
        
        if ((entry & 7) == 0) {
            
            // if the disk doesn't use the dir track, it could produce an endless loop
            if (entry > 250)                
                break;
            
            uint8_t _track = buffer[0];
            uint8_t _sector = buffer[1]; 
                        
            if (_track > tracks)
                break;
            
            if (_track == 0)
                break;

            if ( decodeSector(&gcrTrack[(_track - 1) * 2], buffer, _sector) != ERR_OK)
                break;
            
            ptr = &buffer[0];
        }        
    }
    
    decodeSector( &gcrTrack[17 * 2], buffer, 0 );
    
    unsigned freeBlocks = 0;
    
    for (uint8_t track = 1; track <= tracks; track++) {
        
        uint8_t* bamPtr = track <= TYPICAL_TRACKS
            ? &buffer[4 + 4 * (track - 1)] 
            : &buffer[192 + 4 * (track - TYPICAL_TRACKS - 1)];
        
        if (track != 18)
            freeBlocks += *bamPtr;
    }
    
    listings.push_back( { id++, listing.buildFreeLine( freeBlocks ) } );
    loader.push_back( {'*'} );
}

auto Structure1541::getListing( ) -> std::vector<Emulator::Interface::Listing>& {
    
    listings.clear();
    loader.clear();

    createListing();
        
    return listings;
}

auto Structure1541::selectListing( Emulator::Interface::Media* media, unsigned pos ) -> void {
    
    std::vector<uint8_t> entry;    
    
    if (pos >= loader.size())
        entry.push_back( '*' );
    else
        entry = loader[pos];
    
    entry.insert( entry.begin(), { 'L', 'O', 'A', 'D', '"' } );
    
    entry.insert( entry.end(), { '"', ',' } );        
    
    switch(media->id) {
        case 0:
        default: entry.insert( entry.end(), '8' ); break;
        case 1: entry.insert( entry.end(), '9' ); break;
        case 2: entry.insert( entry.end(), {'1', '0' } ); break;
        case 3: entry.insert( entry.end(), {'1', '1' } ); break;
    }
               
    entry.insert( entry.end(), { ',', '1', '\r' } );        
    
    KeyBuffer::Action action;
    
    action.mode = KeyBuffer::Mode::Input;
    action.buffer = entry;    
    system->keyBuffer->add( action );
    
    action.mode = KeyBuffer::Mode::WaitFor;
    action.buffer = {'S', 'E', 'A', 'R', 'C', 'H', 'I', 'N', 'G'};  
    action.blinkingCursor = false;
    action.delay = 0;
    system->keyBuffer->add( action );
    
    action.mode = KeyBuffer::Mode::WaitFor;
    action.buffer = {'L', 'O', 'A', 'D', 'I', 'N', 'G'};  
    action.alternateBuffer = {'S', 'E', 'A', 'R', 'C', 'H', 'I', 'N', 'G'};  
    action.blinkingCursor = false;
    system->keyBuffer->add( action );
    
    action.mode = KeyBuffer::Mode::WaitFor;
    action.buffer = {'R', 'E', 'A', 'D', 'Y', '.'};  
    action.delay = 120;    
    action.alternateBuffer.clear();
    action.blinkingCursor = true;
    system->keyBuffer->add( action );
    
    action.mode = KeyBuffer::Mode::Input;
    action.buffer = {'R', 'U', 'N', '\r'};    
    system->keyBuffer->add( action );
}

auto Structure1541::create( Type newType, std::string diskName ) -> uint8_t* {
    
    switch( newType ) {
        case Type::D64:
            return createD64( diskName );    
        case Type::G64:
            return createG64( diskName );
    } 
    
    return nullptr;
}

auto Structure1541::createBAM( std::string diskName, uint8_t tracksInImage, uint8_t* buffer ) -> void {
    
    Emulator::PetciiConversion petciiConversion;

    diskName = petciiConversion.encode( diskName );

    auto id = cutId( diskName );
    
    std::memset(buffer, 0, 256);
    
    buffer[0] = 18;
    buffer[1] = 1;
    buffer[2] = 65;
    
    std::memset( buffer + 144, 0xa0, 27 );
    std::memcpy( buffer + 144, diskName.c_str(), diskName.size() );
    std::memcpy( buffer + 162, id.c_str(), id.size() );
    
    buffer[165] = 50;
    buffer[166] = 65;
    
    // to calculate the free blocks bam sector contains a usage bit for all sectors    
    for (uint8_t track = 1; track <= tracksInImage; track++) {
        
        uint8_t sectors = countSectors( track );                
        
        uint8_t* bamPtr = track <= TYPICAL_TRACKS
            ? &buffer[4 + 4 * (track - 1)] 
            : &buffer[192 + 4 * (track - TYPICAL_TRACKS - 1)];
        
        for (uint8_t sector = 0; sector < sectors; sector++) {                        
            
            // sectors in use keep zero
            if (track == 18 && ( sector == 0 || sector == 1 ))
                continue;
            
            // mark unused sectors
            bamPtr[1 + sector / 8] |= (1 << (sector & 7));

            *bamPtr += 1; // first byte count all unused sectors in a track
        }        
    }
}

auto Structure1541::cutId( std::string& diskName ) -> std::string {
    std::string id = "  ";
    
    if (diskName.size() == 0)
        diskName = " ";
            
    std::size_t start = diskName.find_last_of(",");
    
    if (start != std::string::npos) {
        id = diskName.substr(start + 1, 2);    
        
        diskName = diskName.substr(0, start);                    
    }
    
    if (diskName.size() > 16)
        diskName = diskName.substr( 0, 16 );
    
    if (id.size() == 1)
        id += ' ';
        
    return id;
}

auto Structure1541::writeTrack( const GcrTrack* trackPtr, uint8_t halfTrack ) -> void {
    
    if (halfTrack >= (MAX_TRACKS * 2) )
        return;        
    
    switch( type ) {
        case Type::D64:
            writeD64( trackPtr, (halfTrack + 2) / 2 );
            break;
        case Type::G64:
            writeG64( trackPtr, halfTrack );
            break;
    }
}

auto Structure1541::getTrackPtr( uint8_t halfTrack ) -> GcrTrack* {
    
    return &gcrTrack[ halfTrack ];
}

}
