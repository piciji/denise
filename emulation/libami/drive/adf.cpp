
namespace LIBAMI {

auto DiskStructure::analyzeADF(uint8_t* data, unsigned size) -> bool {
    size &= ~511;

    for (unsigned i = 80; i <= 84; i++) {
        if (size == (i * 2 * 11 * 512)) {
            trackCount = i << 1;
            hd = false;
            type = Type::ADF;
            return true;
        }
        if (size == (i * 2 * 22 * 512)) {
            trackCount = i << 1;
            hd = true;
            type = Type::ADF;
            return true;
        }
    }
    return false;
}

auto DiskStructure::prepareADF(uint8_t* data, unsigned size) -> void {
    unsigned bytes = getTrackByteLength();
    unsigned sectors = hd ? 22 : 11;

    for(unsigned i = 0; i < LIBAMI_MAX_TRACKS; i++) {
        Track& track = tracks[i];
        initTrack(track, bytes);

        if (i < trackCount)
            encodeTrack( track, i, data + (i * sectors * 512) );
    }
}

auto DiskStructure::encodeTrack(Track& track, unsigned trackNr, uint8_t* userData) -> void {
    uint8_t* ptr;
    unsigned sectors = hd ? 22 : 11;
    // init with data bit zero for complete revolution, means clock bit is always one, because of the ring nature previous data bit is always zero
    bool lastDataWasAOne = false;
    uint8_t buffer[512];

    for( uint8_t sector = 0; sector < sectors; sector++ ) {
        ptr = track.data + sector * 544 * 2;

        ptr[0] = lastDataWasAOne ? 0x2a : 0xaa;
        ptr[1] = ptr[2] = ptr[3] = 0xaa;
        ptr[4] = 0x44, ptr[5] = 0x89; // encoded byte 0xa1
        ptr[6] = 0x44, ptr[7] = 0x89; // encoded byte 0xa1

        buffer[0] = 0xff, buffer[1] = trackNr, buffer[2] = sector, buffer[3] = sectors - sector;
        separateOddEven( &ptr[8], buffer, 4 );

        std::memset(buffer, 0, 4);
        for(unsigned i = 8; i < 48; i += 4) {
            buffer[0] ^= ptr[i];
            buffer[1] ^= ptr[i+1];
            buffer[2] ^= ptr[i+2];
            buffer[3] ^= ptr[i+3];
        }
        separateOddEven( &ptr[48], buffer, 4 );

        std::memcpy(buffer, userData + (sector * 512), 512);
        separateOddEven( &ptr[64], buffer, 512 );

        lastDataWasAOne = buffer[511] & 1;

        std::memset(buffer, 0, 4);
        for(unsigned i = 64; i < (64 + 512 * 2); i += 4) {
            buffer[0] ^= ptr[i];
            buffer[1] ^= ptr[i+1];
            buffer[2] ^= ptr[i+2];
            buffer[3] ^= ptr[i+3];
        }
        separateOddEven( &ptr[56], buffer, 4 );

        addClockBits(((uint16_t*)ptr) + 4, 512 + 32 - 4 );
    }

    if (lastDataWasAOne) {
        ptr = track.data + sectors * 544 * 2;
        *ptr &= 0x7f;
    }
}

auto DiskStructure::separateOddEven(uint8_t* dst, uint8_t src[], unsigned size) -> void {
    // seperate odd/even bits to make room for clock bits
    for(unsigned i = 0; i < size; i++) {
        dst[i] = (src[i] >> 1) & 0x55;
        dst[i + size] = src[i] & 0x55;
    }
}

auto DiskStructure::shiftData(uint8_t* dst, uint8_t* src, unsigned size, uint8_t shift) -> void {
    for (unsigned i = 0; i < size; ++i, ++src, ++dst)
        *dst = ((*src) << shift) | (*(src + 1) >> (8 - shift));
}

auto DiskStructure::decodeTrack(Track& track, uint8_t* userData) -> void {
    unsigned length = track.length;
    unsigned sectors = hd ? 22 : 11;
    uint32_t compare;
    bool match;
    uint8_t temp;
    unsigned index = 0;
    unsigned b = 0;
    uint8_t buffer[64 + 1024];
    uint8_t* ptr = new uint8_t[track.length + 1100];
    std::memcpy(ptr, track.data, track.length);
    std::memcpy(ptr + track.length, track.data, 1100); // overflow
    compare = Emulator::copyBufferToIntBigEndian<uint32_t>( ptr + track.length - 4 );

    while(1) {
        temp = *(ptr + index);
        if (b) temp <<= b;
        match = false;
        for(; b < 8; b++) {
            if (compare == 0x44894489) {
                match = true;
                break;
            }
            compare = (compare << 1) | (temp >> 7);
            temp <<= 1;
        }

        if (match) {
            shiftData(buffer, ptr + index, 64 + 1024, b);

            uint8_t info[4];
            joinOddEven(info, buffer, 4);
            uint8_t sector = info[2];
            if (sector < sectors)
                joinOddEven(userData + sector * 512, buffer + 64, 512);

            index += 64 + 1024;
            compare = 0;
            if (index >= length)
                break;
        } else {
            if (++index >= length)
                break;
            b = 0;
        }
    }
    delete[] ptr;
}

auto DiskStructure::joinOddEven(uint8_t* dst, uint8_t* src, unsigned size) -> void {
    for (unsigned i = 0; i < size; i++) {
        dst[i] = ((src[i] & 0x55) << 1) | (src[i + size] & 0x55);
    }
}

// <FM> (4 micro per bitcell)
// clock bit is always 1
// data bit	 => FM
// 1 		 => 11  [4 + 4 micro]
// 0 		 => 10  [4 + 4 micro]
//
// <MFM> (2 micro per bitcell)
// data bit => MFM
// 1		=> 01 [2 + 2 micro]
// 0		=> 10 or 00 (if previous data bit is 1)  [2 + 2 micro]
//
// Since there are never two "ones" (flux changes) right next to each other, the bitcell can be reduced from 4 micro to 2 without the flux change too tight.
// and thus achieve a doubling of the data density from FM to MFM

auto DiskStructure::addClockBits( uint16_t* raw, unsigned words) -> void {
    bool dataZeroBefore = false; // last data bit of sync pattern 0x4489 is "one"
    int i;
    uint16_t word;

    while (words--) {
        word = *raw;
        word &= 0x5555; // set all clock bits to zero

        for(i = 14; i >= 0; i -= 2) {
            if ((word >> i) & 1) // data bit "one"
                dataZeroBefore = false;
            else { // data bit "zero"
                if (dataZeroBefore)
                    word|= 2 << i; // in MFM clock bit becomes "one" only, if data bit 0 follows another data bit 0

                dataZeroBefore = true;
            }
        }
        *raw = word;
        raw++;
    }
}

auto DiskStructure::getADFCreationImageSize() -> unsigned {
    unsigned size = 11 * 512 * 160;
    if (hd) size <<= 1;
    return size;
}

auto DiskStructure::markAppendedADFTracks() -> void {
    bool appended = false;

    for(int i = LIBAMI_MAX_TRACKS; i > 0 ; i--) {
        Track& track = tracks[i-1];

        if (i > trackCount) {
            if (appended)
                track.written = 1;
            else if (track.written)
                appended = true;
        }
    }
}

}
