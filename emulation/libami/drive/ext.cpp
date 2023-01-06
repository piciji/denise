
namespace LIBAMI {

auto DiskStructure::analyzeEXT(uint8_t *data, unsigned size) -> bool {
    if (size < 12)
        return false;

    if (std::memcmp(data, "UAE-1ADF", 8))
        return false;

    unsigned _trackCount = (data[10] << 8) | data[11];

    if (size < (12 + _trackCount * 12))
        return false;

    trackCount = _trackCount;

    type = Type::EXT;

    return true;
}

auto DiskStructure::prepareEXT(uint8_t *data, unsigned size) -> void {
    hd = false;
    uint8_t *ptr = data + 12;
    unsigned dataOffset = 12 + trackCount * 12;

    bool mfmTrack;
    unsigned length;
    unsigned bits;

    for (unsigned i = 0; i < LIBAMI_MAX_TRACKS; i++) {
        Track &track = tracks[i];

        if (i < trackCount) {
            mfmTrack = ptr[3];
            length = (ptr[5] << 16) | (ptr[6] << 8) | ptr[7]; // ignore the MSB for sanity reasons
            bits = (ptr[9] << 16) | (ptr[10] << 8) | ptr[11]; // ignore the MSB for sanity reasons

            initTrack(track, mfmTrack ? length : getTrackByteLength(), mfmTrack ? bits : getTrackBitLength());
            if (!mfmTrack) track.written = 0x80;

            if ((dataOffset + length) >= size)
                goto Next;
        } else {
            initTrack(track, getTrackByteLength());
            track.written = 0x80;
            continue;
        }

        if (mfmTrack) {
            if (bits > (length * 8)) {
                bits = length * 8;
                track.bits = bits;
            }

            if (!hd && (bits > (13000 * 8)))
                hd = true; // ((512 + 32) * 11) * 2 (Clock + Data bit) = 11968 + a few more gap bytes

            std::memcpy(track.data, data + dataOffset, track.length);

        } else {
            if (!hd && (bits > (6000 * 8))) hd = true;

            if (length >= ((512 * 11) << hd)) // minimum length, otherwise encoding will crash hard
                encodeTrack(track, i, data + dataOffset);
        }
        Next:
        ptr += 12;
        dataOffset += length;
    }
}

auto DiskStructure::createEXT(unsigned size) -> uint8_t* {
    uint8_t* dest = new uint8_t[size];
    std::memset(dest, 0, size);

    std::memcpy(dest, "UAE-1ADF", 8);
    dest[11] = trackCount;
    dest += 12;

    for (unsigned i = 0; i < trackCount; i++) {
        Track& track = tracks[i];

        dest[3] = 1; // MFM
        Emulator::copyIntToBufferBigEndian<uint32_t>(&dest[4], track.length);
        Emulator::copyIntToBufferBigEndian<uint32_t>(&dest[8], track.bits);
        dest += 12;
    }

    for (unsigned i = 0; i < trackCount; i++) {
        Track& track = tracks[i];
        std::memcpy(dest, track.data, track.length);
        dest += track.length;
        track.written = 0;
    }
    return dest;
}

auto DiskStructure::EXTImageNeedsCompleteRebuild() -> bool {
    for (int i = LIBAMI_MAX_TRACKS; i > 0; i--) {
        Track& track = tracks[i - 1];

        if ((track.written & 0x81) == 0x81) {
            if (i > trackCount)
                trackCount = i;
            return true;
        }
    }
    return false;
}

auto DiskStructure::getEXTCreationImageSize() -> unsigned {
    unsigned size = 0;
    for (unsigned i = 0; i < trackCount; i++) {
        Track& track = tracks[i];
        size += track.length;
    }

    return 12 + trackCount * 12 + size;
}

}
