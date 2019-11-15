
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
    
	auto power() -> void;
    virtual auto reset() -> void;
    auto process() -> void;
    auto setIrq( bool state ) -> void;
    auto setNmi( bool state ) -> void;
    auto setSo( bool state ) -> void;
    auto setRdy( bool state ) -> void;
	auto setMagicForAne( uint8_t magicAne ) -> void;
    auto getMagicForAne() -> uint8_t;
    
    auto setContext( M65Context* context ) -> void;
    auto dataBus() -> uint8_t;	
    auto addressBus() -> uint16_t;
    
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
	
    //memory
    auto read( uint16_t addr, bool lastCycle = false ) -> uint8_t;
    auto readPCInc( bool lastCycle = false ) -> uint8_t;
    auto readPC( bool lastCycle = false ) -> uint8_t;
    auto write( uint16_t addr, uint8_t data, bool lastCycle = false ) -> void;
    auto pushStack( uint8_t data, bool lastCycle = false ) -> void;
    auto pullStack( bool lastCycle = false ) -> uint8_t;   
	auto loadZeroPage( uint8_t addr, bool lastCycle = false ) -> uint8_t;
	auto storeZeroPage( uint8_t addr, uint8_t data, bool lastCycle = false ) -> void;
    
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
	auto indexedIndirectAdr() -> uint16_t;
	auto indirectIndexedAdr( bool forceExtraCycle = false ) -> uint16_t;
	auto zeroPageIndexedAdr( uint8_t index ) -> uint8_t;
	auto absoluteAdr( ) -> uint16_t;
	auto absoluteIndexedAdr( uint8_t index, bool forceExtraCycle = false ) -> uint16_t;
	
    //opcodes
	virtual auto indexedIndirect( Alu alu ) -> void;
	virtual auto indexedIndirectW( uint8_t data ) -> void;
	virtual auto indirectIndexed( Alu alu ) -> void;
	virtual auto indirectIndexedW( uint8_t data ) -> void;
	virtual auto zeroPage( Alu alu, uint8_t& data ) -> void;
	virtual auto zeroPage( Alu alu = nullptr ) -> void;
	virtual auto zeroPageW( uint8_t data ) -> void;
	virtual auto zeroPageM( Alu alu ) -> void;	
	virtual auto zeroPageIndexed( uint8_t index, Alu alu, uint8_t& data ) -> void;
	virtual auto zeroPageIndexed( uint8_t index, Alu alu = nullptr ) -> void;
	virtual auto zeroPageIndexedW( uint8_t index, uint8_t data ) -> void;
	virtual auto zeroPageIndexedM( Alu alu ) -> void;
	virtual auto absolute( Alu alu, uint8_t& data ) -> void;
	virtual auto absolute( Alu alu = nullptr ) -> void;
	virtual auto absoluteW( uint8_t data ) -> void;
	virtual auto absoluteM( Alu alu ) -> void;
	virtual auto absoluteIndexed( uint8_t index, Alu alu, uint8_t& data ) -> void;
	virtual auto absoluteIndexed( uint8_t index, Alu alu = nullptr ) -> void;
	virtual auto absoluteIndexedW( uint8_t index, uint8_t data ) -> void;
	virtual auto absoluteIndexedM( uint8_t index, Alu alu ) -> void;
	virtual auto immediate( Alu alu, uint8_t& data ) -> void;	
	virtual auto implied(Alu alu, uint8_t& data) -> void;
    virtual auto nop() -> void;
    virtual auto rti() -> void;
    virtual auto rts() -> void;
    virtual auto brk() -> void;
    virtual auto clear( bool& flag ) -> void;
    virtual auto set( bool& flag ) -> void;
    virtual auto jmpAbsolute() -> void;
    virtual auto jmpIndirect() -> void;
    virtual auto jsrAbsolute() -> void;
    virtual auto branch( bool& flag, bool state ) -> void;
    virtual auto plp() -> void;
    virtual auto php() -> void;
	virtual auto pha() -> void;
	virtual auto pla() -> void;
    virtual auto transfer(uint8_t src, uint8_t& target, bool flag) -> void;
    
	//undocumented opcodes        
	virtual auto indexedIndirectLax( ) -> void;
	virtual auto indirectIndexedLax( ) -> void;
    virtual auto zeroPageM( Alu alu, Alu alu2 ) -> void;
	virtual auto zeroPageLax() -> void;
	virtual auto zeroPageIndexedLax() -> void;
	virtual auto absoluteLax() -> void;
	virtual auto absoluteIndexedLax() -> void;
    virtual auto immediate() -> void;
	virtual auto immediateLax() -> void;
	virtual auto absoluteIndexedLas() -> void;
	virtual auto absoluteIndexedWSh( uint8_t index, uint8_t index2 ) -> void;
	virtual auto absoluteIndexedWAhx() -> void;
	virtual auto absoluteIndexedWTas() -> void;
	virtual auto immediateAnc() -> void;
	virtual auto immediateAlr() -> void;
	virtual auto immediateArr() -> void;
	virtual auto immediateAne() -> void;
	virtual auto immediateSbx() -> void;
	virtual auto kill() -> void;
	virtual auto indexedIndirectM( Alu alu, Alu alu2 ) -> void;
	virtual auto indirectIndexedWAhx() -> void;
	virtual auto indirectIndexedM( Alu alu, Alu alu2 ) -> void;
	virtual auto zeroPageIndexedM( Alu alu, Alu alu2 ) -> void;
	virtual auto absoluteM( Alu alu, Alu alu2 ) -> void;
	virtual auto absoluteIndexedM( uint8_t index, Alu alu, Alu alu2 ) -> void;
	auto H1AndedWrite( uint16_t absIndexed, uint8_t anded ) -> void;
};

}

