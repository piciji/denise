
#include "structure.h"
#include "../../../tools/crc32.h"

#define CyclesPerRevolution300Rpm 3200000

namespace LIBC64 {

    auto Structure1541::analyzeP64() -> bool {

        uint8_t* ptr = rawData;

        if (rawSize < 32)
            return false; // too small

        if (std::memcmp(rawData, "P64-1541", 8)) // missing this ident ?
            return false;

        ptr += 16;

        uint32_t size = Emulator::copyBufferToInt<uint32_t>( ptr );
        ptr += 4;
        uint32_t checkSum = Emulator::copyBufferToInt<uint32_t>( ptr );
        ptr += 4;

        if ((size + 24) > rawSize)
            return false;

        Emulator::CRC32 crc32( ptr, size, ~0 );

        if (crc32.value() != checkSum)
            return false;

        type = Type::P64;

        return true;
    }

    auto Structure1541::prepareP64() -> void {

        bool inUse[2][MAX_TRACKS_1541 * 2] = { {0}, {0} };
        bool* usePtr = &inUse[0][0];

        uint8_t* ptr = rawData;

        unsigned offset = 0;
        uint32_t size = 0;

        ptr += 8; // header ident, already checked
        ptr += 4; // version: only 0 is known, don't check for it

        uint32_t flags = Emulator::copyBufferToInt<uint32_t>( ptr );
        // flag bit 0 is write protection, we ignore it and let the user decide

        sides = 1 + !!(flags & 2);

        // already checked
        ptr += 12;

        offset = 24;

        const auto coreCount = std::thread::hardware_concurrency();
        std::vector<uint8_t*> jobs[coreCount];

        unsigned core = 0;
        while(1) {
            offset += 12;
            if (offset >= rawSize)
                break;

            jobs[core++].push_back( ptr );
            ptr += 4;
            size = Emulator::copyBufferToInt<uint32_t>( ptr );
            ptr += 4;
            ptr += 4;

            core %= coreCount;

            offset += size;
            if (offset >= rawSize)
                break;

            ptr += size;
        }

        std::vector<std::thread> threadPool;

        for(core = 0; core < coreCount; core++) {
            std::vector<uint8_t*>* workLoad = &jobs[core];

            threadPool.push_back(std::thread([this, workLoad, usePtr] {

                std::vector<Emulator::PredictorEightBitWithPrefix*> predictorPositions;
                std::vector<Emulator::PredictorEightBitWithPrefix*> predictorStrengths;
                Emulator::PredictorOneBit predictorPositionEnable;
                Emulator::PredictorOneBit predictorStrengthEnable;
                Emulator::Fpaq0 fpaq0;
                unsigned size, checkSum;
                uint8_t* _ptr;
                uint8_t** __ptr = &_ptr;
                uint32_t* pSize = &size;

                fpaq0.readIn = [__ptr, pSize](uint8_t*& buffer) {

                    buffer = *__ptr;

                    return *pSize;
                };

                for(unsigned i = 0; i < 4; i++) {
                    predictorPositions.push_back( new Emulator::PredictorEightBitWithPrefix );
                    predictorStrengths.push_back( new Emulator::PredictorEightBitWithPrefix );
                }

                for(auto ptr : *workLoad) {

                    _ptr = ptr;

                    // signature
                    _ptr += 4;
                    size = Emulator::copyBufferToInt<uint32_t>(_ptr);
                    _ptr += 4;
                    checkSum = Emulator::copyBufferToInt<uint32_t>(_ptr);
                    _ptr += 4;

                    if (size == 0) {
                        if (checkSum == 0 && (std::memcmp(_ptr - 12, "DONE", 4) == 0))
                            break;
                    } else {
                        Emulator::CRC32 crc32(_ptr, size, ~0);

                        if (crc32.value() != checkSum)
                            break;

                        if (std::memcmp(_ptr - 12, "HTP", 3) == 0) {
                            uint8_t halfTrack = *(_ptr - 9);
                            uint8_t side = !!(halfTrack & 128);
                            halfTrack &= 127;

                            if ((halfTrack < 2) || (halfTrack > 85))
                                continue;

                            halfTrack -= 2;

                            std::vector<Pulse> &trackPtr = p64Tracks[side][halfTrack];
                            trackPtr.clear();

                            uint32_t pulses = Emulator::copyBufferToInt<uint32_t>(_ptr);
                            _ptr += 4;
                            size = Emulator::copyBufferToInt<uint32_t>(_ptr);
                            _ptr += 4;

                            unsigned deltaPosition = 0;
                            unsigned strength = 0;
                            unsigned position = 0;

                            if (!size) {
                                if (!pulses)
                                    continue;
                                else
                                    break;
                            }

                            unsigned count = 0;

                            predictorPositionEnable.init(0);
                            predictorStrengthEnable.init(0);

                            for (unsigned i = 0; i < 4; i++) {
                                predictorPositions[i]->init();
                                predictorStrengths[i]->init();
                            }

                            fpaq0.init();
                            fpaq0.warmUp();

                            while (count < pulses) {

                                if (fpaq0.decode(&predictorPositionEnable)) {

                                    deltaPosition = decodeP64(fpaq0, predictorPositions);

                                    if (!deltaPosition)
                                        break;
                                }
                                position += deltaPosition;

                                if (fpaq0.decode(&predictorStrengthEnable))
                                    strength += decodeP64(fpaq0, predictorStrengths);

                                addPulse(trackPtr, position, strength);

                                count++;
                            }

                            if (count != pulses)
                                break;

                            encodeGCR(trackPtr, &gcrTracks[halfTrack], halfTrack);

                            usePtr[ (side == 1 ? (MAX_TRACKS_1541 * 2) : 0) + halfTrack ] = pulses > 0;
                        }
                    }
                }

                for(unsigned i = 0; i < 4; i++) {
                    delete predictorPositions[i];
                    delete predictorStrengths[i];
                }
            }));
        }

        for(auto& _t : threadPool)
            _t.join();

        prepareTracksNotInUse( usePtr );

//        for (unsigned i = 0; i < (MAX_TRACKS_1541 * 2 + 1); i++ ) {
//            system->interface->log("track", 1);
//            system->interface->log( i, 0);
//
//            std::vector<Pulse>& p64Track = p64Tracks[0][i];
//
//            for(auto& pulse : p64Track) {
//                system->interface->log(pulse.position, 1, 0);
//                system->interface->log(pulse.strength, 0, 0);
//            }
//        }
    }

    inline auto Structure1541::addPulse( std::vector<Pulse>& trackPtr, uint32_t position, uint32_t strength ) -> void {

        while(position >= CyclesPerRevolution300Rpm)
            position -= CyclesPerRevolution300Rpm;

        if (!trackPtr.size() || trackPtr.back().position < position)
            trackPtr.push_back({position, strength});
        else {
            unsigned index = 0;
            for(auto& pulse : trackPtr) {
                if (pulse.position == position)
                    break;

                else if (pulse.position > position) {
                    trackPtr.insert( trackPtr.begin() + index, {position, strength} );
                    break;
                }

                index++;
            }
        }
    }

    inline auto Structure1541::decodeP64( Emulator::Fpaq0& fpaq0, std::vector<Emulator::PredictorEightBitWithPrefix*>& predictors ) -> unsigned {

        uint32_t result = 0;

        for(int i = 0; i < 4; i++) {

            uint8_t byte = 0;

            for (int bit = 7; bit >= 0; bit--)
                byte = (byte << 1) | fpaq0.decode(predictors[i]);

            predictors[i]->prefix = byte;

            result |= byte << (i << 3);
        }

        return result;
    }

    auto Structure1541::prepareTracksNotInUse(bool* inUse) -> void {

        for (int side = 0; side < sides; side++) {

            for (int halfTrack = 0; halfTrack < (MAX_TRACKS_1541 * 2); halfTrack++) {

                if ( !*inUse) {
                    GcrTrack *gcrPtr = &gcrTracks[halfTrack];

                    if (gcrPtr->data)
                        delete[] gcrPtr->data;

                    gcrPtr->size = countBytes((halfTrack + 2) / 2); // standard length
                    gcrPtr->bits = gcrPtr->size << 3;
                    gcrPtr->data = new uint8_t[gcrPtr->size];
                    std::memset(gcrPtr->data, 0x55, gcrPtr->size);

                    std::vector<Pulse> &pulsePtr = p64Tracks[0][halfTrack];
                    pulsePtr.clear();

                    // set 0x55 pattern
                    unsigned fluxDelta = CyclesPerRevolution300Rpm / (gcrPtr->bits / 2);

                    unsigned pos = fluxDelta / 2;
                    for (unsigned i = 0; i < (gcrPtr->bits / 2); i++) {
                        pulsePtr.push_back({pos, 0xffffffff});

                        pos += fluxDelta;
                        if (pos >= CyclesPerRevolution300Rpm)
                            break;
                    }
                }
                inUse++;
            }
        }
    }

    inline auto Structure1541::encodeGCR(std::vector<Pulse>& pulses, GcrTrack* gcrTrack, uint8_t halfTrack) -> void {
        uint8_t track = (halfTrack >> 1) + 1;
        unsigned trackSize = countBytes( track );
        uint8_t _speedzone = speedzone( track );

        uint32_t lastPosition = 0;
        uint32_t delta;
        bool flipFlop = false;
        bool lastFlipFlop = false;
        unsigned delay;
        uint8_t ue7Counter;
        uint8_t uf4Counter;
        unsigned bits = 0;

        if ( !gcrTrack->data )
            gcrTrack->data = new uint8_t[ trackSize ];

        else if ( trackSize != gcrTrack->size ) {
            delete[] gcrTrack->data;
            gcrTrack->data = new uint8_t[ trackSize ];
        }

        gcrTrack->size = trackSize;
        gcrTrack->bits = trackSize * 8;
        uint8_t* ptr = gcrTrack->data;
        std::memset( ptr, 0, trackSize );

        for(auto& pulse : pulses) {

            if (pulse.strength < 0x80000000)
                continue;

            delta = pulse.position - lastPosition;

            lastPosition = pulse.position;

            flipFlop ^= 1;

            delay = 0;

            do {
                // 2.5 us filters out too short pulses
                if((delay == 40) && (lastFlipFlop != flipFlop)) {
                    lastFlipFlop = flipFlop;
                    ue7Counter = _speedzone;
                    uf4Counter = 0;
                }

                if(ue7Counter == 16) {
                    ue7Counter = _speedzone;
                    uf4Counter = (uf4Counter + 1) & 0xf;

                    if((uf4Counter & 3) == 2) {
                        if (uf4Counter == 2)
                            ptr[bits >> 3] |= 1 << (~bits & 7);

                        bits++;

                        if (bits == gcrTrack->bits)
                            return;
                    }
                }

                ue7Counter++;
            } while(++delay < delta);
        }
    }
}