
#pragma once

#include "blitter.h"
#include "copper.h"
#include "../../cia/new/cia.h"
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
struct RTC;

struct Agnus {

    Agnus(System* system, Cpu& cpu, Denise& denise, Paula& paula, Cia<MOS_8520>& cia1, Cia<MOS_8520>& cia2, Input& input, RTC& rtc);
    ~Agnus();

    enum { Unmapped, CHIP_MEM, SLOW_MEM, KICK_ROM, EXT_ROM, WOM, MMIO_CUSTOM, MMIO_CIA, MMIO_RTC, AUTO_CONF, FAST_MEM };

    enum { EVENT_KBD, EVENT_ONE_CYCLE_DELAY, EVENT_LEAVE_EMULATION, EVENT_POWER_SUPPLY, EVENT_AUDIO_STATE,
            EVENT_HTOTAL, EVENT_SERIAL, EVENT_INTREQ, EVENT_FLOPPY, EVENT_CHANNELS };

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
        VPOSW, VHPOSW, UPD_V_DIW, UPD_DENISE_VHPOS,
    };

    enum { ACT_BLITTER = 1, ACT_COPPER = 2, ACT_BPL = 4, ACT_SPRITE = 8 };

    enum { BUS_FREE, BUS_USAGE_BPL, BUS_USAGE_SPRITE, BUS_USAGE_BLITTER, BUS_USAGE_COPPER, BUS_USAGE_CPU, BUS_USAGE_REFRESH, BUS_USAGE_DMAL };

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
        RapidJob* sibling;
        int job;
        uint16_t data;
        int64_t clock;

        auto reset() -> void {
            job = -1;
            clock = INT64_MAX;
        }
    } rapidJobs[2];

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
    MemChange* fastMemChange;
    unsigned chipMemChangeSize;
    unsigned slowMemChangeSize;
    unsigned fastMemChangeSize;
    unsigned chipMemChangePos;
    unsigned slowMemChangePos;
    unsigned fastMemChangePos;
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
    uint16_t dmaCon;
    uint16_t dmaConImm;
    bool dmaConCop;
    bool dmaConBlt;
    bool dmaConSpr;
    uint16_t bplCon0;
    unsigned countWaitCycles;
    uint32_t rDmaPtr;

    int64_t eClockCycle;
    bool lol;
    bool lof;
    bool lolToggle;
    bool ntsc;
    uint8_t laceMode;
    uint8_t laceFrame;
    int lineVCounter;
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

    struct {
        bool use;
        unsigned line;
        bool called;
    } lineCallback;

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
    uint32_t zorroBaseAdr = 0;

    auto frequency() const -> unsigned;
    auto ecsAndHigher() const -> bool { return model & (Model::ECS | Model::AGA); }
    auto ecs() -> bool const { return model == Model::ECS; }
    auto aga() -> bool const { return model == Model::AGA; }
    auto a1000() -> bool const { return model == Model::OCS_A1000; }
    auto womLocked() -> bool const { return (model != OCS_A1000) || womLock; }

    auto useSpriteDMA() -> bool const { return (dmaConImm & 0x220) == 0x220; }
    auto useBlitterDMA() -> bool const { return (dmaConImm & 0x240) == 0x240; }
    auto useBlitterDMAForQueue() -> bool const { return dmaConBlt; }
    auto useCopperDMA() -> bool const { return (dmaConImm & 0x280) == 0x280; }
    auto useCopperDMAForQueue() -> bool const { return dmaConCop; }
    auto useBitplaneDMA() -> bool const { return (dmaConImm & 0x300) == 0x300; }
    auto blitterNasty() -> bool const { return dmaCon & 0x400; }

    auto power(bool softReset) -> void;
    auto powerOff() -> void;
    auto mapMemory() -> void;
    auto mapRom(bool init = true) -> void;
    auto setOVL(bool state) -> void;
    auto lockWom() -> void;
    auto setChipmem(unsigned size) -> void;
    auto setSlowmem(unsigned size) -> void;
    auto setFastmem(unsigned size) -> void;
    auto createChipSlowMem(unsigned sizeChip, unsigned sizeSlow) -> void;

    auto readZorro(uint32_t adr) -> uint8_t;
    auto writeZorro(uint32_t adr, uint8_t data) -> void;
    auto getZorroSize() -> uint8_t;

    auto readByte(uint32_t adr) -> uint8_t;
    auto writeByte(uint32_t adr, uint8_t value) -> void;
    auto readWord(uint32_t adr) -> uint16_t;
    auto writeWord(uint32_t adr, uint16_t value) -> void;
    auto sync(unsigned cycles) -> void;
    auto dmaCycle() -> void;
    auto addWaitstatesToCPU() -> void;
    auto iackCycle(uint8_t level, uint8_t& vector) -> int;
    auto resetOut() -> void;
    auto pullResetLine(bool state = true) -> void;

    auto msecToDMACycles(unsigned ms) -> unsigned { return 3550 * ms; } // average for PAL/NTSC, todo: check if more accuracy is needed
    auto usecToDMACycles(unsigned us) -> unsigned { return 3.55f * (float)us + 0.5f; }

    auto dmaCyclesToSec(int64_t cycles) -> unsigned { return cycles / 3'550'000; }

    auto writeCustom(uint16_t adr, uint16_t value, uint8_t triggeredBy = Trigger_CPU) -> void;
    template<bool byteAccess = false> auto readCustom(uint16_t adr, bool triggeredByWrite = false) -> uint16_t;

    auto canBlitterUseBus() -> bool;
    template<bool oddCycle1 = false> auto canCopperUseBus() -> bool;
    template<bool oddCycle1 = false> auto allocateCopper() -> bool;
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
    template<uint8_t nr, bool first> auto spriteControl() -> void;
    auto bplControl() -> void;
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

    auto diskDma(bool writeMode) -> void;
    auto fakeDiskDma(uint16_t word) -> void;
    auto fakeDiskDmaNoTracking(uint16_t word) -> void;
    auto fakeDiskDma() -> uint16_t;

    auto observeFrameDuration() -> void;
    auto resetFps() -> void;
    auto updateVCounter() -> void;

    auto serialize(Emulator::Serializer& s, bool light = false) -> void;

    auto rememberChipMem(uint32_t adr) -> void;
    auto rememberSlowMem(uint32_t adr) -> void;
    auto rememberFastMem(uint32_t adr) -> void;
    auto increaseTrackMemStorage(MemChange*& memChange, unsigned& size) -> void;

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
    auto processOneCycleEvent(RapidJob& rJob) -> void;
    template<uint8_t Channel> auto getEventDelay() -> unsigned;
    inline auto addOneCycleEvent(int job, uint16_t data = 0, int delay = 2) -> void;
    auto forceOneCycleEvent(int job) -> void;
    template<bool isPtr> auto inactivateOneCycleEvent(int job) -> void;
    auto powerSupplyEvent() -> void;
    auto leaveEmulationEvent() -> void;
    auto HTotalEvent() -> void;

    auto vposw(uint16_t value) -> void;
    auto vhposw(uint16_t value) -> void;

    auto logDmaUsage(bool waitForCpu = false) -> void;
    auto logDmaCondition() -> bool;

    auto startHblank() -> void;
    auto endHblank() -> void;
    auto startHsync() -> void;

    auto switchToHiresMidframe() -> void;
    inline auto doubleLoresPixel(uint16_t* _ptr, unsigned _xStart) -> void;

    auto updateCropTop() -> void;
    auto updateCropBottom() -> void;
    auto updateCropLeft(int pos) -> void;
    auto updateCropRight(int pos) -> void;

    auto sanitizeCrop(int width, int height) -> void;
    auto checkForRomEncryption() -> void;
};

}
