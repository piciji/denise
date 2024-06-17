
#include "ipf/CommonTypes.h"
#include "ipf/CapsAPI.h"

#define CAPS_FLAGS (DI_LOCK_DENVAR | DI_LOCK_DENNOISE | DI_LOCK_NOISE | DI_LOCK_UPDATEFD | DI_LOCK_TYPE | DI_LOCK_OVLBIT | DI_LOCK_TRKBIT)

#define DLPTR(arg) dlLoader.getPtr(arg)

#ifdef _WIN32
#define PROCCALL __cdecl
#else
#define PROCCALL
#endif

namespace LIBAMI {

typedef int (PROCCALL* pInit)();
typedef int (PROCCALL* pExit)();
typedef int (PROCCALL* pAddImage)();
typedef int (PROCCALL* pRemImage)(int);
typedef int (PROCCALL* pGetVersionInfo)(void*, unsigned);
typedef int (PROCCALL* pLockImageMemory)(int, uint8_t*, unsigned, unsigned);
typedef int (PROCCALL* pGetImageInfo)(void*, int);
typedef int (PROCCALL* pLoadImage)(int, unsigned);
typedef int (PROCCALL* pLockTrack)(void*, int, unsigned, unsigned, unsigned);
typedef int (PROCCALL* pUnlockAllTracks)(int);
typedef int (PROCCALL* pUnlockImage)(int);

Emulator::DLLoader DiskStructure::dlLoader;

auto DiskStructure::initIPF() -> bool {
    if (!dlLoader.hasOpened()) {

#if defined(_WIN32)
        std::string plugins[] = {"CAPSImg", "CAPSImg_x64"};
#elif defined( __APPLE__ )
        std::string plugins[] = {"CAPSImage.framework/CAPSImage","CAPSImg.framework/CAPSImg", "libcapsimage.5.dylib", "libcapsimage.4.dylib","/Library/Frameworks/CAPSImage.framework/CAPSImage","/Library/Frameworks/CAPSImg.framework/CAPSImg", "/usr/local/lib/libcapsimage.5.dylib", "/usr/local/lib/libcapsimage.4.dylib", "/usr/local/lib/libcapsimage.dylib"};
#else
        std::string plugins[] = {"/usr/local/lib/libcapsimage.so.5", "libcapsimage.so.5", "/usr/local/lib/libcapsimage.so.4", "libcapsimage.so.4"};
#endif
        
        for (auto& plugin: plugins) {
            dlLoader.setPath(plugin);
            if (dlLoader.open())
                break;
        }
        
        if (!dlLoader.hasOpened())
            return false;
    }

    if (!dlLoader.markInitialized) {
        std::string calls[CAPS_METHOD_COUNT];
        calls[CAPSInit] = "CAPSInit";
        calls[CAPSExit] = "CAPSExit";
        calls[CAPSAddImage] = "CAPSAddImage";
        calls[CAPSRemImage] = "CAPSRemImage";
        calls[CAPSLockImageMemory] = "CAPSLockImageMemory";
        calls[CAPSUnlockImage] = "CAPSUnlockImage";
        calls[CAPSLoadImage] = "CAPSLoadImage";
        calls[CAPSGetVersionInfo] = "CAPSGetVersionInfo";
        calls[CAPSGetImageInfo] = "CAPSGetImageInfo";
        calls[CAPSLockTrack] = "CAPSLockTrack";
        calls[CAPSUnlockAllTracks] = "CAPSUnlockAllTracks";
        int id = 0;
        for (auto& call: calls) {
            if (!dlLoader.load(id++, call))
                return false;
        }
        
        if (pInit(DLPTR(CAPSInit))() != imgeOk)
            return false;
        
        dlLoader.markInitialized = true;
    }

    CapsVersionInfo vi;
    void* _vi = reinterpret_cast<void*>(&vi);
    
    if (pGetVersionInfo(DLPTR(CAPSGetVersionInfo))(_vi, 0) == imgeOk) {
        if ( (vi.release < 4) || (vi.release == 4 && vi.revision < 2))
            return false;
        if ((vi.flag & (DI_LOCK_TRKBIT | DI_LOCK_OVLBIT)) != (DI_LOCK_TRKBIT | DI_LOCK_OVLBIT))
            return false;

        return true;
    }

    return false;
}

auto DiskStructure::removeIPF() -> void {
    if (capsImageId >= 0) {
        // not needed CPASExit deletes all images
        // fixme: crash on Apple ARM because of destruction order "destroyIPF" calls first
        // pRemImage(DLPTR(CAPSRemImage))(capsImageId);
        capsImageId = -1;
    }
}

auto DiskStructure::destroyIPF() -> void {
    
    if (dlLoader.hasOpened()) {
        void* ptr = DLPTR(CAPSExit);
        if (ptr)
            pExit(DLPTR(CAPSExit))();
        
        dlLoader.close();
    }
}

auto DiskStructure::analyzeIPF(uint8_t* data, unsigned size) -> bool {
    if (std::memcmp(data, "CAPS", 4))
        return false;

    if (!initIPF()) {
        agnus.interface->libraryMissing("CAPS");
        return false;
    }

    if (capsImageId < 0) {
        capsImageId = pAddImage(DLPTR(CAPSAddImage))();
        if (capsImageId < 0)
            return false;
    }

    if(pLockImageMemory(DLPTR(CAPSLockImageMemory))(capsImageId, data, size, DI_LOCK_MEMREF) != imgeOk)
        return false;

    CapsImageInfo ii;
    void* _ii = reinterpret_cast<void*>(&ii);

    if(pGetImageInfo(DLPTR(CAPSGetImageInfo))(_ii, capsImageId) != imgeOk)
        return false;

    if (pLoadImage(DLPTR(CAPSLoadImage))(capsImageId, CAPS_FLAGS) != imgeOk)
        return false;
    
    trackCount = (ii.maxcylinder - ii.mincylinder + 1) * (ii.maxhead - ii.minhead + 1);
    if (trackCount > LIBAMI_MAX_TRACKS)
        trackCount = LIBAMI_MAX_TRACKS;

    hd = false;
    type = Type::IPF;

    return true;
}

auto DiskStructure::prepareIPF(uint8_t* data, unsigned size) -> void {
    
    for(int t = 0; t < LIBAMI_MAX_TRACKS; t++) {
        Track& track = tracks[t];

        if (t < trackCount) {
            CapsTrackInfoT2 ti;
            ti.type = 2;
            int flags = CAPS_FLAGS | DI_LOCK_SETWSEED;
            ti.wseed = rand();
            void* _ti = reinterpret_cast<void*>(&ti);

            if (pLockTrack(DLPTR(CAPSLockTrack))(_ti, capsImageId, t / 2, t & 1, flags) == imgeOk) {

                unsigned length = (ti.tracklen + 7) / 8;
                // We create the buffer at least in the size for standard bitcell timing, because this size is needed for any writes.
                initTrack(track, std::max(length, getTrackByteLength()), ti.tracklen);
                std::memcpy(track.data, ti.trackbuf, length);
                track.length = length;

                if (ti.timebuf)
                    addTimingIPF(track, ti.timelen, ti.timebuf);

                track.options |= (ti.type & CTIT_FLAG_FLAKEY) ? 2 : 0;

                //agnus.interface->log(ti.overlap);
                //agnus.interface->log(ti.type & CTIT_FLAG_FLAKEY ? 1 : 0, 0);

            } else
                initTrack(track, getTrackByteLength(), 0, 0xaa);
        } else
            initTrack(track, getTrackByteLength());
    }
}

auto DiskStructure::loadNextRevIPF(Track& track) -> void {
    if ((type != DiskStructure::IPF) || (track.options & 1) || ((track.options & 2) == 0))
        return;

    CapsTrackInfoT2 ti;
    ti.type = 2;

    void* _ti = reinterpret_cast<void*>(&ti);
    
    if(pLockTrack(DLPTR(CAPSLockTrack))(_ti, capsImageId, track.pos / 2, track.pos & 1, CAPS_FLAGS) == imgeOk) {
        // if mutli REV, next revolution will be fetched to emulate weak bits (oscillation)
        // note: get next REV only, if DI_LOCK_NOUPDATE is not set
        deleteTimingIPF(track);
        initTrack(track, (ti.tracklen + 7) / 8, ti.tracklen);
        std::memcpy(track.data, ti.trackbuf, track.length);

        if (ti.timebuf)
            addTimingIPF(track, ti.timelen, ti.timebuf);

        track.options |= (ti.type & CTIT_FLAG_FLAKEY) ? 2 : 0;
    }
}

auto DiskStructure::deleteTimingIPF(Track& track) -> void {
    if (track.cellWidth) {
        delete[] track.cellWidth;
        track.cellWidth = nullptr;
    }
}

auto DiskStructure::addTimingIPF(Track& track, unsigned tiLength, uint32_t* tiData) -> void {
    if (tiLength > track.length)
        tiLength = track.length; // should never happen

    track.cellWidth = new uint16_t[track.length];
    std::fill_n(track.cellWidth, track.length, 1000);
    for(int i = 0; i < tiLength; i++)
        track.cellWidth[i] = (uint16_t)tiData[i];
}

auto DiskStructure::unloadIPF() -> void {

    if (capsImageId >= 0) {
        pUnlockAllTracks(DLPTR(CAPSUnlockAllTracks))(capsImageId);
        pUnlockImage(DLPTR(CAPSUnlockImage))(capsImageId);
    }
}

}
