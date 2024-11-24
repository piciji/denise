
#include <thread>
#include "structure.h"
#include "../../system/system.h"
#include "../../input/input.h"
#include "../../traps/traps.h"
#include "../../../tools/petcii.h"
#include "dxx.cpp"
#include "gxx.cpp"
#include "pxx.cpp"
#include "prg.cpp"
#include "listing.cpp"
#include "../../../tools/listing.h"
#include "../../system/keyBuffer.h"
#include "../iec.h"
#include "../virtual/virtualDrive.h"
#include "../drive/drive.h"
#include "../../expansionPort/gameCart/warpSpeed.h"
#include "../../expansionPort/gameCart/mach5.h"
#include "../../expansionPort/gameCart/supergames.h"
#include "../../input/input.h"

namespace LIBC64 {
    
const unsigned DiskStructure::MAX_TRACKS = MAX_TRACKS_1541;
const unsigned DiskStructure::TYPICAL_TRACKS = 35;
const unsigned DiskStructure::TYPICAL_SIZE = 174848;  // for 35 tracks in cbm dos
const uint8_t DiskStructure::SECTORS_IN_SPEEDZONE[4] = { 17, 18, 19, 21 };
const unsigned DiskStructure::BYTES_IN_SPEEDZONE[4] = { 6250, 6666, 7142, 7692 };
const uint8_t DiskStructure::GAPS_IN_SPEEDZONE[4] = { 9, 12, 17, 8 };
const float DiskStructure::SKEW[42] = { // measured with CBM DOS 35 track format: logTrackSkew() in gxx.cpp
    // speed DOS 40 track format or disk master 42 track format have not the same SKEWs
    0.903942, 0.359481, 0.815003, 0.270541, 0.726079, 0.181617, 0.637172, 0.092710, 0.548232, 0.003786,
    0.459308, 0.914863, 0.370385, 0.825939, 0.281461, 0.737016, 0.192538, 0.593304, 0.154281, 0.715328,
    0.276376, 0.837423, 0.398470, 0.959518, 0.768827, 0.440875, 0.112924, 0.785010, 0.457096, 0.129144,
    0.351180, 0.129900, 0.908620, 0.687180, 0.463340,
    0.8, 0.4, 0.9, 0.5, 0.1, 0.8, 0.4 // we guess here (tracks > 35)
};


    
DiskStructure::DiskStructure(System* system, Drive* drive) :
system(system),
drive(drive) {
    
    errorMap = nullptr;
    errorMapSize = 0;

    for( unsigned side = 0; side < 2; side++) {
        for (unsigned i = 0; i < (MAX_TRACKS * 2); i++) {
            mTracks[side][i].data = nullptr;
            mTracks[side][i].size = 0;
            mTracks[side][i].bits = 1;
            mTracks[side][i].written = 0;
            mTracks[side][i].mfmSync = nullptr;
        }
    }

    virtualDrive = new VirtualDrive(system, this);
}   

DiskStructure::~DiskStructure() {

    clearTrackData();
}

auto DiskStructure::attach( uint8_t* data, unsigned size ) -> bool {
    rawData = data;
    rawSize = size;
    
    if ( !analyze() )
        return false;
    
    prepare();
    
    return true;
}

auto DiskStructure::detach() -> void {
	
	if (created)
		delete[] created;
	
    rawData = nullptr;
	created = nullptr;
    rawSize = 0;
    
    clearTrackData();
}

auto DiskStructure::clearTrackData() -> void {
    for( unsigned side = 0; side < 2; side++) {
        for (unsigned i = 0; i < (MAX_TRACKS * 2); i++) {
            auto trackPtr = &mTracks[side][i];

            if (trackPtr->data)
                delete[] trackPtr->data;

            if (trackPtr->mfmSync)
                delete[] trackPtr->mfmSync;

            trackPtr->data = nullptr;
            trackPtr->mfmSync = nullptr;
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
    
auto DiskStructure::speedzone( uint8_t track ) -> uint8_t {
    // speedzone: 0 - 3, depends on track sector count
    return (track < 31) + (track < 25) + (track < 18);
}   

auto DiskStructure::countSectors( uint8_t track ) -> uint8_t {
    
    return SECTORS_IN_SPEEDZONE[ speedzone( track ) ];
}

auto DiskStructure::countBytes( uint8_t track ) -> unsigned {
    
    return BYTES_IN_SPEEDZONE[ speedzone( track ) ];
}

auto DiskStructure::gapSize( uint8_t track ) -> unsigned {
    
    return GAPS_IN_SPEEDZONE[ speedzone( track ) ];
}

auto DiskStructure::countSectors( uint8_t track, uint8_t sector ) -> int {
    
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

auto DiskStructure::analyze() -> bool {
    
    type = Type::Unknown;
    sides = 1;
    
    if (!rawData || !rawSize)
        return false;
    
    if ( analyzeD64() )
        return true;

    if ( analyzeD71() )
        return true;

    if ( analyzeD81() )
        return true;
    
    if ( analyzeG64() )
        return true;

    if ( analyzeG71() )
        return true;

    if ( analyzeG81() )
        return true;

    if ( analyzePxx(Type::P64) ) // set P71 in case of two sides
        return true;

    if ( analyzePxx(Type::P81) )
        return true;

    if (!media) // preview
        return false;

    created = DiskStructure::createD64FromPRG( system, system->interface->getFileNameFromMedia(media), rawData, rawSize );
    
	if (created) {
		
		rawData = created;		
		
		rawSize = TYPICAL_SIZE;				
		
		media->guid = (uintptr_t)nullptr;
		
		if (analyzeD64())		
			return true;
	}
	
    return false;
}

auto DiskStructure::prepare() -> void {
    
    switch( type ) {
        case Type::D64:
        case Type::D71:
            prepareDxx();
            break;
        case Type::D81:
            prepareD81();
            break;
        case Type::G64:
        case Type::G71:
            prepareGxx();
            break;
        case Type::G81:
            prepareG81();
            break;
        case Type::P64:
        case Type::P71:
        case Type::P81:
            preparePxx();
            break;
        case Type::Unknown:
            break;
    }            
}

auto DiskStructure::getLogicalTrack(uint8_t _track, int offset) -> uint8_t {
    
    int logicalTrack = _track + offset;

    unsigned tracks = TYPICAL_TRACKS * sides;

    while (logicalTrack > tracks)
        logicalTrack -= tracks;

    while (logicalTrack < 1)
        logicalTrack += tracks;

    return (uint8_t)logicalTrack;        
}

auto DiskStructure::getListing( ) -> std::vector<Emulator::Interface::Listing>& {
    
    listings.clear();
    loader.clear();

    createListing( );
        
    return listings;
}

auto DiskStructure::buildLoadCommand( std::vector<uint8_t> loadPath, bool forShow ) -> std::vector<uint8_t> {
    
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

auto DiskStructure::selectListing( std::string fileName, uint8_t options ) -> void {

    Emulator::PetciiConversion petciiConversion;

    std::vector<uint8_t> petcii;

    petciiConversion.encode( fileName, petcii );

    petcii = buildLoadCommand(petcii);

    prepareKeyBufferActions( petcii, options );
}

auto DiskStructure::selectListing( unsigned pos, uint8_t options ) -> void {

    std::vector<uint8_t> path;
    if (pos < listings.size())
        path = buildLoadCommand( loader[pos] );
    else
        path = buildLoadCommand({'*'});

    prepareKeyBufferActions( path, options );
}

auto DiskStructure::prepareKeyBufferActions( std::vector<uint8_t>& path, uint8_t options ) -> void {
	
    KeyBuffer::Action action;
    
    action.mode = KeyBuffer::Mode::Input;
    action.buffer = path;
    system->keyBuffer->add( action );
    bool mafiosino = dynamic_cast<SuperGames*>(system->expansionPort) && dynamic_cast<SuperGames*>(system->expansionPort)->mafiosino;

    if (mafiosino) {
        action.mode = KeyBuffer::Mode::WaitFor;
        action.buffer = {};
        action.blinkingCursor = true;
        action.delay = 0;
        system->keyBuffer->add(action);

    } else if (!system->secondDriveCable.burstRequested && !dynamic_cast<Mach5*>(system->expansionPort)) {
        if (!(options & 1)) {
            action.mode = KeyBuffer::Mode::WaitFor;
            action.buffer = {'S', 'E', 'A', 'R', 'C', 'H', 'I', 'N', 'G'};
            action.blinkingCursor = false;
            action.delay = 0;
            system->keyBuffer->add(action);
        }
        action.mode = KeyBuffer::Mode::WaitFor;
        action.buffer = {'L', 'O', 'A', 'D', 'I', 'N', 'G'};
        if (dynamic_cast<WarpSpeed*>(system->expansionPort))
            action.buffer = {'W','A','R','P'};

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
    action.waitCallback = [this](KeyBuffer::Action* action) {
        if (system->checkForAutoStarter()) {
            system->keyBuffer->reset();
            system->autoStartFinish(true);
        }
    };
    system->keyBuffer->add( action );

    action.callbackId = 5;
    action.waitCallback = nullptr;
    action.callback = [this]() {
        system->autoStartFinish(false);
    };

    if (mafiosino) {
        action.mode = KeyBuffer::Mode::WaitDelay;
        action.waitCallback = [this](KeyBuffer::Action* action) {
            system->input.setKeycode(0, 5); // F3
        };
        action.delay = 1;
    } else {
        action.mode = KeyBuffer::Mode::Input;
        action.buffer = {'R', 'U', 'N', '\r'};
    }
    system->keyBuffer->add( action );

    autoStarted = true;

    if (options & 0x80) { // override a possible speeder
        drive->extendedMemoryMap = false;
        system->secondDriveCable.parallelPossible = false;
        system->burstOrParallelUpdate();
        drive->setFirmwareByType();
    }

    if (options & 1) { // traps
        system->traps.installSerial();
        system->traps.reset( options & 2 ); // trap send success event to host, error event will be always send
        system->keyBuffer->forceDefaultKernalDelay(); // a possible speeder use shorter boot time
    }
}

auto DiskStructure::create( Type newType, std::string diskName ) -> Emulator::Interface::Data {
    
    switch( newType ) {
        case Type::D64:
            return {createDxx( diskName, 1 ), imageSizeD64() };
        case Type::D71:
            return {createDxx( diskName, 2 ), imageSizeD71() };
        case Type::D81:
            return {createD81( diskName ), imageSizeD81() };
        case Type::G64:
            return { createGxx( diskName, 1 ), imageSizeG64() };
        case Type::G71:
            return { createGxx( diskName, 2 ), imageSizeG71() };
        case Type::G81:
            return { createG81( diskName ), imageSizeG81() };
        case Type::P64:
            return createPxx( diskName, 1 );
        case Type::P71:
            return createPxx( diskName, 2 );
        case Type::P81:
            return createP81( diskName );
        default:
            break;
    } 
    
    return {nullptr, 0};
}

auto DiskStructure::createBAM( std::string diskName, uint8_t* buffer, uint8_t* bufferSecondSide ) -> void {

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

auto DiskStructure::cutId( std::string& diskName ) -> std::string {
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

auto DiskStructure::storeWrittenTracks() -> void {
    bool appendedTracks = false;
    bool errorMapChanged = false;

    if (type == Type::P64 || type == Type::P71 || type == Type::P81)
        writePxx( );
    else if (type == Type::D64 || type == Type::D71)
        appendedTracks = handleAppendedTracksInDxx();
    else if (type == Type::D81)
        appendedTracks = handleAppendedTracksInD81();

    for (uint8_t side = 0; side < sides; side++) {
        for (unsigned track = 0; track < (MAX_TRACKS * 2); track++) {

            MTrack* gcrTrack = getTrackPtr(side, track);

            if (!(gcrTrack->written & 1) || !gcrTrack->size)
                continue;

            switch (type) {
                case Type::D64:
                case Type::D71:
                    writeDxx(gcrTrack, side, (track + 2) / 2, errorMapChanged);
                    break;
                case Type::D81:
                    writeD81(gcrTrack, side, track, errorMapChanged);
                    break;
                case Type::G64:
                case Type::G71:
                    writeGxx(gcrTrack, side, track);
                    break;
                case Type::G81:
                    writeG81(gcrTrack, side, track);
                    break;
                case Type::P64:
                case Type::P71:
                    // can't overwrite single tracks, need to write whole disk.
                    // convert to gcr to update listing outside of emulation
                    encodeGCRFromPulse(gcrTrack, track);
                    break;
                case Type::P81:
                    encodeMfmFromPulse(gcrTrack);
                    break;
                case Type::Unknown:
                    break;
            }

            gcrTrack->written = 0;
        }
    }

    if ((type == Type::D64 || type == Type::D71 || type == Type::D81) && errorMap && (errorMapChanged || appendedTracks )) {
        // error map size is the count of all sectors of this disk. so we use
        // it as an offset for appending the error map data
        write( errorMap, errorMapSize, errorMapSize * 256 );
    }
}

auto DiskStructure::serialize(Emulator::Serializer& s, bool written) -> void {
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

    bool fluxMode = (type == Type::P64) || (type == Type::P71) || (type == Type::P81);
    // 1571 use decoded MFM data in G71
    bool mfmDecodedMode = ((drive->operation & (DRIVE_MODE_157x | ENCODEDDATA_LEVEL)) == (DRIVE_MODE_157x | ENCODEDDATA_LEVEL)) ||
        ((drive->operation & (DRIVE_MODE_158x | DECODEDDATA_LEVEL)) == (DRIVE_MODE_158x | DECODEDDATA_LEVEL));

    for (uint8_t side = 0; side < sides; side++) {
        for (unsigned halfTrack = 0; halfTrack < (MAX_TRACKS * 2); halfTrack++) {

            MTrack* gcrTrack = getTrackPtr(side, halfTrack);

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

                        if (mfmDecodedMode) {
                            if (gcrTrack->mfmSync)
                                delete[] gcrTrack->mfmSync;

                            gcrTrack->mfmSync = nullptr;

                            if (gcrTrack->size)
                                gcrTrack->mfmSync = new uint8_t[gcrTrack->size >> 3];
                        }
                    }
                }

                if (gcrTrack->size) {
                    s.array(gcrTrack->data, gcrTrack->size);

                    if (mfmDecodedMode) {
                        s.array(gcrTrack->mfmSync, gcrTrack->size >> 3);
                    }
                }
            }
        }
    }
}

auto DiskStructure::updateSerializationSize() -> void {
    if (serializationSize)
        system->serializationSize -= serializationSize;

    serializationSize = getStateImageSize();
    system->serializationSize += serializationSize;
}

auto DiskStructure::getStateImageSize() -> unsigned {
    
    unsigned neededSize = 0;
    bool fluxMode = (type == Type::P64) || (type == Type::P71) || (type == Type::P81);
    bool mfmDecodedMode = ((drive->operation & (DRIVE_MODE_157x | ENCODEDDATA_LEVEL)) == (DRIVE_MODE_157x | ENCODEDDATA_LEVEL)) ||
    ((drive->operation & (DRIVE_MODE_158x | DECODEDDATA_LEVEL)) == (DRIVE_MODE_158x | DECODEDDATA_LEVEL));

    for (uint8_t side = 0; side < sides; side++) {
        for (unsigned halfTrack = 0; halfTrack < (MAX_TRACKS * 2); halfTrack++) {

            neededSize += 1;

            MTrack* gcrTrack = getTrackPtr(side, halfTrack);

            if (!(gcrTrack->written & 1))
                continue;

            if (fluxMode) {
                neededSize += 4 + 16 + gcrTrack->pulses.size() * 16;
            } else {
                neededSize += 4 + gcrTrack->size;

                if (mfmDecodedMode) {
                    neededSize += gcrTrack->size >> 3;
                }
            }
        }
    }
    
    return neededSize;
}

auto DiskStructure::getTrackPtr( uint8_t side, uint8_t halfTrack ) -> MTrack* {
    
    return &mTracks[ side ][ halfTrack ];
}

auto DiskStructure::readSector( uint8_t* buffer, uint8_t track, uint8_t sector ) -> bool {

    unsigned offset = 0;
    uint8_t side = 0;

    if (!rawData || (track == 0) )
        return false;

    if (type == Type::D64 || type == Type::D71) {
        if (track > tracksInDxx) {
            offset = countSectors( tracksInDxx, 0 );
            offset += countSectors( tracksInDxx );
            offset <<= 8;
            track -= tracksInDxx;
        }
        return readSector(rawData, buffer, track, sector, offset);

    } else if (type == Type::D81) {
        offset = (track - 1) * 40 * 256 + sector * 256;
        if ((offset + 256) > rawSize)
            return false;

        std::memcpy( buffer, rawData + offset, 256 );
        return true;
    } else if (type == Type::G81 || type == Type::P81) {
        uint8_t _side = 1;
        // convert logical to physical sector
        if (sector >= 20) {
            _side = 0;
            sector -= 20;
        }
        auto _mTrack = getTrackPtr(_side, track-1);
        uint8_t _buf[512];
        if (!decodeSectorMfm( _mTrack, track - 1, (sector >> 1) + 1, &_buf[0]))
            return false;

        std::memcpy( buffer, sector & 1 ? _buf + 256 : _buf, 256);
        return true;
    }

    if (track > 35) {
        track -= 35;
        side = 1;
    }

    track = track * 2 - 2;

    MTrack* trackPtr = &mTracks[side][track];

    int err = decodeSector(trackPtr, buffer, sector);

    return err == ERR_OK;
}

auto DiskStructure::disalignTrack(MTrack& track, unsigned pos) -> void {
    static uint8_t* tempData = nullptr;
    static unsigned tempSize = 0;

    if (!track.size)
        return;

    if (track.size > tempSize) {
        if (tempData)
            delete[] tempData;

        tempData = new uint8_t[track.size];
        tempSize = track.size;
    }

    unsigned offset = SKEW[pos] * float(track.size);
    offset %= track.size;

    std::memcpy(tempData, track.data, track.size);
    std::memcpy(track.data + offset, tempData, track.size - offset);
    std::memcpy(track.data, tempData + (track.size - offset), offset);
}

}

