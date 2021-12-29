
#include "structure.h"

namespace LIBC64 {

auto DiskStructure::analyzeD71() -> bool {

    uint32_t compareSize = TYPICAL_SIZE << 1;
    uint32_t sectors = compareSize / 256;
    tracksInDxx = TYPICAL_TRACKS; // typical 349696 bytes in 70 tracks

    if (errorMap)
        delete[] errorMap;

    errorMapSize = 0;
    errorMap = nullptr;

    while (1) {
        if (rawSize == compareSize)
            break;

        // check if one error byte for each block was appended ?
        if (rawSize == (compareSize + sectors)) {
            errorMapSize = sectors;
            break;
        }
        // now we check for non standard track count
        if (++tracksInDxx > MAX_TRACKS) // up to 42 tracks are possible
            return false;

        sectors += 17 << 1; // tracks > 31 contain 17 sectors
        compareSize = sectors * 256;
    }

    type = Type::D71;
    sides = 2;

    if (errorMapSize) {
        errorMap = new uint8_t[errorMapSize];
        std::memcpy(errorMap, rawData + compareSize, errorMapSize);
    }

    return true;
}

auto DiskStructure::analyzeD64() -> bool {
    
    uint32_t compareSize = TYPICAL_SIZE;
    uint32_t sectors = compareSize / 256;
    tracksInDxx = TYPICAL_TRACKS; // typical 174848 bytes in 35 tracks

    if (errorMap)
        delete[] errorMap;
        
    errorMapSize = 0;
    errorMap = nullptr;
    
    while(1) {        
        if (rawSize == compareSize)
            break;
        
        // check if one error byte for each block was appended ?
        if (rawSize == (compareSize + sectors) ) {
            errorMapSize = sectors;
            break;
        }
        // now we check for non standard track count
        if (++tracksInDxx > MAX_TRACKS) // up to 42 tracks are possible
            return false;

        sectors += 17; // tracks > 31 contain 17 sectors
        compareSize = sectors * 256;       
    }
    
    type = Type::D64;
    sides = 1;

    if (errorMapSize) {
        errorMap = new uint8_t[ errorMapSize ];
        std::memcpy( errorMap, rawData + compareSize, errorMapSize );
    }
    
    return true;
}
    
auto DiskStructure::prepareDxx() -> void {
    
    uint8_t errorCode;
    uint8_t buffer[256];
    unsigned sectorOffset = 0;
    unsigned trackOffset = 0;
    bool doubleSideFlag = false;
    unsigned alignOffset = 0;
    unsigned trackSize = 0;

    // first we fetch the bam sector to extract the id, needed for all sector headers
    int sectors = countSectors( 18, 0 );
    std::memcpy( buffer, rawData + (sectors << 8), 256 );

    uint8_t id1 = buffer[0xa2];
    uint8_t id2 = buffer[0xa3];
    doubleSideFlag = !!(buffer[3] & 0x80);

    for( uint8_t side = 0; side < sides; side++ ) {

        for (uint8_t track = 1; track <= MAX_TRACKS; track++) {
            // tracks count from 1 upwards and are stored in memory as half tracks
            // track 1 -> means half track 0
            // track 1.5 -> means half track 1
            // track 2 -> means half track 2
            // ... d64 doesn't support halftracks 1.5, 2.5 ...

            unsigned lastTrackSize = trackSize;
            unsigned halfTrack = track * 2 - 2;
            trackSize = countBytes(track);
            MTrack* trackPtr = &gcrTracks[side][halfTrack];

            // there wasn't loaded any image before
            if (!trackPtr->data)
                trackPtr->data = new uint8_t[trackSize];

            else if (trackSize != trackPtr->size) {
                delete[] trackPtr->data;
                trackPtr->data = new uint8_t[trackSize];
            }

            trackPtr->size = trackSize;
            trackPtr->bits = trackSize * 8;
            uint8_t* ptr = trackPtr->data;

            std::memset(ptr, 0x55, trackSize); // clear track

            if (track <= tracksInDxx) {

                uint8_t gaps = gapSize(track);

                unsigned sectorsInTrack = countSectors(track);

                for (unsigned sector = 0; sector < sectorsInTrack; sector++) {

                    // count all sectors so far
                    sectors = countSectors(track, sector);

                    if (sectors < 0)
                        break;

                    sectors += sectorOffset;

                    errorCode = errorMap ? errorMap[sectors] : ERR_OK;

                    std::memcpy(buffer, rawData + (sectors << 8), 256);

                    encodeSector(buffer, ptr, track + trackOffset, sector, id1, id2, errorCode);

                    // gcr formated sector size: 64 * 5 = 320 byte data
                    // + 5 (descriptor byte / checksum )
                    // + 5 first sync
                    // + 10 header
                    // + 9 gap
                    // + 5 second sync
                    // + speedzone dependant gap
                    ptr += 340 + 9 + gaps + 5;
                }

                if (disalignTracks)
                    disalignTrack(trackPtr->data, trackSize, lastTrackSize, alignOffset);
            }
            // half tracks are not supported by D64
            trackPtr = &gcrTracks[side][++halfTrack];

            if (trackPtr->data)
                delete[] trackPtr->data;
            trackPtr->data = nullptr;
            trackPtr->size = trackSize;
            trackPtr->bits = trackSize * 8;
        }

        sectorOffset = sectors + 1;
        trackOffset = doubleSideFlag ? tracksInDxx : 0;
    }
}

auto DiskStructure::encodeSector(const uint8_t* src, uint8_t* target, uint8_t track, uint8_t sector, uint8_t id1, uint8_t id2, int errorCode) -> void {
    // if an error map is appended there is one possible error code for each sector only
    // basically a d64 file consists only of raw data, there is no information about the 
    // structure of a track, which would be needed for copy protections.
    // we have to recreate disk structure in CBM-DOS format, counterpart of amiga dos
    // first we generate the sync mark and sector header
    
    Emulator::Gcr gcr;
    uint8_t buf[4];
    uint8_t idm = (errorCode == ERR_SECTOR_ID) ? 0xff : 0x00; // disk sector ID mismatch

    // place sync mark if there is no sync error
    memset( target, (errorCode == ERR_SYNC) ? 0x55 : 0xff, 5 );
    target += 5;

    uint8_t chksum = (errorCode == ERR_HEADER_CHECKSUM) ? 0xff : 0x00; // checksum error in header block
    chksum ^= sector ^ track ^ id1 ^ id2 ^ idm; // header checksum
    // header begins with 0x08 if there is no error
    buf[0] = (errorCode == ERR_HEADER) ? 0xff : 0x08;   // header block not found
    buf[1] = chksum;
    buf[2] = sector;
    buf[3] = track;
    gcr.encode( buf, target );
    target += 5;

    buf[0] = id2;
    buf[1] = id1 ^ idm;
    buf[2] = buf[3] = 0x0f;
    gcr.encode( buf, target );
    target += 5;

    target += 9; // gap
    // another sync mark
    memset(target, (errorCode == ERR_SYNC) ? 0x55 : 0xff, 5); // sync error
    target += 5; // sync

    chksum = (errorCode == ERR_CHECKSUM) ? 0xff : 0x00; // checksum error in data block

    // ok now we write the sector data, which begins with descriptor byte
    buf[0] = (errorCode == ERR_NOBLOCK) ? 0x00 : 0x07;
    // first 3 bytes of raw data
    memcpy(buf + 1, src, 3);
    chksum ^= src[0] ^ src[1] ^ src[2]; // only raw data is checksummed
    gcr.encode(buf, target);
    src += 3;
    target += 5;

    for (int i = 0; i < 63; i++) {
        chksum ^= src[0] ^ src[1] ^ src[2] ^ src[3];
        gcr.encode(src, target);
        src += 4;
        target += 5;
    }

    buf[0] = src[0]; // last byte of raw data
    buf[1] = chksum ^ src[0]; // checksum of complete raw data
    buf[2] = buf[3] = 0;    // 2 off bytes
    gcr.encode(buf, target);
}

auto DiskStructure::handleAppendedTracksInDxx() -> bool {
    bool appended = false;

    for (uint8_t side = 0; side < sides; side++) {
        for (unsigned track = 1; track <= MAX_TRACKS; track++) {
            MTrack* gcrTrack = getTrackPtr(side, track * 2 - 2);

            if ((track > TYPICAL_TRACKS) && (gcrTrack->written & 1)) {
                if (track > tracksInDxx) {
                    appended = true;
                    tracksInDxx = track;
                }
            }
        }
    }

    if (!appended)
        return false;

    if (errorMap) {
        unsigned trackSectors = countSectors( tracksInDxx );
        int sectors = countSectors( tracksInDxx, 0 );

        unsigned newMapSize = sectors + trackSectors;
        if (sides == 2)
            newMapSize <<= 1;

        uint8_t* errorMapTemp = new uint8_t[newMapSize];
        std::memset( errorMapTemp, ERR_OK, newMapSize );

        if (sides == 2) {
            memcpy(errorMapTemp, errorMap, errorMapSize >> 1);
            memcpy(errorMapTemp + (newMapSize >> 1), errorMap + (errorMapSize >> 1), errorMapSize >> 1);

        } else {
            memcpy(errorMapTemp, errorMap, errorMapSize);
        }

        delete[] errorMap;
        errorMap = errorMapTemp;
        errorMapSize = newMapSize;
    }

    for (uint8_t side = 0; side < sides; side++) {
        for (unsigned track = 1; track <= MAX_TRACKS; track++) {
            MTrack* gcrTrack = getTrackPtr(side, track * 2 - 2);

            if (track <= tracksInDxx)
                gcrTrack->written = 1;
        }
    }

    return true;
}

auto DiskStructure::writeDxx(const MTrack* trackPtr, uint8_t side, unsigned track, bool& errorMapChanged) -> bool {
    // ok we need to decode the gcr track back in user data and write it out
    unsigned trackSectors = countSectors( track );
    
    // summed sector count till this track
    int sectors = countSectors( track, 0 );
    
    if (sectors < 0)
        return false; // higher than max track

    unsigned sectorOffset = 0;
    if (side == 1) {
        sectorOffset = countSectors( tracksInDxx, 0 );
        sectorOffset += countSectors( tracksInDxx );
    }
    
    // create a target buffer for all sectors of this track
    uint8_t* buffer = new uint8_t[ trackSectors * 256 ];
    memset( buffer, 0, trackSectors * 256 );
    
    for( unsigned sector = 0; sector < trackSectors; sector++ ) {
        
        int err = decodeSector( trackPtr, &buffer[sector * 256], sector );
        
        if ( err != ERR_OK ) {
            // the error have to be written to error map
            if (!errorMap) {
                // there wasn't an error map before, now we need one
                int allSectors = countSectors( tracksInDxx, 0 );
                
                if (allSectors >= 0) {
                    allSectors += countSectors( tracksInDxx );
                    if (sides == 2)
                        allSectors <<= 1;

                    errorMap = new uint8_t[allSectors];
                    std::memset( errorMap, ERR_OK, allSectors );
                    errorMapSize = allSectors;
                    errorMapChanged = true;
                }
            }
        }
        
        if (errorMap) {
            if ( errorMap[sectorOffset + sectors + sector] != err) {
                errorMap[sectorOffset + sectors + sector] = err;
                errorMapChanged = true;                
            }
        }
    }
    
    unsigned writeSize = trackSectors * 256;
    // "sectors" counts the sectors to the beginning of this track.
    // it's the starting offset for writting this track
    unsigned writtenSize = write( buffer, writeSize, (sectorOffset + sectors) * 256 );
    delete[] buffer;
    
    if (writtenSize != writeSize)        
        return false;
    
    return true;
}

auto DiskStructure::decodeSector( const MTrack* trackPtr, uint8_t* dest, uint8_t sector ) -> int {
    
    // we need to crawl the cbm dos track, which consists of sync, header, sync, sector data and so on
    unsigned offset = 0;
    uint8_t header[4];
    uint8_t buffer[260];
    int offsetTemp = -1;
    
    while(1) {        
        if ( !findSync( trackPtr, offset, trackPtr->size << 3 ) )
            return ERR_SYNC; // sync error, couldn't find a sync mark on whole track
        
        if (offsetTemp == offset)
            return ERR_HEADER; // header error, sync mark found but not the header for requested sector

        if (offsetTemp == -1)
            offsetTemp = offset; // memory first offset to find out later if all sync marks are tested
                                 // otherwise we loop forever
        
        // decode header following the sync mark
        decode( trackPtr, offset, header, 1 );
        
        if (header[0] == 0x08 && header[2] == sector)
            break; // sector header found
    }
    
    // find second sync directly following header
    if ( !findSync( trackPtr, offset, 500 * 8 ) )
        return ERR_SYNC;

    // decode sector gcr -> 65 * 4 byte
    decode( trackPtr, offset, buffer, 65 );

    // first of last 3 byte of sector data contains the checksum
    uint8_t checksum = buffer[257];

    // first byte of sector data should always 0x7
    if (buffer[0] != 0x07)
        return ERR_NOBLOCK; // descriptor byte wrong -> block error
    
    // sector data is not alligned to 4 byte blocks
    // we dont need the first byte and the last three
    for (unsigned i = 0; i < 256; i++) {
        dest[i] = buffer[i + 1];
        checksum ^= dest[i];
    }
    // finaly compare the stored checksum with calculated checksum by exclusive
    // or'ing both.
    // a "exclusive or" results in "1" only if compared bits are different.
    // means the checksum is wrong if any bit is non zero
    
    // software error 
    return checksum ? ERR_CHECKSUM : ERR_OK; // checksum doesn't match with calculated
}

auto DiskStructure::findSync( const MTrack* trackPtr, unsigned& offset, unsigned size ) -> bool {
    // check for sync mark and adjust offset to the bit following the sync mark
    
    if ( !trackPtr->data || !trackPtr->size )
        return false;

    unsigned buffer = 0;
    // offset is bit position in track ring buffer.
    // divide by 8 (out shifting 3 bits) to get related byte from ring buffer.
    // fetched byte is shifted left till current bit is the most significant bit
    uint8_t byte = trackPtr->data[offset >> 3] << (offset & 7);
    
    while ( size-- ) {               
        
        if (byte & 0x80) { 
            // next bit is a one, sync mark is checked only if a zero is incomming
            buffer = (buffer << 1) | 1; // shift in a one
            
        } else {
            // when a zero bit is incomming we have to check if previous 10 bits
            // are non zero in a row. If so we have found a sync mark
            if (( buffer & 0x3ff ) == 0x3ff)
                return true; // sync mark found, offset is the bit following the sync mark
              
            buffer <<= 1; // shift in a zero and go on
        }
        
        offset++; // advance seek offset
        
        if ( (offset & 7) == 0 ) { // offset reaches next byte            
            
            // check for overflow of ring buffer
            if (offset >= (trackPtr->size << 3) )
                offset = 0;
            // fetch next byte
            byte = trackPtr->data[offset >> 3];
                        
        } else
            // shift next bit to msb
            byte <<= 1;
    }
    
    return false;
}

auto DiskStructure::decode( const MTrack* trackPtr, unsigned offset, uint8_t* buffer, unsigned blockCount ) -> void {
    Emulator::Gcr gcr;
    // offset is starting bit in requested track. NOTE: decoding can start mid byte
    // blockCount is the amount of 5-byte gcr blocks we want to decode in 4-byte each
    uint8_t data[5];
    // the alignment doesn't change from byte to byte, so we have
    // to calculate it one time only
    uint8_t shift = 8 - (offset & 7);        
    
    // related byte in ring buffer
    unsigned pos = offset >> 3;
    // we need one byte ahead because of a possible non aligned byte stream
    unsigned stream = trackPtr->data[pos];
        
    for (unsigned i = 0; i < blockCount; i++, buffer += 4) {
        
        // 5 gcr bytes = 1 block for decoding
        for (unsigned j = 0; j < 5; j++) {
            pos++;
            // wrap around
            if (pos >= trackPtr->size)
                pos = 0;
            
            stream <<= 8; // make room for next byte
            stream |= trackPtr->data[ pos ];
            // we have to fetch aligned data from bit stream
            data[j] = ( stream >> shift ) & 0xff;
        }
        
        gcr.decode( data, buffer );
    }
}

auto DiskStructure::imageSizeD64() -> unsigned {
    
    return TYPICAL_SIZE;
}

auto DiskStructure::imageSizeD71() -> unsigned {

    return TYPICAL_SIZE << 1;
}

auto DiskStructure::createDxx( std::string diskName, uint8_t sides ) -> uint8_t* {
    
    uint8_t buffer[256];    
    std::memset(buffer, 0, 256);
    buffer[1] = 255;
    
    uint8_t* temp = new uint8_t[ (sides == 2) ? imageSizeD71() : imageSizeD64() ];
    std::memset( temp, 0, (sides == 2) ? imageSizeD71() : imageSizeD64() );

    writeSector( temp, buffer, 18, 1 ); // write directory sector

    uint8_t bufferBamExtended[256];

    createBAM( diskName, buffer, (sides == 2) ? bufferBamExtended : nullptr ); // resuse buffer for bam sector
    writeSector( temp, buffer, 18, 0 ); // write bam sector
    if (sides == 2)
        writeSector( temp, bufferBamExtended, 18, 0, imageSizeD64() );

    return temp;    
}

auto DiskStructure::writeSector( uint8_t* target, uint8_t* buffer, uint8_t track, uint8_t sector, unsigned offset) -> void {
        
    int sectors = countSectors( track, sector );
    
    if (sectors < 0)
        return;
    
    std::memcpy( target + offset + (sectors << 8), buffer, 256 );
}

auto DiskStructure::readSector( uint8_t* src, uint8_t* buffer, uint8_t track, uint8_t sector, unsigned offset ) -> bool {

    int sectors = countSectors( track, sector );

    if (sectors < 0)
        return false;

    std::memcpy( buffer, src + offset + (sectors << 8), 256 );

    return true;
}


}
