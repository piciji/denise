
#pragma once

#include <cstdint>

namespace Emulator {

    struct CRC32 {

        const uint32_t CRC32Table[16] = {
                0x00000000UL, 0x1db71064UL, 0x3b6e20c8UL, 0x26d930acUL,
                0x76dc4190UL, 0x6b6b51f4UL, 0x4db26158UL, 0x5005713cUL,
                0xedb88320UL, 0xf00f9344UL, 0xd6d6a3e8UL, 0xcb61b38cUL,
                0x9b64c2b0UL, 0x86d3d2d4UL, 0xa00ae278UL, 0xbdbdf21cUL
        };

        CRC32(uint8_t* data, unsigned size, uint32_t _checksum = 0) {

            this->checksum = _checksum;

            if (!size)
                return;

            for (uint32_t pos = 0; pos < size; pos++) {
                checksum ^= data[pos];
                checksum = CRC32Table[checksum & 0xf] ^ (checksum >> 4);
                checksum = CRC32Table[checksum & 0xf] ^ (checksum >> 4);
            }
        }

        auto value() const -> uint32_t {
            return ~checksum;
        }

    private:

        uint32_t checksum = 0;
    };

}