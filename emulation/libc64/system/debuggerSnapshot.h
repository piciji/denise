
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
    uint32_t pcEdge;

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

    Emulator::Interface::DebuggerDma debuggerDma[65];
    uint8_t lineCycles;

    struct {
        uint16_t regA;
        uint16_t regX;
        uint16_t regY;
        uint16_t regS;
        uint16_t pc;
        uint32_t pcEdge;
        uint8_t flags;
    } drives[4];

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
        bool sdrOutput;
    } cia[2];

    struct {
        struct {
            uint8_t pr;
            uint8_t ddr;
            uint8_t io;

            uint16_t timer;
            uint16_t timerLatch;

            bool toggleOut;
        } port[2];

        uint8_t ifr;
        uint8_t ier;
        uint8_t acr;
        uint8_t pcr;
        uint8_t sdr;
        unsigned shiftCount;
    } via[2];

    struct {
        struct {
            uint8_t wave;
            uint16_t frequency;
            uint16_t pulseWidth;
            uint8_t attack;
            uint8_t delay;
            uint8_t sustain;
            uint8_t release;
            uint8_t control;
        } voices[3];

        struct {
            uint16_t cutOff;
            uint8_t resonance;
            uint8_t voices;
            uint8_t mode;
        } filter;

        uint8_t volume;
        uint8_t potX;
        uint8_t potY;
        bool active;
    } sids[8];

    constexpr static const char* dmaModeGroups[] { "Free", "Idle", "Graphics", "Character", "Sprite Pointer", "Sprite Data", "Refresh", "Cpu" };

    constexpr static Emulator::Interface::DebuggerIdent dmaModes[] {
        {0,""},
        {1,"IDL"}, {2,"GRA"}, {3,"CHA"},
        {4,"SPP"}, {5,"SPD"},
        {6,"REF"}, {7,"CPU"}
    };
    constexpr static const char* cpuAccess[] { "-", "RAM", "VIC", "SID", "COL", "IO1", "IO2", "CIA1", "CIA2", "CHAR", "KERN", "BASC", "ROML", "ROMH", "ULT"};

    constexpr static Emulator::Interface::DebuggerIdent breakConditions[] {
    {0, "VPOS"}, {1, "HPOS"}, {2, "PC"}, {3, "REGX"},
    {4, "REGY"}, {5, "REGA"}, {6, "REGS"}, {7, "REGP"},
    {8, "DDR"}, {9, "POR"}, {10, "IO"}, {11, "RDY"},
    {12, "IRQ"}, {13, "NMI"},
    {14, "FLAG-C"}, {15, "FLAG-Z"}, {16, "FLAG-I"}, {17, "FLAG-D"},
    {18, "FLAG-B"}, {19, "FLAG-V"}, {20, "FLAG-N"},
    {100, "MEM:"}, {101, "CPU:"},
    {102, "DRIVE8:"}, {103, "DRIVE9:"},
    {104, "DRIVE10:"},{105, "DRIVE11:"},
    };

    constexpr static Emulator::Interface::DebuggerIdent breakConditionsSCPU[] {
    {0, "VPOS"}, {1, "HPOS"}, {2, "PC"}, {3, "REGX"},
    {4, "REGY"}, {5, "REGA"}, {6, "REGS"}, {7, "REGD"},
    {8, "REGP"}, {9, "DBR"},{10, "PBR"},
    {11, "ME"}, {12, "RDY"}, {13, "IRQ"},
    {14, "NMI"},
    {20, "FLAG-C"}, {21, "FLAG-Z"}, {22, "FLAG-I"}, {23, "FLAG-D"},
    {24, "FLAG-X"}, {25, "FLAG-M"}, {26, "FLAG-V"}, {26, "FLAG-N"},
    {100, "C64RAM:"}, {101, "CPU:"},
    {102, "DRIVE8:"}, {103, "DRIVE9:"},
    {104, "DRIVE10:"},{105, "DRIVE11:"},
    };

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
                {3, "ATN OUT"},{2, "M"},{1, "VA15"},{0, "VA14"}

            },
            { // Port B
                {7, "USER-L"}, {6, "U-K"},{5, "U-J"},{4, "U-H"},
                {3, "U-F"},{2, "U-E"},{1, "U-D"},{0, "U-C"}
            }
        }
    };

    constexpr static Emulator::Interface::DebuggerIdent ViaPorts[2][2][8] = {
        { // VIA A
            { // Port A
                {7, "RDY"},{6, "Bit6"},{5, "Bit5"},{4, "Bit4"},
                {3, "Bit3"},{2, "Bit2"},{1, "Bit1"}, {0, "TRACK-0"}

            },
            { // Port B
                {7, "ATN-IN"},{6, "6"},{5, "5"},{4, "ATN-OUT"},
                {3, "CLK-OUT"},{2, "CLK-IN"},{1, "DATA-OUT"},{0, "DATA-IN"}
            }
        },
        { // VIA B
            { // Port A
                {7, "Bit7"},{6, "Bit6"},{5, "Bit5"},{4, "Bit4"},
                {3, "Bit3"},{2, "Bit2"},{1, "Bit1"},{0, "Bit0"}

            },
            { // Port B
                {7, "SYNC"}, {6, "SZ"},{5, "SZ"},{4, "WP"},
                {3, "LED"},{2, "MTR"},{1, "STEP"},{0, "STEP"}
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
