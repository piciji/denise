
#pragma once

#include <cstdint>
#include <functional>
#include "../../interface.h"
#include "../disk/disk.h"

namespace Emulator {
    struct Serializer;
}

namespace LIBAMI {

struct Agnus;
struct Cpu;
struct Input;

struct Paula {
    Paula(Agnus& agnus, Cpu& cpu, Input& input);

    using EventCallback = std::function<void(uint8_t, uint16_t)>;

    Agnus& agnus;
    Cpu& cpu;
    Input& input;

    EventCallback callbackStateMachine;

    uint16_t intena;
    uint16_t intreq;
    uint16_t adkcon;

    uint64_t irqDelay;

    bool int2Current;
    bool int6Current;

    bool vBlankIntr;
    bool enableFilter;
    bool useLedFilter;

    struct Drive {
        Emulator::Interface::Media* media;
        Disk disk;

    } drives[4];
    std::vector<Drive*> drivesEnabled;

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
    } pot;

    struct Channel {
        bool dma;
        bool dr;
        bool dsr;
        bool intreq2;

        uint64_t clock;
        uint8_t state;
        uint16_t per;
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
    uint64_t sampleCycle;

    bool dmaDisk;
    bool audioOut;

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
    auto disableAudioOut(bool state) -> void;
    auto setLedFilter(bool state) -> void;
    auto setFilter() -> void;

    auto pot0Dat() -> uint16_t;
    auto pot1Dat() -> uint16_t;
    auto potGo(uint16_t value) -> void;
    auto potGoR() -> uint16_t;
    auto setIntena(uint16_t value) -> void;
    auto getIntena() -> uint16_t;
    auto setIntreq(uint16_t value) -> void;
    auto getIntreq() -> uint16_t;
    auto setAdkCon(uint16_t  value) -> void;
    auto getAdkCon() -> uint16_t ;
    template<uint8_t nr> auto audxDat(uint16_t value) -> void;
    template<uint8_t nr> auto audxLen(uint16_t value) -> void;
    template<uint8_t nr> auto audxPer(uint16_t value) -> void;
    template<uint8_t nr> auto audxVol(uint16_t value) -> void;
    auto dmaCon(uint16_t value) -> void;
    auto strhor() -> void;
    auto strequ() -> void;
    auto strvbl() -> void;

    auto dmal() -> uint16_t; // Paula transfers DMA usage bit by bit to Agnus (clocked each DMA cycle)
    auto setInt2(bool state) -> void;
    auto setInt6(bool state) -> void;
    auto pulseInt3() -> void;

    auto progressPot() -> void;
    auto updateModulation() -> void;
    template<uint8_t nr> auto pbufld1() -> void;
    template<uint8_t nr> auto pbufld2() -> void;
    auto updateInt() -> void;
    auto updateAudioEvent() -> void;
    template<uint8_t nr> auto stateMainloop() -> void;
    template<uint8_t nr> auto percntrld() -> void;
    template<uint8_t nr> auto toggleAudioDMA( ) -> void;
    template<uint8_t nr> auto addSample( uint8_t sample ) -> void;

    auto setResampleQuality( uint8_t val ) -> void;
    auto getResampleQuality( ) -> uint8_t;

    auto calcFilter(float sampleFrequency, unsigned cutoffFrequency) -> float;
    template<uint8_t channel> auto lowPassfilter(int32_t sample) -> int32_t;

    template<uint8_t nr> auto setIntAudMoreDelayed() -> void;
    template<uint8_t nr> auto setIntAud() -> void;
};

}
