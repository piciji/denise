
#include <cstdint>
#include <vector>

struct LibSCP {

    ~LibSCP();

    uint8_t* data = nullptr;
    unsigned size = 0;

    unsigned trackCount = 0;
    unsigned revs = 0;

    struct Track {
        uint32_t index[5];
        uint16_t* raw = nullptr;
        int totalTicks;

        uint8_t rev;
        uint32_t offset;
        int accTicks;
        int clock;
        int clockCentre;
        int flux;
        unsigned clockedZeros;
        uint64_t latency;
        uint32_t nextIndex;
    };

    std::vector<Track> tracks;

    auto analyze(uint8_t* data, unsigned size) -> bool;
    auto getTrackCount() -> unsigned { return trackCount; }
    auto isMultiRev() -> bool { return revs > 1; }
    auto load(unsigned trackIndex, uint8_t*& trackData, uint16_t*& trackTiming, unsigned& trackLength) -> bool;
    auto loadFirstRev(unsigned trackIndex, uint8_t*& trackData, uint16_t*& trackTiming, unsigned& trackLength) -> bool;
    auto loadNextRev(unsigned trackIndex, uint8_t*& trackData, uint16_t*& trackTiming, unsigned& trackLength) -> bool;
    auto clear() -> void;

protected:
    auto scpNextFlux(Track& track) -> int;
    auto scpNextBit(Track& track) -> int;
};
