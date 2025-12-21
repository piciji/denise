
#pragma once

#include <cstdint>
#include "../../interface.h"

namespace LIBC64 {

struct DebuggerSnapshot {
    uint16_t regA;
    uint16_t regX;
    uint16_t regY;
    uint16_t regS;
    uint16_t pc;

    uint8_t pbr;
    uint8_t dbr;

    uint8_t ddr;
    uint8_t por;
    uint8_t ioLines;
    uint8_t mode;

    bool modeE;
    bool superCpu;

    uint8_t mapper[16];
    uint8_t mapperSCPU[256];

    constexpr static char flagIdent[] = {'C', 'Z', 'I', 'D', 'B', ' ', 'V', 'N'};
    constexpr static char flagIdent65816[] = {'C', 'Z', 'I', 'D', 'X', 'M', 'V', 'N'};

    uint8_t flags;

    uint8_t hPos;
    uint16_t vPos;

    constexpr static Emulator::Interface::DebuggerException exceptions[] {
        {0xfffe, "IRQ"},
        {0xffff, "NMI"},
    };

    constexpr static Emulator::Interface::DebuggerException exceptions65816[] {
        {0xfffe, "IRQ M-E"},
        {0xfffa, "NMI M-E"},
        {0xffee, "IRQ"},
        {0xffea, "NMI"},
    };
};



}
