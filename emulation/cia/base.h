
#pragma once

#include <functional>

#include "../tools/systimer.h"
#include "../tools/serializer.h"
#include "../tools/branchPrediction.h"

#define CIA_MASK_WRITE0 1
#define CIA_MASK_WRITE1 2

#define CIA_ACK0	4
#define CIA_ACK1	8

#define CIA_INT0	0x10
#define CIA_INT1	0x20

#define CIA_CNT0	0x40
#define CIA_CNT1	0x80
#define CIA_CNT2	0x100

// force load last one cycle only, triggered by an underflow or forced by control register
#define CIA_FL_TA0  0x200
#define CIA_FL_TA1  0x400
#define CIA_FL_TA2  0x800

// underlow lasts one cycle, pb 6/7 may need to know
#define CIA_UF_TA0  0x1000
#define CIA_UF_TA1  0x2000

#define CIA_FL_TB0  0x4000
#define CIA_FL_TB1  0x8000
#define CIA_FL_TB2  0x10000

#define CIA_UF_TB0  0x20000
#define CIA_UF_TB1  0x40000

#define CIA_INT		(CIA_INT0 | CIA_INT1)
#define CIA_CNT		(CIA_CNT0 | CIA_CNT1 | CIA_CNT2)
#define CIA_CNT_NEW	(CIA_CNT1 | CIA_CNT2)

#define CIA_MASK	~(0x80000 | CIA_MASK_WRITE0 | CIA_ACK0 | CIA_INT0 | CIA_CNT0 | CIA_FL_TA0 | CIA_UF_TA0 | CIA_FL_TB0 | CIA_UF_TB0 )

namespace CIA {
    
struct Base {
	Base( uint8_t model, Emulator::SystemTimer* events );
    
    enum Port : unsigned { PORTA, PORTB };
    enum { T_A = 0, T_B = 1 };
    
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
	/**
	 * send shifted out bit to external device 
	 */    
    std::function<void (bool bit)> serialOut;
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
	
    auto serialIn( bool bit ) -> void;
    auto serialIn( uint8_t byte ) -> void;

	/*
	 * external device forces cia to generate an irq
	 */
	auto setFlag() -> void;
	auto positiveCntTransition( ) -> void;
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
	Emulator::SystemTimer* events;
	
	struct Timer {
		// bit 0: -> phase in, bit 1: -> single step in cascade mode
		uint8_t run;	
		// disabling oneshot has a one cycle delay, so we can not use the register
		// value itself
		bool oneshot;
		
		Callback start;		
		Callback stop;
		Callback step;		
		Callback stepOut;		
		Callback disableOneshot;
		
		uint16_t latch;
		uint16_t counter;
		
        uint8_t control;
        
        bool toggle;
	} timer[2];

	// references to control in Timer struct
    uint8_t& cra; 
    uint8_t& crb;
    
    Callback updateIcrAndSetIrq;
    Callback updateIcrOnly;
	Callback startSdr;
	Callback flipCnt;
	Callback finishSdr;	
	Callback flipDummy;
	
	bool newVersion = true; // 6526a, 8520a instead of 6526, 8520
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
	
	uint32_t delay;
	uint8_t intIncomming;
    
    auto timerAUnderflow() -> void;
	auto timerBUnderflow() -> void;
	auto shiftOut() -> void;
	auto switchSerialDirection(bool input) -> void;
    auto handleInterrupt( uint8_t number ) -> void;
	template<uint8_t timerId> auto updateState() -> void;
    auto adjustBit6And7( uint8_t& inOut ) -> void;
    auto interruptControl() -> void;
    auto interruptControlOld() -> void;    
	template<uint8_t timerId> inline auto readCounter( ) -> uint16_t;
};

}
