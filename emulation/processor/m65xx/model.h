
#pragma once

/**
 * emulates the NMOS 6502 (NES) and 6510 (C64) cpus 
 * dont emulates the CMOS 65C02 ( Nec PC Engine )
 * 
 * usage: compile m6502.cpp and m6510.cpp, include this header in your program
 * NOTE: comment out following define so you don't need to compile m6510.cpp
 */
#define SUPPORT_M6510

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
    /** external signal to raise overflow */
    virtual auto setSo( bool state ) -> void = 0;
    /** set rdy */
    virtual auto setRdy( bool state ) -> void = 0;
	/** change magic value for ane */
	virtual auto setMagicForAne( uint8_t magicAne ) -> void = 0;
    /** get magic value for ane */
	virtual auto getMagicForAne( ) -> uint8_t = 0;
	/** last used value on bus  */
	virtual auto dataBus() -> uint8_t = 0;	
    /** last puted address on bus */
    virtual auto addressBus() -> uint16_t = 0;
    
	/** pullup: external device force line hi in input mode */
	/** pulldown: external device force line low in input mode */  
    virtual auto updateIoLines( uint8_t pullup, uint8_t pulldown = 0 ) -> void {}
	
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