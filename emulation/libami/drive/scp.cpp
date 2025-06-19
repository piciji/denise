
namespace LIBAMI {

auto DiskStructure::analyzeSCP(uint8_t* data, unsigned size) -> bool {
    if (!libSCP.analyze(data, size))
        return false;

    trackCount = libSCP.getTrackCount();
    hd = false;
    type = Type::SCP;

    return true;
}

auto DiskStructure::prepareSCP(uint8_t* data, unsigned size) -> void {
    for (int t = 0; t < LIBAMI_MAX_TRACKS; t++) {
        Track& track = tracks[t];
        initTrack(track, 0x4000);

        if (!libSCP.load(t, track.data, track.cellWidth, track.bits))
            initTrack(track, getTrackByteLength());
        else {
            track.length = (track.bits + 7) / 8;
            track.options |= libSCP.isMultiRev() ? 2 : 0; // multi rev
        }
    }    
}

auto DiskStructure::loadFirstRevSCP(Track& track) -> void {
    if (libSCP.loadFirstRev(track.pos, track.data, track.cellWidth, track.bits))
        track.length = (track.bits + 7) / 8;
}

auto DiskStructure::loadNextRevSCP(Track& track) -> void {
    if (libSCP.loadNextRev(track.pos, track.data, track.cellWidth, track.bits))
        track.length = (track.bits + 7) / 8;
}

}
