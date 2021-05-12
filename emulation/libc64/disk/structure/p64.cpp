
#include "structure.h"
#include "../../../tools/crc32.h"

#define SamplesPerRotation 3200000

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

        std::vector<Emulator::PredictorEightBitWithPrefix*> predictorPositions;
        std::vector<Emulator::PredictorEightBitWithPrefix*> predictorStrengths;
        Emulator::PredictorOneBit predictorPositionEnable;
        Emulator::PredictorOneBit predictorStrengthEnable;
        Emulator::Fpaq0 fpaq0;

        uint8_t* ptr = rawData;
        uint8_t** _ptr = &ptr;
        unsigned offset = 0;
        uint32_t size = 0;
        uint32_t* pSize = &size;
        uint32_t checkSum;

        fpaq0.readIn = [_ptr, pSize](uint8_t*& buffer) {

            buffer = *_ptr;

            return *pSize;
        };

        ptr += 8; // header ident, already checked
        ptr += 4; // version: only 0 is known, don't check for it

        uint32_t flags = Emulator::copyBufferToInt<uint32_t>( ptr );
        // flag bit 0 is write protection, we ignore it and let the user decide

        sides = 1 + !!(flags & 2);

        // already checked
        ptr += 12;

        offset = 24;

        for(unsigned i = 0; i < 4; i++) {
            predictorPositions.push_back( new Emulator::PredictorEightBitWithPrefix );
            predictorStrengths.push_back( new Emulator::PredictorEightBitWithPrefix );
        }

        while(1) {
            offset += 12;
            if (offset >= rawSize)
                break;

            // signature
            ptr += 4;
            size = Emulator::copyBufferToInt<uint32_t>( ptr );
            ptr += 4;
            checkSum = Emulator::copyBufferToInt<uint32_t>( ptr );
            ptr += 4;

            offset += size;
            if (offset >= rawSize)
                break;

            if (size == 0) {
                if (checkSum == 0 && (std::memcmp( ptr - 12, "DONE", 4) == 0) )
                    break;
            } else {
                Emulator::CRC32 crc32( ptr, size, ~0 );

                if (crc32.value() != checkSum)
                    break;

                if (std::memcmp( ptr - 12, "HTP", 3) == 0) {
                    uint8_t halfTrack = *(ptr - 9);
                    uint8_t side = !!(halfTrack & 128);
                    halfTrack &= 127;

                    if (halfTrack > 85)
                        continue;

                    P64Track* trackPtr = &p64Tracks[side][halfTrack];
                    trackPtr->pulses.clear();
                    trackPtr->first = -1;
                    trackPtr->last = -1;
                    trackPtr->current = -1;

                    uint32_t pulses = Emulator::copyBufferToInt<uint32_t>( ptr );
                    ptr += 4;
                    size = Emulator::copyBufferToInt<uint32_t>( ptr );
                    ptr += 4;

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

                    for(unsigned i = 0; i < 4; i++) {
                        predictorPositions[i]->init();
                        predictorStrengths[i]->init();
                    }

                    fpaq0.init();
                    fpaq0.warmUp();

                    while( count < pulses ) {

                        if ( fpaq0.decode( &predictorPositionEnable ) ) {

                            deltaPosition = decodeP64( fpaq0, predictorPositions );

                            if (!deltaPosition)
                                break;
                        }
                        position += deltaPosition;

                        if ( fpaq0.decode( &predictorStrengthEnable ) )
                            strength += decodeP64( fpaq0, predictorStrengths );

                        addPulse( trackPtr, position, strength );

                        count++;
                    }

                    if (count != pulses)
                        break;
                }

                ptr += size;

            }
        }

        for(unsigned i = 0; i < 4; i++) {
            delete predictorPositions[i];
            delete predictorStrengths[i];
        }

        for (unsigned i = 0; i < (MAX_TRACKS_1541 * 2 + 1); i++ ) {
            system->interface->log("track", 1);
            system->interface->log( i, 0);

            P64Track* p64Track = &p64Tracks[0][i];

            for(auto& pulse : p64Track->pulses) {
                system->interface->log(pulse.position, 1, 0);
                system->interface->log(pulse.strength, 0, 0);
            }
        }
    }

    inline auto Structure1541::addPulse( P64Track* trackPtr, uint32_t position, uint32_t strength ) -> void {

        int current, index;
        while(position >= SamplesPerRotation)
            position -= SamplesPerRotation;

        current = trackPtr->current;
        if((trackPtr->last >= 0) && (trackPtr->pulses[trackPtr->last].position < position))
            current = -1;
        else {
            if((current < 0) || ((current != trackPtr->first) && ((trackPtr->pulses[current].previous >= 0) && (trackPtr->pulses[trackPtr->pulses[current].previous].position >= position))))
                current = trackPtr->first;

            while((current >= 0) && (trackPtr->pulses[current].position < position))
                current = trackPtr->pulses[current].next;
        }

        if(current < 0) {
            index = trackPtr->pulses.size();
            trackPtr->pulses.push_back( {0, 0, -1, -1} );

            if(trackPtr->last < 0)
                trackPtr->first = index;
            else {
                trackPtr->pulses[trackPtr->last].next = index;
                trackPtr->pulses[index].previous = trackPtr->last;
            }
            trackPtr->last = index;

        } else {
            if(trackPtr->pulses[current].position == position)
                index = current;
            else {
                index = trackPtr->pulses.size();
                trackPtr->pulses.push_back( {0, 0, -1, -1} );

                trackPtr->pulses[index].previous = trackPtr->pulses[current].previous;
                trackPtr->pulses[index].next = current;
                trackPtr->pulses[current].previous = index;
                if(trackPtr->pulses[index].previous < 0)
                    trackPtr->first = index;
                else
                    trackPtr->pulses[trackPtr->pulses[index].previous].next = index;
            }
        }

        trackPtr->pulses[index].position = position;
        trackPtr->pulses[index].strength = strength;
        trackPtr->current = index;
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
}