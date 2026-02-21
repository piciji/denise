
#pragma once

#include "blitter.h"
#include "copper.h"
#include "../../cia/new/cia.h"
#include "../../tools/powersupply.h"
#include "../expansionPort/fastMem.h"
#include "../expansionPort/hdController.h"
#include "../../tools/history.h"
#include "../cpu/m68000/dasmHandler.h"
#include "../cpu/m68000/m68000.h"
#include "../system/debuggerSnapshot.h"
#include "../../tools/crop.h"

/**
 * todos:
 * AGA
 *
 * variable vsync, vblank, hsync, hblank, hcenter
 * UHRES/DUAL stuff
 */

namespace LIBAMI {

struct System;
struct Interface;
struct Cpu;
struct Denise;
struct Paula;
struct Input;
struct RTC;

#define TRACKER_CHIP 0
#define TRACKER_SLOW 1
#define TRACKER_FAST 2

struct Agnus {

    Agnus(System* system, Cpu& cpu, Denise& denise, Paula& paula, Cia<MOS_8520>& cia1, Cia<MOS_8520>& cia2, Input& input, RTC& rtc);
    ~Agnus();

    enum { Unmapped, CHIP_MEM, SLOW_MEM, KICK_ROM, EXT_ROM, WOM,
        MMIO_CUSTOM, MMIO_CIA, MMIO_RTC, AUTO_CONF, FAST_MEM, EXPANSION };

    enum { EVENT_ONE_CYCLE_DELAY, EVENT_AUDIO_STATE, EVENT_HTOTAL, EVENT_INTREQ, EVENT_RARELY,
        EVENT_KBD, EVENT_LEAVE_EMULATION, EVENT_POWER_SUPPLY, EVENT_SERIAL, EVENT_FLOPPY, EVENT_BLANKEN,
        EVENT_DEBUGGER, EVENT_CHANNELS };

    enum { BLT_INIT = 0, DMACON = 1,
        PTR_BLT_A_H, PTR_BLT_A_L, PTR_BLT_B_H, PTR_BLT_B_L, PTR_BLT_C_H, PTR_BLT_C_L, PTR_BLT_D_H, PTR_BLT_D_L,
        PTR_DSK_H, PTR_DSK_L,
        SPR_DATA0, SPR_DATA1, SPR_DATA2, SPR_DATA3, SPR_DATA4, SPR_DATA5, SPR_DATA6, SPR_DATA7,
        SPR_DATB0, SPR_DATB1, SPR_DATB2, SPR_DATB3, SPR_DATB4, SPR_DATB5, SPR_DATB6, SPR_DATB7,
        SPR_CTL0, SPR_CTL1, SPR_CTL2, SPR_CTL3, SPR_CTL4, SPR_CTL5, SPR_CTL6, SPR_CTL7,
        SPR_POS0, SPR_POS1, SPR_POS2, SPR_POS3, SPR_POS4, SPR_POS5, SPR_POS6, SPR_POS7,
        BPL_CON2, INTREQ, INTENA, BPL_CON0, BPL_CON1, BPL_MOD1, BPL_MOD2,
        AUD_LEN0, AUD_LEN1, AUD_LEN2, AUD_LEN3,
        AUD_PER0, AUD_PER1, AUD_PER2, AUD_PER3,
        AUD_DAT0, AUD_DAT1, AUD_DAT2, AUD_DAT3,
        DMACON_3, SER_DAT, DMACON_1, DIW_START, DIW_STOP, BLT_MODA, BLT_MODB, BLT_MODC, BLT_MODD,
        VPOSW, VHPOSW, UPD_V_DIW, UPD_DENISE_VHPOS, STROBE, END_BLT_CONFLICT, BPL_CON3, DIW_HIGH,
        STROBE2,
    };

    enum { ACT_BLITTER = 1, ACT_COPPER = 2, ACT_BPL = 4, ACT_SPRITE = 8, ACT_IPLCOUNTER = 0x10 };

    enum {
        BUS_FREE, BUS_USAGE_BPL, BUS_USAGE_SPRITE, BUS_USAGE_BLITTER, BUS_USAGE_COPPER,
        BUS_USAGE_CPU, BUS_USAGE_REFRESH, BUS_USAGE_DISK, BUS_USAGE_AUDIO,
        BUS_USAGE_BLITTER_CONFLICT_COPPER, BUS_USAGE_BLITTER_CONFLICT_SPRITE,
        BUS_USAGE_BPL_CONFLICT_REFRESH, BUS_USAGE_BPL_CONFLICT_SPRITE,
    };

    enum { PAL, NTSC };

    enum { Trigger_Read, Trigger_CPU, Trigger_Copper, Trigger_Vsync };

    enum Model { OCS_A1000 = 1, OCS = 2, ECS = 4, AGA = 8 } model = OCS;

    System* system;
    Interface* interface;
    Cpu& cpu;
    Denise& denise;
    Paula& paula;
    RTC& rtc;
    Input& input;
    Cia<MOS_8520>& cia1;
    Cia<MOS_8520>& cia2;

    FastMemExpansion fastMemExpansion;
    ExpansionPort* expansions[5];
    ExpansionPort* expansionsConfigured[0x100];
    bool hardDrivesBusy;

    uint16_t vPosLocked;
    bool hPosLocked;
    uint8_t* encryptedRom;

    Emulator::PowerSupply powerSupply;
    Blitter blitter;
    Copper copper;

    int64_t eventClock[EVENT_CHANNELS];
    int64_t clock;
    int64_t nextClock;

    struct RapidJob {
        int job;
        uint16_t data;
        int64_t clock;

        auto reset() -> void {
            job = -1;
            clock = INT64_MAX;
        }
    } rJob1, rJob2, rJob3;

    bool hTotalFirst;

    int actions = 0;
    uint8_t mapper[256] = {0};
    int busUsage;
    int64_t dmaClock;
    uint8_t hPos;
    uint16_t vPos;
    uint16_t vStart;
    uint16_t vStop;
    uint16_t dmal;
    double fps;
    uint64_t frameClock;
    uint8_t fpsChange;

    bool hBlank;
    bool vBlankEnd;
    bool vBlankEndNext;
    bool vBlank;
    bool vBlankStart;
    bool sprInhibited;
    uint8_t hsStrt;
    uint8_t hsStop;
    uint8_t hBStrt;
    uint8_t hBStop;

    uint16_t lines;
    uint16_t vTotal;
    uint16_t vBStrt;
    uint16_t vBStop;
    uint8_t hTotal;
    uint16_t beamCon;
    bool secureRA;

    struct {
        unsigned speed;
        unsigned cycles;
    } overclock;

    struct Debugger {
        Emulator::Interface::DebuggerAction action = Emulator::Interface::DebuggerAction::None;
        uint32_t addr;

        bool dmaLog = false;
        bool requestDmaLog = false;
        bool dmaView = false;

        uint32_t dmaWatchers[4] = { 0 };

        uint8_t* frameLine = nullptr;
        Emulator::Interface::DebuggerDma dma[256];

        uint8_t* dmaFrame = nullptr;
        uint8_t lastHpos = 0xe3;

        Emulator::Interface::DebuggerAction oneTimeAction;
        unsigned stopLine = ~0;

        Emulator::Crop<uint16_t> crop;

        int scrollDirection = 0;
        unsigned scrollCounter = 0;
        auto enableDmaView(bool state, bool withScrolling = true) -> void;
        auto enableDmaLog(bool state) -> void;
    } debugger;

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

    MemState<uint32_t, uint16_t, 3>* memState;

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
    unsigned dmaChipMemMask = 0;

    uint8_t* slowMem = nullptr;
    unsigned slowMemSize = 0;
    uint8_t* fastMem = nullptr;
    unsigned fastMemSize = 0;
    uint8_t* kickRom = nullptr;
    unsigned kickRomSize = 0;
    unsigned kickRomMask = 0;
    uint8_t* extRom = nullptr;
    unsigned extRomSize = 0;
    unsigned extRomMask = 0;
    uint8_t* wom = nullptr;

    bool useRTC = false;
    uint16_t dataBus = 0;
    uint32_t addrBus = 0;
    uint16_t dmaCon;
    uint16_t dmaConImm;
    bool dmaConCop;
    bool dmaConBlt;
    bool dmaConSpr;
    uint16_t bplCon0;
    unsigned countWaitCycles;
    uint32_t rDmaPtr;
    uint32_t rasAdder;

    bool blitterConflict;

    int64_t eClockCycle;
    bool lol;
    bool lof;
    bool lolToggle;
    bool ntsc;
    uint8_t laceMode;
    uint8_t laceFrame;
    int lineVCounter;
    int vBlankOffset;
    uint16_t* frameBuffer;
    bool hTotalChanged = false;

    struct {
        int left;
        int right;
        int top;
        int bottom;

        auto reset() -> void {
            left = right = top = bottom = 0;
        }
    } crop;

    bool initVCounter;
    bool shortLineBefore;
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

    bool womLock = false;
    uint8_t resetFromKeyboard = 0;
    bool ddfEnableCache;

    auto frequency() const -> unsigned;
    auto ecsAndHigher() const -> bool { return model & (Model::ECS | Model::AGA); }
    auto ecs() -> bool const { return model == Model::ECS; }
    auto aga() -> bool const { return model == Model::AGA; }
    auto a1000() -> bool const { return model == Model::OCS_A1000; }
    auto womLocked() -> bool const { return (model != OCS_A1000) || womLock; }
    auto setRas() -> void;
    template<uint8_t slot> auto refreshCycle() -> void;

    auto useSpriteDMA() -> bool const { return (dmaConImm & 0x220) == 0x220; }
    auto useBlitterDMA() -> bool const { return (dmaConImm & 0x240) == 0x240; }
    auto useBlitterDMAForQueue() -> bool const { return dmaConBlt; }
    auto useCopperDMA() -> bool const { return (dmaConImm & 0x280) == 0x280; }
    auto useCopperDMAForQueue() -> bool const { return dmaConCop; }
    auto useBitplaneDMA() -> bool const { return (dmaConImm & 0x300) == 0x300; }
    auto blitterNasty() -> bool const { return dmaCon & 0x400; }

    auto power(bool softReset, bool resetInstruction) -> void;
    auto powerOff() -> void;
    auto mapMemory() -> void;
    auto mapRom(bool init = true) -> void;
    auto setOVL(bool state) -> void;
    auto lockWom() -> void;
    auto setChipmem(unsigned size) -> void;
    auto setSlowmem(unsigned size) -> void;
    auto setFastmem(unsigned size) -> void;
    auto createChipSlowMem(unsigned sizeChip, unsigned sizeSlow) -> void;

    auto readAutoConf(uint32_t addr) -> uint8_t;
    auto writeAutoConf(uint32_t addr, uint8_t value) -> void;
    auto writeAutoConfWord(uint32_t addr, uint16_t value) -> void;
    auto checkHardDrives() -> void;

    auto readByte(uint32_t adr) -> uint8_t;
    template<bool logDma> auto readByte(uint32_t adr) -> uint8_t;
    auto writeByte(uint32_t adr, uint8_t value) -> void;
    template<bool logDma> auto writeByte(uint32_t adr, uint8_t value) -> void;
    auto readWord(uint32_t adr) -> uint16_t;
    template<bool logDma> auto readWord(uint32_t adr) -> uint16_t;
    auto peekWord(uint32_t adr) -> uint16_t;
    auto writeWord(uint32_t adr, uint16_t value) -> void;
    auto editWord(uint32_t adr, uint16_t value) -> void;
    template<bool logDma> auto writeWord(uint32_t adr, uint16_t value) -> void;
    auto memoryDump(uint8_t bank, uint16_t* dump) -> void;

    auto fakeWriteByte(uint32_t adr, uint8_t value) -> void;
    auto fakeWriteByte(uint32_t adr, uint8_t* data, unsigned len) -> void;
    auto fakeWriteLongWord(uint32_t adr, uint32_t value) -> void;
    auto fakeWriteWord(uint32_t adr, uint16_t value) -> void;
    
    auto fakeRead(uint32_t adr, uint8_t* buf, unsigned len) -> void;
    auto fakeReadByte(uint32_t adr) -> uint8_t;
    auto fakeReadWord(uint32_t adr) -> uint16_t;
    auto fakeReadLongWord(uint32_t adr) -> uint32_t {
        return (fakeReadWord(adr) << 16) | fakeReadWord(adr + 2);
    }

    auto sync(unsigned cycles) -> void;
    template<bool logDma> auto sync(unsigned cycles) -> void;
    auto dmaCycle() -> void;
    template<bool logDma> auto addWaitstatesToCPU() -> void;
    auto iackCycle(uint8_t level, uint8_t& vector) -> int;
    auto resetOut() -> void;
    auto pullResetLine(bool state = true) -> void;
    auto setLines() -> void;

    constexpr static auto msecToDMACycles(const unsigned ms) -> unsigned { return 3550 * ms; } // average for PAL/NTSC, todo: check if more accuracy is needed
    auto usecToDMACycles(unsigned us) -> unsigned { return 3.55f * (float)us + 0.5f; }

    auto dmaCyclesToSec(int64_t cycles) -> unsigned { return cycles / 3'550'000; }

    auto writeCustom(uint16_t adr, uint16_t value, uint8_t triggeredBy = Trigger_CPU) -> void;
    template<bool byteAccess = false> auto readCustom(uint16_t adr, bool triggeredByWrite = false) -> uint16_t;
    auto peekCustom(uint16_t addr) -> uint16_t;

    inline auto canBlitterUseBus() -> bool;
    auto canBlitterUseBusExt() -> bool;
    template<bool oddCycle1 = false> auto canCopperUseBus() -> bool;
    template<bool oddCycle1 = false> auto allocateCopper() -> bool;
    auto fetchCopperDma(uint32_t adr, uint16_t& result) -> bool;
    auto fetchCopperDmaNoBUSCheck(uint32_t adr, uint16_t& result) -> void;
    auto checkCopperBlitterConflict(uint32_t& ptr) -> bool;

    template<uint8_t ptrEvent, bool desc, bool add, bool mod, bool check = true> auto fetchBlitterDma(uint32_t& adr, uint16_t& result, const int16_t& modVal = 0) -> bool;
    template<uint8_t ptrEvent, bool desc, bool add, bool mod, bool writeMode> auto handleBlitterConflicts(uint32_t& adr, uint16_t& result, const int16_t& modVal) -> void;
    template<bool desc, bool add, bool mod, bool check = true> auto writeBlitterDma(uint32_t& adr, uint16_t& value, const int16_t& modVal = 0) -> bool;

    auto setRefPtr(uint16_t value) -> void;

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
    template<uint8_t pos, bool addMod, bool strobe> auto fetchPlaneConflict() -> void;
    template<uint8_t pos, bool addMod> auto fetchPlaneSprConflict() -> void;
    template<uint8_t nr, bool first> auto spriteControl() -> void;
    template<bool _ecs, bool start> auto bplControl() -> void;
    auto fetchSprites() -> void;
    template<uint8_t nr, uint8_t options> auto fetchSprite() -> void;
    template<uint8_t nr> auto updateSpriteV() -> void;
    template<uint8_t nr, bool regular> auto checkSpriteV() -> void;
    template<uint8_t nr> auto setSprPos(uint16_t value) -> void;
    template<uint8_t nr> auto setSprCtl(uint16_t value) -> void;

    template<uint8_t num> auto setSprptH(uint16_t value) -> void;
    template<uint8_t num> auto setSprptL(uint16_t value) -> void;
    auto updateHarddis() -> void;
    auto isEquLine() -> bool;
    template<int mode = 0> auto updateVdiw() -> void;
    auto setDiwStrt(uint16_t value) -> void;
    auto setDiwStop(uint16_t value, bool triggerCopper) -> void;
    auto setDiwHigh(uint16_t value, bool triggerCopper) -> void;
    auto lace() const -> bool { return bplCon0 & 4; }

    template<uint8_t nr> auto fetchSample(bool reset) -> void;
    template<uint8_t nr> auto setAudPtH(uint16_t value) -> void;
    template<uint8_t nr> auto setAudPtL(uint16_t value) -> void;

    auto diskDma(uint8_t slot, bool writeMode) -> void;
    auto fakeDiskDma(uint16_t& word) -> void;
    auto fakeDiskDmaNoTracking(uint16_t word) -> void;
    auto fakeDiskDma() -> uint16_t;

    auto observeFrameDuration() -> void;
    auto resetFps() -> void;
    auto updateVCounter() -> void;

    auto serialize(Emulator::Serializer& s, bool light = false) -> void;

    template<uint8_t Channel> auto updateEvent(int delay) -> void;
    template<uint8_t Channel> auto updateEventAbs(int64_t absClock) -> void;
    template<uint8_t Channel> auto setEventInactive() -> void {
        eventClock[Channel] = INT64_MAX;
    }
    auto fallBackCycles( int64_t lastClock ) -> int64_t {
        return clock - lastClock;
    }
    template<uint8_t Channel> auto hasActiveEvent() -> bool {
        return eventClock[Channel] != INT64_MAX;
    }

    auto processEvents(int64_t curClock) -> void;
    auto clearEvents() -> void;
    template<bool tooSoon = false> auto processOneCycleEvent(RapidJob& rJob) -> void;
    template<uint8_t Channel> auto getEventDelay() -> unsigned;
    auto setBltConflictThisCycle() -> void;
    inline auto addOneCycleEvent(int job, uint16_t data = 0, int delay = 2) -> void;
    auto forceOneCycleEvent(int job) -> void;
    auto getOneCycleEvent(int job) -> RapidJob*;
    template<bool isPtr> auto inactivateOneCycleEvent(int job) -> void;
    inline auto updateOneCycleEvent() -> void;
    auto powerSupplyEvent() -> void;
    auto leaveEmulationEvent() -> void;
    auto HTotalEvent() -> void;

    auto vposw(uint16_t value) -> void;
    auto vhposw(uint16_t value) -> void;

    auto startHblank() -> void;
    auto startHblankDebug() -> void;

    auto endHblank() -> void;
    auto startHsync() -> void;

    template<bool quadruple> auto doubleResMidframe(bool fromHires) -> void;
    template<bool quadruple> auto doubleResMidDebugframe(bool fromHires) -> void;
    template<typename T> static auto doublePixel(T* _ptr, unsigned _xStart) -> void;
    template<typename T> static auto quadruplePixel(T* _ptr, unsigned _xStart) -> void;

    auto updateCropTop() -> void;
    auto updateCropBottom() -> void;
    auto updateCropLeft(int pos) -> void;
    auto updateCropRight(int pos) -> void;

    auto sanitizeCrop(int width, int height) -> void;
    auto checkForRomEncryption() -> void;

    auto getSprConflictReg(uint8_t encoded) -> uint16_t;

    auto isChipMem(uint32_t addr) -> bool;
    auto isSlowMem(uint32_t addr) -> bool;
    auto isFastMem(uint32_t addr) -> bool;
    auto isMem(uint32_t addr) -> bool;
    auto csyncPolTrue(bool state) -> bool;

    auto blanken() -> void;

    auto getMemorySize() -> unsigned {
        unsigned _size = chipMemMask + 1 + slowMemSize + fastMemSize;
        if (model == Agnus::OCS_A1000)
            _size += 256 * 1024; // WOM
        return _size;
    }

    inline auto updateDdfEnableCache() -> void;

    auto debugPointReached(int source, unsigned addr) -> void;
    auto oneTimeDebuggerAction() -> void;
    auto updateSnapshot(DebuggerSnapshot& snap) -> void;
    auto updateVideoSnapshot(DebuggerSnapshot& snap) -> void;
    auto updateDmaSnapshot(DebuggerSnapshot& snap) -> void;
    auto updateMemorySnapshot(DebuggerSnapshot& snap) -> void;

    auto addDmaLogEntry() -> void;
    auto peekDmaWatcher(Emulator::Interface::DebuggerDma& dmaLogger) -> void;
    auto debuggerAutoUpdate() -> void;
    auto debuggerUpdateEvent() -> void;
};

}
