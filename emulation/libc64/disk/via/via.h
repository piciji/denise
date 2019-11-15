
/**
 * this implementation should be in a more common area like cpu or cia.
 * while not emulating another devices which uses via chips i would leave it here
 * in context of c64 disk drive emulation.
 * todo: what happens when changing shift modes while shift is running ?
 */

#pragma once

#include <functional>

#include "../../../tools/serializer.h"

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

    // ca1 is input only
    std::function<void (bool state)> ca2Out;
    std::function<void (bool state)> cb1Out;
    std::function<void (bool state)> cb2Out;

    std::function<void (bool state)> irqCall;    
    
    auto pb6Pulse() -> void;

    auto ca1In( bool state ) -> void;
    auto ca2In( bool state ) -> void;
    auto cb1In( bool state ) -> void;
    auto cb2In( bool state ) -> void;
    
    auto read(unsigned pos) -> uint8_t;
    auto write(unsigned pos, uint8_t value) -> void;
	auto writePipelined(unsigned pos, uint8_t value) -> void;
    auto reset() -> void;
    
    auto processHi() -> void;
    auto processLo() -> void;
    auto serialize(Emulator::Serializer& s) -> void;
    
    uint8_t model; // for debugging purposes, not part of master branch    
protected:
    
    struct {
		bool pipelined;
		uint8_t addr;
		uint8_t value;
	} registerWrite;
    
    struct Timer {
        enum Type : unsigned { A = 0, B = 1 };
        
		uint8_t forceloadCycle;
        bool counterUpdated;
		
		uint16_t latch;
		uint16_t counter;
        
        bool toggle;
        bool trigger;
        
        bool step;
	} timer[2];
    
    uint8_t ifr;
    uint8_t ier;
    uint8_t pcr;
    uint8_t acr;
    uint8_t sdr;
    
    bool ca1;
    bool ca2;
    bool cb1;
    bool cb2;
        
    struct {
        bool warmUp;
        bool toggle;
        bool irqTrigger;
        bool active;
        uint8_t count;
    } shift;

    uint8_t ca2StatePulse;
    uint8_t cb2StatePulse;
    
    bool updateIrq;
    bool isShiftT2Control;
    
    auto handleInterrupt( ) -> void;
    inline auto setIrq( uint8_t pos ) -> void;
    inline auto resetIrq( uint8_t pos ) -> void;
    template<unsigned timerId> auto decrement() -> void;
	template<unsigned timerId> auto updateState() -> void;    
    auto shifter() -> void;
    auto shiftCb1Control() -> bool;
    auto shiftT2FreeRunning() -> bool;
    auto shiftSystemClock() -> bool;
    auto shiftT2Control() -> bool;
    auto shiftDisabled() -> bool;
    auto shiftOut() -> bool;
    inline auto handleSystemClockShift() -> void;
    template<bool cb1Output> inline auto shiftTiming() -> void;
};    
    
}