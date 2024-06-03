
#pragma once

#include <functional>

#include "../../tools/serializer.h"
#include "../../tools/macros.h"

#define MOS_6526 0
#define MOS_8520 1

template<uint8_t model>
struct Cia {
    Cia( uint8_t ident);

    enum Port : unsigned { PORTA, PORTB };
    enum { T_A = 0, T_B = 1 };

    uint8_t ident;
    struct Lines {
        uint8_t pra;
        uint8_t prb;
        uint8_t ddra;
        uint8_t ddrb;
        uint8_t ioa;
        uint8_t ioaOld;
        uint8_t iob;
        uint8_t iobOld;
        bool praChange; // otherwise ddra change
        bool prbChange;
    } lines;

    std::function<uint8_t ( Port port, Lines* lines )> readPort;
    std::function<void ( Port port, Lines* lines )> writePort;

    std::function<void (bool spLine, bool cntLine)> serialOut;
    std::function<void (bool state)> irqCall;
    std::function<void (const char* info, bool newLine, bool hex)> logOut;

    auto read(unsigned pos) -> uint8_t;
    auto write(unsigned pos, uint8_t value) -> void;
    auto tod() -> void;
    auto reset() -> void;

    auto clock() -> void;

    auto serialIn(bool spLine) -> void;
    auto setCNTAndSP(bool cntLine, bool spLine = false) -> void;

    auto setFlag() -> void;
    auto positiveCntTransition( ) -> void;
    auto setNewVersion( bool newVersion ) -> void;
    auto isNewVersion() -> bool;
    auto serialize(Emulator::Serializer& s) -> void;

protected:
    struct Timer {
        // bit 0: -> phase in, bit 1: -> single step in cascade mode
        uint8_t run;
        bool oneshot;

        uint16_t latch;
        uint16_t counter;

        uint8_t control;

        bool toggle;
    } timerA, timerB;

    bool newVersion = true; // 6526a
    uint8_t icrTemp;

    uint8_t sdr;
    bool sdrLoaded;
    bool sdrPending;
    uint32_t cnt;
    uint8_t sdrShift;
    unsigned sdrShiftCount;
    bool sdrForceFinish;

    uint8_t icrmask;
    uint8_t icr;

    uint64_t delay;
    uint8_t intIncomming;

    bool todLatched;
    bool todActive;
    uint32_t todLatch;
    uint32_t alarm;
    uint32_t todc;
    unsigned tickCounter;

    auto timerAUnderflow() -> void;
    auto timerBUnderflow() -> void;
    auto shiftOut() -> void;
    auto switchSerialDirection(bool input) -> void;
    auto flipCnt() -> void;
    auto handleInterrupt( uint8_t number ) -> void;
    template<uint8_t timerId> auto updateState() -> void;
    auto adjustBit6And7( uint8_t& inOut ) -> void;
    auto updatePortB() -> void;
    auto interruptControl() -> void;
    auto interruptControlOld() -> void;
    template<uint8_t timerId> inline auto readCounter( ) -> uint16_t;
    auto todIncrement() -> void;

};
