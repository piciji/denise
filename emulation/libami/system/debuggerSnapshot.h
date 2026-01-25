
#pragma once

#include <cstdint>
#include "../../interface.h"

namespace LIBAMI {

struct DebuggerSnapshot : Emulator::Interface::DebuggerSnapshot {
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

    struct {
        Emulator::Interface::DebuggerDma* debuggerDma = nullptr;
        uint8_t lastHPos;
        unsigned model;
        unsigned chipMemMask;
    } agnus;

    struct {
        struct {
            uint16_t data[0x1fff];
            unsigned pos = 0;
            uint16_t x = 0;
            uint16_t vStart = 0; // Agnus
            uint16_t vStop = 0; // Agnus
            bool attached = false;

            uint16_t datA;
            uint16_t datB;
        } spr[8];

        uint16_t colors[32];
        uint16_t bplCon0;
        uint16_t bplCon1;
        uint16_t bplCon2;
        uint16_t bplCon3;
        uint16_t clxDat;
        uint16_t hPos;
        bool hblank;
        bool border;
        bool enableDisplay;
        uint16_t hStart;
        uint16_t hStop;
        uint8_t delayPf1;
        uint8_t delayPf2;

        uint16_t bpl1dat;
        uint16_t bpl2dat;
        uint16_t bpl3dat;
        uint16_t bpl4dat;
        uint16_t bpl5dat;
        uint16_t bpl6dat;

    } denise;

    constexpr static Emulator::Interface::DebuggerIdent dmaModes[] {
        {0, "Free"}, {1, "Bitplanes"},{2, "Sprites"},
        {3, "Blitter"}, {4, "Copper"},{5, "Cpu"},
        {6, "Refresh"}, {7, "Disk"}, {8, "Audio"},
        {9, "Blt-Cop Conflict"}, {10, "Blt-Spr Conflict"},
        {11, "Bpl-Ref Conflict"}, {12, "Bpl-Spr Conflict"}
    };

    constexpr static Emulator::Interface::DebuggerIdent dmaModesShort[] {
        {0, ""}, {1, "BPL"},{2, "SPR"},
        {3, "BLT"}, {4, "COP"},{5, "CPU"},
        {6, "REF"}, {7, "DSK"}, {8, "AUD"},
        {9, "BLT-COP"}, {10, "BLT-SPR"},
        {11, "BPL-REF"}, {12, "BPL-SPR"}
    };

    constexpr static const char* cpuAccess[] {
        "-", "CHIP", "SLOW", "KICK", "EXT", "WOM", "Register", "CIA", "RTC", "AUTOCONF", "FAST", "EXP"
    };

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
                {7, "FIR1"},{6, "FIR0"},{5, "RDY"},{4, "TK0"},
                {3, "WPRO"},{2, "CHNG"},{1, "LED"}, {0, "OVL"}

            },
            { // Port B
                {7, "PB7"},{6, "PB6"},{5, "PB5"},{4, "PB4"},
                {3, "PB3"},{2, "PB2"},{1, "PB1"},{0, "PB0"}
            }
        },
        { // CIA B
            { // Port A
                {7, "DTR"},{6, "RTS"},{5, "CD"},{4, "CTS"},
                {3, "DSR"},{2, "SEL"},{1, "POUT"},{0, "BUSY"}

            },
            { // Port B
                {7, "MTR"}, {6, "SEL3"},{5, "SEL2"},{4, "SEL1"},
                {3, "SEL0"},{2, "SIDE"},{1, "DIR"},{0, "STEP"}
            }
        }
    };

    constexpr static Emulator::Interface::DebuggerIdent exceptions[] {
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

    constexpr static Emulator::Interface::DebuggerIdent registerIdents[] {
        {0x0000, ""},
        {0x0002, "DMACONR"},
        {0x0004, "VPOSR"},
        {0x0006, "VHPOSR"},
        {0x0008, ""},
        {0x000a, "JOY0DAT"},
        {0x000c, "JOY1DAT"},
        {0x000e, "CLXDAT"},
        {0x0010, "ADKCONR"},
        {0x0012, "POT0DAT"},
        {0x0014, "POT1DAT"},
        {0x0016, "POTINP"},
        {0x0018, "SERDATR"},
        {0x001a, "DSKBYTR"},
        {0x001c, "INTENAR"},
        {0x001e, "INTREQR"},

        {0x0020, "DSKPTH"},
        {0x0022, "DSKPTL"},
        {0x0024, "DSKLEN"},
        {0x0026, "DSKDAT"},
        {0x0028, "REFPTR"},
        {0x002a, "VPOSW"},
        {0x002c, "VHPOSW"},
        {0x002e, "COPCON"},
        {0x0030, "SERDAT"},
        {0x0032, "SERPER"},
        {0x0034, "POTGO"},
        {0x0036, "JOYTEST"},
        {0x0038, ""},
        {0x003a, ""},
        {0x003c, ""},
        {0x003e, ""},
        {0x0040, "BLTCON0"},
        {0x0042, "BLTCON1"},
        {0x0044, "BLTAFWM"},
        {0x0046, "BLTALWM"},
        {0x0048, "BLTCPTH"},
        {0x004a, "BLTCPTL"},
        {0x004c, "BLTBPTH"},
        {0x004e, "BLTBPTL"},
        {0x0050, "BLTAPTH"},
        {0x0052, "BLTAPTL"},
        {0x0054, "BLTDPTH"},
        {0x0056, "BLTDPTL"},
        {0x0058, "BLTSIZE"},
        {0x105a, "BLTCON0L"},
        {0x105c, "BLTSIZV"},
        {0x105e, "BLTSIZH"},
        {0x0060, "BLTCMOD"},
        {0x0062, "BLTBMOD"},
        {0x0064, "BLTAMOD"},
        {0x0066, "BLTDMOD"},
        {0x0068, ""},
        {0x006a, ""},
        {0x006c, ""},
        {0x006e, ""},
        {0x0070, "BLTCDAT"},
        {0x0072, "BLTBDAT"},
        {0x0074, "BLTADAT"},
        {0x0076, ""},
        {0x1078, "SPRHDAT"},
        {0x107a, "BPLHDAT"},
        {0x107c, "DENISEID"},
        {0x007e, "DSKSYNC"},
        {0x0080, "COP1LCH"},
        {0x0082, "COP1LCL"},
        {0x0084, "COP2LCH"},
        {0x0086, "COP2LCL"},
        {0x0088, "COPJMP1"},
        {0x008a, "COPJMP2"},
        {0x008c, ""},
        {0x008e, "DIWSTRT"},
        {0x0090, "DIWSTOP"},
        {0x0092, "DDFSTRT"},
        {0x0094, "DDFSTOP"},
        {0x0096, "DMACON"},
        {0x0098, "CLXCON"},
        {0x009a, "INTENA"},
        {0x009c, "INTREQ"},
        {0x009e, "ADKCON"},
        {0x00a0, "AUD0LCH"},
        {0x00a2, "AUD0LCL"},
        {0x00a4, "AUD0LEN"},
        {0x00a6, "AUD0PER"},
        {0x00a8, "AUD0VOL"},
        {0x00aa, "AUD0DAT"},
        {0x00ac, ""},
        {0x00ae, ""},
        {0x00b0, "AUD1LCH"},
        {0x00b2, "AUD1LCL"},
        {0x00b4, "AUD1LEN"},
        {0x00b6, "AUD1PER"},
        {0x00b8, "AUD1VOL"},
        {0x00ba, "AUD1DAT"},
        {0x00bc, ""},
        {0x00be, ""},
        {0x00c0, "AUD2LCH"},
        {0x00c2, "AUD2LCL"},
        {0x00c4, "AUD2LEN"},
        {0x00c6, "AUD2PER"},
        {0x00c8, "AUD2VOL"},
        {0x00ca, "AUD2DAT"},
        {0x00cc, ""},
        {0x00ce, ""},
        {0x00d0, "AUD3LCH"},
        {0x00d2, "AUD3LCL"},
        {0x00d4, "AUD3LEN"},
        {0x00d6, "AUD3PER"},
        {0x00d8, "AUD3VOL"},
        {0x00da, "AUD3DAT"},
        {0x00dc, ""},
        {0x00de, ""},
        {0x00e0, "BPL1PTH"},
        {0x00e2, "BPL1PTL"},
        {0x00e4, "BPL2PTH"},
        {0x00e6, "BPL2PTL"},
        {0x00e8, "BPL3PTH"},
        {0x00ea, "BPL3PTL"},
        {0x00ec, "BPL4PTH"},
        {0x00ee, "BPL4PTL"},
        {0x00f0, "BPL5PTH"},
        {0x00f2, "BPL5PTL"},
        {0x00f4, "BPL6PTH"},
        {0x00f6, "BPL6PTL"},

        {0x20f8, "BPL7PTH"},
        {0x20fa, "BPL7PTL"},
        {0x20fc, "BPL8PTH"},
        {0x20fe, "BPL8PTL"},

        {0x0100, "BPLCON0"},
        {0x0102, "BPLCON1"},
        {0x0104, "BPLCON2"},
        {0x0106, "BPLCON3"},
        {0x0108, "BPL1MOD"},
        {0x010a, "BPL2MOD"},

        {0x210c, "BPLCON4"},
        {0x210e, "CLXCON2"},

        {0x0110, "BPL1DAT"},
        {0x0112, "BPL2DAT"},
        {0x0114, "BPL3DAT"},
        {0x0116, "BPL4DAT"},
        {0x0118, "BPL5DAT"},
        {0x011a, "BPL6DAT"},

        {0x211c, "BPL7DAT"},
        {0x211e, "BPL8DAT"},

        {0x0120, "SPR0PTH"},
        {0x0122, "SPR0PTL"},
        {0x0124, "SPR1PTH"},
        {0x0126, "SPR1PTL"},
        {0x0128, "SPR2PTH"},
        {0x012a, "SPR2PTL"},
        {0x012c, "SPR3PTH"},
        {0x012e, "SPR3PTL"},
        {0x0130, "SPR4PTH"},
        {0x0132, "SPR4PTL"},
        {0x0134, "SPR5PTH"},
        {0x0136, "SPR5PTL"},
        {0x0138, "SPR6PTH"},
        {0x013a, "SPR6PTL"},
        {0x013c, "SPR7PTH"},
        {0x013e, "SPR7PTL"},
        {0x0140, "SPR0POS"},
        {0x0142, "SPR0CTL"},
        {0x0144, "SPR0DATA"},
        {0x0146, "SPR0DATB"},
        {0x0148, "SPR1POS"},
        {0x014a, "SPR1CTL"},
        {0x014c, "SPR1DATA"},
        {0x014e, "SPR1DATB"},
        {0x0150, "SPR2POS"},
        {0x0152, "SPR2CTL"},
        {0x0154, "SPR2DATA"},
        {0x0156, "SPR2DATB"},
        {0x0158, "SPR3POS"},
        {0x015a, "SPR3CTL"},
        {0x015c, "SPR3DATA"},
        {0x015e, "SPR3DATB"},
        {0x0160, "SPR4POS"},
        {0x0162, "SPR4CTL"},
        {0x0164, "SPR4DATA"},
        {0x0166, "SPR4DATB"},
        {0x0168, "SPR5POS"},
        {0x016a, "SPR5CTL"},
        {0x016c, "SPR5DATA"},
        {0x016e, "SPR5DATB"},
        {0x0170, "SPR6POS"},
        {0x0172, "SPR6CTL"},
        {0x0174, "SPR6DATA"},
        {0x0176, "SPR6DATB"},
        {0x0178, "SPR7POS"},
        {0x017a, "SPR7CTL"},
        {0x017c, "SPR7DATA"},
        {0x017e, "SPR7DATB"},
        {0x0180, "COLOR00"}, {0x0182, "COLOR01"}, {0x0184, "COLOR02"}, {0x0186, "COLOR03"},
        {0x0188, "COLOR04"}, {0x018a, "COLOR05"}, {0x018c, "COLOR06"}, {0x018e, "COLOR07"},
        {0x0190, "COLOR08"}, {0x0192, "COLOR09"}, {0x0194, "COLOR10"}, {0x0196, "COLOR11"},
        {0x0198, "COLOR12"}, {0x019a, "COLOR13"}, {0x019c, "COLOR14"}, {0x019e, "COLOR15"},
        {0x01a0, "COLOR16"}, {0x01a2, "COLOR17"}, {0x01a4, "COLOR18"}, {0x01a6, "COLOR19"},
        {0x01a8, "COLOR20"}, {0x01aa, "COLOR21"}, {0x01ac, "COLOR22"}, {0x01ae, "COLOR23"},
        {0x01b0, "COLOR24"}, {0x01b2, "COLOR25"}, {0x01b4, "COLOR26"}, {0x01b6, "COLOR27"},
        {0x01b8, "COLOR28"}, {0x01ba, "COLOR29"}, {0x01bc, "COLOR30"}, {0x01be, "COLOR31"},
        {0x11c0, "HTOTAL"},
        {0x11c2, "HSSTOP"},
        {0x11c4, "HBSTRT"},
        {0x11c6, "HBSTOP"},
        {0x11c8, "VTOTAL"},
        {0x11ca, "VSSTOP"},
        {0x11cc, "VBSTRT"},
        {0x11ce, "VBSTOP"},
        {0x11d0, "SPRHSTRT"},
        {0x11d2, "SPRHSTOP"},
        {0x11d4, "BPLHSTRT"},
        {0x11d6, "SPRHSTOP"},
        {0x11d8, "HHPOSW"},
        {0x11da, "HHPOSR"},
        {0x01dc, "BEAMCON0"},
        {0x11de, "HSSTRT"},
        {0x11e0, "VSSTRT"},
        {0x11e2, "HCENTER"},
        {0x11e4, "DIWHIGH"},
        {0x11e6, "BPLHMOD"},
        {0x11e8, "SPRHPTH"},
        {0x11ea, "SPRHPTL"},
        {0x11ec, "BPLHPTH"},
        {0x11ee, "BPLHPTL"},
        {0x01f0, ""}, {0x01f2, ""}, {0x01f4, ""},
        {0x01f6, ""}, {0x01f8, ""}, {0x01fa, ""},
        {0x21fc, "FMODE"},
        {0x01fe, ""},
    };
};

}
