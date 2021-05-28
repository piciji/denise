
#pragma once

/**
 * special emulation of the 6502 core
 * 
 * gives back control to the caller before a possible read/write to VIA
 * this is usefull when emulating more than one 6502 cpu same time.
 * alternatively you could run each cpu in a thread but that is costly.
 * furthermore generating savestates is difficult because you can not restore
 * the stackframe, means you are forced to restart at clean opcode edge. that could
 * result that both 6502 are not correctly synced when resuming emulation.
 * 
 * this approach has it's flaws too. code is more complex because you need to
 * programmatically resume execution at a later cycle. by jumping out to the caller
 * and jumping back in to next cycle, execution speed is slower of course.
 * 
 * NOTE: this approach is customized for the needs of 1541 emulation. there is no
 * need to step out each cycle. It's enough to step out before a possible VIA read/write
 * and before the step which samples interrupts (last one in most cases).
 * address generation will not be interrupted. (no iec bus dependency)
 * interrupt processing will not be interrupted. (no iec bus dependency)
 * rdy is not used by 1541 and therefore not reworked in this approach.
 * nmi is not used by 1541 and therefore not included in this approach.
 * so we have the chance to give back control to c64 before a read/write to time
 * via access between c64 and drive in a cycle accurate way.
 * 
 */ 

#include <cstdint>
#include <functional>
#include "../../../tools/serializer.h"
#include "../../../tools/branchPrediction.h"

namespace LIBC64 {

struct Drive1541;
    
struct M6502 {

    M6502(Drive1541* drive) : drive(drive) {}
    
    Drive1541* drive;
	bool irqPending;	
	bool interruptSampled;
	
	bool killed;
	
	uint16_t pc;
    
    uint8_t IR;
	
	uint8_t regX;
	
	uint8_t regY;
	
	uint8_t regA;
	
	uint8_t regS;
	
	uint8_t regP;
	
	uint8_t flagZ;
	
	uint8_t flagN;
    
    uint8_t step;
    bool readNext;
		
	uint8_t magicAne = 0xee;
	uint8_t magicLax = 0xee;
    
    uint8_t zeroPage;
    uint16_t absolute;
    uint16_t absIndexed; 
    uint8_t dataBus;
    uint8_t _value;

    uint8_t soBlock;

    uint8_t soSample = 0;

	auto process() -> void;
    
    inline auto isReadNext() -> bool { return readNext; }
	
	template<bool software = false> auto interrupt() -> void;
	
	auto power() -> void;
	
	auto reset() -> void;
	
	auto resetRoutine() -> void;
	
	auto setIrq(bool state) -> void;	
	
	auto setMagicForAne(uint8_t magicAne) -> void;

    auto getMagicForAne() -> uint8_t { return magicAne; }
	
	auto setMagicForLax(uint8_t magicLax) -> void;

    auto getMagicForLax() -> uint8_t { return magicLax; }

    auto triggerSO(uint8_t delay = 1) -> void;
    
    auto handleSo() -> void;
	
	auto serialize(Emulator::Serializer& s) -> void;
};

}
