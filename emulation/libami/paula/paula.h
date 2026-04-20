
#pragma once

#include <cstdint>

namespace Emulator {
    struct Serializer;
}

namespace LIBAMI {

struct System;
struct Agnus;
struct Cpu;
struct Input;
struct DiskDrive;
struct DebuggerSnapshot;

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
    int filterMode;
    bool useLedFilter;
    uint8_t ledChange;

    DiskDrive& disk0;
    DiskDrive& disk1;
    DiskDrive& disk2;
    DiskDrive& disk3;
    DiskDrive* activeDrive = nullptr;

    uint16_t dskLen;
    uint16_t dskSync;
    bool wordEqual;
    uint16_t dskTransferLength;
    uint64_t fifo;
    uint8_t fifoPos;
    int64_t dskSyncCycle;
    uint16_t dskShifter;
    uint8_t dskShifterPos;
    int fdcCycles;
    uint16_t dskBytr;
    bool fifoReady;

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

    uint8_t turboRequested = 0;
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
    uint8_t sampleCounter;
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

    auto power() -> void;
    auto powerOff() -> void;
    auto serialize(Emulator::Serializer& s, bool light = false) -> void;
    auto disableAudioOut(bool state) -> void { audioOut = !state; }
    auto setLedFilter(bool state) -> void;
    auto informPowerLED(bool force) -> void;
    auto setFilter() -> void;
    auto setFilterMode( int val ) -> void;
    auto audioEvent() -> void;

    auto pot0Dat() -> uint16_t;
    auto peekPot0Dat() -> uint16_t;
    auto pot1Dat() -> uint16_t;
    auto peekPot1Dat() -> uint16_t;
    auto potGo(uint16_t value) -> void;
    auto peekPotGoR() -> uint16_t;
    auto potGoR() -> uint16_t;
    auto potOutput(uint16_t data) -> void;
    auto potReset() -> void;
    auto potProgress() -> void;

    auto setIntena(uint16_t value) -> void;
    auto getIntena() -> uint16_t;
    auto setIntreq(uint16_t value) -> void;
    auto getIntreq() -> uint16_t;
    auto setAdkCon(uint16_t  value) -> void;
    auto getAdkCon() -> uint16_t;
    auto getDskBytR() -> uint16_t;
    auto peekDskBytR() -> uint16_t;
    auto wordSync() -> bool const { return adkcon & 0x400; }
    auto msbSync() -> bool const { return adkcon & 0x200; }
    auto fast() -> bool const { return adkcon & 0x100; }
    auto useInstantDriveAccess() -> bool { return turbo == 4; }
    auto instantDriveAccess() -> void;
    auto finishDMA() -> void;

    template<uint8_t nr> auto audxDat(uint16_t value) -> void;
    template<uint8_t nr> auto audxLen(uint16_t value) -> void;
    template<uint8_t nr> auto audxPer(uint16_t value) -> void;
    template<uint8_t nr> auto audxVol(uint16_t value) -> void;
    auto dmaCon(uint16_t value) -> void;

    auto dmal() -> uint16_t; // Paula transfers DMA usage bit by bit to Agnus (clocked each DMA cycle)
    auto scheduleIntreqCia1(bool state) -> void;
    auto scheduleIntreqCia2(bool state) -> void;
    auto setDskSyncInt() -> void;
    auto setExternalInt() -> void;
    auto setDskBlkInt() -> void;
    auto setVblInt() -> void;

    auto prepareIpl() -> void;
    auto intreqEvent() -> void;
    template<uint8_t nr> auto scheduleIntreqAud() -> void;
    auto scheduleIntreqBlt() -> void;
    auto scheduleIntreqTbe() -> void;
    auto scheduleIntreqRbf() -> void;

    auto setDskLen(uint16_t value) -> void;
    auto setDskDat(uint16_t& value) -> bool;
    auto setDskSync(uint16_t value) -> void;
    auto dskDatR(uint8_t slot, uint16_t& out) -> bool;
    auto fdcWriteMode() -> bool { return diskState == DiskState::WRITE || diskState == DiskState::WAIT_SYNC_WRITE; }
    auto setDskState(DiskState next) -> void;
    auto handleFDControllerIdle() -> void;
    auto diskEvent() -> void;
    auto setActiveDrive(DiskDrive* drive) -> void;
    auto setTurbo(int value) -> void;
    auto fdcByteMode() -> bool;

    auto getFromFifo(uint16_t& data) -> bool;
    auto addToFifo(const uint16_t& data) -> bool;
    auto fifoFull() -> bool { return fifoPos == 3; }
    auto fifoEmpty() -> bool { return fifoPos == 0; }

    template<bool readWord = false, bool waitTurbo = false> auto handleFDControllerRead() -> void;
    template<bool readWord = false> auto handleFDControllerReadByte() -> void;
    auto handleFDControllerWrite() -> void;
    
    auto updateModulation() -> void;
    template<uint8_t nr> auto pbufld1() -> void;
    template<uint8_t nr> auto pbufld2() -> void;
    template<uint8_t nr> auto perfin() -> void;
    template<uint8_t nr, bool updEvent> auto percntrld() -> void;
    template<uint8_t nr> auto toggleAudioDMA( ) -> void;
    template<uint8_t nr> auto addSample( uint8_t sample ) -> void;

    auto setResampleQuality( int val ) -> void;
    auto getResampleQuality( ) -> int;

    auto calcFilter(double sampleFrequency, unsigned cutoffFrequency) -> float;
    template<uint8_t channel> auto lowPassfilter(int32_t sample, int filterMode) -> int32_t;

    auto serialEvent() -> void;
    auto getSerdatR() -> uint16_t;
    auto peekSerdatR() -> uint16_t;
    auto setSerdat(uint16_t value) -> void;
    auto setSerper(uint16_t value) -> void;
    auto prepareTransfer() -> void;
    auto updateTxd() -> void;
    auto updateSerialEvent() -> void;
    auto updateSnapshot(DebuggerSnapshot& snap) -> void;
    
    auto iplUpdate() -> void;
    template<bool force = true> auto sampleUpdate() -> void;
};

}
