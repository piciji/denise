
namespace LIBAMI {

// Write processes (e.g. Hiscores) are problematic due to the sync word (track header) and therefore complex to program.
// For this reason, a write operation converts to EXT2. The same happens when the floppy disk is formatted
// or a completely different program is applied with XCopy.

auto DiskStructure::analyzeEXT(uint8_t* data, unsigned size) -> bool {
    if (size < 8)
        return false;

    if (std::memcmp(data, "UAE--ADF", 8))
        return false;

    if (size < (8 + 160 * 4))
        return false;

    trackCount = 160;

    type = Type::EXT;

    return true;
}

auto DiskStructure::prepareEXT(uint8_t* data, unsigned size) -> void {
    hd = false;
    uint8_t* ptr = data + 8;
    unsigned dataOffset = 8 + trackCount * 4;

    uint16_t syncWord;
    unsigned storage;

    for (unsigned i = 0; i < LIBAMI_MAX_TRACKS; i++) {
        Track& track = tracks[i];

        if (i < trackCount) {
            syncWord = (ptr[0] << 8) | ptr[1];
            storage = (ptr[2] << 8) | ptr[3];

            if ((dataOffset + storage) <= size) {

                if (!syncWord) { // ADF
                    unsigned bytes =  getTrackByteLength();
                    initTrack(track, bytes, bytes << 3, 0xaa);
                    if (storage >= (512 * 11))
                        encodeTrack(track, i, data + dataOffset);

                } else {
                    initTrack(track, storage + 2, (storage + 2) << 3, 0xaa);
                    std::memcpy(track.data + 2, data + dataOffset, storage);
                    Emulator::copyIntToBufferBigEndian<uint16_t>(&track.data[0], syncWord);
                }
                track.storage = storage;

            } else {
                initTrack(track, getTrackByteLength());
            }

            ptr += 4;
            dataOffset += storage;

        } else {
            initTrack(track, getTrackByteLength());
        }

        // if any track is written, we recreate the whole image as EXT2 for simplicity
        track.written = 0x80;
    }
}

}
