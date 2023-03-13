
namespace LIBAMI {

auto DiskStructure::analyzeEXT2(uint8_t *data, unsigned size) -> bool {
    if (size < 12)
        return false;

    if (std::memcmp(data, "UAE-1ADF", 8))
        return false;

    unsigned _trackCount = (data[10] << 8) | data[11];

    if (size < (12 + _trackCount * 12))
        return false;

    trackCount = _trackCount;

    type = Type::EXT2;

    return true;
}

auto DiskStructure::prepareEXT2(uint8_t *data, unsigned size) -> void {
    hd = false;
    uint8_t* ptr = data + 12;
    unsigned dataOffset = 12 + trackCount * 12;

    bool mfmTrack;
    unsigned storage;
    unsigned bits;

    for (unsigned i = 0; i < LIBAMI_MAX_TRACKS; i++) {
        Track& track = tracks[i];

        if (i < trackCount) {
            mfmTrack = ptr[3];
            storage = (ptr[5] << 16) | (ptr[6] << 8) | ptr[7]; // ignore the MSB for sanity reasons
            bits = (ptr[9] << 16) | (ptr[10] << 8) | ptr[11]; // ignore the MSB for sanity reasons

            if ((dataOffset + storage) <= size) {
                if (bits > (storage << 3))
                    bits = storage << 3;

                if (!mfmTrack) { // ADF
                    // "bits" contains the user data size: (512 * 11 * 8) << hd
                    // "storage" contains the MFM size
                    if (!hd && ( bits > ((512 * 11 * 8) + (512 * 8) /* save spot */ ) ))
                        hd = true; // assume all other tracks are HD too

                    initTrack(track, getTrackByteLength(), 0, 0xaa);
                    if (storage >= ((512 * 11) << hd))
                        encodeTrack(track, i, data + dataOffset);

                    // if ADF track is written, we recreate the whole image for simplicity
                    track.written = 0x80;
                } else {
                    if (!hd && (bits > (13000 * 8)))
                        hd = true; // ((512 + 32) * 11) * 2 (Clock + Data bit) = 11968 + a few more gap bytes

                    // Unfortunately, programs process the last byte completely, even if the number of bits is not divisible by 8.
                    // we take the exact count of bits for timing only
                    initTrack(track, (bits + 7) / 8, bits, 0xaa);
                    std::memcpy(track.data, data + dataOffset, track.length);
                    track.written = 0;
                }
                track.storage = storage;

            } else {
                initTrack(track, getTrackByteLength());
                track.written = 0x80;
            }

            ptr += 12;
            dataOffset += storage;

        } else {
            initTrack(track, getTrackByteLength());
            track.written = 0x80; // if empty track is written, we recreate the whole image
        }
    }
}

auto DiskStructure::createEXT2(unsigned size) -> uint8_t* {
    uint8_t* dest = new uint8_t[size];
    std::memset(dest, 0, size);
    uint8_t* ptr = dest;

    std::memcpy(ptr, "UAE-1ADF", 8);
    ptr[11] = trackCount;
    ptr += 12;

    for (unsigned i = 0; i < trackCount; i++) {
        Track& track = tracks[i];

        ptr[3] = 1; // MFM
        Emulator::copyIntToBufferBigEndian<uint32_t>(&ptr[4], track.length);
        Emulator::copyIntToBufferBigEndian<uint32_t>(&ptr[8], track.bits);
        ptr += 12;
    }

    for (unsigned i = 0; i < trackCount; i++) {
        Track& track = tracks[i];
        std::memcpy(ptr, track.data, track.length);
        ptr += track.length;
        track.written = 0;
    }
    return dest;
}

auto DiskStructure::EXT2ImageNeedsCompleteRebuild() -> bool {
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

auto DiskStructure::getEXT2CreationImageSize() -> unsigned {
    unsigned size = 0;
    for (unsigned i = 0; i < trackCount; i++) {
        Track& track = tracks[i];
        size += track.length;
    }

    return 12 + trackCount * 12 + size;
}

}
