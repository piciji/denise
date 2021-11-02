
#include "structure.h"
#include "../wd177x/wd1770.h"

namespace LIBC64 {
  
auto DiskStructure::analyzeG64() -> bool {

    if (rawSize < 32)
        return false; // too small
    
    if (rawData[8] != 0) // unknown version
        return false;
    
    if (rawData[9] < 1) // 0 tracks ? wtf
        return false;
    
    if (std::memcmp(rawData, "GCR-1541", 8)) // missing this ident ?
        return false;

    unsigned maxHalfTracks = rawData[9];

    maxTrackLength = rawData[10] | (rawData[11] << 8);    
    
    if (maxHalfTracks > (MAX_TRACKS * 2) ) // more tracks than a 1541 drive can handle
        return false;

    sides = 1;
    type = Type::G64;
    
    return true;
}

auto DiskStructure::analyzeG71() -> bool {

    if (rawSize < 32)
        return false; // too small

    if (rawData[8] != 0) // unknown version
        return false;

    if (rawData[9] < 1) // 0 tracks ? wtf
        return false;

    if (std::memcmp(rawData, "GCR-1571", 8)) // missing this ident ?
        return false;

    unsigned maxHalfTracks = rawData[9];

    maxTrackLength = rawData[10] | (rawData[11] << 8);

    if (maxHalfTracks > (MAX_TRACKS * 2 * 2) ) // more tracks than a 1541 drive can handle
        return false;

    sides = 2;
    type = Type::G71;

    return true;
}

auto DiskStructure::getTrackOffsetGxx( uint8_t halfTrack, int& error ) -> uint32_t {
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
    
auto DiskStructure::prepareGxx() -> void {
    
    uint8_t buf[2];
    unsigned offset;
    unsigned trackLength;
    unsigned maxHalfTracks = rawData[9];
    int error;

    for (uint8_t side = 0; side < sides; side++) {
        for (unsigned halfTrack = 0; halfTrack < (MAX_TRACKS * 2); halfTrack++) {
            MTrack* ptr = &gcrTracks[side][halfTrack];
            unsigned totalHalfTrack = side * MAX_TRACKS * 2 + halfTrack;

            if (ptr->data)
                delete[] ptr->data;

            if (ptr->mfmSync)
                delete[] ptr->mfmSync;

            ptr->mfmSync = nullptr;
            ptr->data = nullptr;
            ptr->size = 0;
            ptr->bits = 1;

            if (totalHalfTrack >= maxHalfTracks)
                continue;

            offset = getTrackOffsetGxx(totalHalfTrack, error);

            if (error < 0)
                continue;

            if (offset > 0) {

                std::memcpy(buf, rawData + offset, 2);

                trackLength = Emulator::copyBufferToInt<uint16_t>(&buf[0]);

                bool mfm = (trackLength & 0x8000) != 0;
                trackLength &= 0x7fff;

                // header area contains the value for maximal track length.
                // each track begins with a 2 byte value about track length.
                // next track isn't following immediately, otherwise a changed
                // track length would overwrite and corrupt next track.
                // so each track has a size of max track length.
                // the gap between real track length and max track length
                // is filled with zero's.
                if ((trackLength < 1) || (trackLength > maxTrackLength))
                    continue;

                if (rawSize < (offset + 2 + trackLength)) // rawfile too small
                    continue;

                ptr->size = trackLength;
                ptr->bits = ptr->size << 3;
                ptr->data = new uint8_t[ptr->size];
                ptr->mfmSync = new uint8_t[ptr->size >> 3];

                if (mfm) {
                    parseMfm(ptr, offset + 2);
                } else {
                    std::memcpy(ptr->data, rawData + offset + 2, trackLength);
                }

            } else { // if track doesn't exists
                ptr->size = countBytes((halfTrack + 2) / 2); // standard length
                ptr->bits = ptr->size << 3;
                ptr->data = new uint8_t[ptr->size];
                std::memset(ptr->data, 0x55, ptr->size);
            }

            if (!ptr->mfmSync) {
                ptr->mfmSync = new uint8_t[ptr->size >> 3];
                std::memset(ptr->mfmSync, 0x00, ptr->size >> 3);
            }

            if (ptr->bits == 0)
                ptr->bits = 1;
        }
    }
}

inline auto DiskStructure::addMfmByte(uint8_t*& dest, uint8_t data, uint16_t& crc) -> void {
    *dest++ = data;
    calcMfmCrc(data, crc);
}

inline auto DiskStructure::calcMfmCrc(uint8_t data, uint16_t& crc) -> void {
    crc = WD1770::CRC1021[(crc >> 8) ^ data] ^ (crc << 8);
}

auto DiskStructure::parseMfm(MTrack* trackPtr, unsigned offset) -> void {
    unsigned pos;
    uint16_t crc;

    std::memset(trackPtr->data, 0x4e, trackPtr->size);
    trackPtr->mfmSync = new uint8_t[trackPtr->size >> 3];
    std::memset(trackPtr->mfmSync, 0x00, trackPtr->size >> 3);

    if ((offset + (5 * 32 + 2)) >= rawSize)
        return;

    uint8_t sectorCount = rawData[offset++];
    uint8_t version = rawData[offset++];

    unsigned dataOffset = offset + 32 * 5;

    if (sectorCount > 32)
        sectorCount = 32;

    uint8_t* ptr = trackPtr->data;

    unsigned todo = 6250;
    if (trackPtr->size < todo)
        todo = trackPtr->size;

    if (todo < 96)
        return;
    todo -= 96;

    memset(ptr, 0x4e, 80); ptr += 80;
    memset(ptr, 0x0, 12); ptr += 12;

    pos = ptr - trackPtr->data;
    trackPtr->mfmSync[pos >> 3] |= 1 << (pos & 7); pos++;
    trackPtr->mfmSync[pos >> 3] |= 1 << (pos & 7); pos++;
    trackPtr->mfmSync[pos >> 3] |= 1 << (pos & 7); pos++;

    addMfmByte(ptr, 0xc2, crc);
    addMfmByte(ptr, 0xc2, crc);
    addMfmByte(ptr, 0xc2, crc);
    addMfmByte(ptr, 0xfc, crc);

    memset(ptr, 0x4e, 50); ptr += 50;

    //memset(ptr, 0x4e, 60); ptr += 60;

    for (unsigned i = 0; i < sectorCount; i++) {

        uint8_t track = rawData[offset++];
        uint8_t side = rawData[offset++];
        uint8_t sector = rawData[offset++];
        uint8_t sectorSize = rawData[offset++];
        uint8_t errorByte = rawData[offset++];

        sectorSize &= 3;

        auto blockSize = 12 + 4 + 6 + 22 + 12 + 4 + (128 << sectorSize) + 2 + 22;

        if (todo < blockSize)
            return;

        todo -= blockSize;

        memset(ptr, 0x0, 12); ptr += 12;

        pos = ptr - trackPtr->data;
        trackPtr->mfmSync[pos >> 3] |= 1 << (pos & 7); pos++;
        trackPtr->mfmSync[pos >> 3] |= 1 << (pos & 7); pos++;
        trackPtr->mfmSync[pos >> 3] |= 1 << (pos & 7); pos++;

        crc = (errorByte & 1) ? 0 : 0xffff;
        addMfmByte(ptr, 0xa1, crc);
        addMfmByte(ptr, 0xa1, crc);
        addMfmByte(ptr, 0xa1, crc);
        addMfmByte(ptr, 0xfe, crc);

        addMfmByte(ptr, track, crc);
        addMfmByte(ptr, side, crc);
        addMfmByte(ptr, sector, crc);
        addMfmByte(ptr, sectorSize, crc);
        *ptr++ = crc >> 8;
        *ptr++ = crc & 0xff;
        memset(ptr, 0x4e, 22); ptr += 22;
        memset(ptr, 0x0, 12); ptr += 12;

        if ((errorByte & 4) == 0) {
            pos = ptr - trackPtr->data;
            trackPtr->mfmSync[pos >> 3] |= 1 << (pos & 7);
            pos++;
            trackPtr->mfmSync[pos >> 3] |= 1 << (pos & 7);
            pos++;
            trackPtr->mfmSync[pos >> 3] |= 1 << (pos & 7);
            pos++;

            crc = (errorByte & 2) ? 0 : 0xffff;
            addMfmByte(ptr, 0xa1, crc);
            addMfmByte(ptr, 0xa1, crc);
            addMfmByte(ptr, 0xa1, crc);
            addMfmByte(ptr, (errorByte & 0x10) ? 0xf8 : 0xfb, crc);

            if (dataOffset + (128 << sectorSize) >= rawSize)
                return;

            for (unsigned j = 0; j < (128 << sectorSize); j++) {
                addMfmByte(ptr, rawData[dataOffset++], crc);
            }

            *ptr++ = crc >> 8;
            *ptr++ = crc & 0xff;
        }

        memset(ptr, 0x4e, 22); ptr += 22;
    }
}

auto DiskStructure::writeMfm(const MTrack* trackPtr, unsigned offset) -> bool {
    unsigned startOffset = offset;
    bool isSync = false;
    uint8_t buf[1] = {0}; // version or errors
    uint16_t crc;
    uint16_t crcFetched;

    uint8_t* ptr = trackPtr->data;

    offset += 2;
    unsigned dataOffset = offset + 32 * 5;
    unsigned sectorSize;
    unsigned sectorCount = 0;
    bool align = false;
    uint8_t error = 0;

    unsigned todo = 6250;
    if (trackPtr->size < todo)
        todo = trackPtr->size;

    for(unsigned i = 0; i < todo; i++) {

        if (isSync && (ptr[i] == 0xfc || ptr[i] == 0xfd || ptr[i] == 0xfe || ptr[i] == 0xff) ) {
            if (sectorCount == 32)
                break;

            // Track, Side, Sector, Sector Size
            if ( write( ptr + i + 1, 4, offset ) != 4)
                return false;

            sectorSize = 128 << (ptr[i + 1 + 3] & 3);

            error = 4;
            crc = 0xffff;
            calcMfmCrc( 0xa1, crc );
            calcMfmCrc( 0xa1, crc );
            calcMfmCrc( 0xa1, crc );
            calcMfmCrc( ptr[i], crc );
            calcMfmCrc( ptr[i + 1], crc );
            calcMfmCrc( ptr[i + 2], crc );
            calcMfmCrc( ptr[i + 3], crc );
            calcMfmCrc( ptr[i + 4], crc );

            uint8_t* _dataCrc = ptr + i + 5;
            crcFetched = (_dataCrc[0] << 8) | _dataCrc[1];

            if(crc != crcFetched) {
                error |= 1;
            }

            buf[0] = error;
            if ( write( &buf[0], 1, offset + 4 ) != 1) // errors
                return false;

            offset += 5;

            align = true;
            sectorCount++;

        } else if (align && isSync && (ptr[i] == 0xf8 || ptr[i] == 0xf9 || ptr[i] == 0xfa || ptr[i] == 0xfb) ) {

            align = false;
            error &= ~4;

            if (ptr[i] == 0xf8 || ptr[i] == 0xf9)
                error |= 0x10;

            crc = 0xffff;
            calcMfmCrc( 0xa1, crc );
            calcMfmCrc( 0xa1, crc );
            calcMfmCrc( 0xa1, crc );
            calcMfmCrc( ptr[i], crc );

            for (unsigned j = 0; j < sectorSize; j++) {
                calcMfmCrc( *(ptr + i + 1 + j), crc);
            }

            if ( write( ptr + i + 1, sectorSize, dataOffset ) != sectorSize)
                return false;

            uint8_t* _dataCrc = ptr + i + 1 + sectorSize;
            crcFetched = (_dataCrc[0] << 8) | _dataCrc[1];

            if(crc != crcFetched) {
                error |= 2;
            }

            buf[0] = error;
            if ( write( &buf[0], 1, offset - 1 ) != 1) // errors
                return false;

            dataOffset += sectorSize;
        }

        isSync = ((trackPtr->mfmSync[i >> 3] & (1 << (i & 7))) != 0);
        if (isSync && (ptr[i] != 0xa1) ) {
            isSync = false;
        }
    }

    buf[0] = 0;
    write( &buf[0], 1, startOffset + 1 ); // version
    buf[0] = sectorCount;
    write( &buf[0], 1, startOffset ); //sectors

    return true;
}

auto DiskStructure::writeGxx(const MTrack* trackPtr, uint8_t side, unsigned halfTrack) -> bool {
    int error;
    uint8_t buf[4];
    bool appendTrack = false;

    unsigned trackOffset = side * MAX_TRACKS * 2;
    
    long int offset = getTrackOffsetGxx( halfTrack + trackOffset, error );
    
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

    if (trackPtr->written & 0x80)
        // mark mfm track in highest bit of track length
        buf[1] |= 0x80;

    // first 2 bytes are track length
    if ( write( &buf[0], 2, offset ) != 2)
        return false;
    // next is gcr encoded data of track or mfm user data (no mfm encoded data)
    if (trackPtr->written & 0x80) {
        if (!writeMfm(trackPtr, offset + 2))
            return false;
    } else {
        if ( write( trackPtr->data, trackPtr->size, offset + 2 ) != trackPtr->size )
            return false;
    }

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
        
        if ( write( &buf[0], 4, 12 + ( (trackOffset + halfTrack) * 4) ) != 4 )
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
        
        if ( write( &buf[0], 4, 12 + (MAX_TRACKS_1541 * 2 * sides * 4) + ((halfTrack + trackOffset) * 4) ) != 4 )
            return false;
    }
    
    return true;
}

auto DiskStructure::imageSizeG64() -> unsigned {
    
    unsigned maxBytes = 7928;
    
    return 12 + MAX_TRACKS_1541 * 2 * 8 + TYPICAL_TRACKS * (maxBytes + 2);
}

auto DiskStructure::imageSizeG71() -> unsigned {

    unsigned maxBytes = 7928;

    return 12 + MAX_TRACKS_1541 * 2 * 2 * 8 + TYPICAL_TRACKS * 2 * (maxBytes + 2);
}

auto DiskStructure::createGxx( std::string diskName, uint8_t sides ) -> uint8_t* {
    
    uint8_t* temp = new uint8_t[ (sides == 2) ? imageSizeG71() : imageSizeG64() ];
    uint8_t* ptr = temp;
    
    std::memset( ptr, 0, (sides == 2) ? imageSizeG71() : imageSizeG64() );
    
    uint8_t buffer[256];    
    std::memset( buffer, 0, 256 );
    
    uint8_t bufferDir[256];    
    std::memset( bufferDir, 0, 256 );
    bufferDir[1] = 255;

    uint8_t bufferBam[256];
    uint8_t bufferBamExtended[256];

    createBAM( diskName, bufferBam, (sides == 2) ? bufferBamExtended : nullptr );
    
    // max bytes per track
    unsigned maxBytes = 7928;
    
    // 12 byte header
    std::memcpy( ptr, (sides == 2) ? "GCR-1571" : "GCR-1541", 8 );
    
    ptr[8] = 0;
    
    ptr[9] = MAX_TRACKS * 2 * sides;
    
    Emulator::copyIntToBuffer<uint16_t>( &ptr[10], maxBytes );    
    
    ptr += 12;
    
    // we prepare the image with standard 35 tracks and without half tracks.
    for (uint8_t side = 0; side < sides; side++) {

        unsigned trackOffset = (side == 1) ? (MAX_TRACKS << 1) : 0;

        for (unsigned track = 0; track < TYPICAL_TRACKS; track++) {

            // not to waste space we dont't include half tracks during image creation.
            // therefore we leave room (4 bytes) between the offset values. offsets for half tracks will be
            // added later if needed.
            // to calculate the offset value we have to keep in mind that a speed map is following the
            // track offsets. 12 byte header + 4 byte track offset * max half tracks + 4 byte speed map * max half tracks.
            // in main header the maximum amount of bytes for a track is specified. so we make
            // that amount of room for each track to avoid realigning of multiple tracks later on.
            // furthermore we keep in mind that each track begins with a 2 byte length value.
            Emulator::copyIntToBuffer<uint32_t>(&ptr[(trackOffset * 4) + (track * 2 * 4)],
                (12 + MAX_TRACKS_1541 * 2 * 8 * sides) + ((side * TYPICAL_TRACKS) + track) * (maxBytes + 2));
        }
    }
    
    ptr += MAX_TRACKS_1541 * 2 * 4 * sides;

    for (uint8_t side = 0; side < sides; side++) {
        unsigned trackOffset = (side == 1) ? (MAX_TRACKS << 1) : 0;

        for (unsigned track = 0; track < TYPICAL_TRACKS; track++)
            // we use the typical speedzone for a track
            Emulator::copyIntToBuffer<uint32_t>(&ptr[(trackOffset * 4) + (track * 2 * 4)], speedzone(track + 1));
    }

    ptr += MAX_TRACKS_1541 * 2 * 4 * sides;

    for (uint8_t side = 0; side < sides; side++) {
        for (unsigned track = 1; track <= TYPICAL_TRACKS; track++) {
            // we init the structure in CBM DOS format
            uint8_t gaps = gapSize(track);

            unsigned trackSize = countBytes(track);

            Emulator::copyIntToBuffer<uint16_t>(ptr, trackSize);

            ptr += 2;

            std::memset(ptr, 0x55, trackSize);

            uint8_t* sectorPtr = ptr;

            for (unsigned sector = 0; sector < countSectors(track); sector++) {

                uint8_t* useBuffer = buffer;

                if (side == 0 && track == 18 && sector == 1) // directory sector
                    useBuffer = bufferDir;
                else if (side == 0 && track == 18 && sector == 0) // bam sector
                    useBuffer = bufferBam;
                else if (side == 1 && track == 18 && sector == 0) // bam sector
                    useBuffer = bufferBamExtended;

                encodeSector(useBuffer, sectorPtr, track + (side * TYPICAL_TRACKS), sector, 0xa0, 0xa0, ERR_OK);

                sectorPtr += 340 + 9 + gaps + 5;
            }

            ptr += maxBytes;
        }
    }
    
    return temp;
}

}
