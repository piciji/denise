
#pragma once

#include <cstdint>
#include <cstring>
#include <algorithm>

namespace Firmware {

    namespace CRC32 {
        // http://sasfepu78.fr/Kickstart.php
        static const uint32_t KICK_07_27003_BETA = 0x428A9A4B;
        static const uint32_t KICK_10_30_NTSC = 0x299790FF;
        static const uint32_t KICK_11_31034_NTSC = 0xD060572A;
        static const uint32_t KICK_11_31034_PAL = 0xEC86DAE2;
        static const uint32_t KICK_12_33166 = 0x9ED783D0;
        static const uint32_t KICK_12_33180 = 0xA6CE1636;
        static const uint32_t KICK_12_33180_GUARDIAN_HACK = 0x85067666;
        static const uint32_t KICK_12_33180_MRAS_HACK = 0xF80F0FC5;
        static const uint32_t KICK_121_34004 = 0xDB4C8033;
        static const uint32_t KICK_13_34005_A500 = 0xC4F0F55F;
        static const uint32_t KICK_13_34005_A3000 = 0xE0F37258;
        static const uint32_t KICK_13_34005_GUARDIAN_HACK = 0x74680D37;
                                
    }

    extern const uint8_t mtecAT500[8192]; // 133w

    extern const uint8_t mras0Rom[2890];

    extern const uint8_t mras0RomAsyc[2996];

};
