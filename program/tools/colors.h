
#pragma once

#include <cstdint>

struct ColorTools {

    static auto mix(unsigned colA, unsigned colB, unsigned alpha) -> unsigned {

        uint8_t r1 = (colA >> 16) & 0xff;
        uint8_t g1 = (colA >> 8) & 0xff;
        uint8_t b1 = colA & 0xff;

        uint8_t r2 = (colB >> 16) & 0xff;
        uint8_t g2 = (colB >> 8) & 0xff;
        uint8_t b2 = colB & 0xff;

        uint8_t invAlpha = 100 - alpha;

        r1 = (r1 * alpha + r2 * invAlpha) / 100;
        g1 = (g1 * alpha + g2 * invAlpha) / 100;
        b1 = (b1 * alpha + b2 * invAlpha) / 100;

        return r1 << 16 | g1 << 8 | b1;
    }

};