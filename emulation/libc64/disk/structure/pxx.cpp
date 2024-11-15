
// altered version of P64 from BeRo
// separated Matt Mahoney's fpaq0 algorithm to be used for other tasks.
// Tracks are decoded parallel to each other

/*
*************************************************************
** P64 reference implementation by Benjamin 'BeRo' Rosseaux *
*************************************************************
**
** Copyright (c) 2011-2012, Benjamin Rosseaux
**
** This software is provided 'as-is', without any express or implied
** warranty. In no event will the authors be held liable for any damages
** arising from the use of this software.
**
** Permission is granted to anyone to use this software for any purpose,
** including commercial applications, and to alter it and redistribute it
** freely, subject to the following restrictions:
**
**    1. The origin of this software must not be misrepresented; you must not
**    claim that you wrote the original software. If you use this software
**    in a product, an acknowledgment in the product documentation would be
**    appreciated but is not required.
**
**    2. Altered source versions must be plainly marked as such, and must not be
**    misrepresented as being the original software.
**
**    3. This notice may not be removed or altered from any source
**   distribution.
**
*/

#include "structure.h"
#include "../../../tools/crc32.h"

#define CyclesPerRevolution300Rpm 3200000

namespace LIBC64 {

    auto DiskStructure::analyzePxx(const std::string& ident, Type newType ) -> bool {

        uint8_t* ptr = rawData;

        if (rawSize < 32)
            return false; // too small

        if (std::memcmp(rawData, ident.c_str(), 8)) // missing this ident ?
            return false;

        ptr += 12;
        uint32_t flags = Emulator::copyBufferToInt<uint32_t>( ptr );
        // flag bit 0 is write protection, we ignore it and let the user decide
        sides = 1 + !!(flags & 2);

        ptr += 4;
        uint32_t size = Emulator::copyBufferToInt<uint32_t>( ptr );
        ptr += 4;
        uint32_t checkSum = Emulator::copyBufferToInt<uint32_t>( ptr );
        ptr += 4;

        if ((size + 24) > rawSize)
            return false;

        Emulator::CRC32 crc32( ptr, size, ~0 );

        if (crc32.value() != checkSum)
            return false;

        type = newType;

        return true;
    }

    auto DiskStructure::writeP64ToMem(unsigned& memSize) -> uint8_t* {

#define BOUND_CHECK(length) \
    if ( (*pOffset + length) > *pMaxSize) { \
        do {                 \
            *pMaxSize <<= 1;   \
        } while( (*pOffset + length) > *pMaxSize );                 \
        uint8_t* newBuf = new uint8_t[*pMaxSize];                \
        std::memset(newBuf, 0, *pMaxSize);                \
        std::memcpy( newBuf, *__buf, *pOffset );                   \
        delete[] *__buf;   \
        *__buf = newBuf;                        \
    }

        uint32_t offset = 0;
        uint32_t* pOffset = &offset;
        uint32_t maxSize = 400 * 1024;
        uint32_t* pMaxSize = &maxSize;

        uint8_t* buf = new uint8_t[maxSize];
        uint8_t** __buf = &buf;

        std::memset( buf, 0, maxSize );

        // don't change P64-1541 in P64-1571 in case of two sides, keep existing ident
        std::memcpy( buf, rawData, 8 );
        offset += 8;
        // version = 0
        offset += 4;

        buf[offset] = sides == 2 ? 2 : 0;
        offset += 4;

        // size, checksum
        offset += 8;
        std::vector<Emulator::PredictorEightBitWithPrefix*> predictorPositions;
        std::vector<Emulator::PredictorEightBitWithPrefix*> predictorStrengths;
        Emulator::PredictorOneBit predictorPositionEnable;
        Emulator::PredictorOneBit predictorStrengthEnable;
        Emulator::Fpaq0 fpaq0;

        for(unsigned i = 0; i < 4; i++) {
            predictorPositions.push_back( new Emulator::PredictorEightBitWithPrefix );
            predictorStrengths.push_back( new Emulator::PredictorEightBitWithPrefix );
        }

        fpaq0.writeOut = [this, pOffset, pMaxSize, __buf](uint8_t* buffer, unsigned length) {

            BOUND_CHECK(length)
            std::memcpy(*__buf + *pOffset, buffer, length);
            *pOffset += length;
        };

        for (int side = 0; side < sides; side++) {
            for (unsigned halfTrack = 0; halfTrack < (MAX_TRACKS * 2); halfTrack++) {
                MTrack* gcrTrack = getTrackPtr(side, halfTrack);

                BOUND_CHECK(20)

                // beginning from here we need to access 'buf' from double pointer, because 'BOUND_CHECK' could recreate the memory area
                *(*__buf + offset + 0) = 'H';
                *(*__buf + offset + 1) = 'T';
                *(*__buf + offset + 2) = 'P';
                if (side == 1)
                    *(*__buf + offset + 3) = 0x80;

                *(*__buf + offset + 3) |= (halfTrack + 2);
                offset += 12;

                unsigned chunkOffset = offset;
                offset += 8;

                predictorPositionEnable.init(0);
                predictorStrengthEnable.init(0);

                for (unsigned i = 0; i < 4; i++) {
                    predictorPositions[i]->init();
                    predictorStrengths[i]->init();
                }

                fpaq0.init();

                unsigned lastPosition = 0;
                unsigned lastDelta = 0;
                unsigned lastStrength = 0;

                unsigned countPulses = 0;

                if (gcrTrack->written) {
                    int32_t index = gcrTrack->firstPulse;

                    while (index >= 0) {
                        DiskStructure::Pulse& pulse = gcrTrack->pulses[index];

                        unsigned delta = pulse.position - lastPosition;

                        if (delta != lastDelta) {

                            lastDelta = delta;

                            fpaq0.encode(&predictorPositionEnable, true);

                            encodeP64(fpaq0, predictorPositions, delta);

                        } else
                            fpaq0.encode(&predictorPositionEnable, false);

                        lastPosition = pulse.position;

                        if (lastStrength != pulse.strength) {

                            fpaq0.encode(&predictorStrengthEnable, true);

                            encodeP64(fpaq0, predictorStrengths, pulse.strength - lastStrength);
                        } else
                            fpaq0.encode(&predictorStrengthEnable, false);

                        lastStrength = pulse.strength;

                        index = pulse.next;

                        countPulses++;
                    }
                }

                fpaq0.encode(&predictorPositionEnable, true);
                encodeP64(fpaq0, predictorPositions, 0);
                fpaq0.flush();


                Emulator::copyIntToBuffer<uint32_t>(*__buf + chunkOffset, countPulses);

                unsigned chunkSize = offset - chunkOffset;

                // encoded data size
                Emulator::copyIntToBuffer<uint32_t>(*__buf + chunkOffset + 4, chunkSize - 8);

                // chunk size
                Emulator::copyIntToBuffer<uint32_t>(*__buf + chunkOffset - 8, chunkSize);

                Emulator::CRC32 crc32(*__buf + chunkOffset, chunkSize, ~0);
                Emulator::copyIntToBuffer<uint32_t>(*__buf + chunkOffset - 4, crc32.value());
            }
        }

        BOUND_CHECK(12)

        *(*__buf + offset + 0) = 'D';
        *(*__buf + offset + 1) = 'O';
        *(*__buf + offset + 2) = 'N';
        *(*__buf + offset + 3) = 'E';
        offset += 4;

        Emulator::copyIntToBuffer<uint32_t>(*__buf + offset, 0);
        offset += 4;
        Emulator::copyIntToBuffer<uint32_t>(*__buf + offset, 0);
        offset += 4;

        Emulator::CRC32 crc32( *__buf + 24, offset - 24, ~0);
        Emulator::copyIntToBuffer<uint32_t>(*__buf + 16, offset - 24);
        Emulator::copyIntToBuffer<uint32_t>(*__buf + 20, crc32.value());

        for(unsigned i = 0; i < 4; i++) {
            delete predictorPositions[i];
            delete predictorStrengths[i];
        }

        memSize = offset;

        return *__buf;
    }

    auto DiskStructure::writePxx() -> bool {

        unsigned memSize = 0;

        uint8_t* temp = writeP64ToMem(memSize);

        if (!temp || !memSize)
            return false;

        system->interface->truncateMedia( media );

        bool result = false;
        if (write(temp, memSize, 0) == memSize)
            result = true;

        delete[] temp;

        return result;
    }

    auto DiskStructure::decodeJob( std::vector<uint8_t*>* workLoad, bool* usePtr ) -> bool {

        std::vector<Emulator::PredictorEightBitWithPrefix*> predictorPositions;
        std::vector<Emulator::PredictorEightBitWithPrefix*> predictorStrengths;
        Emulator::PredictorOneBit predictorPositionEnable;
        Emulator::PredictorOneBit predictorStrengthEnable;
        Emulator::Fpaq0 fpaq0;
        unsigned size, checkSum;
        uint8_t* _ptr;
        uint8_t** __ptr = &_ptr;
        uint32_t* pSize = &size;
        bool res = false;

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

                    if ( (halfTrack < 2) || (halfTrack > 85))
                        continue;

                    halfTrack -= 2;

                    MTrack* gcrPtr = &mTracks[side][halfTrack];
                    gcrPtr->pulses.clear();

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

                        addPulse(gcrPtr, position, strength);

                        count++;
                    }

                    if (count != pulses)
                        break;

                    if (type == Type::P81)
                        encodeMfmFromPulse(gcrPtr);
                    else
                        encodeGCRFromPulse(gcrPtr, halfTrack);

                    usePtr[ (side == 1 ? (MAX_TRACKS_1541 * 2) : 0) + halfTrack ] = pulses > 0;

                    res = true;
                }
            }
        }

        for(unsigned i = 0; i < 4; i++) {
            delete predictorPositions[i];
            delete predictorStrengths[i];
        }

        return res;
    }

    auto DiskStructure::preparePxx() -> void {

        bool inUse[2][MAX_TRACKS_1541 * 2] = { {0}, {0} };
        bool* usePtr = &inUse[0][0];

        uint8_t* ptr = rawData;

        unsigned offset = 0;
        uint32_t size = 0;

        ptr += 8; // header ident, already checked
        ptr += 4; // version: only 0 is known, don't check for it

        // already checked
        ptr += 12;

        offset = 24;

        uint8_t coreCount = std::thread::hardware_concurrency();
        coreCount = (coreCount < 4) ? 1 : coreCount >> 1;
        if (coreCount > 8)
            coreCount = 8;

        std::vector<uint8_t*> jobs[8];

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

            if (core == coreCount)
                core = 0;

            offset += size;
            if (offset >= rawSize)
                break;

            ptr += size;
        }

        if (coreCount == 1) {
            this->decodeJob( &jobs[0], usePtr );
        } else {
            std::vector<std::thread> threadPool;
            for (core = 0; core < coreCount; core++) {
                std::vector<uint8_t*> *workLoad = &jobs[core];

                threadPool.push_back(std::thread([this, workLoad, usePtr] {

                    this->decodeJob(workLoad, usePtr);
                }));
            }

            for (auto &_t : threadPool)
                _t.join();
        }

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

    inline auto DiskStructure::allocatePulse( std::vector<Pulse>& pulses ) -> unsigned {

        unsigned capacity = pulses.capacity();
        unsigned size = pulses.size();

        if (size == capacity) {

            if (capacity == 0)
                capacity = 128;

            capacity <<= 1;

            // to prevent reallocation for each single pulse
            pulses.reserve(capacity);
        }

        return size;
    }

    auto DiskStructure::freePulse( MTrack* gcrTrack, int32_t index ) -> void {
        DiskStructure::Pulse& pulse = gcrTrack->pulses[index];

        if (gcrTrack->currentPulse == index)
            gcrTrack->currentPulse = pulse.next;

        if (pulse.previous < 0)
            gcrTrack->firstPulse = pulse.next;
        else
            gcrTrack->pulses[pulse.previous].next = pulse.next;

        if (pulse.next < 0)
            gcrTrack->lastPulse = pulse.previous;
        else
            gcrTrack->pulses[pulse.next].previous = pulse.previous;
    }

    auto DiskStructure::addPulse( MTrack* gcrTrack, uint32_t position, uint32_t strength ) -> void {
        // use double linked list for faster write emulation
        // have tried a simple sorted vector but inserting new elements during write emulation completly kill performance

        int32_t currentPulse = gcrTrack->currentPulse;
        int32_t index;

        while(position >= CyclesPerRevolution300Rpm)
            position -= CyclesPerRevolution300Rpm;

        if((gcrTrack->lastPulse >= 0) && (gcrTrack->pulses[gcrTrack->lastPulse].position < position)) {
            currentPulse = -1;

        } else {

            if ((currentPulse < 0) ||
                ((currentPulse != gcrTrack->firstPulse) && (gcrTrack->pulses[currentPulse].position >= position)))
                currentPulse = gcrTrack->firstPulse;

            while ((currentPulse >= 0) && (gcrTrack->pulses[currentPulse].position < position))
                currentPulse = gcrTrack->pulses[currentPulse].next;
        }

        if (currentPulse < 0) {

            index = allocatePulse( gcrTrack->pulses );
            gcrTrack->pulses.push_back({position, strength, gcrTrack->lastPulse, -1});

            if (gcrTrack->lastPulse < 0)
                gcrTrack->firstPulse = index;
            else
                gcrTrack->pulses[gcrTrack->lastPulse].next = index;

            gcrTrack->lastPulse = index;

        } else {
            if (gcrTrack->pulses[currentPulse].position == position) {
                gcrTrack->pulses[currentPulse].strength = strength;
                index = currentPulse;
            } else {
                index = allocatePulse(gcrTrack->pulses);

                gcrTrack->pulses.push_back({position, strength, gcrTrack->pulses[currentPulse].previous, currentPulse});

                gcrTrack->pulses[currentPulse].previous = index;

                if (gcrTrack->pulses[index].previous < 0)
                    gcrTrack->firstPulse = index;
                else
                    gcrTrack->pulses[gcrTrack->pulses[index].previous].next = index;
            }
        }

        gcrTrack->currentPulse = index;
    }

    inline auto DiskStructure::decodeP64( Emulator::Fpaq0& fpaq0, std::vector<Emulator::PredictorEightBitWithPrefix*>& predictors ) -> unsigned {

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

    inline auto DiskStructure::encodeP64( Emulator::Fpaq0& fpaq0, std::vector<Emulator::PredictorEightBitWithPrefix*>& predictors, unsigned value ) -> void {

        for(int i = 0; i < 4; i++) {

            uint8_t byte = (value >> (i << 3)) & 0xff;

            for (int bit = 7; bit >= 0; bit--)
                fpaq0.encode(predictors[i], (byte >> bit) & 1 );

            predictors[i]->prefix = byte;
        }
    }

    auto DiskStructure::prepareTracksNotInUse(bool* inUse) -> void {

        for (int side = 0; side < sides; side++) {
            for (int halfTrack = 0; halfTrack < (MAX_TRACKS_1541 * 2); halfTrack++) {

                if ( !*inUse) {
                    MTrack* gcrPtr = &mTracks[side][halfTrack];

                    if (gcrPtr->data)
                        delete[] gcrPtr->data;

                    gcrPtr->size = countBytes((halfTrack + 2) / 2); // standard length
                    gcrPtr->bits = gcrPtr->size << 3;
                    gcrPtr->data = new uint8_t[gcrPtr->size];
                    gcrPtr->written = 0;
                    std::memset(gcrPtr->data, 0x55, gcrPtr->size);

                    gcrPtr->pulses.clear();

                    createPulsesFromEncoded( gcrPtr );
                }
                inUse++;
            }
        }
    }

    auto DiskStructure::createPulsesFromEncoded( MTrack* gcrTrack ) -> void {
        uint32_t positionHi, positionLo, incrementHi, incrementLo, bit;

        incrementHi = CyclesPerRevolution300Rpm / gcrTrack->bits;
        incrementLo = CyclesPerRevolution300Rpm % gcrTrack->bits;
        positionHi = (CyclesPerRevolution300Rpm >> 1) / gcrTrack->bits;
        positionLo = (CyclesPerRevolution300Rpm >> 1) % gcrTrack->bits;

        for(bit = 0; bit < gcrTrack->bits; bit++) {

            if( (gcrTrack->data[bit >> 3]) & (1 << (~bit & 7)) )
                addPulse(gcrTrack, positionHi, 0xffffffff);

            positionHi += incrementHi;
            positionLo += incrementLo;

            while(positionLo >= gcrTrack->bits) {
                positionLo -= gcrTrack->bits;
                positionHi++;
            }
        }
    }

    // this is needed to generate content list (TOC) outside emulation
    auto DiskStructure::encodeGCRFromPulse(MTrack* gcrTrack, uint8_t halfTrack) -> void {
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

        // these tracks exist from beginning and need to write back if any BIT on disk was written.
        // in P64 we can not write back updated tracks only
        gcrTrack->written = 0x80;
        uint8_t* ptr = gcrTrack->data;
        std::memset( ptr, 0, trackSize );

        int32_t index = gcrTrack->firstPulse;

        while (index >= 0) {
            DiskStructure::Pulse& pulse = gcrTrack->pulses[index];
            index = pulse.next;

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

    auto DiskStructure::encodeMfmFromPulse(MTrack* gcrTrack) -> void {
        uint32_t lastPosition = 0;
        uint32_t delta = 1;
        unsigned pulseDuration = 0;
        unsigned bits = 0;
        unsigned trackSize = 6250 * 2;
        int pulseWidth = 0;
        unsigned todo;
        Emulator::Rand randomizer;
        randomizer.initXorShift( 0x1234abcd );

        if ( !gcrTrack->data )
            gcrTrack->data = new uint8_t[ trackSize ];

        else if ( trackSize != gcrTrack->size ) {
            delete[] gcrTrack->data;
            gcrTrack->data = new uint8_t[ trackSize ];
        }

        gcrTrack->size = trackSize;
        gcrTrack->bits = trackSize * 8;

        // these tracks exist from beginning and need to write back if any BIT on disk was written.
        // in P64 we can not write back updated tracks only
        gcrTrack->written = 0x80;
        uint8_t* ptr = gcrTrack->data;
        std::memset( ptr, 0, trackSize );
        int32_t index = gcrTrack->firstPulse;

        while (index >= 0) {
            todo = delta;

            if (pulseDuration && (pulseDuration < todo))
                todo = pulseDuration;

            if (pulseDuration) {
                pulseDuration -= todo;

                if (!pulseDuration) {
                    switch(pulseWidth++) {
                        case 0: pulseDuration = 16; break;  // 2.375 + 1.0 = 3.375
                        case 1: pulseDuration = 16; bits++; break;  // 3.375 + 1.0 = 4.375
                        // fuzzy bits (4 or 6 micro)
                        case 2: pulseDuration = 16; break;  // 4.375 + 1.0 = 5.375
                        case 3: pulseDuration = 16; bits++; break; // 5.375 + 1.0 = 6.375
                        // fuzzy bits (6 or 8 micro)
                        case 4: pulseDuration = 16; break;  // 6.375 + 1.0 = 7.375
                        case 5: pulseDuration = 16; bits++; break; // 7.375 + 1.0 = 8.375

                        case 6: pulseDuration = 16; if ( (randomizer.xorShift() >> 16 ) & 1) pulseWidth = 9; break;  // 8.375 + 1.0 = 9.375
                        case 7: pulseDuration = 16; bits++; break; // 9.375 + 1.0 = 10.375
                        case 8: pulseDuration = 16; break;  // 10.375 + 1.0 = 11.375
                        case 9: pulseDuration = 16; bits++; break; // 11.375 + 1.0 = 12.375
                        case 10: pulseDuration = 16; if ((randomizer.xorShift() >> 16 ) & 1) pulseWidth = 13; break;  // 12.375 + 1.0 = 13.375
                        case 11: pulseDuration = 16; bits++; break; // 13.375 + 1.0 = 14.375
                        case 12: pulseDuration = 16; break;  // 14.375 + 1.0 = 15.375
                        case 13: pulseDuration = 16; bits++; pulseWidth = 0; break; // 15.375 + 1.0 = 16.375
                    }
                }
            }

            delta -= todo;
            if (!delta) {
                DiskStructure::Pulse& pulse = gcrTrack->pulses[index];
                index = pulse.next;

                if (pulse.strength < 0x80000000)
                    continue;

                delta = pulse.position - lastPosition;
                lastPosition = pulse.position;

                if (pulse.strength == 0xffffffff) {
                    if (bits >= gcrTrack->bits)
                        return;

                    if (pulseWidth & 1) {
                        ptr[bits >> 3] |= 1 << (~bits & 7);
                    } else if (pulseWidth == 0) {
                        // no flux area
                    } else
                        ptr[bits >> 3] |= 1 << (~bits & 7);

                    bits++;
                    pulseDuration = 38;    // 2.375
                    pulseWidth = 0;
                }
            }
        }
    }

    auto DiskStructure::createPxx( std::string diskName, uint8_t sides ) -> Emulator::Interface::Data {

        auto temp = createGxx( diskName, sides );

        DiskStructure structure(nullptr);

        structure.rawData = temp;

        structure.rawSize = (sides == 2) ? imageSizeG71() : imageSizeG64();

        if (!structure.analyzeG64() && !structure.analyzeG71())
            return {nullptr, 0};

        structure.prepareGxx();

        for (int side = 0; side < sides; side++) {
            for (unsigned track = 0; track < TYPICAL_TRACKS; track++) {

                unsigned halfTrack = track << 1;

                MTrack* gcrPtr = &structure.mTracks[side][halfTrack];

                gcrPtr->written = gcrPtr->bits > 0;

                if (gcrPtr->written)
                    structure.createPulsesFromEncoded(gcrPtr);
            }
        }

        if (sides == 2)
            std::memcpy( structure.rawData, "P64-1571", 8 );
        else
            std::memcpy( structure.rawData, "P64-1541", 8 );

        unsigned memSize = 0;

        uint8_t* temp2 = structure.writeP64ToMem(memSize);

        delete[] structure.rawData;

        return {temp2, memSize};
    }

    auto DiskStructure::createP81( const std::string diskName ) -> Emulator::Interface::Data {

        auto temp = createG81( diskName );

        DiskStructure structure(nullptr);

        structure.rawData = temp;

        structure.rawSize = imageSizeG81();

        if (!structure.analyzeG81())
            return {nullptr, 0};

        structure.prepareG81();

        for (int side = 0; side < 2; side++) {
            for (unsigned track = 0; track < 80; track++) {

                MTrack* mTrack = &structure.mTracks[side][track];

                mTrack->written = mTrack->bits > 0;

                if (mTrack->written)
                    structure.createPulsesFromEncoded(mTrack);
            }
        }

        std::memcpy( structure.rawData, "P64-1581", 8 );

        unsigned memSize = 0;

        uint8_t* temp2 = structure.writeP64ToMem(memSize);

        delete[] structure.rawData;

        return {temp2, memSize};
    }
}