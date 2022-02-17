
#pragma once

#include <functional>

#include "../../../tools/serializer.h"
#include "../../../tools/macros.h"

#define VIA_FORCE_LOAD_TIMERA0 1
#define VIA_FORCE_LOAD_TIMERA1 2

#define VIA_FORCE_LOAD_TIMERB0 4
#define VIA_FORCE_LOAD_TIMERB1 8

#define VIA_STEP_TIMERB0 0x10
#define VIA_STEP_TIMERB1 0x20

#define VIA_CA2_PULSE0 0x40
#define VIA_CA2_PULSE1 0x80

#define VIA_CB2_PULSE0 0x100
#define VIA_CB2_PULSE1 0x200

#define VIA_UPDATE_IRQ0 0x400
#define VIA_UPDATE_IRQ1 0x800
#define VIA_UPDATE_IRQ2 0x1000

#define VIA_SHIFT_WARMUP0 0x2000
#define VIA_SHIFT_WARMUP1 0x4000

#define VIA_MASK ~(0x8000 | VIA_FORCE_LOAD_TIMERA0 | VIA_FORCE_LOAD_TIMERB0 | VIA_STEP_TIMERB0 | VIA_CA2_PULSE0 | VIA_CB2_PULSE0 | VIA_UPDATE_IRQ0 | VIA_SHIFT_WARMUP0)

namespace LIBC64 {
    
// Via 6522    
struct Via {
    Via( uint8_t model );    
    
    enum class Port : unsigned { A = 0, B = 1 };    
    
    struct Lines {
        uint8_t pra;
        uint8_t prb;
        uint8_t ddra;
        uint8_t ddrb;
        uint8_t ioa;
        uint8_t ioaOld;
        uint8_t iob;      
        uint8_t iobOld;      
        uint8_t latchA;
        uint8_t latchB;
    } lines;

    std::function<uint8_t ( Port port, Lines* lines )> readPort;
    std::function<void ( Port port, Lines* lines )> writePort;

    // outgoing transitions
    // ca1 is input only
    std::function<void (bool direction)> ca2Out;
    std::function<void (bool direction)> cb1Out;
    std::function<void (bool direction)> cb2Out;

    std::function<void (bool state)> irqCall;    
    
    auto pb6Pulse() -> void;

    // incomming transitions
    auto ca1In( bool direction, bool irqNextCycle = true ) -> void;
    auto ca2In( bool direction, bool irqNextCycle = true ) -> void;
    auto cb1In( bool direction, bool pulse = true, bool irqNextCycle = true ) -> void;
    auto cb2In( bool direction, bool irqNextCycle = true ) -> void;
    
    auto read(uint16_t pos) -> uint8_t;
    auto write(uint16_t pos, uint8_t value) -> void;
    auto reset() -> void;
    
    auto process() -> void;
    auto serialize(Emulator::Serializer& s) -> void;
    auto handleInterrupt( ) -> void;
    
    uint8_t model;   
    
//protected:
    uint16_t timerACounter;
    uint16_t timerALatch;
    uint16_t timerACounterRead;
    bool timerATrigger;
    bool timerAToggle;
    
    uint16_t timerBCounter;
    uint8_t timerBLatch;
    uint16_t timerBCounterRead;
    bool timerBTrigger;
    
    uint8_t ifr;
    uint8_t ier;
    uint8_t pcr;
    uint8_t acr;
    uint8_t sdr;

    bool ca2;
    bool cb2;
    
    unsigned delay;
        
    struct {
        bool toggle;    // cb1
        bool irqTrigger;
        bool active;
        uint8_t count;
    } shift;
    
    bool isShiftT2Control;
        
    inline auto setIrq( uint8_t pos, unsigned irqDelay = VIA_UPDATE_IRQ2 ) -> void;
    inline auto resetIrq( uint8_t pos ) -> void;
    inline auto updateTimerA( ) -> void;
    inline auto updateTimerB( ) -> void;
    auto shifter() -> void;
    auto shiftCb1Control() -> bool;
    auto shiftT2FreeRunning() -> bool;
    inline auto shiftSystemClock() -> bool;
    auto shiftT2Control() -> bool;
    auto shiftDisabled() -> bool;
    auto shiftOut() -> bool;
    inline auto shiftTiming(bool cb1) -> void;
};    
    
}