
#pragma once

/**
 * emulates the NMOS 6502 (NES, VC1541 Floppy) and 6510 (C64) cpus 
 * doesn't emulates the CMOS 65C02 ( Nec PC Engine )
 * 
 * NOTE: doesn't emulates AEC line of 6510. you can manage this by yourself within callbacks for read/write.
 * if AEC line is pulled low, the external BUS is decoupled from 6510. 
 * of course running the CPU without connected BUS doesn't make much sense. thats why AEC is mostly used with
 * RDY to stop the CPU. RDY stops CPU in read cycles only, means if AEC and RDY happens the same time you have to
 * make sure that all write cycles before (3 in worst case) are not processed.
 * the C64 graphic chip lowers AEC 3 cycles after RDY, so you don't need to take care of AEC.
 * the C64 extension port DMA request lowers AEC/RDY same time, so take care of write cycles like explained above.
 * 
 * usage: compile m6502.cpp and m6510.cpp, include this header in your program.
 * NOTE: when you commenting out following define, you don't need to compile m6510.cpp
 */
#define SUPPORT_M6510

/**
 * comment out following 'define' if:
 * 1. you want to emulate a 6510 only, which doesn't support external overflow
 * 2. or for performance reasons, if the line is not in use for your 6502
 */
//#define SUPPORT_SO
//#define SUPPORT_HALF_CYCLE

#include "context.h"

namespace MOS65FAMILY {
	
struct M65Model {

	/** power */
	virtual auto power() -> void = 0;
    /** reset */
	virtual auto reset() -> void = 0;
	/** process next opcode */
	virtual auto process() -> void = 0;
	/** set irq */
	virtual auto setIrq( bool state ) -> void = 0;
	/** set nmi */
    virtual auto setNmi( bool state ) -> void = 0;
    /** external signal to raise overflow (6502 only) */
    virtual auto setSo( bool state ) -> void {}
    /** set rdy */
    virtual auto setRdy( bool state ) -> void = 0;
	/** change magic value for ane */
	virtual auto setMagicForAne( uint8_t magicAne ) -> void = 0;
    /** get magic value for ane */
	virtual auto getMagicForAne( ) -> uint8_t = 0;
	/** change magic value for lax */
	virtual auto setMagicForLax( uint8_t magicLax ) -> void = 0;
    /** get magic value for lax */
	virtual auto getMagicForLax( ) -> uint8_t = 0;
	/** last used value on bus */
	virtual auto dataBus() -> uint8_t = 0;	
    /** last puted address on bus */
    virtual auto addressBus() -> uint16_t = 0;
    /** last cycle was a write cycle */
    virtual auto isWriteCycle() -> bool = 0;
    
	/** pullup: external device force line hi in input mode */
	/** pulldown: external device force line low in input mode */  
    virtual auto updateIoLines( uint8_t pullup, uint8_t pulldown = 0 ) -> void {}
	
    /** jump out opcode when rdy is blocking execution. */
    /** use this, if you need to update UI. auto resumes opcode at interrupted cycle with correct context */
    /** it's slow, so don't use it for syncing purposes */
    virtual auto hintUnblockedExecution( ) -> void = 0;

    
	/** 
     * set a prepared context by defining callbacks,
	 * for resuming you have to set internal values too
     */	
	virtual auto setContext( M65Context* context ) -> void = 0;
	/** creates a new context */
	static auto createContext() -> M65Context* { return new M65Context; }
	/** creates a 6502 cpu instance */
    static auto create6502() -> M65Model*;
	/** creates a 6510 cpu instance */
#ifdef SUPPORT_M6510    
    static auto create6510() -> M65Model*;
#endif	
	virtual ~M65Model() = default;
};

//shortcuts
/** creates a new context */
static auto createContext() -> M65Context* {
	return M65Model::createContext();
}
/** creates a 6502 cpu instance */
static auto create6502() -> M65Model* {
	return M65Model::create6502();
}
/** creates a 6510 cpu instance */
#ifdef SUPPORT_M6510    
    static auto create6510() -> M65Model* {
        return M65Model::create6510();
    }
#endif
}

typedef MOS65FAMILY::M65Model MOS65Model;
typedef MOS65FAMILY::M65Context MOS65Context;