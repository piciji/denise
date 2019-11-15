
#pragma once

#define EXTERNAL_INCLUDE_6502
#include "../../../processor/m65xx/m6502/m6502.h"
#include "../../../tools/serializer.h"

/**
 * special emulation of the 6502 core
 * 
 * gives back control to the caller before a possible read/write
 * this is usefull when emulating more than one 6502 cpu same time.
 * alternatively you could run each cpu in a thread but that is costly.
 * furthermore generating savestates is difficult because you can not restore
 * the stackframe, means you are forced to restart at clean opcode edge. that could
 * result that both 6502 are not correctly synced when resuming emulation.
 * 
 * this approach has it's flaws too. code is more complex because you need to
 * programmatically resume execution at a later cycle. by jumping out to the caller
 * and jumping back in to next cycle, execution speed is slower of course.
 * so it would be faster to run the second 6502 as a slave of the first 6502. this
 * way you only need to emulate one cpu core with this slow approach.
 * 
 * NOTE: this approach is customized for the needs of 1541 emulation. there is no
 * need to step out each cycle. It's enough to step out before a memory read/write
 * and before the step which samples interrupts (last one in most cases).
 * address generation will not be interrupted. (no iec bus dependency)
 * interrupt processing will not be interrupted. (no iec bus dependency)
 * rdy is not used by 1541 and therefore not reworked in this approach.
 * so we have the chance to give back control to c64 before a read/write to time
 * via access between c64 and drive in a cycle accurate way.
 * 
 */ 

namespace LIBC64 {

struct M6502Custom : MOS65FAMILY::M6502 {
    
    auto process() -> void override;
    auto power() -> void override;
    inline auto isReadNext() -> bool { return readNext; }
    auto detectIrq() -> void;
    auto serialize(Emulator::Serializer& s) -> void;
    
protected:    
    auto indexedIndirect( Alu alu ) -> void override;
	auto indexedIndirectW( uint8_t data ) -> void override;
	auto indirectIndexed( Alu alu ) -> void override;
	auto indirectIndexedW( uint8_t data ) -> void override;
	auto zeroPage( Alu alu, uint8_t& data ) -> void override;
	auto zeroPageW( uint8_t data ) -> void override;
	auto zeroPageM( Alu alu ) -> void override;	
	auto zeroPageIndexed( uint8_t index, Alu alu, uint8_t& data ) -> void override;
	auto zeroPageIndexedW( uint8_t index, uint8_t data ) -> void override;
	auto zeroPageIndexedM( Alu alu ) -> void override;
	auto absolute( Alu alu, uint8_t& data ) -> void override;
	auto absoluteW( uint8_t data ) -> void override;
	auto absoluteM( Alu alu ) -> void override;
	auto absoluteIndexed( uint8_t index, Alu alu, uint8_t& data ) -> void override;
	auto absoluteIndexedW( uint8_t index, uint8_t data ) -> void override;
	auto absoluteIndexedM( uint8_t index, Alu alu ) -> void override;
	auto immediate( Alu alu, uint8_t& data ) -> void override;	
	auto implied(Alu alu, uint8_t& data) -> void override;
    auto nop() -> void override;
    auto rti() -> void override;
    auto rts() -> void override;
    auto clear( bool& flag ) -> void override;
    auto set( bool& flag ) -> void override;
    auto jmpAbsolute() -> void override;
    auto jmpIndirect() -> void override;
    auto jsrAbsolute() -> void override;
    auto branch( bool& flag, bool state ) -> void override;
    auto plp() -> void override;
    auto php() -> void override;
	auto pha() -> void override;
	auto pla() -> void override;
    auto transfer(uint8_t src, uint8_t& target, bool flag) -> void override;
        
    auto indexedIndirectLax( ) -> void override;
	auto indirectIndexedLax( ) -> void override;
    auto zeroPageM( Alu alu, Alu alu2 ) -> void override;
	auto zeroPageLax() -> void override;
	auto zeroPageIndexedLax() -> void override;
	auto absoluteLax() -> void override;
	auto absoluteIndexedLax() -> void override;
    auto immediate() -> void override;
	auto immediateLax() -> void override;
	auto absoluteIndexedLas() -> void override;
	auto absoluteIndexedWSh( uint8_t index, uint8_t index2 ) -> void override;
	auto absoluteIndexedWAhx() -> void override;
	auto absoluteIndexedWTas() -> void override;
	auto immediateAnc() -> void override;
	auto immediateAlr() -> void override;
	auto immediateArr() -> void override;
	auto immediateAne() -> void override;
	auto immediateSbx() -> void override;
	auto indexedIndirectM( Alu alu, Alu alu2 ) -> void override;
	auto indirectIndexedWAhx() -> void override;
	auto indirectIndexedM( Alu alu, Alu alu2 ) -> void override;
	auto zeroPageIndexedM( Alu alu, Alu alu2 ) -> void override;
	auto absoluteM( Alu alu, Alu alu2 ) -> void override;
	auto absoluteIndexedM( uint8_t index, Alu alu, Alu alu2 ) -> void override;
    
    unsigned step;
    uint16_t adrTemp;
    uint8_t zeroAdrTemp;
    uint8_t dataTemp;
    int8_t displacement;
    bool readNext;
};

}