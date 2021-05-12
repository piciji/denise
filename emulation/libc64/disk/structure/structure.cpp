
#include "structure.h"
#include "../../system/system.h"
#include "../../../tools/petcii.h"
#include "d64.cpp"
#include "g64.cpp"
#include "p64.cpp"
#include "prg.cpp"
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
        gcrTrack[i].written = false;
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
	
	if (created)
		delete[] created;
	
    rawData = nullptr;
	created = nullptr;
    rawSize = 0;
	media = nullptr;
    
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
        trackPtr->written = false;
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
    sides = 1;
    
    if (!rawData || !rawSize)
        return false;
    
    if ( analyzeD64() )
        return true;
    
    if ( analyzeG64() )
        return true;

    if ( analyzeP64() )
        return true;

    created = Structure1541::createD64FromPRG( system->interface->getFileNameFromMedia(media), rawData, rawSize );
    
	if (created) {
		
		rawData = created;		
		
		rawSize = TYPICAL_SIZE;				
		
		media->guid = (uintptr_t)nullptr;
		
		if (analyzeD64())		
			return true;
	}
	
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
        case Type::P64:
            prepareP64();
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

auto Structure1541::getLogicalTrack(uint8_t _track, int offset) -> uint8_t {
    
    int logicalTrack = _track + offset;

    if (logicalTrack > tracks)
        logicalTrack -= tracks;
    else if (logicalTrack < 1)
        logicalTrack += tracks;   
    
    return (uint8_t)logicalTrack;        
}

auto Structure1541::createListing( ) -> void {
    return;
    if (!rawData || (type == Type::Unknown))
        return;
    
    if (tracks < 18)
        return;
    
    Emulator::C64Listing listing;
    listing.convertToScreencode = system->interface->convertToScreencode;
        
    unsigned id = 0;
    
    uint8_t buffer[256];  
    uint8_t _track = 18;
    uint8_t _sector = 0;
    int trackOffset = 0;
    uint8_t tries = tracks + 1;
    
    while (--tries) {        
        decodeSector( &gcrTrack[(_track - 1) * 2], buffer, _sector );
        uint8_t _trackLogical = buffer[0];
        
        if (_trackLogical == 18)
            break;
        
        if ((_trackLogical == 0) || (_trackLogical > tracks)) {   
            _track = getLogicalTrack(_track, 1);
        } else {
        
            trackOffset = _track - _trackLogical;
        
            _track = getLogicalTrack(_track, trackOffset);
        }        
    }    
    
    if (!tries) {
        trackOffset = 0;
        _track = 18;
        decodeSector( &gcrTrack[(_track - 1) * 2], buffer, _sector );
    }
    
    unsigned freeBlocks = 0;
    
    for (uint8_t track = 1; track <= 35; track++) {
        
        uint8_t* bamPtr = track <= TYPICAL_TRACKS
            ? &buffer[4 + 4 * (track - 1)] 
            : &buffer[192 + 4 * (track - TYPICAL_TRACKS - 1)]; // speed dos
        
        if (track != 18) {
            /**
             * detailed map about used sectors, should be identical with count in first byte
             */            
//            unsigned sectorMap =  bamPtr[1] << 16;
//            sectorMap |=  bamPtr[2] << 8;
//            auto sectors = countSectors( track );
//            sectors -= 16;
//            
//            uint8_t mask = 0;
//            for(unsigned i = 0; i < sectors; i++)               
//                mask |= 1 << i;           
//            
//            sectorMap |= bamPtr[3] & mask;
//            
//            for(unsigned i = 0; i < 24; i++)
//                if (sectorMap & (1 << i) )
//                    freeBlocks++;
            
            freeBlocks += *bamPtr;
        }
    }
    
    listings.push_back( { id++, listing.buildHeadline( buffer + 0x90, buffer + 0xa5, buffer + 0xa2 ), listing.decodeToScreencode( buildLoadCommand({'*'}, true) ) } );
	loader.push_back( {'*'} );  
    
    decodeSector( &gcrTrack[(_track - 1) * 2], buffer, ++_sector );
    
    uint8_t* ptr = &buffer[0];
    
    unsigned entry = 0;
    
    while(1) {
        
        unsigned listingSize = *(ptr + 0x1f) * 256 + *(ptr + 0x1e);        
        
        if ( *(ptr + 0x2) != 0 ) {     
            std::vector<uint8_t> entry = listing.buildListing( ptr + 0x5, listingSize, *(ptr + 0x2) );
            
            std::vector<uint8_t> loadCommand;
            
            if (listingSize)
                loadCommand = listing.decodeToScreencode( buildLoadCommand( listing.loader, true ) );
            
            listings.push_back( { id++, entry, loadCommand } );
			loader.push_back( listing.loader );
        }
        
        ptr += 0x20;
        entry++;
        
        if ((entry & 7) == 0) {
            
            // if the disk doesn't use the dir track, it could produce an endless loop
            if (entry > 250)                
                break;
            
            _track = buffer[0];
            _sector = buffer[1]; 
            
            if (trackOffset)
                _track = getLogicalTrack(_track, trackOffset);
                        
            if (_track > tracks)
                break;
            
            if (_track == 0)
                break;

            if ( decodeSector(&gcrTrack[(_track - 1) * 2], buffer, _sector) != ERR_OK)
                break;
            
            ptr = &buffer[0];
        }        
    }
    
    listings.push_back( { id++, listing.buildFreeLine( freeBlocks ), listing.decodeToScreencode( buildLoadCommand({'*'}, true) ) } );
	loader.push_back( {'*'} );
}

auto Structure1541::getListing( ) -> std::vector<Emulator::Interface::Listing>& {
    
    listings.clear();
    loader.clear();

    createListing();
        
    return listings;
}

auto Structure1541::buildLoadCommand( std::vector<uint8_t> loadPath, bool forShow ) -> std::vector<uint8_t> {
    
	if (forShow)
		loadPath.insert( loadPath.begin(), { 'L', 'O', 'A', 'D', ' ', '"' } );    	
	else
		loadPath.insert( loadPath.begin(), { 'L', 'O', 'A', 'D', '"' } );    	
	
    loadPath.insert( loadPath.end(), { '"', ',' } );        
    
    switch(number) {
        case 0:
        default: loadPath.insert( loadPath.end(), '8' ); break;
        case 1: loadPath.insert( loadPath.end(), '9' ); break;
        case 2: loadPath.insert( loadPath.end(), {'1', '0' } ); break;
        case 3: loadPath.insert( loadPath.end(), {'1', '1' } ); break;
    }
       
	if (forShow)
		loadPath.insert( loadPath.end(), { ',', '1' } );   	
	else
		loadPath.insert( loadPath.end(), { ',', '1', '\r' } );   	
	
	return loadPath;
}

auto Structure1541::selectListing(  unsigned pos ) -> void {
	
    KeyBuffer::Action action;
    
    action.mode = KeyBuffer::Mode::Input;
	if (pos < listings.size())
		action.buffer = buildLoadCommand( loader[pos] );    
	else
		action.buffer = buildLoadCommand({'*'});
			
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

    action.callbackId = 4;
    action.mode = KeyBuffer::Mode::WaitFor;
    action.buffer = {'R', 'E', 'A', 'D', 'Y', '.'};  
    action.delay = 180;    
    action.alternateBuffer.clear();
    action.blinkingCursor = true;
    action.waitCallback = [this]() {
        if (system->checkForAutoStarter()) {
            system->keyBuffer->reset();
            system->interface->autoStartFinish(true);
        }
    };
    system->keyBuffer->add( action );

    action.callbackId = 5;
    action.waitCallback = nullptr;
    action.callback = [this]() {
        system->interface->autoStartFinish(false);
    };
    action.mode = KeyBuffer::Mode::Input;
    action.buffer = {'R', 'U', 'N', '\r'};
    system->keyBuffer->add( action );

    autoStarted = true;
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

auto Structure1541::storeWrittenTracks() -> void {
    
    for ( unsigned halfTrack = 0; halfTrack < (MAX_TRACKS * 2); halfTrack++ ) {
        
        GcrTrack* gcrTrack = getTrackPtr( halfTrack );
        
        if (!gcrTrack->written || !gcrTrack->size)
            continue;
        
        writeTrack( gcrTrack, halfTrack );
        
        gcrTrack->written = false;
    }
}

auto Structure1541::serialize(Emulator::Serializer& s, bool written) -> void {
    // serialize structure only, if at least one bit was written

    s.integer( autoStarted );

    s.integer( rawSize );
    
    s.integer( tracks );
    
    s.integer( maxHalfTracks );
    
    s.integer( maxTrackLength );
    
    s.integer( (int&)type );
    
    if (!written || (s.mode() == Emulator::Serializer::Mode::Size))
        return;
    
    for (unsigned halfTrack = 0; halfTrack < (MAX_TRACKS * 2); halfTrack++) {

        GcrTrack* gcrTrack = getTrackPtr(halfTrack);

        unsigned _trackSize = gcrTrack->size;
        
        s.integer( gcrTrack->written );     
        
        s.integer( gcrTrack->size );        
            
        if (s.mode() == Emulator::Serializer::Mode::Load) {
            gcrTrack->bits = gcrTrack->size << 3;           
            
            if (_trackSize != gcrTrack->size) {
                
                if (gcrTrack->data)
                    delete[] gcrTrack->data;

                gcrTrack->data = nullptr;
                    
                if (gcrTrack->size) 
                    gcrTrack->data = new uint8_t[ gcrTrack->size ];   
            }                                                                      
        }
        
        if (gcrTrack->size)
            s.array( gcrTrack->data, gcrTrack->size );
    }
}

auto Structure1541::getStateImageSize() -> unsigned {
    
    unsigned neededSize = 0;
    
    for (unsigned halfTrack = 0; halfTrack < (MAX_TRACKS * 2); halfTrack++) {

        GcrTrack* gcrTrack = getTrackPtr( halfTrack );

        neededSize += 1 + 4 + gcrTrack->size;
    }
    
    return neededSize;
}

auto Structure1541::getTrackPtr( uint8_t halfTrack ) -> GcrTrack* {
    
    return &gcrTrack[ halfTrack ];
}

}
