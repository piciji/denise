
#pragma once

#include <cstdint>
#include "../../interface.h"

namespace LIBC64 {

struct DebuggerSnapshot {
    uint8_t regA;
    uint8_t regX;
    uint8_t regY;
    uint8_t regS;
    uint16_t pc;

    uint8_t ddr;
    uint8_t por;
    uint8_t ioLines;
    uint8_t mode;

    uint8_t mapper[16];

    constexpr static char flagIdent[] = {'C', 'Z', 'I', 'D', 'B', ' ', 'V', 'N'};
    uint8_t flags;

    uint8_t hPos;
    uint16_t vPos;

    constexpr static Emulator::Interface::DebuggerException exceptions[] {
        {0xfffe, "IRQ"},
        {0xffff, "NMI"},
    };
};



}
