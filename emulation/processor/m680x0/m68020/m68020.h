
#pragma once

#include "../base/base.h"

namespace M68FAMILY {	
		
struct M68020 : Base {
	M68020();
	
	auto power() -> void;
	auto reset() -> void;
	auto process() -> void;
	auto setInterrupt( uint8_t level, unsigned cycle ) -> void;
	auto openBus() -> uint32_t ;	
    auto raiseBusError(uint32_t addr) -> void;	
	auto addWaitStates(unsigned cycles) -> void;
	auto getFunctionCodes() -> uint8_t;

    std::vector<std::vector<std::vector<uint8_t>>> LWordMap = { {{1, 1, 1, 1}}, { {2, 2}, {1, 2, 1} }, { {4}, {3, 1}, {2, 2}, {1, 3} } };    
	
protected:
	auto getSR() -> uint16_t;	
	auto setSR(uint16_t data) -> void;

	//effective address
	template<uint8_t Size, bool calcEaOnly, uint8_t specialCase = None>
		auto fetch(EffectiveAddress& ea) -> uint32_t;
	template<uint8_t Size> auto read(EffectiveAddress& ea) -> uint32_t;
	template<uint8_t Size> auto write(EffectiveAddress& ea, uint32_t data) -> void;
	auto fullextensionAddressing(uint32_t& adr, uint32_t dispReg, uint16_t ew) -> void;
	
    auto sampleIrq() -> void;
    auto addBus(unsigned cycles) -> void;
	auto addSequencer(unsigned cycles, bool syncBus = false) -> void;
	auto syncToBus() -> void;
	auto catchUpSequencer() -> void;
    
    //exception
    auto group1exceptions() -> bool;
    auto interruptException( uint8_t level ) -> void;
	auto traceException() -> void;
	auto illegalException(uint8_t vector) -> void;
	auto privilegeException() -> void;
	auto switchToSupervisor() -> void;
	auto executeAt(uint8_t vector) -> void;
    
    //memory
	template<bool incrementPC = true> auto readExtensionWord() -> void;
	template<bool incrementPC = true, bool extensionWord = false> auto prefetch(bool initial = false) -> void;
    template<uint8_t Size> auto read(uint32_t addr) -> uint32_t;
    template<uint8_t Size> auto write(uint32_t addr, uint32_t value) -> void;
	
	//instructions	
	template<uint8_t Size, uint8_t Mode, bool dynamic = false> auto opImmShift(uint8_t shift, DataRegister modify) -> void;
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
	auto opMoveFromSr(EffectiveAddress modify) -> void;
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

	template<uint8_t Mode> auto opBitField(EffectiveAddress modify) -> void;
	
	auto opCallm(EffectiveAddress modify) -> void;
	template<uint8_t Size> auto opCas(EffectiveAddress modify) -> void;
	template<uint8_t Size> auto opCas2() -> void;
	template<uint8_t Size> auto opChk2(EffectiveAddress modify) -> void;
	auto opBkpt(uint8_t vector) -> void;
	auto opDivl(EffectiveAddress src) -> void;
	auto opMull(EffectiveAddress src) -> void;
	auto opExtb(DataRegister modify) -> void;
	auto opMoveFromCcr(EffectiveAddress modify) -> void;
	auto opPack(DataRegister src, DataRegister dest) -> void;
	auto opPack(EffectiveAddress src, EffectiveAddress dest) -> void;
	auto opUnpack(DataRegister src, DataRegister dest) -> void;
	auto opUnpack(EffectiveAddress src, EffectiveAddress dest) -> void;
	auto opRtd() -> void;
	auto opRtm(DataRegister src) -> void;
	auto opRtm(AddressRegister src) -> void;
	template<uint8_t Mode> auto opTrapcc(uint8_t cond) -> void;
	template<bool toControl> auto opMovec() -> void;
	template<uint8_t Size> auto opMoves(EffectiveAddress ea) -> void;
};	

}
