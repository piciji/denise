
#pragma once

#include "../../cia/new/cia.h"
#include "../../tools/events.h"
#include "../../tools/powersupply.h"

// todo: Bitplane DMA conflicts

#define FREQUENCY_PAL   28375160
#define FREQUENCY_NTSC  28636360

namespace LIBAMI {

struct Cpu;
struct Denise;
struct Blitter;
struct Copper;
struct Input;

struct Agnus : Emulator::Events<10> {

    Agnus(Cpu& cpu, Denise& denise, Blitter& blitter, Copper& copper, Cia<MOS_8520>& cia1, Cia<MOS_8520>& cia2, Input& input);

    enum { Unmapped, CHIP_MEM, SLOW_MEM, KICK_ROM, EXT_ROM, WOM, MMIO_CUSTOM, MMIO_CIA, MMIO_RTC };

    enum { EVENT_KBD, EVENT_ONE_CYCLE_DELAY, EVENT_LEAVE_EMULATION, EVENT_POWER_SUPPLY };

    enum { DMA_None = 0, DMACON = 1,
           PTR_BLT_A_H, PTR_BLT_A_L, PTR_BLT_B_H, PTR_BLT_B_L, PTR_BLT_C_H, PTR_BLT_C_L, PTR_BLT_D_H, PTR_BLT_D_L,
           BLT_INIT, BLT_BUSY_DELAY,
    };

    enum { ACT_BLITTER = 1, ACT_COPPER = 2, ACT_BPL = 4, ACT_SPRITE = 8 };

    enum { BUS_FREE, BUS_USAGE_BPL, BUS_USAGE_SPRITE, BUS_USAGE_BLITTER, BUS_USAGE_COPPER, BUS_USAGE_CPU, BUS_USAGE_REFRESH, BUS_USAGE_AUDIO };

    enum { PAL, NTSC };

    enum { Trigger_Read, Trigger_CPU, Trigger_Copper, Trigger_Vsync };

    enum Mode { OCS = 1, ECS = 2, AGA = 4 } mode; // AGA not supported at the moment

    enum Model { A1000, A500 } model;

    Cpu& cpu;
    Denise& denise;
    Blitter& blitter;
    Copper& copper;
    Input& input;
    Emulator::PowerSupply powerSupply;
    Cia<MOS_8520>& cia1;
    Cia<MOS_8520>& cia2;

    Emulator::EventCallback oneCycleDelay;
    Emulator::EventCallback leaveEmulation;
    Emulator::EventCallback countDownPowerSupply;
    uint8_t actions = 0;

    uint8_t mapper[256] = {0};
    uint8_t busUsage;
    uint8_t hPos;
    uint16_t vPos;
    uint16_t vStart;
    uint16_t vStop;

    bool vBlankEnd;
    bool vBlankEndNext;
    bool vBlank;
    bool vBlankStart;
    int sprStartLimit;
    uint16_t beamCon;

    struct Sprite {
        uint32_t ptr;
        uint16_t pos;
        uint16_t ctl;
        uint16_t vStart;
        uint16_t vStop;
        bool fetchData;
        uint8_t enable;
    } sprites[8];

    uint8_t ddfStart;
    uint8_t ddfStop;

    uint32_t bpl1pt;
    uint32_t bpl2pt;
    uint32_t bpl3pt;
    uint32_t bpl4pt;
    uint32_t bpl5pt;
    uint32_t bpl6pt;

    int16_t bpl1Mod;
    int16_t bpl2Mod;

    uint8_t* chipMem = nullptr;
    unsigned chipMemMask = 0;
    uint8_t* slowMem = nullptr;
    unsigned slowMemSize = 0;
    uint8_t* kickRom = nullptr;
    unsigned kickRomMask = 0;
    uint8_t* extRom = nullptr;
    unsigned extRomMask = 0;
    uint8_t* wom = nullptr;

    bool useRTC = false;
    uint16_t dataBus = 0;
    uint16_t dmaCon;
    uint16_t dmaConImm;
    uint16_t bplCon0;
    unsigned countWaitCycles;
    uint32_t rDmaPtr;

    unsigned eClockCycle;
    bool lol;
    bool lof;
    bool lolToggle;
    bool lofToggle;
    bool ntsc;
    unsigned lines;
    bool initVCounter;
    bool shortLineBefore;
    bool womLock = false;
    uint8_t resetFromKeyboard = 0;
    bool stopFetching;
    uint16_t bplCycle;
    uint32_t bplQueue;
    uint32_t sprQueue;
    uint8_t ddfStartMatch;
    bool harddis;
    bool ddfEnableBefore;
    uint8_t bplState;
    bool hardStop;

    bool diwFlipFlop;

    auto ecsAndHigher() -> bool const { return mode & (Mode::ECS | Mode::AGA); }
    auto ecs() -> bool const { return mode == Mode::ECS; }
    auto aga() -> bool const { return mode == Mode::AGA; }

    auto useSpriteDMA() -> bool const { return (dmaConImm & 0x220) == 0x220; }
    auto useBlitterDMA() -> bool const { return (dmaCon & 0x240) == 0x240; }
    auto useCopperDMA() -> bool const { return (dmaCon & 0x280) == 0x280; }
    auto useBitplaneDMA() -> bool const { return (dmaConImm & 0x300) == 0x300; }
    auto blitterNasty() -> bool const { return dmaCon & 0x400; }

    auto reset(bool softReset) -> void;
    auto setMemory(unsigned typeId, unsigned size) -> void;
    auto mapMemory() -> void;
    auto setOVL(bool state) -> void;

    auto readByte(uint32_t adr) -> uint8_t;
    auto writeByte(uint32_t adr, uint8_t value) -> void;
    auto readWord(uint32_t adr) -> uint16_t;
    auto writeWord(uint32_t adr, uint16_t value) -> void;
    auto sync(uint16_t cycles) -> void;
    auto dmaCycle() -> void;
    auto addWaitstatesToCPU() -> void;
    auto iackCycle(uint8_t level, uint8_t& vector) -> uint8_t;
    auto resetOut() -> void;
    auto pullResetLine(bool state = true) -> void;

    auto msecToDMACycles(unsigned ms) -> unsigned { return 3550 * ms; } // average for PAL/NTSC, todo: check if more accuracy is needed
    auto usecToDMACycles(unsigned us) -> unsigned { return 3.55f * (float)us + 0.5f; }

    auto writeCustom(uint16_t adr, uint16_t value, uint8_t triggeredBy = Trigger_CPU) -> void;
    template<bool byteAccess = false> auto readCustom(uint16_t adr, bool triggeredByWrite = false) -> uint16_t;

    auto canBlitterUseBus() -> bool;
    auto canCopperUseBus() -> bool;
    auto allocateCopper() -> bool;
    template<uint8_t ptrEvent> auto fetchBlitterDma(uint32_t adr, uint16_t& result) -> bool;
    auto fetchCopperDma(uint32_t adr, uint16_t& result) -> bool;
    auto fetchCopperDmaNoBUSCheck(uint32_t adr, uint16_t& result) -> void;
    auto writeBlitterDma(uint32_t adr, uint16_t value) -> bool;

    template<uint8_t ptrEvent> auto fetchBlitterDmaNoBUSCheck(uint32_t adr, uint16_t& result) -> void;
    auto writeBlitterDmaNoBUSCheck(uint32_t adr, uint16_t value) -> void;

    auto setRefPtr(uint16_t value) -> void;

    auto copperLongGap() -> bool { return shortLineBefore && (hPos == 2); }
    auto POSR(bool vhpos) -> uint16_t;

    auto setBpl1ptH(uint16_t value) -> void;
    auto setBpl1ptL(uint16_t value) -> void;
    auto setBpl2ptH(uint16_t value) -> void;
    auto setBpl2ptL(uint16_t value) -> void;
    auto setBpl3ptH(uint16_t value) -> void;
    auto setBpl3ptL(uint16_t value) -> void;
    auto setBpl4ptH(uint16_t value) -> void;
    auto setBpl4ptL(uint16_t value) -> void;
    auto setBpl5ptH(uint16_t value) -> void;
    auto setBpl5ptL(uint16_t value) -> void;
    auto setBpl6ptH(uint16_t value) -> void;
    auto setBpl6ptL(uint16_t value) -> void;

    auto initCiaClock() -> void;
    auto dmaControl(uint16_t data) -> void;

    auto waitKeyboardReset() -> void;
    template<bool onlyProgressQueue = false> auto fetchPlanes() -> void;
    template<uint8_t pos, bool addMod> auto fetchPlane() -> void;
    template<uint8_t num, bool first> auto spriteControl() -> void;
    auto bplStartStop() -> void;
    auto fetchSprites() -> void;
    template<uint8_t nr, uint8_t target> auto fetchSprite() -> void;
    template<uint8_t nr> auto SPRxCTL() -> void;

    template<uint8_t num> auto setSpr1ptH(uint16_t value) -> void;
    template<uint8_t num> auto setSpr1ptL(uint16_t value) -> void;
    auto updateHarddis() -> void;
    auto isEquLine() -> bool;
    auto updateVdiw() -> void;
    auto setDiwStrt(uint16_t value) -> void;
    auto setDiwStop(uint16_t value) -> void;
};

}
