
/*
 *
 * Support for reading .SCP (Supercard Pro) disk flux dumps.
 *
 * By Keir Fraser in 2014.
 *
 * This file is free and unencumbered software released into the public domain.
 * For more information, please refer to <http://unlicense.org/>
 */

#include "scp.h"
#include "../../../tools/buffer.h"

#define SCP_CLOCK_CENTRE  2000   /* 2000ns = 2us */
#define SCP_CLOCK_MAX_ADJ 10     /* +/- 10% adjustment */
#define SCP_CLOCK_MIN(_c) (((_c) * (100 - SCP_CLOCK_MAX_ADJ)) / 100)
#define SCP_CLOCK_MAX(_c) (((_c) * (100 + SCP_CLOCK_MAX_ADJ)) / 100)

LibSCP::~LibSCP() {
    clear();
}

auto LibSCP::clear() -> void {
    for(auto& track : tracks) {
        if (track.raw) {
            delete[] track.raw;
            track.raw = nullptr;
        }
    }
    revs = 0;
    trackCount = 0;
    data = nullptr;
    size = 0;
}

auto LibSCP::analyze(uint8_t* data, unsigned size) -> bool {
    clear();

    if (size < 16) // at least header size
        return false;

    if (std::memcmp(data, "SCP", 3) != 0)
        return false;

    if (data[5] == 0) // revolution count
        return false;

    if (data[9] != 0 && data[9] != 16) // unsupported bit cell width
        return false;

    this->data = data;
    this->size = size;
    trackCount = data[7] + 1;
    revs = std::min((int)data[5], 5);
    tracks.resize(trackCount);

    return true;
}

auto LibSCP::load(unsigned trackIndex, uint8_t*& trackData, uint16_t*& trackTiming, unsigned& trackLength) -> bool {
    unsigned tdhOffset;

    if (!revs)
        return false;

    if (trackIndex >= trackCount)
        return false;

    Track& track = tracks[trackIndex];

    if (track.raw) {
        delete[] track.raw;
        track.raw = nullptr;
    }

    tdhOffset = ToU32LE(data + (0x10 + trackIndex * 4));

    if (!tdhOffset)
        return false;

    if (std::memcmp(data + tdhOffset, "TRK", 3) != 0)
        return false;

    if (*(data + tdhOffset + 3) != trackIndex)
        return false; // track data header contains unexpected track number

    unsigned fluxSize = 0;
    uint32_t trackOffset[5];
    track.totalTicks = 0;

    for (int rev = 0; rev < revs; rev++) {
        unsigned _offset = tdhOffset + rev * 12 + 4;

        trackOffset[rev] = tdhOffset + ToU32LE(data + _offset + 8);
        track.index[rev] = ToU32LE(data + _offset + 4);
        track.totalTicks += ToU32LE(data + _offset);

        fluxSize += track.index[rev];
    }

    track.raw = new uint16_t[fluxSize];

    unsigned _offset = 0;
    for (int rev = 0; rev < revs; rev++) {
        std::memcpy(track.raw + _offset, data + trackOffset[rev], track.index[rev] * 2);
        _offset += track.index[rev];
        track.index[rev] = _offset;
    }

    return loadFirstRev(trackIndex, trackData, trackTiming, trackLength);
}

auto LibSCP::loadFirstRev(unsigned trackIndex, uint8_t*& trackData, uint16_t*& trackTiming, unsigned& trackLength) -> bool {
    if (trackIndex >= trackCount)
        return false;
        
    Track& track = tracks[trackIndex];
    track.nextIndex = track.index[0];
    track.offset = 0;
    track.rev = 0;
    track.accTicks = 0;
    track.clockedZeros = 0;
    track.clock = track.clockCentre = SCP_CLOCK_CENTRE;
    track.flux = 0;

    return loadNextRev(trackIndex, trackData, trackTiming, trackLength);
}

auto LibSCP::loadNextRev(unsigned trackIndex, uint8_t*& trackData, uint16_t*& trackTiming, unsigned& trackLength) -> bool {
    if (trackIndex >= trackCount)
        return false;

    if (!revs)
        return false;

    Track& track = tracks[trackIndex];

    if (!track.raw)
        return false;

    uint64_t prevLatency;
    uint32_t avLatency;
    unsigned int i, j;
    int b;

    if (!trackData)
        trackData = new uint8_t[0x4000];

    if (!trackTiming)
        trackTiming = new uint16_t[0x4000];

    track.latency = prevLatency = 0;
    for (i = 0; (b = scpNextBit(track)) != -1; i++) {
        if (i == (0x4000 << 3))
            break;

        if ((i & 7) == 0)
            trackData[i >> 3] = 0;

        if (b)
            trackData[i >> 3] |= 0x80 >> (i & 7);

        if ((i & 7) == 7) {
            trackTiming[i >> 3] = (uint16_t)(track.latency - prevLatency);
            prevLatency = track.latency;
        }
    }

    if (i & 7)
        trackTiming[i >> 3] = (uint16_t)(((track.latency - prevLatency) * 8) / (i & 7));

    avLatency = (uint32_t)(prevLatency / (i >> 3));
    for (j = 0; j < (i + 7) >> 3; j++)
        trackTiming[j] = ((uint32_t)trackTiming[j] * 1000u) / avLatency;

    trackLength = i;

    return true;
}

auto LibSCP::scpNextFlux(Track& track) -> int {
    uint32_t val = 0;
    uint16_t t;

    if (track.rev == revs) {
        track.rev = track.offset = 0;
        val = track.totalTicks - track.accTicks;
        track.accTicks = 0 - val;
    }

    for (;;) {
        if (track.offset >= track.nextIndex) {
            track.nextIndex = track.index[++track.rev % revs];
            return -1;
        }

        t = track.raw[track.offset++];
        t = (t << 8) | (t >> 8);

        if (t == 0) { /* overflow */
            val += 0x10000;
            continue;
        }

        val += (uint32_t)t;
        break;
    }

    track.accTicks += val;

    return val * 25u; // 25ns per bitcell
}

auto LibSCP::scpNextBit(Track& track) -> int {
    int newFlux;

    while (track.flux < (track.clock / 2)) {
        if ((newFlux = scpNextFlux(track)) == -1)
            return -1;
        track.flux += newFlux;
        track.clockedZeros = 0;
    }

    track.latency += track.clock;
    track.flux -= track.clock;

    if (track.flux >= (track.clock / 2)) {
        track.clockedZeros++;
        return 0;
    }

    /* PLL: Adjust clock frequency according to phase mismatch. */
    if ((track.clockedZeros >= 1) && (track.clockedZeros <= 3)) {
        /* In sync: adjust base clock by 10% of phase mismatch. */
        int diff = track.flux / (int)(track.clockedZeros + 1);
        track.clock += diff / 10;
    } else {
        /* Out of sync: adjust base clock towards centre. */
        track.clock += (track.clockCentre - track.clock) / 10;
    }

    /* Clamp the clock's adjustment range. */
    track.clock = std::max(SCP_CLOCK_MIN(track.clockCentre),
        std::min(SCP_CLOCK_MAX(track.clockCentre), track.clock));

    /* Authentic PLL: Do not snap the timing window to each flux transition. */
    newFlux = track.flux / 2;

    track.latency += track.flux - newFlux;
    track.flux = newFlux;

    return 1;
}
