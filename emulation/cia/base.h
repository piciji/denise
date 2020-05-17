
#pragma once

#define CIA_GLOBAL_EVENTS

#include <functional>

#include "../tools/event.h"
#include "../tools/serializer.h"

namespace CIA {
    
struct Base {
	Base( uint8_t model, Emulator::Events* events = nullptr );          
    
    enum Port : unsigned { PORTA, PORTB };
    enum { T_A = 0, T_B = 1 };
    
    struct Lines {
        uint8_t pra;
        uint8_t prb;
        uint8_t ddra;
        uint8_t ddrb;
        uint8_t ioa;
        uint8_t iob;  
        bool praChange; // otherwise ddra change
        bool prbChange;
    } lines;
	    
    std::function<uint8_t ( Port port, Lines* lines )> readPort;
    std::function<void ( Port port, Lines* lines )> writePort;
	/**
	 * send shifted out bit to external device 
	 */    
    std::function<void (bool bit)> serialCall;
    /**
	 * inform external devices if irq line switches to low state
	 */
	std::function<void (bool state)> irqCall;
	
    virtual auto read(unsigned pos) -> uint8_t;
    virtual auto write(unsigned pos, uint8_t value) -> void;
    virtual auto reset() -> void;
	/**
	 * define it in derived class
	 */
	virtual auto tod() -> void = 0;
	
	/**
     * amiga 500: a cia clock lasts 10 cpu cycles
     * hi cycle 4/10 of e-clock (cpu register access)
	 * lo cycle 6/10 of e-clock
	 */	
    auto clock() -> void;
	
	/**
	 * shift in bits
	 * shifting too fast behavior is not emulated
	 */
    auto serialIn( bool bit ) -> void;   
	/*
	 * external device forces cia to generate an irq
	 */
	auto setFlag() -> void;
	auto setCnt( bool state ) -> void;
    auto setNewVersion( bool newVersion ) -> void;
    auto isNewVersion() -> bool;
    virtual auto serialize(Emulator::Serializer& s) -> void;
	
	uint8_t model; // for debugging purposes
protected:
	/**
	 * cia's have an annoying timing
	 * rules:
	 * when start bit = 1 and system clock bit = 0 cia warms up 2 cycles
	 * in third cycle cia begins decrementing.
	 * I assume decrementing happens in second half cycle (4/10 eclock) during register access,
	 * but reading timer value from register gets the timer state of the beginning
	 * of half cycle, means before decrementing.
	 * decrementing happens only when value is > 0, means a start value of 0 or 1
	 * underflows same time.
	 * the first half cycle (6/10 eclock) checks if the timer reached zero
	 * if so the timer underflows and sets the appropriate bit in icr
	 * a new cia sets the highest bit in icr too and fire interrupt to cpu
	 * in the first half cycle.
	 * an old cia does this a full cycle later
	 * the cia reloads the timer from the latch first half cycle too.
	 * so a possible read in the second half cycle never fetch a zero.
	 * when in first half cycle the counter becomes zero, there is no decrementing
	 * of the reloaded timer in second half cycle.
	 * so you would read the reloaded value for two cycles.
	 * a write to timer during this non decrementing cycle will latch the timer
	 * otherwise the timer will be latched only when the start bit is unset
	 * Note: during warm up a timer write will not latch anymore
	 * a force load can happen in parallel to warm up or later during decrementing
	 * what makes it complicated is, that force load has a 2 cycle warm up too
	 * if you start the timer parallel to force load command, the warm takes one
	 * cycle more for start the decrementing, because the timer is latched 
	 * the last cycle of warm up and the next cycle is a non decrementing cycle like
	 * explained above.
	 * if you stop the timer or remove the system clock input, decrementing doesn't
	 * stop immediately, it runs one more cyle.
	 * oneshot mode latch the timers when underflows like normal operation but
	 * unsets the start bit, so no more decrementing.
	 * removing the oneshot state isn't immediate too, it keeps activated another
	 * cycle. so disabling the oneshot state by a register write just before the underflow
	 * happens in next half cycle means the oneshot is still triggering and the timer stops.
	 * in cascaded mode decrementing and underflow of a timer is triggereed by underflow of
     * another timer. cascade mode has one cycle delay, so when timer A underflows in first 
     * half cycle timer B will not decrementing in second half cycle. in following first 
     * half cycle when start bit is set timer B will be checked for underflow condition and if
     * not it will decrementing one time in second half cycle.
     * so after decrementing to zero timer B needs another underflow step of timer A to underflow
     * itself
	 */

	using Callback = std::function<void ()>;
	Emulator::Events* events;
	
	struct Timer {
		// bit 0: -> phase in, bit 1: -> single step in cascade mode
		uint8_t run;	
		// disabling oneshot has a one cycle delay, so we can not use the register
		// value itself
		bool oneshot;
		// lasts one cycle only, pb 6/7 may need to know
		bool underflowCycle;
		// last one cycle only, triggered by an underflow or forced by control register
		bool forceloadCycle;

		Callback start;		
		Callback stop;
		Callback step;		
		Callback disableOneshot;
		Callback forceLoad;
        Callback disableForceLoad;
		
		uint16_t latch;
		uint16_t counter;
        uint16_t counterRead;
		
        uint8_t control;
        
        bool toggle;
	} timer[2];

	// references to control in Timer struct
    uint8_t& cra; 
    uint8_t& crb;
    
    Callback updateIcrAndSetIrq;
    Callback updateIcrOnly;
	bool newVersion = true; // 6526a, 8520a instead of 6526, 8520
    uint8_t acknowledgeCycle; 
	uint8_t maskWriteCycle;
	uint8_t intDelay;	
	uint8_t serialDelay;
	uint8_t icrTemp;
    bool flagRaised;	
            
    uint8_t sdr;
	bool sdrValid;
	bool cnt;
    uint8_t shift;
    unsigned shiftCount;
	
	uint8_t icrmask;
    uint8_t icr;
	bool ciaShiftRespawnBug;
	        	
    auto timerAUnderflow() -> void;
	auto timerBUnderflow() -> void;
	auto serialOut() -> void;
	auto serialFlagRespawn() -> void;
    auto handleInterrupt( uint8_t number ) -> void;
	virtual auto processTod() -> void = 0;
	template<uint8_t timerId> auto decrement() -> void;
	template<uint8_t timerId> auto updateState() -> void;
    auto adjustBit6And7( uint8_t& inOut ) -> void;
    auto interruptControl() -> void;
    auto interruptControlOld() -> void;    
};

}
