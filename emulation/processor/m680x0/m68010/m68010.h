
#pragma once

#include "../m68000/m68000.h"

namespace M68FAMILY {	

struct M68010 : M68000 {
	M68010();
	
	auto reset() -> void;
	auto getFunctionCodes() -> uint8_t;
	
	enum : uint8_t { Loop_Normal, Loop_Custom1, Loop_Custom2 };
protected:	
	std::function<uint8_t ()> opLoop[0x10000]; //68010 loop mode instructions

	auto group0exception(uint32_t addr, uint8_t type) -> void;
	auto interruptException( uint8_t level ) -> void;	
	auto traceException() -> void;
	auto illegalException(uint8_t vector) -> void;	
	auto trapException(uint8_t vector) -> void;
	auto executeAt(uint8_t vector) -> void;	
	
	auto opBkpt(uint8_t vector) -> void;
	auto opMoveFromCcr(EffectiveAddress modify) -> void;
	auto opMoveFromSr(EffectiveAddress modify) -> void;
	auto opRtd() -> void;
	auto opRte() -> void;
	template<bool toControl> auto opMovec() -> void;
	template<uint8_t Size> auto opMoves(EffectiveAddress ea) -> void;
	template<uint8_t Size> auto opClr(EffectiveAddress modify) -> void;
	auto opScc(EffectiveAddress modify, uint8_t cc) -> void;
	auto opDbcc(DataRegister modify, uint8_t cond ) -> void;
	
	auto opLoopDbcc(DataRegister modify, uint8_t cond ) -> void;			
	template<uint8_t Size> auto opLoopMove(EffectiveAddress src, EffectiveAddress dest) -> uint8_t;
	template<uint8_t Size, uint8_t Mode> auto opLoopArithmetic(DataRegister modify, EffectiveAddress src) -> uint8_t;
	template<uint8_t Size> auto opLoopCmp(DataRegister dest, EffectiveAddress src) -> uint8_t;
	template<uint8_t Size, uint8_t Mode> auto opLoopArithmetic(EffectiveAddress modify, DataRegister src) -> uint8_t;
	template<uint8_t Size> auto opLoopEor(EffectiveAddress modify, DataRegister src) -> uint8_t;
	template<uint8_t Size> auto opLoopAdda(AddressRegister modify, EffectiveAddress src) -> uint8_t;
	template<uint8_t Size> auto opLoopSuba(AddressRegister modify, EffectiveAddress src) -> uint8_t;
	template<uint8_t Size> auto opLoopCmpa(AddressRegister dest, EffectiveAddress src) -> uint8_t;
	template<uint8_t Size> auto opLoopClr(EffectiveAddress modify) -> uint8_t;
	template<uint8_t Size, bool Extend> auto opLoopNeg(EffectiveAddress modify) -> uint8_t;
	template<uint8_t Size> auto opLoopNot(EffectiveAddress modify) -> uint8_t;
	template<uint8_t Size> auto opLoopTst(EffectiveAddress modify) -> uint8_t;
	auto opLoopNbcd(EffectiveAddress modify) -> uint8_t;
	template<uint8_t Size, uint8_t Mode> auto opLoopArithmeticX(EffectiveAddress src, EffectiveAddress dest) -> uint8_t;
	template<uint8_t Size> auto opLoopCmpm(EffectiveAddress src, EffectiveAddress dest) -> uint8_t;
	template<uint8_t Mode> auto opLoopEaShift(EffectiveAddress modify) -> uint8_t;
	
	auto cyclesMoves(uint8_t mode) -> void;
	template<uint8_t Size> auto noFetchTail(EffectiveAddress& ea) -> void;
};

}