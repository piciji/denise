
/**
 * there are some std::move calls in cpu code which doesn't seem to make any sense. so why ?
 * this approach is able to jump out emulation of opcode during any read cycle in order to prevent stalling
 * while rdy halts cpu for more than a frame. we need th handle UI events :-)
 * that is working by swapping in a dummy context, progressing the rest of the opcode, jump out emulation and
 * handle UI, jump back in opcode, progressing to interrupted cycle in dummy context, swap in real context and go on.
 * ok back to the std::move calls. a lot of function returns are values which will be written to register.
 * i.e. ctx->A = readPC(); 
 * when the context is swapping in read cycle, 'ctx' should point to new context. the target address of the above assignment
 * is seted before the function is called, means the write goes to old context instead of swapped in. 
 * ctx->A = std::move( readPC() ); now the target address is specified after the function call.
 */

#pragma once

#ifndef EXTERNAL_INCLUDE_6502
    #define A ctx->a
    #define X ctx->x
    #define Y ctx->y
    #define S ctx->s
    #define PC ctx->pc

    #define C ctx->c
    #define Z ctx->z
    #define I ctx->i
    #define D ctx->d
    #define V ctx->v
    #define N ctx->n
#endif

#include "../model.h"

namespace MOS65FAMILY {

struct M6502 : M65Model {
    
    M6502() {}
    
    M65Context* ctx = nullptr;
    M65Context* workCtx = nullptr;
    
    bool dontBlockExecution = false;
    
	auto power() -> void;
    virtual auto reset() -> void;
    auto process() -> void;
    auto setIrq( bool state ) -> void;
    auto setNmi( bool state ) -> void;
    virtual auto setSo( bool state ) -> void;
    auto setRdy( bool state ) -> void;
	auto setMagicForAne( uint8_t magicAne ) -> void;
    auto getMagicForAne() -> uint8_t;
    
    auto setContext( M65Context* context ) -> void;
    auto dataBus() -> uint8_t;	
    auto addressBus() -> uint16_t;
    auto isWriteCycle() -> bool;
    
    auto hintUnblockedExecution() -> void;
    
    enum Reg { RegA, RegS, RegX, RegY, RegAX };
    enum Flag { FlagC, FlagN, FlagZ, FlagV, FlagI, FlagD };
    
protected:    
    using Alu = auto (M6502::*)(uint8_t) -> uint8_t;
    
    auto getFlags() -> uint8_t;
    auto setFlags( uint8_t data ) -> void;
    auto interrupt( bool software = false ) -> void;
    inline auto sampleInterrupt() -> void;
    inline auto detectInterrupt() -> void;
    inline auto handleSo() -> void;
    auto setPCL( uint8_t data ) -> void;
    auto setPCH( uint8_t data ) -> void;
	auto decode( uint8_t IR ) -> void;
	virtual auto busRead( uint16_t addr ) -> uint8_t;
	virtual auto busWrite( uint16_t addr, uint8_t data ) -> void;
    virtual auto busWatch() -> uint8_t;
    auto resetRoutine() -> void;
    auto restoreContext() -> void;
    auto setDummyContext() -> void;
    
    //memory
    template<uint8_t cycle> inline auto read( uint16_t addr, bool lastCycle = false ) -> uint8_t;
    template<uint8_t cycle> inline auto readPCInc( bool lastCycle = false ) -> uint8_t;
    template<uint8_t cycle> inline auto readPC( bool lastCycle = false ) -> uint8_t;
    inline auto write( uint16_t addr, uint8_t data, bool lastCycle = false ) -> void;
    inline auto pushStack( uint8_t data, bool lastCycle = false ) -> void;
    template<uint8_t cycle> inline auto pullStack( bool lastCycle = false ) -> uint8_t;   
	template<uint8_t cycle> inline auto loadZeroPage( uint8_t addr, bool lastCycle = false ) -> uint8_t;
	inline auto storeZeroPage( uint8_t addr, uint8_t data, bool lastCycle = false ) -> void;
    
    //logic
    auto _and( uint8_t data ) -> uint8_t;
    auto _ora( uint8_t data ) -> uint8_t;
    auto _eor( uint8_t data ) -> uint8_t;
    auto _ror( uint8_t data ) -> uint8_t;
    auto _rol( uint8_t data ) -> uint8_t;
    auto _asl( uint8_t data ) -> uint8_t;
    auto _lsr( uint8_t data ) -> uint8_t;
    auto _bit( uint8_t data ) -> uint8_t;
    auto _cmp( uint8_t data ) -> uint8_t;
    auto _cpx( uint8_t data ) -> uint8_t;
    auto _cpy( uint8_t data ) -> uint8_t;
    auto _dec( uint8_t data ) -> uint8_t;
    auto _inc( uint8_t data ) -> uint8_t;
    auto _ld( uint8_t data ) -> uint8_t;
    auto _adc( uint8_t data ) -> uint8_t;
    auto _sbc( uint8_t data ) -> uint8_t;    
	auto _ane( uint8_t data ) -> uint8_t;
	auto _sbx( uint8_t data ) -> uint8_t;
	auto _arr( uint8_t data ) -> uint8_t;
	auto _las( uint8_t data ) -> uint8_t;
	auto _lax( uint8_t data ) -> uint8_t;
	
	//address
	auto indexedIndirectAdr() -> void;
	auto indirectIndexedAdr( bool forceExtraCycle = false ) -> void;
	template<Reg regIndex> auto zeroPageIndexedAdr( ) -> void;
	auto absoluteAdr( ) -> void;
	template<Reg regIndex> auto absoluteIndexedAdr( bool forceExtraCycle = false ) -> void;
	
    //opcodes
	auto indexedIndirect( Alu alu ) -> void;
	template<Reg reg> auto indexedIndirectW( ) -> void;
	auto indirectIndexed( Alu alu ) -> void;
	auto indirectIndexedW( ) -> void;
	template<Reg reg = RegA> auto zeroPage( Alu alu = nullptr) -> void;
	template<Reg reg> auto zeroPageW( ) -> void;
	auto zeroPageM( Alu alu ) -> void;	
	template<Reg regIndex, Reg reg = RegA> auto zeroPageIndexed( Alu alu = nullptr ) -> void;
	template<Reg regIndex, Reg reg> auto zeroPageIndexedW( ) -> void;
	auto zeroPageIndexedM( Alu alu ) -> void;
	template<Reg reg = RegA> auto absolute( Alu alu = nullptr ) -> void;
	template<Reg reg = RegA> auto absoluteW( ) -> void;
	auto absoluteM( Alu alu ) -> void;
	template<Reg regIndex, Reg reg = RegA> auto absoluteIndexed( Alu alu = nullptr ) -> void;
	template<Reg regIndex, Reg reg> auto absoluteIndexedW() -> void;
	template<Reg regIndex> auto absoluteIndexedM( Alu alu ) -> void;
	template<Reg reg> auto immediate( Alu alu ) -> void;	
	template<Reg reg> auto implied(Alu alu) -> void;
    auto nop() -> void;
    auto rti() -> void;
    auto rts() -> void;
    auto brk() -> void;
    template<Flag flag> auto clear( ) -> void;
    template<Flag flag> auto set( ) -> void;
    auto jmpAbsolute() -> void;
    auto jmpIndirect() -> void;
    auto jsrAbsolute() -> void;
    template<Flag flag> auto branch( bool state ) -> void;
    auto plp() -> void;
    auto php() -> void;
	auto pha() -> void;
	auto pla() -> void;
    template<Reg src, Reg target> auto transfer(bool flag) -> void;
    
	//undocumented opcodes        
	auto indexedIndirectLax( ) -> void;
	auto indirectIndexedLax( ) -> void;
    auto zeroPageM( Alu alu, Alu alu2 ) -> void;
	auto zeroPageLax() -> void;
	auto zeroPageIndexedLax() -> void;
	auto absoluteLax() -> void;
	auto absoluteIndexedLax() -> void;
    auto immediate() -> void;
	auto immediateLax() -> void;
	auto absoluteIndexedLas() -> void;
	template<Reg regIndex, Reg reg> auto absoluteIndexedWSh( ) -> void;
	auto absoluteIndexedWAhx() -> void;
	auto absoluteIndexedWTas() -> void;
	auto immediateAnc() -> void;
	auto immediateAlr() -> void;
	auto immediateArr() -> void;
	auto immediateAne() -> void;
	auto immediateSbx() -> void;
	auto kill() -> void;
	auto indexedIndirectM( Alu alu, Alu alu2 ) -> void;
	auto indirectIndexedWAhx() -> void;
	auto indirectIndexedM( Alu alu, Alu alu2 ) -> void;
	auto zeroPageIndexedM( Alu alu, Alu alu2 ) -> void;
	auto absoluteM( Alu alu, Alu alu2 ) -> void;
	template<Reg regIndex> auto absoluteIndexedM( Alu alu, Alu alu2 ) -> void;
	auto H1AndedWrite( uint8_t anded ) -> void;
};

}

