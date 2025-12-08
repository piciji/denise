
#pragma once

#include <cstdint>
#include "../../interface.h"

namespace LIBAMI {

struct DebuggerSnapshot {
    uint32_t regsD[8];
    uint32_t regsA[8];
    uint32_t pc;

    uint32_t usp;
    uint32_t ssp;

    uint16_t irc;
    uint16_t ird;

    uint8_t mapper[256];

    constexpr static char flagIdent[] = {'C', 'V', 'Z', 'N', 'X', ' ', ' ', ' ',
        'I', 'I', 'I', ' ', ' ', 'S', ' ', 'T'};
    uint16_t flags;

    uint8_t ipl;
    bool hlt;
    bool stp;

    uint8_t hPos;
    uint16_t vPos;

    constexpr static Emulator::Interface::DebuggerException exceptions[] {
        {2, "BUS error (2)"},
        {3, "Address error (3)"},
        {4, "Illegal (4)"},
        {5, "Div. By Zero (5)"},
        {6, "CHK (6)"},
        {7, "TRAPV (7)"},
        {8, "Priv. violation (8)"},
        {9, "Trace (9)"},
        {10, "Line-A (10)"},
        {11, "Line-F (11)"},
        {25, "Level 1 (25)"},
        {26, "Level 2 (26)"},
        {27, "Level 3 (27)"},
        {28, "Level 4 (28)"},
        {29, "Level 5 (29)"},
        {30, "Level 6 (30)"},
        {31, "Level 7 (31)"},
        {32, "TRAP (32)"},
        {33, "TRAP (33)"},
        {34, "TRAP (34)"},
        {35, "TRAP (35)"},
        {36, "TRAP (36)"},
        {37, "TRAP (37)"},
        {38, "TRAP (38)"},
        {39, "TRAP (39)"},
        {40, "TRAP (40)"},
        {41, "TRAP (41)"},
        {42, "TRAP (42)"},
        {43, "TRAP (43)"},
        {44, "TRAP (44)"},
        {45, "TRAP (45)"},
        {46, "TRAP (46)"},
        {47, "TRAP (47)"},
    };
};

}
