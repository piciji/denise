
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
    auto _indexedIndirect( Alu alu ) -> void;
	template<Reg reg> auto _indexedIndirectW( ) -> void;
	auto _indirectIndexed( Alu alu ) -> void;
	auto _indirectIndexedW( ) -> void;
	template<Reg reg = RegA> auto _zeroPage( Alu alu = nullptr ) -> void;
	template<Reg reg> auto _zeroPageW( ) -> void;
	auto _zeroPageM( Alu alu ) -> void;	
	template<Reg regIndex, Reg reg = RegA> auto _zeroPageIndexed( Alu alu = nullptr ) -> void;
	template<Reg regIndex, Reg reg> auto _zeroPageIndexedW( ) -> void;
	auto _zeroPageIndexedM( Alu alu ) -> void;
	template<Reg reg = RegA> auto _absolute( Alu alu = nullptr ) -> void;
	template<Reg reg = RegA> auto _absoluteW( ) -> void;
	auto _absoluteM( Alu alu ) -> void;
	template<Reg regIndex, Reg reg = RegA> auto _absoluteIndexed( Alu alu = nullptr ) -> void;
	template<Reg regIndex, Reg reg> auto _absoluteIndexedW( ) -> void;
	template<Reg regIndex> auto _absoluteIndexedM( Alu alu ) -> void;
	template<Reg reg> auto _immediate( Alu alu ) -> void;	
	template<Reg reg> auto _implied(Alu alu) -> void;
    auto _nop() -> void;
    auto _rti() -> void;
    auto _rts() -> void;
    auto _brk() -> void;
    template<Flag flag> auto _clear( ) -> void;
    template<Flag flag> auto _set( ) -> void;
    auto _jmpAbsolute() -> void;
    auto _jmpIndirect() -> void;
    auto _jsrAbsolute() -> void;
    template<Flag flag> auto _branch( bool state ) -> void;
    auto _plp() -> void;
    auto _php() -> void;
	auto _pha() -> void;
	auto _pla() -> void;
    template<Reg src, Reg target> auto _transfer( bool flag) -> void;
        
    auto _indexedIndirectLax( ) -> void;
	auto _indirectIndexedLax( ) -> void;
    auto _zeroPageM( Alu alu, Alu alu2 ) -> void;
	auto _zeroPageLax() -> void;
	auto _zeroPageIndexedLax() -> void;
	auto _absoluteLax() -> void;
	auto _absoluteIndexedLax() -> void;
    auto _immediate() -> void;
	auto _immediateLax() -> void;
	auto _absoluteIndexedLas() -> void;
	template<Reg regIndex, Reg reg> auto _absoluteIndexedWSh( ) -> void;
	auto _absoluteIndexedWAhx() -> void;
	auto _absoluteIndexedWTas() -> void;
	auto _immediateAnc() -> void;
	auto _immediateAlr() -> void;
	auto _immediateArr() -> void;
	auto _immediateAne() -> void;
	auto _immediateSbx() -> void;
    auto _kill() -> void;
	auto _indexedIndirectM( Alu alu, Alu alu2 ) -> void;
	auto _indirectIndexedWAhx() -> void;
	auto _indirectIndexedM( Alu alu, Alu alu2 ) -> void;
	auto _zeroPageIndexedM( Alu alu, Alu alu2 ) -> void;
	auto _absoluteM( Alu alu, Alu alu2 ) -> void;
	template<Reg regIndex> auto _absoluteIndexedM( Alu alu, Alu alu2 ) -> void;
    
    auto _decode( uint8_t IR ) -> void;
    
    unsigned step;
    bool readNext;
};

}