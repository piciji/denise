
#pragma once

#include <cstdint>
#include <functional>

namespace Emulator {
    struct Serializer;
}

namespace LIBAMI {

struct System;
struct Agnus;
struct Cpu;
struct Input;
struct DiskDrive;

struct Paula {
    Paula(System* system, Agnus& agnus, Cpu& cpu, Input& input, DiskDrive& disk0, DiskDrive& disk1, DiskDrive& disk2, DiskDrive& disk3);

    enum class DiskState { OFF, WAIT_SYNC_READ, WAIT_SYNC_WRITE, READ, WRITE, INSTANT_BLK_INT } diskState;

    System* system;
    Agnus& agnus;
    Cpu& cpu;
    Input& input;

    uint16_t intena;
    uint16_t intreq;
    uint16_t adkcon;

    bool int2Current;
    bool int6Current;

    bool vBlankIntr;
    bool enableFilter;
    bool useLedFilter;

    DiskDrive& disk0;
    DiskDrive& disk1;
    DiskDrive& disk2;
    DiskDrive& disk3;
    DiskDrive* activeDrive = nullptr;

    uint16_t dskLen;
    uint16_t dskSync;
    uint16_t dskTransferLength;
    uint64_t fifo;
    uint8_t fifoPos;
    int64_t dskEventCycle;
    int64_t dskSyncCycle;
    uint16_t dskShifter;
    uint8_t dskShifterPos;
    int dmaCycles;
    uint16_t dskBytr;

    int ipl;
    int iplCounter;

    int64_t intreqAud0Clock;
    int64_t intreqAud1Clock;
    int64_t intreqAud2Clock;
    int64_t intreqAud3Clock;
    int64_t intreqBltClock;
    int64_t intreqTbeClock;
    int64_t intreqRbfClock;
    int64_t intreqCia1Clock;
    int64_t intreqCia2Clock;

    uint16_t serDat;
    uint16_t serPer;
    int serShifter;
    int receiveShifter;
    int receiveCounter;
    uint16_t serdatR;
    bool loopBack;
    bool rxd;
    bool txd;
    bool overrun;
    int64_t serialTransferEvent;
    int64_t serialReceiveEvent;

    uint8_t turbo = 0;

    struct {
        uint8_t cntX0;
        uint8_t cntY0;
        uint8_t cntX1;
        uint8_t cntY1;

        uint8_t capX0;
        uint8_t capY0;
        uint8_t capX1;
        uint8_t capY1;

        uint16_t go;
        uint8_t dischargeCounter;
        bool running;
    } pot;

    struct Channel {
        bool AUDxON;
        bool dr;
        bool dsr;
        bool intreq2;

        int64_t percount;
        uint8_t state;
        uint16_t perLatch;
        uint16_t len;
        uint16_t lenLatch;
        int8_t vol;
        uint8_t volLatch;
        uint16_t dat;
        uint16_t buffer;
        int16_t sample;

        bool audav;
        bool audap;
        bool napnav;
    } channels[4];

    uint8_t sampleLimit;
    int64_t sampleCycle;

    int64_t intUpdClock;
    uint16_t intreqLast;

    bool dmaDisk;
    bool audioOut = true;

    struct Filter {
        float rc1, rc2, rc3, rc4, rc5;

        auto reset() -> void {
            rc1 = rc2 = rc3 = rc4 = rc5 = 0.0f;
        }
    } filters[2];

    float filter1A0;
    float filter2A0;
    float filterA0;

    auto process() -> void;
    auto power() -> void;
    auto powerOff() -> void;
    auto serialize(Emulator::Serializer& s, bool light = false) -> void;
    auto disableAudioOut(bool state) -> void { audioOut = !state; }
    auto setLedFilter(bool state) -> void;
    auto setFilter() -> void;
    auto audioEvent() -> void;

    auto pot0Dat() -> uint16_t;
    auto pot1Dat() -> uint16_t;
    auto potGo(uint16_t value) -> void;
    auto potGoR() -> uint16_t;
    auto setIntena(uint16_t value) -> void;
    auto getIntena() -> uint16_t;
    auto setIntreq(uint16_t value) -> void;
    auto getIntreq() -> uint16_t;
    auto setAdkCon(uint16_t  value) -> void;
    auto getAdkCon() -> uint16_t;
    auto getDskBytR() -> uint16_t;
    auto wordSync() -> bool const { return adkcon & 0x400; }
    auto msbSync() -> bool const { return adkcon & 0x200; }
    auto fast() -> bool const { return adkcon & 0x100; }
    auto useInstantDriveAccess() -> bool { return turbo == 4; }
    auto instantDriveAccess() -> void;
    auto finishDMA(bool delayed = false) -> void;

    template<uint8_t nr> auto audxDat(uint16_t value) -> void;
    template<uint8_t nr> auto audxLen(uint16_t value) -> void;
    template<uint8_t nr> auto audxPer(uint16_t value) -> void;
    template<uint8_t nr> auto audxVol(uint16_t value) -> void;
    auto dmaCon(uint16_t value) -> void;
    auto strhor() -> void;
    auto strequ() -> void;
    auto strvbl() -> void;

    auto dmal() -> uint16_t; // Paula transfers DMA usage bit by bit to Agnus (clocked each DMA cycle)
    auto scheduleIntreqCia1(bool state) -> void;
    auto scheduleIntreqCia2(bool state) -> void;
    auto setDskSyncInt() -> void;
    auto setDskBlkInt() -> void;
    auto setVblInt() -> void;

    auto prepareIpl() -> void;
    auto intreqEvent() -> void;
    template<uint8_t nr> auto scheduleIntreqAud() -> void;
    auto scheduleIntreqBlt() -> void;
    auto scheduleIntreqTbe() -> void;
    auto scheduleIntreqRbf() -> void;

    auto setDskLen(uint16_t value) -> void;
    auto setDskDat(uint16_t value) -> void;
    auto setDskSync(uint16_t value) -> void;
    auto dskDatR() -> uint16_t;
    auto setFdcEvent() -> void;
    auto fdcWriteMode() -> bool { return diskState == DiskState::WRITE || diskState == DiskState::WAIT_SYNC_WRITE; }
    auto setDskState(DiskState next) -> void;
    auto processDiskIdleCycles() -> void;

    auto getFromFifo(uint16_t& data) -> bool;
    auto addToFifo(uint16_t data) -> void;

    template<bool readWord = false, bool waitTurbo = false> auto handleFDControllerRead() -> void;
    auto handleFDControllerWrite() -> void;

    auto progressPot() -> void;
    auto updateModulation() -> void;
    template<uint8_t nr> auto pbufld1() -> void;
    template<uint8_t nr> auto pbufld2() -> void;
    template<uint8_t nr> auto perfin() -> void;
    template<uint8_t nr, bool updEvent> auto percntrld() -> void;
    template<uint8_t nr> auto toggleAudioDMA( ) -> void;
    template<uint8_t nr> auto addSample( uint8_t sample ) -> void;

    auto setResampleQuality( int val ) -> void;
    auto getResampleQuality( ) -> int;

    auto calcFilter(float sampleFrequency, unsigned cutoffFrequency) -> float;
    template<uint8_t channel> auto lowPassfilter(int32_t sample) -> int32_t;

    auto serialEvent() -> void;
    auto getSerdatR() -> uint16_t;
    auto setSerdat(uint16_t value) -> void;
    auto setSerper(uint16_t value) -> void;
    auto prepareTransfer() -> void;
    auto updateTxd() -> void;
    auto updateSerialEvent() -> void;
};

}
