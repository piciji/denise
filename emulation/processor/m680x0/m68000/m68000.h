
#pragma once

#include "../base/base.h"

namespace M68FAMILY {	

struct M68000 : Base {
	M68000(bool derived = false);	
	
	auto power() -> void;
	auto reset() -> void;
	auto process() -> void;
	auto setInterrupt( uint8_t level, unsigned cycle = 0 ) -> void;
	auto openBus() -> uint32_t ;	
    auto raiseBusError(uint32_t addr) -> void;

protected:
	auto sampleIrq() -> void;
	auto switchToSupervisor() -> void;

	//effective address
	template<uint8_t Size, uint8_t specialCase = None> auto fetch(EffectiveAddress& ea) -> uint32_t;
	template<uint8_t Size, uint8_t specialCase = None> auto read(EffectiveAddress& ea) -> uint32_t;
	template<uint8_t Size> auto write(EffectiveAddress& ea, uint32_t data, bool lastBusCyle = true) -> void;
			
	//memory
	auto readExtensionWord() -> void;
	auto prefetch(bool lastBusCycle = true) -> void;
	template<uint8_t Size> auto read(uint32_t addr, bool lastBusCycle = false) -> uint32_t;
	template<uint8_t Size> auto write(uint32_t addr, uint32_t value, bool lastBusCycle = false) -> void;
	auto writeStack(uint32_t value, bool lastBusCycle = false) -> void;
	template<bool exception = false> auto fullPrefetch() -> void;
	
	//exception
	virtual auto group0exception(uint32_t addr, uint8_t type) -> void;
	auto group1exceptions() -> bool;
	virtual auto interruptException( uint8_t level ) -> void;	
	virtual auto traceException() -> void;
	auto illegalException(uint8_t vector) -> void;
	auto privilegeException() -> void;	
	virtual auto trapException(uint8_t vector) -> void;
	virtual auto executeAt(uint8_t vector) -> void;	
	
	//cycle calculation
	template<uint8_t Mode> auto cyclesBit(uint8_t bit) -> void;
	auto cyclesDivu(uint32_t dividend, uint16_t divisor) -> unsigned;
	auto cyclesDivs(int32_t dividend, int16_t divisor) -> unsigned;
	auto cyclesJmp(uint8_t mode) -> void;

	//instructions		
	template<uint8_t Size, uint8_t Mode> auto opImmShift(uint8_t shift, DataRegister modify) -> void;
	template<uint8_t Size, uint8_t Mode> auto opRegShift(DataRegister shift, DataRegister modify) -> void;
	template<uint8_t Mode> auto opEaShift(EffectiveAddress modify) -> void;
	
	template<uint8_t Size, uint8_t Mode> auto opBit(EffectiveAddress modify, DataRegister bitreg) -> void;
	template<uint8_t Size, uint8_t Mode> auto opImmBit(EffectiveAddress modify) -> void;
	
	template<uint8_t Size> auto opClr(EffectiveAddress modify) -> void;
	auto opNbcd(EffectiveAddress modify) -> void;
	template<uint8_t Size, bool Extend> auto opNeg(EffectiveAddress modify) -> void;
	template<uint8_t Size> auto opNot(EffectiveAddress modify) -> void;
	auto opScc(EffectiveAddress modify, uint8_t cc) -> void;
	auto opTas(EffectiveAddress modify) -> void;
	template<uint8_t Size> auto opTst(EffectiveAddress modify) -> void;
	template<uint8_t Size, uint8_t Mode> auto opArithmetic(DataRegister modify, EffectiveAddress src) -> void;
	template<uint8_t Size, uint8_t Mode> auto opArithmetic(EffectiveAddress modify, DataRegister src) -> void;	
	template<uint8_t Size> auto opAdda(AddressRegister modify, EffectiveAddress src) -> void;
	template<uint8_t Size> auto opCmp(DataRegister dest, EffectiveAddress src) -> void;
	template<uint8_t Size> auto opCmpa(AddressRegister dest, EffectiveAddress src) -> void;
	template<uint8_t Size> auto opSuba(AddressRegister modify, EffectiveAddress src) -> void;
	template<uint8_t Size> auto opEor(EffectiveAddress modify, DataRegister src) -> void;
	auto opMulu(DataRegister modify, EffectiveAddress src) -> void;
	auto opMuls(DataRegister modify, EffectiveAddress src) -> void;
	auto opDivu(DataRegister modify, EffectiveAddress src) -> void;
	auto opDivs(DataRegister modify, EffectiveAddress src) -> void;
	
	template<uint8_t Size, uint8_t specialCase> auto opMove(EffectiveAddress src, EffectiveAddress dest) -> void;
	template<uint8_t Size> auto opMovePreDec(EffectiveAddress src, EffectiveAddress dest) -> void;
	template<uint8_t Size> auto opMovea(EffectiveAddress src, AddressRegister modify) -> void;
	template<uint8_t Size, uint8_t Mode> auto opArithmeticI(EffectiveAddress modify) -> void;
	template<uint8_t Size, uint8_t Mode> auto opArithmeticQ(EffectiveAddress modify, uint8_t immediate) -> void;
	template<uint8_t Mode> auto opArithmeticQ(AddressRegister areg, uint8_t immediate) -> void;
	template<uint8_t Size> auto opCmpi(EffectiveAddress dest) -> void;
	auto opMoveq(DataRegister modify, uint8_t data) -> void;
	template<uint8_t Size> auto opBcc(uint8_t cond, uint8_t displacement) -> void;
	template<uint8_t Size> auto opBra(uint8_t displacement) -> void;
	template<uint8_t Size> auto opBsr(uint8_t displacement) -> void;
	auto opDbcc(DataRegister modify, uint8_t cond ) -> void;
	auto opJmp(EffectiveAddress src) -> void;
	auto opJsr(EffectiveAddress src) -> void;
	auto opLea(EffectiveAddress src, AddressRegister modify) -> void;
	auto opPea(EffectiveAddress src) -> void;
	
	template<uint8_t Size, bool memToReg> auto opMovem(EffectiveAddress src) -> void;
	template<uint8_t Size> auto opMovemPostInc(AddressRegister modify) -> void;
	template<uint8_t Size> auto opMovemPreDec(AddressRegister modify) -> void;
	
	template<uint8_t Size, uint8_t Mode> auto opArithmeticX(DataRegister src, DataRegister dest) -> void;
	template<uint8_t Size, uint8_t Mode> auto opArithmeticX(EffectiveAddress src, EffectiveAddress dest) -> void;
	template<uint8_t Size> auto opCmpm(EffectiveAddress src, EffectiveAddress dest) -> void;
	
	template<uint8_t Mode> auto opCcr() -> void;
	template<uint8_t Mode> auto opSr() -> void;
	
	template<uint8_t Size> auto opChk(EffectiveAddress src, DataRegister srcD) -> void;
	auto opMoveFromSr(EffectiveAddress modify) -> void; //override for 68010
	auto opMoveToCcr(EffectiveAddress modify) -> void;
	auto opMoveToSr(EffectiveAddress modify) -> void;
	
	auto opExg(DataRegister d1, DataRegister d2) -> void;
	auto opExg(AddressRegister a1, AddressRegister a2) -> void;
	auto opExg(DataRegister d, AddressRegister a) -> void;
	template<uint8_t Size> auto opExt(DataRegister modify) -> void;
	template<uint8_t Size> auto opLink(AddressRegister modify) -> void;
	
	template<bool toUsp> auto opMoveUsp(AddressRegister modify) -> void;
	auto opNop() -> void;
	auto opReset() -> void;
	auto opRte() -> void;
	auto opRtr() -> void;
	auto opRts() -> void;
	auto opStop() -> void;
	auto opSwap(DataRegister modify) -> void;
	auto opTrap(uint8_t vector) -> void;
	auto opTrapv() -> void;
	auto opUnlink(AddressRegister modify) -> void;
	template<uint8_t Mode> auto opMovep(EffectiveAddress ea, DataRegister dataReg) -> void;
};

}
