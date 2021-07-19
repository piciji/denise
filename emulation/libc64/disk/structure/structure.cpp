
#include <thread>
#include "structure.h"
#include "../../system/system.h"
#include "../../../tools/petcii.h"
#include "dxx.cpp"
#include "gxx.cpp"
#include "pxx.cpp"
#include "prg.cpp"
#include "../../../tools/listing.h"
#include "../../system/keyBuffer.h"
#include "../iec.h"

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

    for( unsigned side = 0; side < 2; side++) {
        for (unsigned i = 0; i < (MAX_TRACKS * 2); i++) {
            gcrTracks[side][i].data = nullptr;
            gcrTracks[side][i].size = 0;
            gcrTracks[side][i].bits = 1;
            gcrTracks[side][i].written = 0;
        }
    }
}   

Structure1541::~Structure1541() {
    
    clearTrackData();   
}

auto Structure1541::attach( uint8_t* data, unsigned size, bool loadGracefully ) -> bool {
    rawData = data;
    rawSize = size;
    
    if ( !analyze() )
        return false;

    if (loadGracefully && (type == Type::P64) ) {
        encodingGraceful.status = 1;
        iecBus->diskInsertInProgress = true;
        return false;
    }
    
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
    encodingGraceful.reset();
}

auto Structure1541::clearTrackData() -> void {
    for( unsigned side = 0; side < 2; side++) {
        for (unsigned i = 0; i < (MAX_TRACKS * 2); i++) {
            auto trackPtr = &gcrTracks[side][i];

            if (trackPtr->data)
                delete[] trackPtr->data;

            trackPtr->data = nullptr;
            trackPtr->size = 0;
            trackPtr->bits = 1;
            trackPtr->written = 0;

            trackPtr->firstPulse = -1;
            trackPtr->currentPulse = -1;
            trackPtr->lastPulse = -1;
            trackPtr->pulses.clear();
            // free memory
            trackPtr->pulses.shrink_to_fit();
        }
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

    if ( analyzeD71() )
        return true;
    
    if ( analyzeG64() )
        return true;

    if ( analyzeG71() )
        return true;

    if ( analyzeP64() )
        return true;

    if ( analyzeP71() )
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
        case Type::D71:
            prepareDxx();
            break;
        case Type::G64:
        case Type::G71:
            prepareGxx();
            break;
        case Type::P64:
        case Type::P71:
            preparePxx();
            break;
    }            
}

auto Structure1541::getLogicalTrack(uint8_t _track, int offset) -> uint8_t {
    
    int logicalTrack = _track + offset;

    unsigned tracks = TYPICAL_TRACKS * sides;

    while (logicalTrack > tracks)
        logicalTrack -= tracks;

    while (logicalTrack < 1)
        logicalTrack += tracks;

    return (uint8_t)logicalTrack;        
}

// support for following DIR chain to second side (don't know if C64 DOS using this)
#define GTP(_T) \
    (sides == 1) ? &gcrTracks[0][(_T - 1) * 2] \
    : &gcrTracks[ (_T > TYPICAL_TRACKS) ? 1 : 0][ (((_T > TYPICAL_TRACKS) ? (_T - TYPICAL_TRACKS) : _T) - 1) * 2]

// C64 DOS (support 35 tracks per side)
auto Structure1541::createListing( ) -> void {

    if (!rawData || (type == Type::Unknown))
        return;
    
    Emulator::C64Listing listing;
    listing.convertToScreencode = system->interface->convertToScreencode;
        
    unsigned id = 0;
    
    uint8_t buffer[256];  
    uint8_t _track = 18;
    uint8_t _sector = 0;
    int trackOffset = 0;
    uint8_t tries = TYPICAL_TRACKS * sides + 1;

    while (--tries) {        
        decodeSector( GTP( _track ), buffer, _sector );
        uint8_t _trackLogical = buffer[0];
        
        if (_trackLogical == 18)
            break;
        
        if ((_trackLogical == 0) || (_trackLogical > (TYPICAL_TRACKS * sides))) {
            _track = getLogicalTrack(_track, 1);

        } else {
            trackOffset = _track - _trackLogical;
        
            _track = getLogicalTrack(_track, trackOffset);
        }        
    }    
    
    if (!tries) {
        trackOffset = 0;
        _track = 18;
        decodeSector( GTP( _track ), buffer, _sector );
    }
    
    unsigned freeBlocks = 0;

    // c64 DOS count free sectors for 35 tracks only
    for (uint8_t track = 1; track <= TYPICAL_TRACKS; track++) {
        
        uint8_t* bamPtr = &buffer[4 + 4 * (track - 1)];
        
        if (track != 18) {
            freeBlocks += *bamPtr;
        }
    }

    if (sides == 2) {
        for (uint8_t track = 1; track <= TYPICAL_TRACKS; track++) {
            freeBlocks += buffer[0xdd + (track - 1)];
        }
    }

    uint8_t buffer2[256];
    decodeSector( GTP( _track ), buffer2, ++_sector );
    uint8_t* ptr = &buffer2[0];

    bool addedHeadline = false;
    std::vector<uint8_t> _headlineCmd = {':', '*'};

    unsigned entry = 0;
    
    while(1) {
        
        unsigned listingSize = *(ptr + 0x1f) * 256 + *(ptr + 0x1e);        
        
        if ( *(ptr + 0x2) != 0 ) {

            if (!addedHeadline) {
                addedHeadline = true;
                uint8_t type = *(ptr + 0x2);

                if ((type & 7) != 2) // when first file is not a PRG
                    _headlineCmd = {'*'};

                listings.push_back( { id++, listing.buildHeadline( buffer + 0x90, buffer + 0xa5, buffer + 0xa2 ), listing.decodeToScreencode( buildLoadCommand(_headlineCmd, true) ) } );
                loader.push_back( _headlineCmd );
            }

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
            
            _track = buffer2[0];
            _sector = buffer2[1];
            
            if (trackOffset)
                _track = getLogicalTrack(_track, trackOffset);
                        
            if (_track > (TYPICAL_TRACKS * sides))
                break;
            
            if (_track == 0)
                break;

            if ( decodeSector(GTP( _track ), buffer2, _sector) != ERR_OK)
                break;
            
            ptr = &buffer2[0];
        }        
    }

    if (!addedHeadline) {
        listings.push_back( { 0, listing.buildHeadline( buffer + 0x90, buffer + 0xa5, buffer + 0xa2 ) } );
        loader.push_back( {'*'} );
    }
    
    listings.push_back( { id++, listing.buildFreeLine( freeBlocks ), listing.decodeToScreencode( buildLoadCommand( _headlineCmd, true) ) } );
	loader.push_back( _headlineCmd );
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

    if (!system->userPort.burstRequested) {
        action.mode = KeyBuffer::Mode::WaitFor;
        action.buffer = {'S', 'E', 'A', 'R', 'C', 'H', 'I', 'N', 'G'};
        action.blinkingCursor = false;
        action.delay = 0;
        system->keyBuffer->add(action);

        action.mode = KeyBuffer::Mode::WaitFor;
        action.buffer = {'L', 'O', 'A', 'D', 'I', 'N', 'G'};
        action.alternateBuffer = {'S', 'E', 'A', 'R', 'C', 'H', 'I', 'N', 'G'};
        action.blinkingCursor = false;
        system->keyBuffer->add(action);
    }
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

auto Structure1541::create( Type newType, std::string diskName ) -> Emulator::Interface::Data {
    
    switch( newType ) {
        case Type::D64:
            return {createDxx( diskName, 1 ), imageSizeD64() };
        case Type::D71:
            return {createDxx( diskName, 2 ), imageSizeD71() };
        case Type::G64:
            return { createGxx( diskName, 1 ), imageSizeG64() };
        case Type::G71:
            return { createGxx( diskName, 2 ), imageSizeG71() };
        case Type::P64:
            return createPxx( diskName, 1 );
        case Type::P71:
            return createPxx( diskName, 2 );
    } 
    
    return {nullptr, 0};
}

auto Structure1541::createBAM( std::string diskName, uint8_t* buffer, uint8_t* bufferSecondSide ) -> void {

    Emulator::PetciiConversion petciiConversion;

    diskName = petciiConversion.encode( diskName );

    auto id = cutId( diskName );
    
    std::memset(buffer, 0, 256);
    if (bufferSecondSide)
        std::memset(bufferSecondSide, 0, 256);
    
    buffer[0] = 18;
    buffer[1] = 1;
    buffer[2] = 65;
    if (bufferSecondSide)
        buffer[3] = 0x80;
    
    std::memset( buffer + 144, 0xa0, 27 );
    std::memcpy( buffer + 144, diskName.c_str(), diskName.size() );
    std::memcpy( buffer + 162, id.c_str(), id.size() );
    
    buffer[165] = 50;
    buffer[166] = 65;
    
    // to calculate the free blocks bam sector contains a usage bit for all sectors
    for (uint8_t track = 1; track <= TYPICAL_TRACKS; track++) {

        uint8_t sectors = countSectors( track );

        uint8_t* bamPtr = &buffer[4 + 4 * (track - 1)];

        for (uint8_t sector = 0; sector < sectors; sector++) {

            // sectors in use keep zero
            if (track == 18 && ( sector == 0 || sector == 1 ))
                continue;

            // mark unused sectors
            bamPtr[1 + sector / 8] |= (1 << (sector & 7));

            *bamPtr += 1; // first byte count all unused sectors in a track
        }
    }

    if (bufferSecondSide) {
        for (uint8_t track = 1; track <= TYPICAL_TRACKS; track++) {

            uint8_t sectors = countSectors( track );

            uint8_t* bamPtr = &bufferSecondSide[3 * (track - 1)];

            for (uint8_t sector = 0; sector < sectors; sector++) {

                // sectors in use keep zero
                if (track == 18 )
                    continue;

                // mark unused sectors
                bamPtr[sector / 8] |= (1 << (sector & 7));

                buffer[0xdd + (track - 1)]++;
            }
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

auto Structure1541::storeWrittenTracks() -> void {
    bool appendedTracks = false;
    bool errorMapChanged = false;

    if (type == Type::P64 || type == Type::P71)
        writePxx( );
    else if (type == Type::D64 || type == Type::D71)
        appendedTracks = handleAppendedTracksInDxx();

    for (uint8_t side = 0; side < sides; side++) {
        for (unsigned halfTrack = 0; halfTrack < (MAX_TRACKS * 2); halfTrack++) {

            GcrTrack* gcrTrack = getTrackPtr(side, halfTrack);

            if (!(gcrTrack->written & 1) || !gcrTrack->size)
                continue;

            switch (type) {
                case Type::D64:
                case Type::D71:
                    writeDxx(gcrTrack, side, (halfTrack + 2) / 2, errorMapChanged);
                    break;
                case Type::G64:
                case Type::G71:
                    writeGxx(gcrTrack, side, halfTrack);
                    break;
                case Type::P64:
                case Type::P71:
                    // can't overwrite single tracks, need to write whole disk.
                    // convert to gcr to update listing outside of emulation
                    encodeGCR(gcrTrack, halfTrack);
                    break;
            }

            gcrTrack->written = 0;
        }
    }

    if ((type == Type::D64 || type == Type::D71) && errorMap && (errorMapChanged || appendedTracks )) {
        // error map size is the count of all sectors of this disk. so we use
        // it as an offset for appending the error map data
        write( errorMap, errorMapSize, errorMapSize * 256 );
    }
}

auto Structure1541::serialize(Emulator::Serializer& s, bool written) -> void {
    // serialize structure only, if at least one bit was written

    s.integer( autoStarted );

    s.integer( serializationSize );

    s.integer( rawSize );
    
    s.integer( tracksInDxx );

    s.integer( maxTrackLength );

    s.integer( sides );
    
    s.integer( (int&)type );
    
    if (!written || (s.mode() == Emulator::Serializer::Mode::Size))
        return;

    bool fluxMode = (type == Type::P64) || (type == Type::P71);

    for (uint8_t side = 0; side < sides; side++) {
        for (unsigned halfTrack = 0; halfTrack < (MAX_TRACKS * 2); halfTrack++) {

            GcrTrack* gcrTrack = getTrackPtr(side, halfTrack);

            s.integer(gcrTrack->written);

            if (!(gcrTrack->written & 1))
                continue;

            unsigned _trackSize = gcrTrack->size;

            s.integer(gcrTrack->size);

            if (fluxMode) {

                s.integer(gcrTrack->firstPulse);
                s.integer(gcrTrack->lastPulse);
                s.integer(gcrTrack->currentPulse);

                unsigned pulseSize = gcrTrack->pulses.size();
                s.integer(pulseSize);

                if (s.mode() == Emulator::Serializer::Mode::Save) {
                    for (unsigned i = 0; i < pulseSize; i++) {

                        Pulse& pulse = gcrTrack->pulses[i];

                        s.integer(pulse.position);
                        s.integer(pulse.strength);
                        s.integer(pulse.next);
                        s.integer(pulse.previous);
                    }
                } else if (s.mode() == Emulator::Serializer::Mode::Load) {

                    gcrTrack->pulses.clear();
                    gcrTrack->pulses.reserve(pulseSize);
                    Pulse pulse;

                    for (unsigned i = 0; i < pulseSize; i++) {
                        s.integer(pulse.position);
                        s.integer(pulse.strength);
                        s.integer(pulse.next);
                        s.integer(pulse.previous);

                        gcrTrack->pulses.push_back(pulse);
                    }
                }
            } else {

                if (s.mode() == Emulator::Serializer::Mode::Load) {
                    gcrTrack->bits = gcrTrack->size << 3;
                    if (!gcrTrack->bits)
                        gcrTrack->bits = 1;

                    if (_trackSize != gcrTrack->size) {

                        if (gcrTrack->data)
                            delete[] gcrTrack->data;

                        gcrTrack->data = nullptr;

                        if (gcrTrack->size)
                            gcrTrack->data = new uint8_t[gcrTrack->size];
                    }
                }

                if (gcrTrack->size)
                    s.array(gcrTrack->data, gcrTrack->size);
            }
        }
    }
}

auto Structure1541::updateSerializationSize() -> void {
    if (serializationSize)
        system->serializationSize -= serializationSize;

    serializationSize = getStateImageSize();
    system->serializationSize += serializationSize;
}

auto Structure1541::getStateImageSize() -> unsigned {
    
    unsigned neededSize = 0;
    bool fluxMode = (type == Type::P64) || (type == Type::P71);

    for (uint8_t side = 0; side < sides; side++) {
        for (unsigned halfTrack = 0; halfTrack < (MAX_TRACKS * 2); halfTrack++) {

            neededSize += 1;

            GcrTrack* gcrTrack = getTrackPtr(side, halfTrack);

            if (!(gcrTrack->written & 1))
                continue;

            if (fluxMode) {
                neededSize += 4 + 16 + gcrTrack->pulses.size() * 16;
            } else
                neededSize += 4 + gcrTrack->size;
        }
    }
    
    return neededSize;
}

auto Structure1541::getTrackPtr( uint8_t side, uint8_t halfTrack ) -> GcrTrack* {
    
    return &gcrTracks[ side ][ halfTrack ];
}


}

