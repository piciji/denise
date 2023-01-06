
#pragma once

#include "blitter.h"
#include "copper.h"
#include "../../cia/new/cia.h"
#include "../../tools/events.h"
#include "../../tools/powersupply.h"

/**
 * todos:
 * AGA
 *
 * Bitplane <> Strobe, Refresh, DMAL conflicts
 *      Is there software that specifically triggers such a conflict to achieve something meaningful ?
 *
 * variable vsync, hsync start/stop, hcenter (interlace: vsync on long fields)
 *      alter the position when the electron beam is moved back. This should happen during the blanking period. (no color output)
 *      otherwise, artifacts will occur during this process.
 *      can this be used in any sense from an emulation point of view ? is there software for this ?
 *      note: altered syncing/blanking don't change FPS
 *
 * variable hblank start/stop
 *      what is it used for? hsync should be enough, because only csync (h+v sync) is visible for ECS Denise.
 *      OCS Denise has hardwired horizontal blanking.
 *
 * UHRES/DUAL stuff (using two screens independantly ?)
 *      was there ever software for this?
 */

namespace LIBAMI {

struct System;
struct Interface;
struct Cpu;
struct Denise;
struct Paula;
struct Input;

struct Agnus : Emulator::Events<6> {

    Agnus(System* system, Cpu& cpu, Denise& denise, Paula& paula, Cia<MOS_8520>& cia1, Cia<MOS_8520>& cia2, Input& input);
    ~Agnus();

    enum { Unmapped, CHIP_MEM, SLOW_MEM, KICK_ROM, EXT_ROM, WOM, MMIO_CUSTOM, MMIO_CIA, MMIO_RTC };

    enum { EVENT_KBD, EVENT_ONE_CYCLE_DELAY, EVENT_LEAVE_EMULATION, EVENT_POWER_SUPPLY, EVENT_AUDIO_STATE, EVENT_HTOTAL };

    enum { DMA_None = 0, DMACON = 1,
           PTR_BLT_A_H, PTR_BLT_A_L, PTR_BLT_B_H, PTR_BLT_B_L, PTR_BLT_C_H, PTR_BLT_C_L, PTR_BLT_D_H, PTR_BLT_D_L,
           PTR_DSK_H, PTR_DSK_L,
           BLT_INIT, BLT_BUSY_DELAY,
    };

    enum { ACT_BLITTER = 1, ACT_COPPER = 2, ACT_BPL = 4, ACT_SPRITE = 8, ACT_IRQ_DELAY = 16 };

    enum { BUS_FREE, BUS_USAGE_BPL, BUS_USAGE_SPRITE, BUS_USAGE_BLITTER, BUS_USAGE_COPPER, BUS_USAGE_CPU, BUS_USAGE_REFRESH, BUS_USAGE_DMAL };

    enum { PAL, NTSC };

    enum { Trigger_Read, Trigger_CPU, Trigger_Copper, Trigger_Vsync };

    enum Model { OCS_A1000 = 1, OCS = 2, ECS = 4, AGA = 8 } model = OCS;

    System* system;
    Interface* interface;
    Cpu& cpu;
    Denise& denise;
    Paula& paula;
    Input& input;
    Cia<MOS_8520>& cia1;
    Cia<MOS_8520>& cia2;

    Emulator::PowerSupply powerSupply;
    Blitter blitter;
    Copper copper;

    Emulator::EventCallback oneCycleDelay;
    Emulator::EventCallback leaveEmulation;
    Emulator::EventCallback countDownPowerSupply;
    Emulator::EventCallback eventHTotal;
    uint8_t actions = 0;

    uint8_t mapper[256] = {0};
    uint8_t busUsage;
    uint8_t hPos;
    uint16_t vPos;
    uint16_t vStart;
    uint16_t vStop;
    uint16_t dmal;
    double fps;
    uint64_t frameClock;
    uint8_t fpsChange;

    bool vBlankEnd;
    bool vBlankEndNext;
    bool vBlank;
    bool vBlankStart;
    bool sprInhibited;

    uint16_t lines;
    uint16_t vTotal;
    uint16_t vBStrt;
    uint16_t vBStop;
    uint8_t hTotal;

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

    struct AudioDmaChannel {
        uint32_t ptr;
        uint32_t ptrLatch;
    } audioDmaChannels[4];

    uint32_t dskpt;

    // need this for runAhead
    struct MemChange {
        uint32_t address;
        uint16_t value;
    };

    MemChange* chipMemChange;
    MemChange* slowMemChange;
    unsigned chipMemChangeSize;
    unsigned slowMemChangeSize;
    unsigned chipMemChangePos;
    unsigned slowMemChangePos;
    bool trackMemChanges;

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

    uint64_t eClockCycle;
    bool lol;
    bool lof;
    bool lolToggle;
    bool ntsc;

    bool initVCounter;
    bool shortLineBefore;
    bool womLock = false;
    uint8_t resetFromKeyboard = 0;
    bool stopFetching;
    uint16_t bplCycle;
    uint32_t bplQueue;
    uint32_t sprQueue;
    uint8_t ddfStartMatch;
    bool harddisH;
    bool harddisV;
    bool ddfEnableBefore;
    uint8_t bplState;
    bool hardStop;

    bool diwFlipFlop;

    auto frequency() -> unsigned;
    auto ecsAndHigher() -> bool const { return model & (Model::ECS | Model::AGA); }
    auto ecs() -> bool const { return model == Model::ECS; }
    auto aga() -> bool const { return model == Model::AGA; }
    auto womLocked() -> bool const { return (model != OCS_A1000) || womLock; }

    auto useSpriteDMA() -> bool const { return (dmaConImm & 0x220) == 0x220; }
    auto useBlitterDMA() -> bool const { return (dmaCon & 0x240) == 0x240; }
    auto useCopperDMA() -> bool const { return (dmaCon & 0x280) == 0x280; }
    auto useBitplaneDMA() -> bool const { return (dmaConImm & 0x300) == 0x300; }
    auto blitterNasty() -> bool const { return dmaCon & 0x400; }

    auto power(bool softReset) -> void;
    auto powerOff() -> void;
    auto mapMemory() -> void;
    auto setOVL(bool state) -> void;
    auto setChipmem(unsigned size) -> void;
    auto setSlowmem(unsigned size) -> void;

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

    auto setDskPtH(uint16_t value) -> void;
    auto setDskPtL(uint16_t value) -> void;

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
    auto setDiwHigh(uint16_t value) -> void;
    auto lace() const -> bool { return bplCon0 & 4; }

    template<uint8_t nr> auto fetchSample(bool reset) -> void;
    template<uint8_t nr> auto setAudPtH(uint16_t value) -> void;
    template<uint8_t nr> auto setAudPtL(uint16_t value) -> void;

    auto diskDma(bool writeMode) -> void;
    auto fakeDiskDma(uint16_t word) -> void;
    auto fakeDiskDma() -> uint16_t;

    auto observeFrameDuration() -> void;
    auto resetFps() -> void;
    auto updateVCounter() -> void;

    auto serialize(Emulator::Serializer& s, bool light = false) -> void;

    auto rememberChipMem(uint32_t adr) -> void;
    auto rememberSlowMem(uint32_t adr) -> void;
};

}
