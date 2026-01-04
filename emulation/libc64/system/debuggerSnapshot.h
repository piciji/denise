
#pragma once

#include <cstdint>
#include "../../interface.h"

namespace LIBC64 {

struct DebuggerSnapshot : Emulator::Interface::DebuggerSnapshot {
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

    struct {
        struct {
            uint8_t data[0x3fff];
            unsigned pos = 0;

            bool expandX;
            bool expandY;
            bool multiColor;
            bool prioMD;
            uint16_t x;
            uint8_t y;
            uint16_t addr;
            uint8_t mcBase;
        } spr[8];
        uint8_t spriteForegroundCollided;
        uint8_t spriteSpriteCollided;

        uint16_t xPos;
        uint16_t vc;
        uint16_t vcBase;
        uint8_t rc;

        bool den;
        bool badLine;
        bool visibleLine;
        bool hFlipFlip;
        bool vFlipFlip;
        bool idleMode;
        uint8_t vmli;
        uint8_t mode;
        uint8_t irqLatch;
        uint8_t irqEnable;
        uint16_t irqLine;

        uint8_t xScroll;
        uint8_t yScroll;

        uint16_t vicBank;
        uint16_t screenMemory;
        uint16_t charMemory;

        uint8_t lpx;
        uint8_t lpy;
        bool lpPin;
        bool lpLatched;
        uint8_t controlReg1;
        uint8_t controlReg2;

    } vicII;

    struct {
        struct {
            uint8_t pr;
            uint8_t ddr;
            uint8_t io;

            uint16_t timer;
            uint16_t timerLatch;

            bool timerRunning;
            bool oneshot;
            bool pbOut;
            bool toggleOut;
        } port[2];

        uint8_t icr;
        uint8_t icrMask;
        uint32_t tod;
        uint32_t todAlarm;

        uint8_t sdr;
        unsigned shiftCount;
    } cia[2];

    constexpr static Emulator::Interface::DebuggerIdent CiaPorts[2][2][8] = {
        { // CIA A
            { // Port A
                {7, "COL7"},{6, "COL6"},{5, "COL5"},{4, "BTNB"},
                {3, "JOYB3"},{2, "JOYB2"},{1, "JOYB1"}, {0, "JOYB0"}

            },
            { // Port B
                    {7, "ROW7"},{6, "ROW6"},{5, "ROW5"},{4, "BTNA"},
                    {3, "JOYA3"},{2, "JOYA2"},{1, "JOYA1"},{0, "JOYA0"}
            }
        },
        { // CIA B
                { // Port A
                    {7, "DATA IN"},{6, "CLK IN"},{5, "DATA OUT"},{4, "CLK OUT"},
                    {3, "ATN OUT"},{2, "USER M"},{1, "VA15"},{0, "VA14"}

                },
                { // Port B
                    {7, "USER L"}, {6, "USER K"},{5, "USER J"},{4, "USER H"},
                    {3, "USER F"},{2, "USER E"},{1, "USER D"},{0, "USER C"}
                }
        }
    };

    constexpr static Emulator::Interface::DebuggerIdent exceptions[] {
        {0xfffe, "IRQ"},
        {0xffff, "NMI"},
    };

    constexpr static Emulator::Interface::DebuggerIdent exceptions65816[] {
        {0xfffe, "IRQ M-E"},
        {0xfffa, "NMI M-E"},
        {0xffee, "IRQ"},
        {0xffea, "NMI"},
    };
};



}
