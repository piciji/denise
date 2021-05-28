
#include "structure.h"

namespace LIBC64 {
  
auto Structure1541::analyzeG64() -> bool {

    if (rawSize < 32)
        return false; // too small
    
    if (rawData[8] != 0) // unknown version
        return false;
    
    if (rawData[9] < 1) // 0 tracks ? wtf
        return false;
    
    if (std::memcmp(rawData, "GCR-1541", 8)) // missing this ident ?
        return false;
    
    tracks = rawData[9] / 2;
    maxHalfTracks = rawData[9];
    maxTrackLength = rawData[10] | (rawData[11] << 8);    
    
    if (maxHalfTracks > (MAX_TRACKS * 2) ) // more tracks than a 1541 drive can handle
        return false;        
    
    type = Type::G64;
    
    return true;
}    
    
auto Structure1541::getTrackOffsetG64( uint8_t halfTrack, int& error ) -> uint32_t {
    uint8_t buf[4];
    error = 0;
    
    // for each half track there is a 4 byte offset
    uint32_t offset = 12 + (halfTrack * 4);

    if (rawSize < (offset + 4)) { // raw file too small
        error = -1;
        return 0;
    }
    
    std::memcpy(buf, rawData + offset, 4);    
        
    offset = Emulator::copyBufferToInt<uint32_t>( &buf[0] );
    
    if ( rawSize < (offset + 2)) // offset not in bounds of raw file ... wtf
        error = -2;
        
    return offset;
}
    
auto Structure1541::prepareG64() -> void {
    
    uint8_t buf[2];
    unsigned offset;
    unsigned trackLength;
    int error;
    
    for ( unsigned halfTrack = 0; halfTrack < (MAX_TRACKS * 2); halfTrack++) {
        GcrTrack* ptr = &gcrTracks[ halfTrack ];
        
        if ( ptr->data )
            delete[] ptr->data;
            
        ptr->data = nullptr;
        ptr->size = 0;
        ptr->bits = 1;
        
        if (halfTrack >= maxHalfTracks)
            continue;
        
        offset = getTrackOffsetG64( halfTrack, error );
        
        if (error < 0)
            continue;
        
        if (offset > 0) {           
            
            std::memcpy( buf, rawData + offset, 2 );
            
            trackLength = Emulator::copyBufferToInt<uint16_t>( &buf[0] );
            
            // header area contains the value for maximal track length.
            // each track begins with a 2 byte value about track length.
            // next track isn't following immediately, otherwise a changed
            // track length would overwrite and corrupt next track.
            // so each track has a size of max track length.
            // the difference between real track length and max track length
            // is filled with zero's.
            if ( (trackLength < 1) || (trackLength > maxTrackLength) )
                continue;           
            
            if ( rawSize < (offset + 2 + trackLength)) // rawfile too small
                continue;
            
            ptr->size = trackLength;
            ptr->bits = ptr->size << 3;
            ptr->data = new uint8_t[ ptr->size ];
            std::memcpy( ptr->data, rawData + offset + 2, trackLength );
            
        } else { // if track doesn't exists         
            ptr->size = countBytes( (halfTrack + 2) / 2 ); // standard length
            ptr->bits = ptr->size << 3;
            ptr->data = new uint8_t[ ptr->size ];
            std::memset( ptr->data, 0x55, ptr->size );
        }

        if (ptr->bits == 0)
            ptr->bits = 1;
    }
}
    
auto Structure1541::writeG64(const GcrTrack* trackPtr, unsigned halfTrack) -> bool {
    int error;
    uint8_t buf[4];
    bool appendTrack = false;
    
    long int offset = getTrackOffsetG64( halfTrack, error );    
    
    if (error < 0)
        return false;
    
    if (trackPtr->size > maxTrackLength) 
        // the changed track is bigger than max length ?
        return false;
    
    if (offset == 0) { // track doesn't exists. we append track at the end of raw file        
        offset = rawSize; 
        rawSize += 2 + maxTrackLength; // update image size
        appendTrack = true;
    }        
    
    // now we write the changed track out to raw file, either overwrite the old one
    // or append it at end of file, see above
    Emulator::copyIntToBuffer<uint16_t>( &buf[0], (uint16_t)trackPtr->size );

    // first 2 bytes are track length
    if ( write( &buf[0], 2, offset ) != 2)
        return false;
    // next is gcr encoded data of track
    if ( write( trackPtr->data, trackPtr->size, offset + 2 ) != trackPtr->size )
        return false;
    
    // we need to fill the gap between this track and next one with zeros
    unsigned gapSize = maxTrackLength - trackPtr->size;
    
    if (gapSize > 0) {
        uint8_t* tempGap = new uint8_t[gapSize];
        std::memset(tempGap, 0, gapSize);
        
        unsigned bytesWritten = write( tempGap, gapSize, offset + 2 + trackPtr->size );
        
        delete[] tempGap;
        
        if (bytesWritten != gapSize)
            return false;
    }

    if (appendTrack) {
        // for a new appended track we need to update the offset in header area
        Emulator::copyIntToBuffer<uint32_t>( &buf[0], offset );
        
        if ( write( &buf[0], 4, 12 + (halfTrack * 4) ) != 4 )
            return false;
        
        // the speedzone part of the g64 spec is not emulated at the moment.
        // basically you can write data in one of four possible speeds, you can
        // even change the speed more times during a track.
        // after track offset area, there is a speedzone area in the same manner like
        // track offset area, 4 bytes per track. values from 0 - 3 select one of
        // the four possible speed zones for a single track, values of 4 and above
        // means an offset to an extended speedzone area, where each byte of a track
        // is assigned by a 2-bit speed zone value [0 - 3]
        // we write the typical speedzone of a track to raw file.
        // NOTE: there is no known software using this feature
        
        Emulator::copyIntToBuffer<uint32_t>( &buf[0], speedzone( (halfTrack + 2) / 2) );
        
        if ( write( &buf[0], 4, 12 + (MAX_TRACKS_1541 * 2 * 4) + (halfTrack * 4) ) != 4 )
            return false;
    }
    
    return true;
}

auto Structure1541::imageSizeG64() -> unsigned {
    
    unsigned maxBytes = 7928;
    
    return 12 + MAX_TRACKS_1541 * 2 * 8 + TYPICAL_TRACKS * (maxBytes + 2);
}

auto Structure1541::createG64( std::string diskName ) -> uint8_t* {
    
    uint8_t* temp = new uint8_t[ imageSizeG64() ];
    uint8_t* ptr = temp;
    
    std::memset( ptr, 0, imageSizeG64() );
    
    uint8_t buffer[256];    
    std::memset( buffer, 0, 256 );
    
    uint8_t bufferDir[256];    
    std::memset( bufferDir, 0, 256 );
    bufferDir[1] = 255;

    uint8_t bufferBam[256];  
    createBAM( diskName, MAX_TRACKS, bufferBam );
    
    // max bytes per track
    unsigned maxBytes = 7928;
    
    // 12 byte header
    std::memcpy( ptr, "GCR-1541", 8 );
    
    ptr[8] = 0;
    
    ptr[9] = MAX_TRACKS * 2;
    
    Emulator::copyIntToBuffer<uint16_t>( &ptr[10], maxBytes );    
    
    ptr += 12;
    
    // we prepare the image with standard 35 tracks and without half tracks.
    for( unsigned track = 0; track < TYPICAL_TRACKS; track++ ) {
        
        // not to waste space we dont't include half tracks during image creation.
        // therefore we leave room (4 bytes) between the offset values. offsets for half tracks will be
        // added later if needed.
        // to calculate the offset value we have to keep in mind that a speed map is following the
        // track offsets. 12 byte header + 4 byte track offset * max half tracks + 4 byte speed map * max half tracks.
        // in main header the maximum amount of bytes for a track is specified. so we make 
        // that amount of room for each track to avoid realigning of multiple tracks later on.
        // furthermore we keep in mind that each track begins with a 2 byte length value.
        Emulator::copyIntToBuffer<uint32_t>( &ptr[ track * 2 * 4 ], 12 + MAX_TRACKS_1541 * 2 * 8 + track * (maxBytes + 2) );    
    }
    
    ptr += MAX_TRACKS_1541 * 2 * 4;
    
    for( unsigned track = 0; track < TYPICAL_TRACKS; track++ )
        // we use the typical speedzone for a track
        Emulator::copyIntToBuffer<uint32_t>( &ptr[ track * 2 * 4 ], speedzone( track + 1 ) );
    
    ptr += MAX_TRACKS_1541 * 2 * 4;    
        
    for( unsigned track = 1; track <= TYPICAL_TRACKS; track++ ) {
        // we init the structure in CBM DOS format
        uint8_t gaps = gapSize( track );
        
        unsigned trackSize = countBytes( track );
        
        Emulator::copyIntToBuffer<uint16_t>( ptr, trackSize );    
        
        ptr += 2;
        
        std::memset( ptr, 0x55, trackSize );
        
        uint8_t* sectorPtr = ptr;
        
        for ( unsigned sector = 0; sector < countSectors( track ); sector++ ) {
            
            uint8_t* useBuffer = buffer;
            
            if ( track == 18 && sector == 1 ) // directory sector
                useBuffer = bufferDir;
            else if ( track == 18 && sector == 0 ) // bam sector
                useBuffer = bufferBam;
            
            encodeSector( useBuffer, sectorPtr, track, sector, 0xa0, 0xa0, ERR_OK );
            
            sectorPtr += 340 + 9 + gaps + 5;
        }
        
        ptr += maxBytes;                
    }
    
    return temp;
}

}
