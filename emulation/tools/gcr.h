
#pragma once

#include <cstdint>

namespace Emulator {

struct Gcr {
    
    const uint8_t RAW_TO_GCR[16] = {
        0x0a, 0x0b, 0x12, 0x13,
        0x0e, 0x0f, 0x16, 0x17,
        0x09, 0x19, 0x1a, 0x1b,
        0x0d, 0x1d, 0x1e, 0x15
    };
    
    const uint8_t GCR_TO_RAW[32] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 8, 0, 1, 0, 12, 4, 5,
        0, 0, 2, 3, 0, 15, 6, 7,
        0, 9, 10, 11, 0, 13, 14, 0
    };
    
    auto encode(const uint8_t* source, uint8_t* dest) -> void {
        // gcr format doesn't allow more than 2 0-bits in a row because reading successive
        // 0-bits is unreliable on a magnetic medium. you could read out the wrong amount of 0-bits.
        // it costs additional 2 bits per byte. 
        // to get complete bytes each call of this function handles 4 bytes of source data.
        // 4 * 10 / 8 = 5 byte of gcr encoded data

        unsigned int buffer = 0;

        for (int i = 2; i < 10; i += 2, source++, dest++) {

            buffer <<= 5;  
            buffer |= RAW_TO_GCR[(*source) >> 4]; // for each nybble exists a 5-bit replacement

            buffer <<= 5;  
            buffer |= RAW_TO_GCR[(*source) & 0x0f]; // conversion of lower nybble

            // in first iteration we have 10 bits and grab upper 8 bits.
            // so there are two bits left at which the next 10 bits will be appended.
            // in second iteration we need to shift out 4 bits to grab the next 8 bits
            // and so on.
            *dest = (uint8_t)(buffer >> i);
        }

        // after 4 iterations there are 8 bits left, we grab them as our fifth byte
        *dest = (uint8_t)buffer;
    }

    // decodes 5 byte of gcr data back to 4 byte source data
    auto decode(const uint8_t* source, uint8_t* dest) -> void {

        uint32_t buffer = *source; // buffer first gcr byte

        buffer <<= 8; // make room for next, we need at least 2 bytes for start

        for (unsigned i = 0; i < 8; i += 2, dest++) {
            source++;
            
            buffer |= ((uint32_t)(*source)) << i;

            // get 4 bit raw data for most significant 5 gcr bits
            *dest = GCR_TO_RAW[(buffer >> 11) & 0x1f] << 4;
            // shift out the already handled 5 gcr bits
            buffer <<= 5;

            // and handle the next 5 bits
            *dest |= GCR_TO_RAW[(buffer >> 11) & 0x1f];
            buffer <<= 5;

            // after first iteration we have 6 unhandled bits left, which are left
            // shifted by 10 bits, so we insert the next source byte 2 bits left
            // shifted to keep source buffer aligned
        }
    }
};

}
