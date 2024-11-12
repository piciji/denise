
#include "structure.h"
#include "../../system/testbench.h"
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

    if (maxHalfTracks > (MAX_TRACKS * 2 * 2) ) // more tracks than a 1571 drive can handle
        return false;

    sides = 2;
    type = Type::G71;

    return true;
}

auto DiskStructure::analyzeG81() -> bool {
    if (rawSize < 32)
        return false; // too small

    if (rawData[8] != 0) // unknown version
        return false;

    if (rawData[9] < 1) // 0 tracks ? wtf
        return false;

    if (std::memcmp(rawData, "MFM-1581", 8)) // missing this ident ?
        return false;

    unsigned maxTracks = rawData[9];

    maxTrackLength = rawData[10] | (rawData[11] << 8);

    if (maxTracks > (MAX_TRACKS_1581 * 2) ) // more tracks than a 1581 drive can handle
        return false;

    sides = 2;
    type = Type::G81;

    return true;
}

auto DiskStructure::getTrackOffsetGxx( uint8_t _track, int& error ) -> uint32_t {
    uint8_t buf[4];
    error = 0;
    
    // for each track there is a 4 byte offset
    uint32_t offset = 12 + (_track * 4);

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

auto DiskStructure::prepareG81() -> void {
    uint8_t buf[4];
    unsigned offset;
    unsigned bitLength;
    unsigned maxTracks = rawData[9];
    int error;

    for( uint8_t side = 0; side < sides; side++ ) {
        for (uint8_t track = 0; track < MAX_TRACKS_1581; track++) {
            MTrack* trackPtr = &gcrTracks[side][track];
            //unsigned trackPos = (track << 1) | side;
            unsigned trackPos = side * MAX_TRACKS_1581 + track;

            if (trackPtr->data)
                delete[] trackPtr->data;
            if (trackPtr->mfmSync)
                delete[] trackPtr->mfmSync;

            trackPtr->mfmSync = nullptr;
            trackPtr->data = nullptr;
            trackPtr->size = 0;
            trackPtr->bits = 0;

            if (trackPos >= maxTracks)
                continue;

            offset = getTrackOffsetGxx(trackPos, error);
            if (error < 0)
                continue;

            if (offset > 0) {
                std::memcpy(buf, rawData + offset, 4);

                bitLength = Emulator::copyBufferToInt<uint32_t>(&buf[0]);
                unsigned trackLength = (bitLength + 7) / 8;

                if ((bitLength == 0) || (trackLength > maxTrackLength))
                    continue;

                if (rawSize < (offset + 4 + trackLength)) // raw file too small
                    continue;

                trackPtr->size = trackLength;
                trackPtr->bits = bitLength;
                trackPtr->data = new uint8_t[trackLength];
                std::memcpy(trackPtr->data, rawData + offset + 4, trackLength);

            } else { // if track doesn't exist
                trackPtr->size = 6250 * 2; // for clock bits
                trackPtr->bits = trackPtr->size << 3;
                trackPtr->data = new uint8_t[trackPtr->size];
                std::memset(trackPtr->data, 0x4e, trackPtr->size);
            }
        }
    }
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

                if (rawSize < (offset + 2 + trackLength)) // raw file too small
                    continue;

                ptr->size = trackLength;
                ptr->bits = ptr->size << 3;
                ptr->data = new uint8_t[ptr->size];

                if (mfm) {
                    parseMfm(ptr, offset + 2);
                } else {
                    std::memcpy(ptr->data, rawData + offset + 2, trackLength);
                }

            } else { // if track doesn't exist
                ptr->size = countBytes((halfTrack + 2) / 2); // standard length
                ptr->bits = ptr->size << 3;
                ptr->data = new uint8_t[ptr->size];
                std::memset(ptr->data, 0x55, ptr->size);
            }

            // this test expects the tracks to be realigned against the specification in G64. in contrast to the D64,
            // a G64 has the possibility to align the tracks to each other according to the original.
            // this test doesn't make sense to me.
            if (system && system->debugCart->enable) // "true" for testbench only
                disalignTrack(*ptr, halfTrack >> 1);

            if (!ptr->mfmSync) {
                ptr->mfmSync = new uint8_t[ptr->size >> 3];
                std::memset(ptr->mfmSync, 0x00, ptr->size >> 3);
            }
        }
    }

    // logTrackSkew();
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

auto DiskStructure::writeG81(const MTrack* trackPtr, uint8_t side, unsigned track) -> bool {
    int error;
    uint8_t buf[4];
    bool appendTrack = false;

    //unsigned trackPos = (track << 1) | side;
    unsigned trackPos = side * MAX_TRACKS_1581 + track;

    uint32_t offset = getTrackOffsetGxx( trackPos, error );

    if (error < 0)
        return false;

    if (trackPtr->size > maxTrackLength)
        // the changed track is bigger than max length ?
        return false;

    if (offset == 0) { // track doesn't exist. we append track at the end of raw file
        offset = rawSize;
        rawSize += 4 + maxTrackLength; // update image size
        appendTrack = true;
    }

    // now we write the changed track out to raw file, either overwrite the old one
    // or append it at end of file, see above
    Emulator::copyIntToBuffer<uint32_t>( &buf[0], trackPtr->bits );

    // first 2 bytes are track length
    if ( write( &buf[0], 4, offset ) != 4)
        return false;

    if ( write( trackPtr->data, trackPtr->size, offset + 4 ) != trackPtr->size )
        return false;

    // we need to fill the gap between this track and next one with zeros
    unsigned gapSize = maxTrackLength - trackPtr->size;

    if (gapSize > 0) {
        auto tempGap = new uint8_t[gapSize];
        std::memset(tempGap, 0, gapSize);

        unsigned bytesWritten = write( tempGap, gapSize, offset + 4 + trackPtr->size );

        delete[] tempGap;

        if (bytesWritten != gapSize)
            return false;
    }

    if (appendTrack) {
        // for a new appended track we need to update the offset in header area
        Emulator::copyIntToBuffer<uint32_t>( &buf[0], offset );

        if ( write( &buf[0], 4, 12 + (trackPos * 4) ) != 4 )
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

auto DiskStructure::imageSizeG81() -> unsigned {
    unsigned maxBytes = 6250 << 1;

    return 12 + MAX_TRACKS_1581 * 2 * 4 + 80 * 2 * (maxBytes + 4 );
}

auto DiskStructure::createG81(const std::string& diskName) -> uint8_t* {
    uint8_t* temp = createD81(diskName);

    DiskStructure structure(nullptr);
    structure.rawData = temp;
    structure.rawSize = imageSizeD81();

    if (!structure.analyzeD81())
        return nullptr;

    structure.prepareD81();

    auto temp2 = new uint8_t[ imageSizeG81() ];
    std::memset( temp2, 0, imageSizeG81() );
    uint8_t* ptr = temp2;
    std::memcpy( ptr, "MFM-1581", 8 );
    ptr[8] = 0;
    ptr[9] = MAX_TRACKS_1581 * 2;
    unsigned maxBytes = 6250 << 1;

    Emulator::copyIntToBuffer<uint16_t>( &ptr[10], maxBytes );
    ptr += 12;

    for( uint8_t side = 0; side < 2; side++ ) {
        for (uint8_t track = 0; track < TYPICAL_TRACKS_1581; track++) {
            //unsigned trackPos = (track << 1) | side;
            unsigned trackPos = side * MAX_TRACKS_1581 + track;
            unsigned trackOffset = 12 + MAX_TRACKS_1581 * 2 * 4 + ((side * TYPICAL_TRACKS_1581) + track) * (maxBytes + 4);

            MTrack* trackPtr = &structure.gcrTracks[side][track];
            structure.encodeMfm(trackPtr);

            Emulator::copyIntToBuffer<uint32_t>(&ptr[trackPos * 4], trackOffset);

            Emulator::copyIntToBuffer<uint32_t>(temp2 + trackOffset, trackPtr->bits);

            std::memcpy( temp2 + trackOffset + 4, trackPtr->data, maxBytes );
        }
    }

    delete[] temp;
    return temp2;
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

auto DiskStructure::decodeSectorMfm(MTrack* mTrack, uint8_t _track, uint8_t _sector, uint8_t* _data) -> bool {
    bool clockBit = true;
    uint8_t byte = 0;
    unsigned readBuffer = 0;
    bool syncMark;
    uint8_t bitCounter = 0;
    uint8_t countA1 = 0;
    uint8_t state = 0;
    unsigned delay = 0;
    unsigned _offset = 0;
    unsigned head = 0;
    bool overlap = false;

    if (!mTrack->data)
        return false;

    while(true) {
        syncMark = false;
        readBuffer <<= 1;
        readBuffer |= (mTrack->data[head>> 3] >> ((~head) & 7)) & 1;

        if ((readBuffer & 0x7fff) == 0x4489) // a1
            syncMark = true;

        if (!clockBit) {
            byte <<= 1;
            byte |= readBuffer & 1;

            if (++bitCounter == 8) {
                bitCounter = 0;

                if (syncMark && (byte == 0xa1)) {
                    if (++countA1 == 2) {
                        state = state == 8 ? 9 : 1;
                        countA1 = 0;
                        _offset = 0;
                    }
                } else {
                    countA1 = 0;

                    switch (state) {
                        case 0:
                        default:
                            break;
                        case 1: state = (byte == 0xfc || byte == 0xfd || byte == 0xfe || byte == 0xff) ? 2 : 0; break;
                        case 2: state = byte == _track ? 3 : 0; break;
                        case 3: state++; break; // side
                        case 4: state = byte == _sector ? 5 : 0; break;
                        case 5: state = byte == 2 ? 6 : 0; break; // sector size 2: 128 << 2 = 512 byte
                        case 6:
                        case 7: state++; // don't check CRC
                            delay = 44;
                            break;
                        case 8:
                            if (!--delay)
                                state = 0;
                            break;
                        case 9: state = (byte == 0xf8 || byte == 0xf9 || byte == 0xfa || byte == 0xfb) ? 10 : 0; break;
                        case 10:
                            _data[_offset++] = byte;
                            if (_offset == 512)
                                return true;
                            break;
                    }
                }
            }
        }

        clockBit ^= 1;
        if (syncMark) {
            clockBit = true;
            bitCounter = 0;
        }

        if (++head == mTrack->bits) {
            head = 0;
            overlap = true;
        } else if (overlap) {
            if (head > 2048)
               break; // no overlapped sector
        }
    }
    return false;
}

auto DiskStructure::encodeMfm(MTrack* mTrack) -> void {
    uint16_t word;
    bool dataZeroBefore = true;
    auto temp = new uint8_t[mTrack->size << 1];

    for(unsigned i = 0; i < mTrack->size; i++) {
        uint8_t _data = mTrack->data[i];
        bool _sync = mTrack->mfmSync[i >> 3] & (1 << (i & 7));

        word = ((_data & 0x80) >> 1) | ((_data & 0x40) >> 2) | ((_data & 0x20) >> 3) | ((_data & 0x10) >> 4);
        word <<= 8;
        word |= ((_data & 0x8) << 3) | ((_data & 0x4) << 2) | ((_data & 0x2) << 1) | ((_data & 0x1) << 0);

        for(int j = 14; j >= 0; j -= 2) {
            if ((word >> j) & 1) // data bit "one"
                dataZeroBefore = false;
            else { // data bit "zero"
                if (dataZeroBefore)
                    word |= 2 << j; // in MFM clock bit becomes "one" only, if data bit 0 follows another data bit 0

                dataZeroBefore = true;
            }
        }

        if (_sync) {
            if (_data == 0xa1)
                word &= ~0x20;
            else if (_data == 0xc2)
                word &= ~0x80;
        }

        temp[i << 1] = word >> 8;
        temp[(i << 1) + 1] = word & 0xff;
    }

    if (!dataZeroBefore)
        temp[0] &= 0x7f;

    delete[] mTrack->data;
    mTrack->data = temp;
    mTrack->size <<= 1;
    mTrack->bits <<= 1;
}

auto DiskStructure::logTrackSkew() -> void {
    for (uint8_t track = 1; track <= MAX_TRACKS; track++) {
        unsigned halfTrack = track * 2 - 2;
        MTrack* trackPtr = &gcrTracks[0][halfTrack];
        unsigned offset = 0;
        int offsetTemp = -1;
        uint8_t header[4];

        while(1) {
            if ( !findSync( trackPtr, offset, trackPtr->size << 3 ) )
                break; // sync error, couldn't find a sync mark on whole track

            if (offsetTemp == offset)
                break; // header error, sync mark found but not the header for requested sector

            if (offsetTemp == -1)
                offsetTemp = offset; // memory first offset to find out later if all sync marks are tested
            // otherwise we loop forever

            // decode header following the sync mark
            decode( trackPtr, offset, header, 1 );

            if (header[0] == 0x08 && header[2] == 0) // first sector
                break; // sector header found
        }

        float skew = (float)offset / (float)(trackPtr->size << 3);
        system->interface->log(track);
        system->interface->log(std::to_string(skew), false);
    }
}

}
