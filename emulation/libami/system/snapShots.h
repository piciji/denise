
#pragma once

#include <cstdint>

namespace LIBAMI {

struct CpuSnapshot {
    uint32_t regsD[8];
    uint32_t regsA[8];
    uint32_t pc;
    uint32_t pcOpEdge;

    uint32_t usp;
    uint32_t ssp;

    uint16_t irc;
    uint16_t ird;

    constexpr static char flagIdent[] = {'C', 'V', 'Z', 'N', 'X', ' ', ' ', ' ',
        'I', 'I', 'I', ' ', ' ', 'S', ' ', 'T'};
    uint16_t flags;

    uint8_t ipl;
    bool hlt;
    bool stp;
};

struct AgnusSnapshot {
    uint8_t hPos;
    uint16_t vPos;
};

}
